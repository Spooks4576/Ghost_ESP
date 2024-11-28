#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "sys/time.h"
#include "driver/uart.h"
#include <errno.h>
#include <sys/stat.h>
#include "vendor/GPS/gps_logger.h"
#include "managers/gps_manager.h"
#include "managers/sd_card_manager.h"
#include "vendor/GPS/MicroNMEA.h"
#include "core/callbacks.h"

static const char *GPS_TAG = "GPS";
static const char *CSV_TAG = "CSV";

static bool is_valid_date(const gps_date_t* date);

#define CSV_BUFFER_SIZE 512

static FILE *csv_file = NULL;
static char csv_buffer[BUFFER_SIZE];
static size_t buffer_offset = 0;

esp_err_t csv_write_header(FILE* f) {
    // Wigle pre-header
    const char* pre_header = "WigleWifi-1.6,appRelease=1.0,model=ESP32,release=1.0,device=GhostESP,"
                            "display=NONE,board=ESP32,brand=Espressif,star=Sol,body=3,subBody=0\n";
    
    // Wigle main header
    const char* header = "MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,"
                        "CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type\n";

    if (f == NULL) {
        const char* mark_begin = "[BUF/BEGIN]";
        const char* mark_close = "[BUF/CLOSE]";
        uart_write_bytes(UART_NUM_0, mark_begin, strlen(mark_begin));
        uart_write_bytes(UART_NUM_0, pre_header, strlen(pre_header));
        uart_write_bytes(UART_NUM_0, header, strlen(header));
        uart_write_bytes(UART_NUM_0, mark_close, strlen(mark_close));
        uart_write_bytes(UART_NUM_0, "\n", 1);
        return ESP_OK;
    } else {
        size_t written = fwrite(pre_header, 1, strlen(pre_header), f);
        if (written != strlen(pre_header)) {
            return ESP_FAIL;
        }
        written = fwrite(header, 1, strlen(header), f);
        return (written == strlen(header)) ? ESP_OK : ESP_FAIL;
    }
}

void get_next_csv_file_name(char *file_name_buffer, const char* base_name) {
    int next_index = get_next_csv_file_index(base_name);  // Modify this to be CSV specific
    snprintf(file_name_buffer, MAX_FILE_NAME_LENGTH, "/mnt/ghostesp/gps/%s_%d.csv", base_name, next_index);
}

esp_err_t csv_file_open(const char* base_file_name) {
    char file_name[MAX_FILE_NAME_LENGTH];


    if (sd_card_exists("/mnt/ghostesp/gps"))
    {
        get_next_csv_file_name(file_name, base_file_name);
        csv_file = fopen(file_name, "w");
    }

    esp_err_t ret = csv_write_header(csv_file);
    if (ret != ESP_OK) {
        printf("Failed to write CSV header.");
        fclose(csv_file);
        csv_file = NULL;
        return ret;
    }

    printf("Storage: Created new log file: %s\n", file_name);
    return ESP_OK;
}

esp_err_t csv_write_data_to_buffer(wardriving_data_t *data) {
    if (!data) return ESP_ERR_INVALID_ARG;
    
    // Get GPS data from the global handle
    gps_t* gps = &((esp_gps_t*)nmea_hdl)->parent;
    if (!gps) return ESP_ERR_INVALID_STATE;
    
    char timestamp[35];
    if (!is_valid_date(&gps->date) || 
        gps->tim.hour > 23 || gps->tim.minute > 59 || gps->tim.second > 59) {
        // Use a fallback timestamp or return error
        ESP_LOGW(GPS_TAG, "Invalid date/time for CSV entry");
        return ESP_ERR_INVALID_STATE;
    }
    
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             gps_get_absolute_year(gps->date.year), 
             gps->date.month, gps->date.day,
             gps->tim.hour, gps->tim.minute, gps->tim.second, 
             gps->tim.thousand);

    char data_line[CSV_BUFFER_SIZE];
    int len = snprintf(data_line, CSV_BUFFER_SIZE, 
        "%s,%s,%s,%s,%d,%d,%d,%.6f,%.6f,%.1f,%.1f,WIFI\n",
        data->bssid,                    // MAC
        data->ssid,                     // SSID
        data->encryption_type,          // AuthMode (WEP, WPA, etc)
        timestamp,                      // FirstSeen
        data->channel,                  // Channel
        (2412 + (data->channel-1)*5),  // Frequency (MHz)
        data->rssi,                     // RSSI
        data->latitude,                 // CurrentLatitude
        data->longitude,                // CurrentLongitude
        data->altitude,                 // AltitudeMeters
        data->accuracy                 // AccuracyMeters (HDOP * 5m)
    );

    if (buffer_offset + len > BUFFER_SIZE) {
        printf("Buffer full, flushing to file.\n");
        esp_err_t ret = csv_flush_buffer_to_file();
        if (ret != ESP_OK) {
            return ret;
        }
    }

    memcpy(csv_buffer + buffer_offset, data_line, len);
    buffer_offset += len;

    return ESP_OK;
}

esp_err_t csv_flush_buffer_to_file() {
    if (csv_file == NULL) {
        printf("Storage: No SD card found, logging to serial\n");
        const char* mark_begin = "[BUF/BEGIN]";
        const char* mark_close = "[BUF/CLOSE]";

        uart_write_bytes(UART_NUM_0, mark_begin, strlen(mark_begin));
        uart_write_bytes(UART_NUM_0, csv_buffer, buffer_offset);
        uart_write_bytes(UART_NUM_0, mark_close, strlen(mark_close));
        uart_write_bytes(UART_NUM_0, "\n", 1);

        buffer_offset = 0;
        return ESP_OK;
    }

    size_t written = fwrite(csv_buffer, 1, buffer_offset, csv_file);
    if (written != buffer_offset) {
        printf("Failed to write buffer to file.\n");
        return ESP_FAIL;
    }

    printf("Flushed %zu bytes to CSV file.\n", buffer_offset);
    buffer_offset = 0;

    return ESP_OK;
}

void csv_file_close() {
    if (csv_file != NULL) {
        if (buffer_offset > 0) {
            printf("Flushing remaining buffer before closing file.\n");
            csv_flush_buffer_to_file();
        }
        fclose(csv_file);
        csv_file = NULL;
        printf("CSV file closed.\n");
    }
}

static bool is_valid_date(const gps_date_t* date) {
    if (!date) return false;
    
    // Check year (0-99 represents 2000-2099)
    if (!gps_is_valid_year(date->year)) return false;
    
    // Check month (1-12)
    if (date->month < 1 || date->month > 12) return false;
    
    // Check day (1-31 depending on month)
    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Adjust February for leap years
    uint16_t absolute_year = gps_get_absolute_year(date->year);
    if ((absolute_year % 4 == 0 && absolute_year % 100 != 0) || 
        (absolute_year % 400 == 0)) {
        days_in_month[1] = 29;
    }
    
    if (date->day < 1 || date->day > days_in_month[date->month - 1]) return false;
    
    return true;
}