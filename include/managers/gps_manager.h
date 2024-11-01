#ifndef GPSMANAGER_H
#define GPSMANAGER_H

#include <stdint.h>
#include "vendor/GPS/MicroNMEA.h"

// Struct definition for GPSManager
typedef struct {
    MicroNMEA nmea;
    char nmeaBuffer[100];  // Buffer size for NMEA sentences
} GPSManager;

// Function prototypes
void gps_manager_init(GPSManager* manager);
void gps_manager_process_char(GPSManager* manager, char c);
void gps_manager_log_values(GPSManager* manager);


GPSManager g_gpsManager;

#endif // GPSMANAGER_H