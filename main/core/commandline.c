// command.c

#include "core/commandline.h"
#include "managers/wifi_manager.h"
#include "managers/rgb_manager.h"
#include "managers/ap_manager.h"
#include "managers/ble_manager.h"
#include "managers/settings_manager.h"
#include <stdlib.h>
#include <string.h>
#include <vendor/dial_client.h>
#include "managers/dial_manager.h"
#include "core/callbacks.h"
#include <esp_timer.h>
#include "vendor/pcap.h"
#include <sys/socket.h>
#include <netdb.h>
#include <managers/gps_manager.h>
#include "vendor/printer.h"

static Command *command_list_head = NULL;

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
    wifi_manager_start_scan();
    wifi_manager_print_scan_results_with_oui();
}

void cmd_wifi_scan_stop(int argc, char **argv) {
    pcap_file_close();
    printf("WiFi scan stopped.\n");
}

void cmd_wifi_scan_results(int argc, char **argv) {
    wifi_manager_print_scan_results_with_oui();
    printf("WiFi scan results displayed with OUI matching.\n");
}

void handle_list(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        cmd_wifi_scan_results(argc, argv);
        return;
    } 
    else if (argc > 1 && strcmp(argv[1], "-s") == 0)
    {
        wifi_manager_list_stations();
        printf("Listed Stations...");
        return;
    }
    else {
        printf("Usage: list -a (for Wi-Fi scan results)\n");
    }
}

void handle_beaconspam(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        printf("Starting Random beacon spam...\n");
        wifi_manager_start_beacon(NULL);
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-rr") == 0) {
        printf("Starting Rickroll beacon spam...\n");
        wifi_manager_start_beacon("RICKROLL");
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        printf("Starting AP List beacon spam...\n");
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
    }
}


void handle_stop_spam(int argc, char **argv)
{
    wifi_manager_stop_beacon();
    printf("Beacon Spam Stopped...");
}

void handle_sta_scan(int argc, char **argv)
{
    wifi_manager_start_monitor_mode(wifi_stations_sniffer_callback);
    printf("Started Station Scan...");
}


void handle_attack_cmd(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        printf("Deauth Attack Starting...");
        wifi_manager_start_deauth();
        return;
    }
    else 
    {
        printf("Usage: attack -d (for deauthing access points)\n");
    }
}


void handle_stop_deauth(int argc, char **argv)
{
    wifi_manager_stop_deauth();
    printf("Deauthing Stopped....\n");
}


void handle_select_cmd(int argc, char **argv)
{
    if (argc != 3) {
        printf("Invalid number of arguments. Usage: select -a <number>\n");
        return;
    }

    if (strcmp(argv[1], "-a") == 0) {
        char *endptr;
        
        int num = (int)strtol(argv[2], &endptr, 10);


        if (*endptr == '\0') {
            wifi_manager_select_ap(num);
        } else {
            printf("Error: is not a valid number.\n");
        }
    } else {
        printf("Invalid option. Usage: select -a <number>\n");
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
    }

    vTaskDelete(NULL);
}

void handle_stop_flipper(int argc, char** argv)
{
    wifi_manager_stop_deauth();
#ifndef CONFIG_IDF_TARGET_ESP32S2
    ble_stop();
#endif
}

void handle_dial_command(int argc, char** argv)
{
    xTaskCreate(&discover_task, "discover_task", 10240, NULL, 5, NULL);
}

void handle_wifi_connection(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <SSID> <PASSWORD>\n", argv[0]);
        return;
    }

    const char* ssid = argv[1];
    const char* password = argv[2];

    if (strlen(ssid) == 0 || strlen(password) == 0) {
        printf("SSID and password cannot be empty");
        return;
    }

    printf("Connecting to SSID: %s\n", ssid);
    
    wifi_manager_connect_wifi(ssid, password);

    if (VisualizerHandle == NULL)
    {
#ifdef WITH_SCREEN
    xTaskCreate(screen_music_visualizer_task, "udp_server", 4096, NULL, 5, &VisualizerHandle);
#else
    xTaskCreate(animate_led_based_on_amplitude, "udp_server", 4096, NULL, 5, &VisualizerHandle);
#endif
    }
}


#ifndef CONFIG_IDF_TARGET_ESP32S2

void handle_ble_scan_cmd(int argc, char**argv)
{
    if (argc > 1 && strcmp(argv[1], "-f") == 0) {
        printf("Starting Find the Flippers...\n");
        ble_start_find_flippers();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-ds") == 0) {
        printf("Starting BLE Spam Detector...\n");
        ble_start_blespam_detector();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        printf("Starting AirTag Scanner...\n");
        ble_start_airtag_scanner();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        printf("Scanning for Raw Packets\n");
        ble_start_raw_ble_packetscan();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        printf("Stopping BLE Scan...\n");
        ble_stop();
        return;
    }

    printf("Invalid Command Syntax...");
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
        printf("Usage: %s <URL> <SSID> <Password> <AP_ssid> <DOMAIN>\n", argv[0]);
        printf("or\n");
        printf("Usage: %s <filepath> <APSSID> <Domain>\n", argv[0]);
        return;
    }


    if (url == NULL || url[0] == '\0') {
        printf("Error: URL or File Path cannot be empty.\n");
        return;
    }

    if (ap_ssid == NULL || ap_ssid[0] == '\0') {
        printf("Error: AP SSID cannot be empty.\n");
        return;
    }

    if (domain == NULL || domain[0] == '\0') {
        printf("Error: Domain cannot be empty.\n");
        return;
    }

    if (ssid && ssid[0] != '\0' && password && password[0] != '\0' && !offlinemode) {
        printf("Starting portal with SSID: %s, Password: %s, AP_SSID: %s, Domain: %s\n", ssid, password, ap_ssid, domain);
        wifi_manager_start_evil_portal(url, ssid, password, ap_ssid, domain);
    }
    else if (offlinemode){
        printf("Starting portal in offline mode with AP_SSID: %s, Domain: %s\n", ap_ssid, domain);
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

void handle_tp_link_test(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: tp_link_test <on|off|loop>\n");
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
            close(sock);
            return;
        }

        printf("Broadcast message sent: %s\n", command);

        
        struct timeval timeout = {2, 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        uint8_t recv_buf[128];
        socklen_t addr_len = sizeof(dest_addr);
        int len = recvfrom(sock, recv_buf, sizeof(recv_buf) - 1, 0,
                           (struct sockaddr *)&dest_addr, &addr_len);
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("No response from any device\n");
            } else {
                printf("Error receiving response: errno %d\n", errno);
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

void handle_capture_scan(int argc, char** argv)
{
    if (argc != 2) {
        printf("Error: Incorrect number of arguments.\n");
        return;
    }

    char *capturetype = argv[1];

    if (capturetype == NULL || capturetype[0] == '\0') {
        printf("Error: Capture Type cannot be empty.\n");
        return;
    }

    if (strcmp(capturetype, "-probe") == 0)
    {
        int err = pcap_file_open("probescan");
        
        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_probe_scan_callback);
    }

    if (strcmp(capturetype, "-deauth") == 0)
    {
        int err = pcap_file_open("deauthscan");
        
        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_deauth_scan_callback);
    }

    if (strcmp(capturetype, "-beacon") == 0)
    {
        int err = pcap_file_open("beaconscan");
        
        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_beacon_scan_callback);
    }

    if (strcmp(capturetype, "-raw") == 0)
    {
        int err = pcap_file_open("rawscan");
        
        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_raw_scan_callback);
    }

    if (strcmp(capturetype, "-eapol") == 0)
    {
        int err = pcap_file_open("eapolscan");
        
        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_eapol_scan_callback);
    }

    if (strcmp(capturetype, "-pwn") == 0)
    {
        int err = pcap_file_open("pwnscan");
        
        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_pwn_scan_callback);
    }

    if (strcmp(capturetype, "-wps") == 0)
    {
        int err = pcap_file_open("wpsscan");

        should_store_wps = 0;
        
        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            return;
        }
        wifi_manager_start_monitor_mode(wifi_wps_detection_callback);
    }

    if (strcmp(capturetype, "-stop") == 0)
    {
        wifi_manager_stop_monitor_mode();
        pcap_file_close();
    }
}

void stop_portal(int argc, char **argv)
{
    wifi_manager_stop_evil_portal();
}

void handle_reboot(int argc, char **argv)
{
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

#ifdef CONFIG_HAS_GPS

    if (stop_flag) {
        gps_manager_deinit(&g_gpsManager);
        wifi_manager_stop_monitor_mode();
        printf("Wardriving stopped.\n");
    } else {
        gps_manager_init(&g_gpsManager);
        wifi_manager_start_monitor_mode(wardriving_scan_callback);
        printf("Wardriving started.\n");
    }
#else 
    printf("Your ESP / Build Does not Support GPS\n");
#endif
}


void handle_crash(int argc, char **argv)
{
    int *ptr = NULL;
    *ptr = 42;
}

void handle_help(int argc, char **argv) {
    printf("\n Ghost ESP Commands:\n\n");
    
    printf("help\n");
    printf("    Description: Display this help message.\n");
    printf("    Usage: help\n\n");

    printf("scanap\n");
    printf("    Description: Start a Wi-Fi access point (AP) scan.\n");
    printf("    Usage: scanap\n\n");

    printf("scansta\n");
    printf("    Description: Start scanning for Wi-Fi stations.\n");
    printf("    Usage: scansta\n\n");

    printf("stopscan\n");
    printf("    Description: Stop any ongoing Wi-Fi scan.\n");
    printf("    Usage: stopscan\n\n");

    printf("attack\n");
    printf("    Description: Launch an attack (e.g., deauthentication attack).\n");
    printf("    Usage: attack -d\n");
    printf("    Arguments:\n");
    printf("        -d  : Start deauth attack\n\n");

    printf("list\n");
    printf("    Description: List Wi-Fi scan results or connected stations.\n");
    printf("    Usage: list -a | list -s\n");
    printf("    Arguments:\n");
    printf("        -a  : Show access points from Wi-Fi scan\n");
    printf("        -s  : List connected stations\n\n");

    printf("beaconspam\n");
    printf("    Description: Start beacon spam with different modes.\n");
    printf("    Usage: beaconspam [OPTION]\n");
    printf("    Arguments:\n");
    printf("        -r   : Start random beacon spam\n");
    printf("        -rr  : Start Rickroll beacon spam\n");
    printf("        -l   : Start AP List beacon spam\n");
    printf("        [SSID]: Use specified SSID for beacon spam\n\n");

    printf("stopspam\n");
    printf("    Description: Stop ongoing beacon spam.\n");
    printf("    Usage: stopspam\n\n");

    printf("stopdeauth\n");
    printf("    Description: Stop ongoing deauthentication attack.\n");
    printf("    Usage: stopdeauth\n\n");

    printf("select\n");
    printf("    Description: Select an access point by index from the scan results.\n");
    printf("    Usage: select -a <number>\n");
    printf("    Arguments:\n");
    printf("        -a  : AP selection index (must be a valid number)\n\n");

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

    printf("stopportal\n");
    printf("    Description: Stop Evil Portal\n");
    printf("    Usage: stopportal\n\n");

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


    printf("connect\n");
    printf("    Description: Connects to Specific WiFi Network\n");
    printf("    Usage: connect <SSID> <Password>\n");

    printf("dialconnect\n");
    printf("    Description: Cast a Random Youtube Video on all Smart TV's on your LAN (Requires You to Run Connect First)\n");
    printf("    Usage: dialconnect\n");


    printf("powerprinter\n");
    printf("    Description: Print Custom Text to a Printer on your LAN (Requires You to Run Connect First)\n");
    printf("    Usage: powerprinter <Printer IP> <Text> <FontSize> <alignment>\n");
    printf("    aligment options: CM = Center Middle, TL = Top Left, TR = Top Right, BR = Bottom Right, BL = Bottom Left\n\n");
}

void register_commands() {
    register_command("help", handle_help);
    register_command("scanap", cmd_wifi_scan_start);
    register_command("scansta", handle_sta_scan);
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
#ifdef DEBUG
    register_command("crash", handle_crash); // For Debugging
#endif
#ifndef CONFIG_IDF_TARGET_ESP32S2
    register_command("blescan", handle_ble_scan_cmd);
#endif
    printf("Registered Commands\n");
}
