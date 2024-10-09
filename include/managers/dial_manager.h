#ifndef DIAL_MANAGER_H
#define DIAL_MANAGER_H

#include "vendor/dial_client.h"
#include "esp_err.h"
#include "esp_http_client.h"

// Enum for supported apps
typedef enum {
    APP_YOUTUBE,
    APP_NETFLIX,
    APP_UNKNOWN
} DIALAppType;

// DIAL Manager structure
typedef struct {
    DIALClient *client;
} DIALManager;

// Initialize DIAL Manager
esp_err_t dial_manager_init(DIALManager *manager, DIALClient *client);

// Check the app status (YouTube, Netflix, etc.)
esp_err_t check_app_status(DIALManager *manager, DIALAppType app, const char *appUrl, Device *device);

// Helper to extract IP and port from app URL
esp_err_t extract_ip_and_port(const char *url, char *ip_out, uint16_t *port_out);

// Helper to construct app-specific path for checking status
const char* get_app_path(DIALAppType app);

bool launch_app(DIALManager *manager, DIALAppType app, const char *appUrl);

void explore_network(DIALManager *manager);

char* get_dial_application_url(const char *location_url);

#endif // DIAL_MANAGER_H