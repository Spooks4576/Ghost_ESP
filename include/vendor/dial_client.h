#ifndef DIAL_CLIENT_H
#define DIAL_CLIENT_H

#include "esp_err.h"
#include "esp_log.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  char uniqueServiceName[128];
  char location[256];
  char usn[128];
  char screenID[64];
  char UUID[37];          // UUID for the session
  char YoutubeToken[128]; // YouTube lounge token
  char gsession[64];      // gsession ID
  char SID[64];           // Session ID (SID)
  char listID[64];        // Playlist list ID (LID)
  char RID[64];
} Device;

// DIAL Client functions
typedef struct {
  int socket_fd;
  struct sockaddr_in multicast_addr;
} DIALClient;

// Initialize the DIAL Client
esp_err_t dial_client_init(DIALClient *client);

// Discover devices on the network
esp_err_t dial_client_discover_devices(DIALClient *client, Device *devices,
                                       size_t max_devices,
                                       size_t *device_count);

// Parse SSDP Response
bool parse_ssdp_response(const char *response, Device *device);

// Deinitialize the DIAL Client
void dial_client_deinit(DIALClient *client);

#endif // DIAL_CLIENT_H