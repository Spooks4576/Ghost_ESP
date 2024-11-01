#include <stdio.h>
#include <string.h>
#include "managers/gps_manager.h"


void gps_manager_init(GPSManager* manager) {
    microNMEA_init(&manager->nmea, manager->nmeaBuffer, sizeof(manager->nmeaBuffer));
}


void gps_manager_process_char(GPSManager* manager, char c) {
    if (microNMEA_process(&manager->nmea, c)) {
        gps_manager_log_values(manager);
    }
}


void gps_manager_log_values(GPSManager* manager) {
    MicroNMEA* nmea = &manager->nmea;

    printf("Talker ID: %c\n", nmea->talkerID);
    printf("Message ID: %s\n", nmea->messageID);
    printf("Latitude: %ld (degrees * 10^6)\n", nmea->latitude);
    printf("Longitude: %ld (degrees * 10^6)\n", nmea->longitude);
    printf("Altitude: %ld meters\n", nmea->altitude);
    printf("Speed: %ld knots\n", nmea->speed);
    printf("Course: %ld degrees\n", nmea->course);
    printf("Number of Satellites: %u\n", nmea->numSat);
    printf("HDOP: %u\n", nmea->hdop);
    printf("Date (YYYY-MM-DD): %04u-%02u-%02u\n", nmea->year, nmea->month, nmea->day);
    printf("Time (HH:MM:SS): %02u:%02u:%02u.%02u\n", nmea->hour, nmea->minute, nmea->second, nmea->hundredths);
    printf("Fix Validity: %s\n", nmea->isValid ? "Valid" : "Invalid");
    printf("Altitude Validity: %s\n", nmea->altitudeValid ? "Valid" : "Invalid");
    printf("----------\n");
}