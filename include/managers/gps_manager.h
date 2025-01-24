#ifndef GPSMANAGER_H
#define GPSMANAGER_H

#include "vendor/GPS/MicroNMEA.h"
#include "vendor/GPS/gps_logger.h"
#include <esp_types.h>
#include <stdint.h>

extern nmea_parser_handle_t nmea_hdl;
extern gps_date_t cacheddate;

// Struct definition for GPSManager
typedef struct {
    bool isinitilized;
} GPSManager;

// Function prototypes
void gps_manager_init(GPSManager *manager);
void gps_manager_deinit(GPSManager *manager);
esp_err_t gps_manager_log_wardriving_data(wardriving_data_t *data);
bool gps_is_timeout_detected(void);
GPSManager g_gpsManager;

#endif // GPSMANAGER_H