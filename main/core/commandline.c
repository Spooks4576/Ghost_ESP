// command.c

#include "core/commandline.h"
#include "core/callbacks.h"
#include "managers/wifi_manager.h"
#include "managers/rgb_manager.h"
#include "managers/ap_manager.h"
#include "managers/ble_manager.h"
#include "managers/settings_manager.h"
#include <stdlib.h>
#include <string.h>
#include <vendor/dial_client.h>
#include "managers/dial_manager.h"
#include <esp_timer.h>
#include "vendor/pcap.h"
#include <sys/socket.h>
#include <netdb.h>
#include <managers/gps_manager.h>
#include <managers/views/terminal_screen.h>
#include "vendor/printer.h"
#include "esp_sntp.h"

static Command *command_list_head = NULL;
TaskHandle_t VisualizerHandle = NULL;

void command_init() {
    command_list_head = NULL;
}

void register_command(const char *name, CommandFunction function) {
    // Check if the command already exists
    Command *current = command_list_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            // Command already registered
            return;
        }
        current = current->next;
    }

    // Create a new command
    Command *new_command = (Command *)malloc(sizeof(Command));
    if (new_command == NULL) {
        // Handle memory allocation failure
        return;
    }
    new_command->name = strdup(name);
    new_command->function = function;
    new_command->next = command_list_head;
    command_list_head = new_command;
}

void unregister_command(const char *name) {
    Command *current = command_list_head;
    Command *previous = NULL;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            // Found the command to remove
            if (previous == NULL) {
                command_list_head = current->next;
            } else {
                previous->next = current->next;
            }
            free(current->name);
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

CommandFunction find_command(const char *name) {
    Command *current = command_list_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->function;
        }
        current = current->next;
    }
    return NULL;
}

void cmd_wifi_scan_start(int argc, char **argv) {
    printf("WiFi scan started.\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi scan started.\n");
    wifi_manager_start_scan();
    wifi_manager_print_scan_results_with_oui();
}

void cmd_wifi_scan_stop(int argc, char **argv) {
    wifi_manager_stop_monitor_mode();
    pcap_file_close();
    printf("WiFi scan stopped.\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi scan stopped.\n");
}

void cmd_wifi_scan_results(int argc, char **argv) {
    printf("WiFi scan results displaying with OUI matching.\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi scan results displaying with OUI matching.\n");
    wifi_manager_print_scan_results_with_oui();
}

void handle_list(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        cmd_wifi_scan_results(argc, argv);
        return;
    } 
    else if (argc > 1 && strcmp(argv[1], "-s") == 0)
    {
        wifi_manager_list_stations();
        printf("Listed Stations...\n");
        TERMINAL_VIEW_ADD_TEXT("Listed Stations...\n");
        return;
    }
    else {
        printf("Usage: list -a (for Wi-Fi scan results)\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: list -a (for Wi-Fi scan results)\n");
    }
}

void handle_beaconspam(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        printf("Starting Random beacon spam...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting Random beacon spam...\n");
        wifi_manager_start_beacon(NULL);
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-rr") == 0) {
        printf("Starting Rickroll beacon spam...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting Rickroll beacon spam...\n");
        wifi_manager_start_beacon("RICKROLL");
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        printf("Starting AP List beacon spam...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting AP List beacon spam...\n");
        wifi_manager_start_beacon("APLISTMODE");
        return;
    }

    if (argc > 1)
    {
        wifi_manager_start_beacon(argv[1]);
        return;
    }
    else {
        printf("Usage: beaconspam -r (for Beacon Spam Random)\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: beaconspam -r (for Beacon Spam Random)\n");
    }
}


void handle_stop_spam(int argc, char **argv)
{
    wifi_manager_stop_beacon();
    printf("Beacon Spam Stopped...\n");
    TERMINAL_VIEW_ADD_TEXT("Beacon Spam Stopped...\n");
}

void handle_sta_scan(int argc, char **argv)
{
    wifi_manager_start_monitor_mode(wifi_stations_sniffer_callback);
    printf("Started Station Scan...\n");
    TERMINAL_VIEW_ADD_TEXT("Started Station Scan...\n");
}


void handle_attack_cmd(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        printf("Deauth Attack Starting...\n");
        TERMINAL_VIEW_ADD_TEXT("Deauth Attack Starting...\n");
        wifi_manager_start_deauth();
        return;
    }
    else 
    {
        printf("Usage: attack -d (for deauthing access points)\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: attack -d (for deauthing access points)\n");
    }
}


void handle_stop_deauth(int argc, char **argv)
{
    wifi_manager_stop_deauth();
    printf("Deauthing Stopped....\n");
    TERMINAL_VIEW_ADD_TEXT("Deauthing Stopped....\n");
}


void handle_select_cmd(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: select -a <number>\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: select -a <number>\n");
        return;
    }

    if (strcmp(argv[1], "-a") == 0) {
        char *endptr;
        
        int num = (int)strtol(argv[2], &endptr, 10);


        if (*endptr == '\0') {
            wifi_manager_select_ap(num);
        } else {
            printf("Error: is not a valid number.\n");
            TERMINAL_VIEW_ADD_TEXT("Error: is not a valid number.\n");
        }
    } else {
        printf("Invalid option. Usage: select -a <number>\n");
        TERMINAL_VIEW_ADD_TEXT("Invalid option. Usage: select -a <number>\n");
    }
}


void discover_task(void *pvParameter) {
    DIALClient client;
    DIALManager manager;

    if (dial_client_init(&client) == ESP_OK) {
       
        dial_manager_init(&manager, &client);

        
        explore_network(&manager);

        
        dial_client_deinit(&client);
    } else {
        printf("Failed to initialize DIAL client.\n");
        TERMINAL_VIEW_ADD_TEXT("Failed to initialize DIAL client.\n");
    }

    vTaskDelete(NULL);
}

void handle_stop_flipper(int argc, char** argv)
{
    wifi_manager_stop_deauth();
#ifndef CONFIG_IDF_TARGET_ESP32S2
    ble_stop();
#endif
    csv_flush_buffer_to_file();  // Ensure any buffered data is written
    csv_file_close();           // Close any open CSV files
    gps_manager_deinit(&g_gpsManager);  // Clean up GPS if active
    wifi_manager_stop_monitor_mode();    // Stop any active monitoring
    printf("All activities stopped and files closed.\n");
    TERMINAL_VIEW_ADD_TEXT("All activities stopped and files closed.\n");
}

void handle_dial_command(int argc, char** argv)
{
    xTaskCreate(&discover_task, "discover_task", 10240, NULL, 5, NULL);
}

void handle_wifi_connection(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <SSID> <PASSWORD>\n", argv[0]);
        TERMINAL_VIEW_ADD_TEXT("Usage: %s <SSID> <PASSWORD>\n", argv[0]);
        return;
    }

    const char* ssid = argv[1];
    const char* password = "";

    if (argc == 3 && strlen(argv[2]) > 8)
    {
        password = argv[2];
    }

    if (strlen(ssid) == 0) {
        printf("SSID and password cannot be empty\n");
        TERMINAL_VIEW_ADD_TEXT("SSID and password cannot be empty\n");
        return;
    }

    printf("Connecting to SSID: %s\n", ssid);
    TERMINAL_VIEW_ADD_TEXT("Connecting to SSID: %s\n", ssid);
    
    wifi_manager_connect_wifi(ssid, password);

    if (VisualizerHandle == NULL)
    {
#ifdef WITH_SCREEN
    xTaskCreate(screen_music_visualizer_task, "udp_server", 4096, NULL, 5, &VisualizerHandle);
#else
    xTaskCreate(animate_led_based_on_amplitude, "udp_server", 4096, NULL, 5, &VisualizerHandle);
#endif
    }

#ifdef CONFIG_HAS_RTC_CLOCK
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
#endif

}


#ifndef CONFIG_IDF_TARGET_ESP32S2

void handle_ble_scan_cmd(int argc, char**argv)
{
    if (argc > 1 && strcmp(argv[1], "-f") == 0) {
        printf("Starting Find the Flippers...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting Find the Flippers...\n");
        ble_start_find_flippers();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-ds") == 0) {
        printf("Starting BLE Spam Detector...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting BLE Spam Detector...\n");
        ble_start_blespam_detector();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        printf("Starting AirTag Scanner...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting AirTag Scanner...\n");
        ble_start_airtag_scanner();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        printf("Scanning for Raw Packets\n");
        TERMINAL_VIEW_ADD_TEXT("Scanning for Raw Packets\n");
        ble_start_raw_ble_packetscan();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        printf("Stopping BLE Scan...\n");
        TERMINAL_VIEW_ADD_TEXT("Stopping BLE Scan...\n");
        ble_stop();
        return;
    }

    printf("Invalid Command Syntax...\n");
    TERMINAL_VIEW_ADD_TEXT("Invalid Command Syntax...\n");
}

#endif


void handle_start_portal(int argc, char **argv)
{
    
    const char* URLorFilePath = settings_get_portal_url(&G_Settings);
    const char* SSID = settings_get_portal_ssid(&G_Settings);
    const char* Password = settings_get_portal_password(&G_Settings);
    const char* AP_SSID = settings_get_portal_ap_ssid(&G_Settings);
    const char* Domain = settings_get_portal_domain(&G_Settings);
    bool offlinemode = settings_get_portal_offline_mode(&G_Settings);

    const char *url = URLorFilePath;
    const char *ssid = SSID;
    const char *password = Password;
    const char *ap_ssid = AP_SSID;
    const char *domain = Domain;

    if (argc == 6)
    {
        url = (argv[1] && argv[1][0] != '\0') ? argv[1] : url;
        ssid = (argv[2] && argv[2][0] != '\0') ? argv[2] : ssid;
        password = (argv[3] && argv[3][0] != '\0') ? argv[3] : password;
        ap_ssid = (argv[4] && argv[4][0] != '\0') ? argv[4] : ap_ssid;
        domain = (argv[5] && argv[5][0] != '\0') ? argv[5] : domain;
    }
    else if (argc == 4)
    {
        url = (argv[1] && argv[1][0] != '\0') ? argv[1] : url;
        ap_ssid = (argv[2] && argv[2][0] != '\0') ? argv[2] : ap_ssid;
        domain = (argv[3] && argv[3][0] != '\0') ? argv[3] : domain;
    }
    else if (argc != 1)
    {
        printf("Error: Incorrect number of arguments.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Incorrect number of arguments.\n");
        printf("Usage: %s <URL> <SSID> <Password> <AP_ssid> <DOMAIN>\n", argv[0]);
        TERMINAL_VIEW_ADD_TEXT("Usage: %s <URL> <SSID> <Password> <AP_ssid> <DOMAIN>\n", argv[0]);
        printf("or\n");
        TERMINAL_VIEW_ADD_TEXT("or\n");
        printf("Usage: %s <filepath> <APSSID> <Domain>\n", argv[0]);
        TERMINAL_VIEW_ADD_TEXT("Usage: %s <filepath> <APSSID> <Domain>\n", argv[0]);
        return;
    }


    if (url == NULL || url[0] == '\0') {
        printf("Error: URL or File Path cannot be empty.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: URL or File Path cannot be empty.\n");
        return;
    }

    if (ap_ssid == NULL || ap_ssid[0] == '\0') {
        printf("Error: AP SSID cannot be empty.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: AP SSID cannot be empty.\n");
        return;
    }

    if (domain == NULL || domain[0] == '\0') {
        printf("Error: Domain cannot be empty.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Domain cannot be empty.\n");
        return;
    }

    if (ssid && ssid[0] != '\0' && password && password[0] != '\0' && !offlinemode) {
        printf("Starting portal with SSID: %s, Password: %s, AP_SSID: %s, Domain: %s\n", ssid, password, ap_ssid, domain);
        TERMINAL_VIEW_ADD_TEXT("Starting portal with SSID: %s, Password: %s, AP_SSID: %s, Domain: %s\n", ssid, password, ap_ssid, domain);
        wifi_manager_start_evil_portal(url, ssid, password, ap_ssid, domain);
    }
    else if (offlinemode){
        printf("Starting portal in offline mode with AP_SSID: %s, Domain: %s\n", ap_ssid, domain);
        TERMINAL_VIEW_ADD_TEXT("Starting portal in offline mode with AP_SSID: %s, Domain: %s\n", ap_ssid, domain);
        wifi_manager_start_evil_portal(url, NULL, NULL, ap_ssid, domain);
    }
}

bool ip_str_to_bytes(const char* ip_str, uint8_t* ip_bytes) {
    int ip[4];
    if (sscanf(ip_str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4) {
        for (int i = 0; i < 4; i++) {
            if (ip[i] < 0 || ip[i] > 255) return false;
            ip_bytes[i] = (uint8_t)ip[i];
        }
        return true;
    }
    return false;
}

bool mac_str_to_bytes(const char *mac_str, uint8_t *mac_bytes) {
    int mac[6];
    if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            if (mac[i] < 0 || mac[i] > 255) return false;
            mac_bytes[i] = (uint8_t) mac[i];
        }
        return true;
    }
    return false;
}

void encrypt_tp_link_command(const char *input, uint8_t *output, size_t len)
{
    uint8_t key = 171;
    for (size_t i = 0; i < len; i++) {
        output[i] = input[i] ^ key;
        key = output[i];
    }
}

void decrypt_tp_link_response(const uint8_t *input, char *output, size_t len)
{
    uint8_t key = 171;
    for (size_t i = 0; i < len; i++) {
        output[i] = input[i] ^ key;
        key = input[i];
    }
}

void handle_tp_link_test(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: tp_link_test <on|off|loop>\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: tp_link_test <on|off|loop>\n");
        return;
    }

    bool isloop = false;

    
    if (strcmp(argv[1], "loop") == 0) {
        isloop = true;
    } else if (strcmp(argv[1], "on") != 0 && strcmp(argv[1], "off") != 0) {
        printf("Invalid argument. Use 'on', 'off', or 'loop'.\n");
        return;
    }

    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(9999);


    int iterations = isloop ? 10 : 1;

    for (int i = 0; i < iterations; i++) {
        const char *command;
        if (isloop) {
            command = (i % 2 == 0) ?
                "{\"system\":{\"set_relay_state\":{\"state\":1}}}" :  // "on"
                "{\"system\":{\"set_relay_state\":{\"state\":0}}}";   // "off"
        } else {
            
            command = (strcmp(argv[1], "on") == 0) ?
                "{\"system\":{\"set_relay_state\":{\"state\":1}}}" :
                "{\"system\":{\"set_relay_state\":{\"state\":0}}}";
        }

        
        uint8_t encrypted_command[128];
        memset(encrypted_command, 0, sizeof(encrypted_command));

        size_t command_len = strlen(command);
        if (command_len >= sizeof(encrypted_command)) {
            printf("Command too large to encrypt\n");
            return;
        }

        encrypt_tp_link_command(command, encrypted_command, command_len);

        
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            printf("Failed to create socket: errno %d\n", errno);
            return;
        }

        
        int broadcast = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

        
        int err = sendto(sock, encrypted_command, command_len, 0,
                         (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            printf("Error occurred during sending: errno %d\n", errno);
            TERMINAL_VIEW_ADD_TEXT("Error occurred during sending: errno %d\n", errno);
            close(sock);
            return;
        }

        printf("Broadcast message sent: %s\n", command);
        TERMINAL_VIEW_ADD_TEXT("Broadcast message sent: %s\n", command);

        
        struct timeval timeout = {2, 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        uint8_t recv_buf[128];
        socklen_t addr_len = sizeof(dest_addr);
        int len = recvfrom(sock, recv_buf, sizeof(recv_buf) - 1, 0,
                           (struct sockaddr *)&dest_addr, &addr_len);
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("No response from any device\n");
                TERMINAL_VIEW_ADD_TEXT("No response from any device\n");
            } else {
                printf("Error receiving response: errno %d\n", errno);
                TERMINAL_VIEW_ADD_TEXT("Error receiving response: errno %d\n", errno);
            }
        } else {
            recv_buf[len] = 0;
            char decrypted_response[128];
            decrypt_tp_link_response(recv_buf, decrypted_response, len);
            decrypted_response[len] = 0;
            printf("Response: %s\n", decrypted_response);
        }


        close(sock);

        
        if (isloop && i < 9) {
            vTaskDelay(pdMS_TO_TICKS(700));
        }
    }
}

void handle_ip_lookup(int argc, char** argv) {
    printf("Starting IP lookup...\n");
    TERMINAL_VIEW_ADD_TEXT("Starting IP lookup...\n");
    wifi_manager_start_ip_lookup();
}

void handle_capture_scan(int argc, char** argv) {
    if (argc != 2) {
        printf("Error: Incorrect number of arguments.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Incorrect number of arguments.\n");
        return;
    }

    char *capturetype = argv[1];

    if (capturetype == NULL || capturetype[0] == '\0') {
        printf("Error: Capture Type cannot be empty.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Capture Type cannot be empty.\n");
        return;
    }

    if (strcmp(capturetype, "-probe") == 0) {
        printf("Starting probe request capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting probe request capture...\n");
        int err = pcap_file_open("probescan", PCAP_CAPTURE_WIFI);
        
        if (err != ESP_OK) {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_probe_scan_callback);
    }

    if (strcmp(capturetype, "-deauth") == 0) {
        printf("Starting deauth packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting deauth packet capture...\n");
        int err = pcap_file_open("deauthscan", PCAP_CAPTURE_WIFI);
        
        if (err != ESP_OK) {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_deauth_scan_callback);
    }

    if (strcmp(capturetype, "-beacon") == 0) {
        printf("Starting beacon packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting beacon packet capture...\n");
        int err = pcap_file_open("beaconscan", PCAP_CAPTURE_WIFI);
        
        if (err != ESP_OK) {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_beacon_scan_callback);
    }

    if (strcmp(capturetype, "-raw") == 0) {
        printf("Starting raw packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting raw packet capture...\n");
        int err = pcap_file_open("rawscan", PCAP_CAPTURE_WIFI);
        
        if (err != ESP_OK) {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_raw_scan_callback);
    }

    if (strcmp(capturetype, "-eapol") == 0) {
        printf("Starting EAPOL packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting EAPOL packet capture...\n");
        int err = pcap_file_open("eapolscan", PCAP_CAPTURE_WIFI);
        
        if (err != ESP_OK) {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_eapol_scan_callback);
    }

    if (strcmp(capturetype, "-pwn") == 0) {
        printf("Starting PWN packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting PWN packet capture...\n");
        int err = pcap_file_open("pwnscan", PCAP_CAPTURE_WIFI);
        
        if (err != ESP_OK) {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_pwn_scan_callback);
    }

    if (strcmp(capturetype, "-wps") == 0) {
        printf("Starting WPS packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting WPS packet capture...\n");
        int err = pcap_file_open("wpsscan", PCAP_CAPTURE_WIFI);

        should_store_wps = 0;
        
        if (err != ESP_OK) {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_wps_detection_callback);
    }

    if (strcmp(capturetype, "-stop") == 0) {
        printf("Stopping packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Stopping packet capture...\n");
        wifi_manager_stop_monitor_mode();
#ifndef CONFIG_IDF_TARGET_ESP32S2
        ble_stop();
        ble_stop_skimmer_detection();
#endif
        pcap_file_close();
    }
#ifndef CONFIG_IDF_TARGET_ESP32S2
    if (strcmp(capturetype, "-ble") == 0) {
        printf("Starting BLE packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting BLE packet capture...\n");
        ble_start_capture();
    }

    if (strcmp(capturetype, "-skimmer") == 0) {
            printf("Starting BLE\nSkimmer detection...\n");
            TERMINAL_VIEW_ADD_TEXT("Starting BLE\nSkimmer detection...\n");
            int err = pcap_file_open("skimmer_scan", PCAP_CAPTURE_BLUETOOTH);
            if (err != ESP_OK) {
                printf("Warning: PCAP capture failed to start\n");
                TERMINAL_VIEW_ADD_TEXT("Warning: PCAP capture failed to start\n");
            } else {
                printf("PCAP capture started\nSuspicious devices will be logged\n");
                TERMINAL_VIEW_ADD_TEXT("PCAP capture started\nSuspicious devices will be logged\n");
            }
        // Start skimmer detection
        ble_start_skimmer_detection();
    }
#endif
}

void stop_portal(int argc, char **argv)
{
    wifi_manager_stop_evil_portal();
    printf("Stopping evil portal...\n");
    TERMINAL_VIEW_ADD_TEXT("Stopping evil portal...\n");
}

void handle_reboot(int argc, char **argv)
{
    printf("Rebooting system...\n");
    TERMINAL_VIEW_ADD_TEXT("Rebooting system...\n");
    esp_restart();
}

void handle_startwd(int argc, char **argv) {
    bool stop_flag = false;

    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            stop_flag = true;
            break;
        }
    }

    if (stop_flag) {
        gps_manager_deinit(&g_gpsManager);
        wifi_manager_stop_monitor_mode();
        printf("Wardriving stopped.\n");
        TERMINAL_VIEW_ADD_TEXT("Wardriving stopped.\n");
    } else {
        gps_manager_init(&g_gpsManager);
        wifi_manager_start_monitor_mode(wardriving_scan_callback);
        printf("Wardriving started.\n");
        TERMINAL_VIEW_ADD_TEXT("Wardriving started.\n");
    }
}


void handle_scan_ports(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("scanports local [-C/-A/start_port-end_port]\n");
        printf("scanports [IP] [-C/-A/start_port-end_port]\n");
        return;
    }

    bool is_local = strcmp(argv[1], "local") == 0;
    const char* target_ip = NULL;
    const char* port_arg = NULL;

    // Parse arguments based on whether it's a local scan
    if (is_local) {
        if (argc < 3) {
            printf("Missing port argument for local scan\n");
            return;
        }
        port_arg = argv[2];
    } else {
        if (argc < 3) {
            printf("Missing port argument for IP scan\n");
            return;
        }
        target_ip = argv[1];
        port_arg = argv[2];
    }

    if (is_local) {
        wifi_manager_scan_subnet();
        return;
    }

    host_result_t result;
    if (strcmp(port_arg, "-C") == 0) {
        scan_ports_on_host(target_ip, &result);
        if (result.num_open_ports > 0) {
            printf("Open ports on %s:\n", target_ip);
            for (int i = 0; i < result.num_open_ports; i++) {
                printf("Port %d\n", result.open_ports[i]);
            }
        }
    } else {
        int start_port, end_port;
        if (strcmp(port_arg, "-A") == 0) {
            start_port = 1;
            end_port = 65535;
        } else if (sscanf(port_arg, "%d-%d", &start_port, &end_port) != 2 || 
                  start_port < 1 || end_port > 65535 || start_port > end_port) {
            printf("Invalid port range\n");
            return;
        }
        scan_ip_port_range(target_ip, start_port, end_port);
    }
}


void handle_crash(int argc, char **argv)
{
    int *ptr = NULL;
    *ptr = 42;
}

void handle_help(int argc, char **argv) {
    printf("\n Ghost ESP Commands:\n\n");
    TERMINAL_VIEW_ADD_TEXT("\n Ghost ESP Commands:\n\n");
    
    printf("help\n");
    printf("    Description: Display this help message.\n");
    printf("    Usage: help\n\n");
    TERMINAL_VIEW_ADD_TEXT("help\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Display this help message.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: help\n\n");

    printf("scanap\n");
    printf("    Description: Start a Wi-Fi access point (AP) scan.\n");
    printf("    Usage: scanap\n\n");
    TERMINAL_VIEW_ADD_TEXT("scanap\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Start a Wi-Fi access point (AP) scan.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: scanap\n\n");

    printf("scansta\n");
    printf("    Description: Start scanning for Wi-Fi stations.\n");
    printf("    Usage: scansta\n\n");
    TERMINAL_VIEW_ADD_TEXT("scansta\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Start scanning for Wi-Fi stations.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: scansta\n\n");

    printf("stopscan\n");
    printf("    Description: Stop any ongoing Wi-Fi scan.\n");
    printf("    Usage: stopscan\n\n");
    TERMINAL_VIEW_ADD_TEXT("stopscan\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Stop any ongoing Wi-Fi scan.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: stopscan\n\n");

    printf("attack\n");
    printf("    Description: Launch an attack (e.g., deauthentication attack).\n");
    printf("    Usage: attack -d\n");
    printf("    Arguments:\n");
    printf("        -d  : Start deauth attack\n\n");
    TERMINAL_VIEW_ADD_TEXT("attack\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Launch an attack (e.g., deauthentication attack).\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: attack -d\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        -d  : Start deauth attack\n\n");

    printf("list\n");
    printf("    Description: List Wi-Fi scan results or connected stations.\n");
    printf("    Usage: list -a | list -s\n");
    printf("    Arguments:\n");
    printf("        -a  : Show access points from Wi-Fi scan\n");
    printf("        -s  : List connected stations\n\n");
    TERMINAL_VIEW_ADD_TEXT("list\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: List Wi-Fi scan results or connected stations.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: list -a | list -s\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        -a  : Show access points from Wi-Fi scan\n");
    TERMINAL_VIEW_ADD_TEXT("        -s  : List connected stations\n\n");

    printf("beaconspam\n");
    printf("    Description: Start beacon spam with different modes.\n");
    printf("    Usage: beaconspam [OPTION]\n");
    printf("    Arguments:\n");
    printf("        -r   : Start random beacon spam\n");
    printf("        -rr  : Start Rickroll beacon spam\n");
    printf("        -l   : Start AP List beacon spam\n");
    printf("        [SSID]: Use specified SSID for beacon spam\n\n");
    TERMINAL_VIEW_ADD_TEXT("beaconspam\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Start beacon spam with different modes.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: beaconspam [OPTION]\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        -r   : Start random beacon spam\n");
    TERMINAL_VIEW_ADD_TEXT("        -rr  : Start Rickroll beacon spam\n");
    TERMINAL_VIEW_ADD_TEXT("        -l   : Start AP List beacon spam\n");
    TERMINAL_VIEW_ADD_TEXT("        [SSID]: Use specified SSID for beacon spam\n\n");

    printf("stopspam\n");
    printf("    Description: Stop ongoing beacon spam.\n");
    printf("    Usage: stopspam\n\n");
    TERMINAL_VIEW_ADD_TEXT("stopspam\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Stop ongoing beacon spam.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: stopspam\n\n");

    printf("stopdeauth\n");
    printf("    Description: Stop ongoing deauthentication attack.\n");
    printf("    Usage: stopdeauth\n\n");
    TERMINAL_VIEW_ADD_TEXT("stopdeauth\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Stop ongoing deauthentication attack.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: stopdeauth\n\n");

    printf("select\n");
    printf("    Description: Select an access point by index from the scan results.\n");
    printf("    Usage: select -a <number>\n");
    printf("    Arguments:\n");
    printf("        -a  : AP selection index (must be a valid number)\n\n");
    TERMINAL_VIEW_ADD_TEXT("select\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Select an access point by index from the scan results.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: select -a <number>\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        -a  : AP selection index (must be a valid number)\n\n");

    printf("startportal\n");
    printf("    Description: Start a portal with specified SSID and password.\n");
    printf("    Usage: startportal <URL> <SSID> <Password> <AP_ssid> <Domain>\n");
    printf("    Arguments:\n");
    printf("        <URL>       : URL for the portal\n");
    printf("        <SSID>      : Wi-Fi SSID for the portal\n");
    printf("        <Password>  : Wi-Fi password for the portal\n");
    printf("        <AP_ssid>   : SSID for the access point\n\n");
    printf("        <Domain>    : Custom Domain to Spoof In Address Bar\n\n");
    printf("  OR \n\n");
    printf("Offline Usage: startportal <FilePath> <AP_ssid> <Domain>\n");
    TERMINAL_VIEW_ADD_TEXT("startportal\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Start a portal with specified SSID and password.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: startportal <URL> <SSID> <Password> <AP_ssid> <Domain>\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        <URL>       : URL for the portal\n");
    TERMINAL_VIEW_ADD_TEXT("        <SSID>      : Wi-Fi SSID for the portal\n");
    TERMINAL_VIEW_ADD_TEXT("        <Password>  : Wi-Fi password for the portal\n");
    TERMINAL_VIEW_ADD_TEXT("        <AP_ssid>   : SSID for the access point\n\n");
    TERMINAL_VIEW_ADD_TEXT("        <Domain>    : Custom Domain to Spoof In Address Bar\n\n");
    TERMINAL_VIEW_ADD_TEXT("  OR \n\n");
    TERMINAL_VIEW_ADD_TEXT("Offline Usage: startportal <FilePath> <AP_ssid> <Domain>\n");

    printf("stopportal\n");
    printf("    Description: Stop Evil Portal\n");
    printf("    Usage: stopportal\n\n");
    TERMINAL_VIEW_ADD_TEXT("stopportal\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Stop Evil Portal\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: stopportal\n\n");

#ifndef CONFIG_IDF_TARGET_ESP32S2
    printf("blescan\n");
    printf("    Description: Handle BLE scanning with various modes.\n");
    printf("    Usage: blescan [OPTION]\n");
    printf("    Arguments:\n");
    printf("        -f   : Start 'Find the Flippers' mode\n");
    printf("        -ds  : Start BLE spam detector\n");
    printf("        -a   : Start AirTag scanner\n");
    printf("        -r   : Scan for raw BLE packets\n");
    printf("        -s   : Stop BLE scanning\n\n");
    TERMINAL_VIEW_ADD_TEXT("blescan\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Handle BLE scanning with various modes.\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: blescan [OPTION]\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        -f   : Start 'Find the Flippers' mode\n");
    TERMINAL_VIEW_ADD_TEXT("        -ds  : Start BLE spam detector\n");
    TERMINAL_VIEW_ADD_TEXT("        -a   : Start AirTag scanner\n");
    TERMINAL_VIEW_ADD_TEXT("        -r   : Scan for raw BLE packets\n");
    TERMINAL_VIEW_ADD_TEXT("        -s   : Stop BLE scanning\n\n");
#endif

    printf("capture\n");
    printf("    Description: Start a WiFi Capture (Requires SD Card or Flipper)\n");
    printf("    Usage: capture [OPTION]\n");
    printf("    Arguments:\n");
    printf("        -probe   : Start Capturing Probe Packets\n");
    printf("        -beacon  : Start Capturing Beacon Packets\n");
    printf("        -deauth   : Start Capturing Deauth Packets\n");
    printf("        -raw   :   Start Capturing Raw Packets\n");
    printf("        -wps   :   Start Capturing WPS Packets and there Auth Type");
    printf("        -pwn   :   Start Capturing Pwnagotchi Packets");
    printf("        -stop   : Stops the active capture\n\n");
    TERMINAL_VIEW_ADD_TEXT("capture\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Start a WiFi Capture (Requires SD Card or Flipper)\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: capture [OPTION]\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        -probe   : Start Capturing Probe Packets\n");
    TERMINAL_VIEW_ADD_TEXT("        -beacon  : Start Capturing Beacon Packets\n");
    TERMINAL_VIEW_ADD_TEXT("        -deauth   : Start Capturing Deauth Packets\n");
    TERMINAL_VIEW_ADD_TEXT("        -raw   :   Start Capturing Raw Packets\n");
    TERMINAL_VIEW_ADD_TEXT("        -wps   :   Start Capturing WPS Packets and there Auth Type");
    TERMINAL_VIEW_ADD_TEXT("        -pwn   :   Start Capturing Pwnagotchi Packets");
    TERMINAL_VIEW_ADD_TEXT("        -stop   : Stops the active capture\n\n");


    printf("connect\n");
    printf("    Description: Connects to Specific WiFi Network\n");
    printf("    Usage: connect <SSID> <Password>\n");
    TERMINAL_VIEW_ADD_TEXT("connect\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Connects to Specific WiFi Network\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: connect <SSID> <Password>\n");

    printf("dialconnect\n");
    printf("    Description: Cast a Random Youtube Video on all Smart TV's on your LAN (Requires You to Run Connect First)\n");
    printf("    Usage: dialconnect\n");
    TERMINAL_VIEW_ADD_TEXT("dialconnect\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Cast a Random Youtube Video on all Smart TV's on your LAN (Requires You to Run Connect First)\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: dialconnect\n");


    printf("powerprinter\n");
    printf("    Description: Print Custom Text to a Printer on your LAN (Requires You to Run Connect First)\n");
    printf("    Usage: powerprinter <Printer IP> <Text> <FontSize> <alignment>\n");
    printf("    aligment options: CM = Center Middle, TL = Top Left, TR = Top Right, BR = Bottom Right, BL = Bottom Left\n\n");
    TERMINAL_VIEW_ADD_TEXT("powerprinter\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Print Custom Text to a Printer on your LAN (Requires You to Run Connect First)\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: powerprinter <Printer IP> <Text> <FontSize> <alignment>\n");
    TERMINAL_VIEW_ADD_TEXT("    aligment options: CM = Center Middle, TL = Top Left, TR = Top Right, BR = Bottom Right, BL = Bottom Left\n\n");

    printf("blewardriving\n");
    printf("    Description: Start/Stop BLE wardriving with GPS logging\n");
    printf("    Usage: blewardriving [-s]\n");
    printf("    Arguments:\n");
    printf("        -s  : Stop BLE wardriving\n\n");
    TERMINAL_VIEW_ADD_TEXT("blewardriving\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Start/Stop BLE wardriving with GPS logging\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: blewardriving [-s]\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        -s  : Stop BLE wardriving\n\n");

    printf("Port Scanner\n");
    printf("    Description: Scan ports on local subnet or specific IP\n");
    printf("    Usage: scanports local [-C/-A/start_port-end_port]\n");
    printf("           scanports [IP] [-C/-A/start_port-end_port]\n");
    printf("    Arguments:\n");
    printf("        -C  : Scan common ports only\n");
    printf("        -A  : Scan all ports (1-65535)\n");
    printf("        start_port-end_port : Custom port range (e.g. 80-443)\n\n");
    TERMINAL_VIEW_ADD_TEXT("Port Scanner\n");
    TERMINAL_VIEW_ADD_TEXT("    Description: Scan ports on local subnet or specific IP\n");
    TERMINAL_VIEW_ADD_TEXT("    Usage: scanports local [-C/-A/start_port-end_port]\n");
    TERMINAL_VIEW_ADD_TEXT("           scanports [IP] [-C/-A/start_port-end_port]\n");
    TERMINAL_VIEW_ADD_TEXT("    Arguments:\n");
    TERMINAL_VIEW_ADD_TEXT("        -C  : Scan common ports only\n");
    TERMINAL_VIEW_ADD_TEXT("        -A  : Scan all ports (1-65535)\n");
    TERMINAL_VIEW_ADD_TEXT("        start_port-end_port : Custom port range (e.g. 80-443)\n\n");
}

void handle_capture(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: capture [-probe|-beacon|-deauth|-raw|-ble]\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: capture [-probe|-beacon|-deauth|-raw|-ble]\n");
        return;
    }
#ifndef CONFIG_IDF_TARGET_ESP32S2
    if (strcmp(argv[1], "-ble") == 0) {
        printf("Starting BLE packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting BLE packet capture...\n");
        ble_start_capture();
    }
#endif
}

void handle_gps_info(int argc, char **argv) {
    bool stop_flag = false;
    static TaskHandle_t gps_info_task_handle = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            stop_flag = true;
            break;
        }
    }

    if (stop_flag) {
        if (gps_info_task_handle != NULL) {
            vTaskDelete(gps_info_task_handle);
            gps_info_task_handle = NULL;
            gps_manager_deinit(&g_gpsManager);
            printf("GPS info display stopped.\n");
            TERMINAL_VIEW_ADD_TEXT("GPS info display stopped.\n");
        }
    } else {
        if (gps_info_task_handle == NULL) {
            gps_manager_init(&g_gpsManager);
            
            // Wait a brief moment for GPS initialization
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Start the info display task
            xTaskCreate(gps_info_display_task, "gps_info", 4096, NULL, 1, &gps_info_task_handle);
            printf("GPS info display started.\n");
            TERMINAL_VIEW_ADD_TEXT("GPS info display started.\n");
        }
    }
}

#ifndef CONFIG_IDF_TARGET_ESP32S2
void handle_ble_wardriving(int argc, char **argv) {
    bool stop_flag = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            stop_flag = true;
            break;
        }
    }

    if (stop_flag) {
        ble_stop();
        gps_manager_deinit(&g_gpsManager);
        csv_flush_buffer_to_file();
        csv_file_close();
        printf("BLE wardriving stopped.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE wardriving stopped.\n");
    } else {
        if (!g_gpsManager.isinitilized) {
            gps_manager_init(&g_gpsManager);
        }
        
        // Open CSV file for BLE wardriving
        esp_err_t err = csv_file_open("ble_wardriving");
        if (err != ESP_OK) {
            printf("Failed to open CSV file for BLE wardriving\n");
            return;
        }
        
        ble_register_handler(ble_wardriving_callback);
        ble_start_scanning();
        printf("BLE wardriving started.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE wardriving started.\n");
    }
}
#endif

void handle_pineap_detection(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        printf("Stopping PineAP detection...\n");
        TERMINAL_VIEW_ADD_TEXT("Stopping PineAP detection...\n");
        stop_pineap_detection();
        wifi_manager_stop_monitor_mode();
        pcap_file_close();
        return;
    }
    // Open PCAP file for logging detections
    int err = pcap_file_open("pineap_detection", PCAP_CAPTURE_WIFI);
    if (err != ESP_OK) {
        printf("Warning: Failed to open PCAP file for logging\n");
        TERMINAL_VIEW_ADD_TEXT("Warning: Failed to open PCAP file for logging\n");
    }
    
    // Start PineAP detection with channel hopping
    start_pineap_detection();
    wifi_manager_start_monitor_mode(wifi_pineap_detector_callback);
    
    printf("Monitoring for PineAP\n activity\n\n");
    TERMINAL_VIEW_ADD_TEXT("Monitoring for PineAP\n activity\n\n");
}

void register_commands() {
    register_command("help", handle_help);
    register_command("scanap", cmd_wifi_scan_start);
    register_command("scansta", handle_sta_scan);
    register_command("scanlocal", handle_ip_lookup);
    register_command("stopscan", cmd_wifi_scan_stop);
    register_command("attack", handle_attack_cmd);
    register_command("list", handle_list);
    register_command("beaconspam", handle_beaconspam);
    register_command("stopspam", handle_stop_spam);
    register_command("stopdeauth", handle_stop_deauth);
    register_command("select", handle_select_cmd);
    register_command("capture", handle_capture_scan);
    register_command("startportal", handle_start_portal);
    register_command("stopportal", stop_portal);
    register_command("connect", handle_wifi_connection);
    register_command("dialconnect", handle_dial_command);
    register_command("powerprinter", handle_printer_command);
    register_command("tplinktest", handle_tp_link_test);
    register_command("stop", handle_stop_flipper);
    register_command("reboot", handle_reboot);
    register_command("startwd", handle_startwd);
    register_command("gpsinfo", handle_gps_info);
    register_command("scanports", handle_scan_ports);
#ifndef CONFIG_IDF_TARGET_ESP32S2
    register_command("blescan", handle_ble_scan_cmd);
    register_command("blewardriving", handle_ble_wardriving);
#endif
#ifdef DEBUG
    register_command("crash", handle_crash); // For Debugging
#endif
    register_command("pineap", handle_pineap_detection);
    printf("Registered Commands\n");
    TERMINAL_VIEW_ADD_TEXT("Registered Commands\n");
}
