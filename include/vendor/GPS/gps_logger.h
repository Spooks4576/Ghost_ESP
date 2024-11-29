#ifndef WARDRIVING_CSV_H
#define WARDRIVING_CSV_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "esp_err.h"
#include "vendor/GPS/MicroNMEA.h"

// Define constants
#define MAX_FILE_NAME_LENGTH 64
#define BUFFER_SIZE 4096
#define MIN_SPEED_THRESHOLD 0.1     // Minimum 0.1 m/s (~0.36 km/h)
#define MAX_SPEED_THRESHOLD 340.0   // Maximum 340 m/s (~1224 km/h)

// wardriving data structure
typedef struct {
    char ssid[32];
    char bssid[18];
    int rssi;
    int channel;
    double latitude;
    double longitude;
    double altitude;
    double accuracy;
    char encryption_type[8];  // WPA2, WPA, WEP, or OPEN
    
    // New optional GPS quality metrics
    struct {
        uint8_t satellites_used;
        float hdop;
        float speed;          // in m/s
        float course;         // in degrees
        uint8_t fix_quality;  // 0=no fix, 1=GPS fix, 2=DGPS fix
        float magnetic_var;   // magnetic variation
        float geoid_sep;      // geoid separation
        bool has_valid_fix;   // indicates if GPS data is valid
    } gps_quality;
    
} wardriving_data_t;

// Function prototypes
esp_err_t csv_write_header(FILE* f);
void get_next_csv_file_name(char *file_name_buffer, const char* base_name);
int get_next_csv_file_index(const char* base_name);
esp_err_t csv_file_open(const char* base_file_name);
esp_err_t csv_write_data_to_buffer(wardriving_data_t *data);
esp_err_t csv_flush_buffer_to_file();
void csv_file_close();

// New helper functions
void populate_gps_quality_data(wardriving_data_t *data, const gps_t *gps);
const char* get_gps_quality_string(const wardriving_data_t *data);
void gps_info_display_task(void *pvParameters);

#endif // WARDRIVING_CSV_H