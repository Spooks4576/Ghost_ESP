#ifndef AP_MANAGER_H
#define AP_MANAGER_H

#include <esp_err.h>

// Initialize the Access Point, DNS server, and HTTP server
esp_err_t ap_manager_init(void);

// Deinitialize and stop the servers
void ap_manager_deinit(void);

// Function to add log messages
void ap_manager_add_log(const char *log_message);

// only indeded to be used after ap_manager_init has been called once
void ap_manager_stop_services();

// only indeded to be used after ap_manager_init has been called once
esp_err_t ap_manager_start_services();

#endif // AP_MANAGER_H