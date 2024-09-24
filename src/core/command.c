// command.c

#include "core/command.h"
#include "managers/wifi_manager.h"
#include <stdlib.h>
#include <string.h>

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
}

void cmd_wifi_scan_stop(int argc, char **argv) {
    wifi_manager_stop_scan();
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
            printf("Error: '%s' is not a valid number.\n", argv[2]);
        }
    } else {
        printf("Invalid option. Usage: select -a <number>\n");
    }
}

void register_wifi_commands() {
    register_command("scanap", cmd_wifi_scan_start);
    register_command("scansta", handle_sta_scan);
    register_command("stopscan", cmd_wifi_scan_stop);
    register_command("attack", handle_attack_cmd);
    register_command("list", handle_list);
    register_command("beaconspam", handle_beaconspam);
    register_command("stopspam", handle_stop_spam);
    register_command("stopdeauth", handle_stop_deauth);
    register_command("select", handle_select_cmd);
}
