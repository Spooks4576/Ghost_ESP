#ifndef GPSMANAGER_H
#define GPSMANAGER_H

#include <stdint.h>
#include "vendor/GPS/MicroNMEA.h"
#include "vendor/GPS/gps_logger.h"
#include <esp_types.h>

// Struct definition for GPSManager
typedef struct {
    MicroNMEA nmea;
    char nmeaBuffer[100];  // Buffer size for NMEA sentences
    bool isinitilized;
} GPSManager;

// Function prototypes
void gps_manager_init(GPSManager* manager);
void gps_manager_process_char(GPSManager* manager, char c);
void gps_manager_log_values(GPSManager* manager);
void gps_manager_deinit(GPSManager* manager);
esp_err_t gps_manager_log_wardriving_data(wardriving_data_t* data);

GPSManager g_gpsManager;

#endif // GPSMANAGER_H