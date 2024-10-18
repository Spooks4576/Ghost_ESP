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
#include <wps/pixie.h>
#include "core/callbacks.h"
#include <esp_timer.h>
#include "vendor/pcap.h"
#include <sys/socket.h>
#include <netdb.h>
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
    ap_manager_add_log("WiFi scan started.\n");
    wifi_manager_start_scan();
    wifi_manager_print_scan_results_with_oui();
}

void cmd_wifi_scan_stop(int argc, char **argv) {
    pcap_file_close();
    ap_manager_add_log("WiFi scan stopped.\n");
}

void cmd_wifi_scan_results(int argc, char **argv) {
    wifi_manager_print_scan_results_with_oui();
    ap_manager_add_log("WiFi scan results displayed with OUI matching.\n");
}

void handle_list(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        cmd_wifi_scan_results(argc, argv);
        return;
    } 
    else if (argc > 1 && strcmp(argv[1], "-s") == 0)
    {
        wifi_manager_list_stations();
        ap_manager_add_log("Listed Stations...");
        return;
    }
    else {
        ap_manager_add_log("Usage: list -a (for Wi-Fi scan results)\n");
    }
}

void handle_beaconspam(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        ap_manager_add_log("Starting Random beacon spam...\n");
        wifi_manager_start_beacon(NULL);
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-rr") == 0) {
        ap_manager_add_log("Starting Rickroll beacon spam...\n");
        wifi_manager_start_beacon("RICKROLL");
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        ap_manager_add_log("Starting AP List beacon spam...\n");
        wifi_manager_start_beacon("APLISTMODE");
        return;
    }

    if (argc > 1)
    {
        wifi_manager_start_beacon(argv[1]);
        return;
    }
    else {
        ap_manager_add_log("Usage: beaconspam -r (for Beacon Spam Random)\n");
    }
}


void handle_stop_spam(int argc, char **argv)
{
    wifi_manager_stop_beacon();
    ap_manager_add_log("Beacon Spam Stopped...");
}

void handle_sta_scan(int argc, char **argv)
{
    wifi_manager_start_monitor_mode(wifi_stations_sniffer_callback);
    ap_manager_add_log("Started Station Scan...");
}


void handle_attack_cmd(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        ap_manager_add_log("Deauth Attack Starting...");
        wifi_manager_start_deauth();
        return;
    }
    else 
    {
        ap_manager_add_log("Usage: attack -d (for deauthing access points)\n");
    }
}


void handle_stop_deauth(int argc, char **argv)
{
    wifi_manager_stop_deauth();
    ap_manager_add_log("Deauthing Stopped....\n");
}


void handle_select_cmd(int argc, char **argv)
{
    if (argc != 3) {
        ap_manager_add_log("Invalid number of arguments. Usage: select -a <number>\n");
        return;
    }

    if (strcmp(argv[1], "-a") == 0) {
        char *endptr;
        
        int num = (int)strtol(argv[2], &endptr, 10);


        if (*endptr == '\0') {
            wifi_manager_select_ap(num);
        } else {
            ap_manager_add_log("Error: is not a valid number.\n");
        }
    } else {
        ap_manager_add_log("Invalid option. Usage: select -a <number>\n");
    }
}


void wps_phase_2()
{
    wifi_manager_stop_monitor_mode();

    for (int i = 0; i < MAX_WPS_NETWORKS; i++) {
        wps_network_t *network = &detected_wps_networks[i];


        bool is_valid_bssid = memcmp(network->bssid, "\x00\x00\x00\x00\x00\x00", 6) != 0;
        bool is_valid_ssid = strlen(network->ssid) > 0;
        
        if (network->wps_enabled && is_valid_bssid && is_valid_ssid && network->wps_mode == WPS_MODE_PIN) {
            ESP_LOGI("WPS_PHASE_2", "Valid network detected: SSID: %s, BSSID: %02x:%02x:%02x:%02x:%02x:%02x", 
                network->ssid,
                network->bssid[0], network->bssid[1], network->bssid[2], 
                network->bssid[3], network->bssid[4], network->bssid[5]);

            wps_start_connection(network->bssid);
            break;
        }
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
        ESP_LOGE("AppMain", "Failed to initialize DIAL client.");
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
        ESP_LOGE("Command Line", "Usage: %s <SSID> <PASSWORD>", argv[0]);
        return;
    }

    const char* ssid = argv[1];
    const char* password = argv[2];

    if (strlen(ssid) == 0 || strlen(password) == 0) {
        ESP_LOGE("Command Line", "SSID and password cannot be empty");
        return;
    }

    ESP_LOGI("Command Line", "Connecting to SSID: %s", ssid);
    
    wifi_manager_connect_wifi(ssid, password);
    xTaskCreate(screen_music_visualizer_task, "udp_server", 4096, NULL, 5, NULL);
}


void wps_test(int argc, char** argv)
{

    should_store_wps = 1;

    wifi_manager_start_monitor_mode(wifi_wps_detection_callback);

    const esp_timer_create_args_t stop_timer_args = {
        .callback = &wps_phase_2,
        .name = "stop_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&stop_timer_args, &stop_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(stop_timer, 15 * 1000000));

}


#ifndef CONFIG_IDF_TARGET_ESP32S2

void handle_ble_scan_cmd(int argc, char**argv)
{
    if (argc > 1 && strcmp(argv[1], "-f") == 0) {
        ap_manager_add_log("Starting Find the Flippers...\n");
        ble_start_find_flippers();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-ds") == 0) {
        ap_manager_add_log("Starting BLE Spam Detector...\n");
        ble_start_blespam_detector();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-a") == 0) {
        ap_manager_add_log("Starting AirTag Scanner...\n");
        ble_start_airtag_scanner();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-r") == 0) {
        ap_manager_add_log("Scanning for Raw Packets\n");
        ble_start_raw_ble_packetscan();
        return;
    }

    if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        ap_manager_add_log("Stopping BLE Scan...\n");
        ble_stop();
        return;
    }

    ap_manager_add_log("Invalid Command Syntax...");
}

#endif

void handle_set_setting(int argc, char **argv)
{
    if (argc < 3) {
        ap_manager_add_log("Error: Insufficient arguments. Expected 2 integers after the command.\n");
        return;
    }
    
    char *endptr1;
    int first_arg = strtol(argv[1], &endptr1, 10);
    
    
    if (*endptr1 != '\0') {
        ap_manager_add_log("Error: First argument is not a valid integer.\n");
        return;
    }

    
    char *endptr2;
    int second_arg = strtol(argv[2], &endptr2, 10);

    
    if (*endptr2 != '\0') {
        ap_manager_add_log("Error: Second argument is not a valid integer.\n");
        return;
    }

    int ActualSettingsIndex = first_arg;
    int ActualSettingsValue = second_arg;

    if (ActualSettingsIndex == 1) // RGB Mode
    {
        switch (ActualSettingsValue)
        {
        case 1:
        {
            settings_set_rgb_mode(&G_Settings, RGB_MODE_STEALTH);
            break;
        }
        case 2:
        {
            settings_set_rgb_mode(&G_Settings, RGB_MODE_NORMAL);
            break;
        }
        case 3:
        {
            settings_set_rgb_mode(&G_Settings, RGB_MODE_RAINBOW);
            break;
        }
        }
    }

    if (ActualSettingsIndex == 2)
    {
        switch (ActualSettingsValue)
        {
        case 1:
        {
            settings_set_channel_switch_delay(&G_Settings, 0.5);
            break;
        }
        case 2:
        {
            settings_set_channel_switch_delay(&G_Settings, 1);
            break;
        }
        case 3:
        {
            settings_set_channel_switch_delay(&G_Settings, 2);
            break;
        }
        case 4:
        {
            settings_set_channel_switch_delay(&G_Settings, 3);
            break;
        }
        case 5:
        {
            settings_set_channel_switch_delay(&G_Settings, 4);
            break;
        }
        }
    }

    if (ActualSettingsIndex == 3)
    {
        switch (ActualSettingsValue)
        {
        case 1:
        {
            settings_set_channel_hopping_enabled(&G_Settings, false);
            break;
        }
        case 2:
        {
            settings_set_channel_hopping_enabled(&G_Settings, true);
            break;
        }
        }
    }

    if (ActualSettingsIndex == 4)
    {
        switch (ActualSettingsValue)
        {
        case 1:
        {
            settings_set_random_ble_mac_enabled(&G_Settings, false);
            break;
        }
        case 2:
        {
            settings_set_random_ble_mac_enabled(&G_Settings, true);
            break;
        }
        }
    }

    settings_save(&G_Settings);
}


void handle_start_portal(int argc, char **argv)
{
    if (argc != 6 && argc != 4) {
        printf("Error: Incorrect number of arguments.\n");
        printf("Usage: %s <URL> <SSID> <Password> <AP_ssid> <DOMAIN>\n", argv[0]);
        printf("or\n");
        printf("Usage: %s <filepath> <APSSID> <Domain>\n", argv[0]);
        return;
    }

    if (argc == 6)
    {
        char *url = argv[1];
        char *ssid = argv[2];
        char *password = argv[3];
        char *ap_ssid = argv[4];
        char *domain = argv[5];


        if (ssid == NULL || ssid[0] == '\0') {
            printf("Error: SSID cannot be empty.\n");
            return;
        }

        if (password == NULL || password[0] == '\0') {
            printf("Error: Password cannot be empty.\n");
            return;
        }

        if (ap_ssid == NULL || ap_ssid[0] == '\0') {
            printf("Error: AP_ssid cannot be empty.\n");
            return;
        }

        if (url == NULL || url[0] == '\0') {
            printf("Error: url cannot be empty.\n");
            return;
        }

        if (domain == NULL || domain[0] == '\0') {
            printf("Error: domain cannot be empty.\n");
            return;
        }
        
        printf("Starting portal with SSID: %s, Password: %s, AP_ssid: %s\n", ssid, password, ap_ssid);

        
        wifi_manager_start_evil_portal(url, ssid, password, ap_ssid, domain);
    }
    else if (argc == 4)
    {
        char *filepath = argv[1];
        char *ap_ssid = argv[2];
        char *domain = argv[3];

        if (filepath == NULL || filepath[0] == '\0') {
            printf("Error: File Path cannot be empty.\n");
            return;
        }
        
        if (ap_ssid == NULL || ap_ssid[0] == '\0') {
            printf("Error: SSID cannot be empty.\n");
            return;
        }

        if (domain == NULL || domain[0] == '\0') {
            printf("Error: domain cannot be empty.\n");
            return;
        }

        printf("Starting portal with AP_ssid: %s\n", ap_ssid);


        wifi_manager_start_evil_portal(filepath, NULL, NULL, ap_ssid, domain);
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
        ESP_LOGE("TPLINK", "Usage: tp_link_test <on|off|loop>");
        return;
    }

    bool isloop = false;

    
    if (strcmp(argv[1], "loop") == 0) {
        isloop = true;
    } else if (strcmp(argv[1], "on") != 0 && strcmp(argv[1], "off") != 0) {
        ESP_LOGE("TPLINK", "Invalid argument. Use 'on', 'off', or 'loop'.");
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
            ESP_LOGE("TPLINK", "Command too large to encrypt");
            return;
        }

        encrypt_tp_link_command(command, encrypted_command, command_len);

        
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            ESP_LOGE("TPLINK", "Failed to create socket: errno %d", errno);
            return;
        }

        
        int broadcast = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

        
        int err = sendto(sock, encrypted_command, command_len, 0,
                         (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE("TPLINK", "Error occurred during sending: errno %d", errno);
            close(sock);
            return;
        }

        ESP_LOGI("TPLINK", "Broadcast message sent: %s", command);

        
        struct timeval timeout = {2, 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        uint8_t recv_buf[128];
        socklen_t addr_len = sizeof(dest_addr);
        int len = recvfrom(sock, recv_buf, sizeof(recv_buf) - 1, 0,
                           (struct sockaddr *)&dest_addr, &addr_len);
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                ESP_LOGW("TPLINK", "No response from any device");
            } else {
                ESP_LOGE("TPLINK", "Error receiving response: errno %d", errno);
            }
        } else {
            recv_buf[len] = 0;
            char decrypted_response[128];
            decrypt_tp_link_response(recv_buf, decrypted_response, len);
            decrypted_response[len] = 0;
            ESP_LOGI("TPLINK", "Response: %s", decrypted_response);
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

void print_art()
{
    printf("@@@@@@@@@@@@@@@@@@@@@@#SSS#@@@@@@@@@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@@@#&?*+;:*?**?&S#@@@@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@S*+::+;:,;??****??&#@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@S+;:;:+:,,,????***+**+?@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@?;::+;::,,,,*&&??***+?*,+@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@*::,;:,,,,,,,,*S&??***+;,,+@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@S::,,::,,,,,,,,*#&&&&;,,,,,,&@@@@@@@@@@@\n");
    printf("@@@@@@@@@@+::,,,::,,,,,,:?;:,;+,,,,,:,;@@@@@@@@@@@\n");
    printf("@@@@@@@@@@+::,,,:;:,,,,:;,,,,,,,,,,,::;@@@@@@@@@@@\n");
    printf("@@@@@@@@@@+:,,;&*;::::;:,,,,,,:;*&:,,,+@@@@@@@@@@@\n");
    printf("@@@@@@#@@@&::,*@@&S?*+::,:,:+&S@@@*,,,&@@@#@@@@@@@\n");
    printf("@@@@@@&#@@@+:,+@&;*+&#S+,+S#??+S@@+,:+@@@S&@@@@@@@\n");
    printf("@@@@@@&?&S##+;:*&S&+S@#;,;#@&??&@*:;+@#S&??@@@@@@@\n");
    printf("@@@@@@S**+?S+;+;;*&&&?++*;+&&S&*;++;+S?++*S@@@@@@@\n");
    printf("@@@@@@@S?+;?*;;*:,,,,:+&&&;:,,,,:*;;*?++?S@@@@@@@@\n");
    printf("@@@@@@@@@#&&#&+;:,,,,:?&&&?:,,,::++&#&&#@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@;,:,,,,::;::,,,,::+@@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@+;*+*;;;+;+;;;**?;*@@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@&++&&S*?&&*&&??SSS+**?@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@#?;:*+++++;;;;;?++??**?;:+#@@@@@@@@@@@@\n");
    printf("@@@@@@@@@#*;,,:*;;;:,:::::+..,++++:::S@@@@@@@@@@@@\n");
    printf("@@@@@@@@@S+::,,++;;;;++++++....,,+*;:#@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@&*:,,,::;;:;::;:,;;,,,:+:;@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@#?++&??*+++;+;+++***&+++?@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@@#?+;::,,,,,::;+?@@@@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@@@@@#S&?*?*?&S#@@@@@@@@@@@@@@@@@@@@\n");
    printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
}

void handle_crash(int argc, char **argv)
{
    int *ptr = NULL;
    *ptr = 42;
}

void handle_help(int argc, char **argv) {
    printf("\n Ghost ESP Commands:\n\n");

    //print_art();
    
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

    printf("setsetting\n");
    printf("    Description: Set various device settings.\n");
    printf("    Usage: setsetting <index> <value>\n");
    printf("    Arguments:\n");
    printf("        <index>: Setting index (1: RGB mode, 2: Channel switch delay, 3: Channel hopping, 4: Random BLE MAC)\n");
    printf("        <value>: Value corresponding to the setting (varies by setting index)\n");
    printf("        RGB Mode Values:\n");
    printf("            1: Stealth Mode\n");
    printf("            2: Normal Mode\n");
    printf("            3: Rainbow Mode\n");
    printf("        Channel Switch Delay Values:\n");
    printf("            1: 0.5s\n");
    printf("            2: 1s\n");
    printf("            3: 2s\n");
    printf("            4: 3s\n");
    printf("            5: 4s\n");
    printf("        Channel Hopping Values:\n");
    printf("            1: Disabled\n");
    printf("            2: Enabled\n");
    printf("        Random BLE MAC Values:\n");
    printf("            1: Disabled\n");
    printf("            2: Enabled\n\n");

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
    printf("        -stop   : Stops the active capture\n\n");


    printf("connect\n");
    printf("    Description: Connects to Specific WiFi Network\n");
    printf("    Usage: connect <SSID> <Password>\n");

    printf("dialconnect\n");
    printf("    Description: Cast a Random Youtube Video on all Smart TV's on your LAN (Requires You to Run Connect First)\n");
    printf("    Usage: dialconnect\n");


    printf("powerprinter\n");
    printf("    Description: Print Custom Text to a Printer on your LAN (Requires You to Run Connect First)\n");
    printf("    Usage: connect <Printer IP> <Text> <FontSize> <alignment>\n");
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
    register_command("setsetting", handle_set_setting);
    register_command("capture", handle_capture_scan);
    register_command("startportal", handle_start_portal);
    register_command("stopportal", stop_portal);
    register_command("connect", handle_wifi_connection);
    register_command("dialconnect", handle_dial_command);
    register_command("powerprinter", handle_printer_command);
    register_command("wpstest", wps_test);
    register_command("tplinktest", handle_tp_link_test);
    register_command("stop", handle_stop_flipper);
#ifdef DEBUG
    register_command("crash", handle_crash); // For Debugging
#endif
#ifndef CONFIG_IDF_TARGET_ESP32S2
    register_command("blescan", handle_ble_scan_cmd);
#endif
}
