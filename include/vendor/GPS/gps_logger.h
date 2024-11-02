#ifndef WARDRIVING_CSV_H
#define WARDRIVING_CSV_H

#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"

// Define constants
#define MAX_FILE_NAME_LENGTH 64
#define BUFFER_SIZE 4096

// Define the wardriving data structure
typedef struct {
    char ssid[32];
    char bssid[18];
    int rssi;
    int channel;
    double latitude;
    double longitude;
    char encryption_type[8];  // WPA2, WPA, WEP, or OPEN
} wardriving_data_t;

// Function prototypes
esp_err_t csv_write_header(FILE* f);
void get_next_csv_file_name(char *file_name_buffer, const char* base_name);
int get_next_csv_file_index(const char* base_name);
esp_err_t csv_file_open(const char* base_file_name);
esp_err_t csv_write_data_to_buffer(wardriving_data_t *data);
esp_err_t csv_flush_buffer_to_file();
void csv_file_close();

#endif // WARDRIVING_CSV_H