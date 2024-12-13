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
#include "managers/views/terminal_screen.h"

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

    // Add Bluetooth header after WiFi header
    const char* ble_header = "# Bluetooth\n"
                            "MAC,Name,RSSI,FirstSeen,CurrentLatitude,CurrentLongitude,"
                            "AltitudeMeters,AccuracyMeters,Type\n";

    if (f == NULL) {
        const char* mark_begin = "[BUF/BEGIN]";
        const char* mark_close = "[BUF/CLOSE]";
        uart_write_bytes(UART_NUM_0, mark_begin, strlen(mark_begin));
        uart_write_bytes(UART_NUM_0, pre_header, strlen(pre_header));
        uart_write_bytes(UART_NUM_0, header, strlen(header));
        uart_write_bytes(UART_NUM_0, ble_header, strlen(ble_header));
        uart_write_bytes(UART_NUM_0, mark_close, strlen(mark_close));
        uart_write_bytes(UART_NUM_0, "\n", 1);
        return ESP_OK;
    } else {
        size_t written = fwrite(pre_header, 1, strlen(pre_header), f);
        if (written != strlen(pre_header)) {
            return ESP_FAIL;
        }
        written = fwrite(header, 1, strlen(header), f);
        if (written != strlen(header)) {
            return ESP_FAIL;
        }
        written = fwrite(ble_header, 1, strlen(ble_header), f);
        if (written != strlen(ble_header)) {
            return ESP_FAIL;
        }
        return ESP_OK;
    }
}

void get_next_csv_file_name(char *file_name_buffer, const char* base_name) {
    int next_index = get_next_csv_file_index(base_name);
    snprintf(file_name_buffer, GPS_MAX_FILE_NAME_LENGTH, "/mnt/ghostesp/gps/%s_%d.csv", base_name, next_index);
}

esp_err_t csv_file_open(const char* base_file_name) {
    char file_name[GPS_MAX_FILE_NAME_LENGTH];


    if (sd_card_exists("/mnt/ghostesp/gps"))
    {
        get_next_csv_file_name(file_name, base_file_name);
        csv_file = fopen(file_name, "w");
    }

    esp_err_t ret = csv_write_header(csv_file);
    if (ret != ESP_OK) {
        printf("Failed to write CSV header.");
        TERMINAL_VIEW_ADD_TEXT("Failed to write CSV header.");
        fclose(csv_file);
        csv_file = NULL;
        return ret;
    }

    printf("Storage: Created new log file: %s\n", file_name);
    TERMINAL_VIEW_ADD_TEXT("Storage: Created new log file: %s\n", file_name);
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
        ESP_LOGW(GPS_TAG, "Invalid date/time for CSV entry");
        return ESP_ERR_INVALID_STATE;
    }
    
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             gps_get_absolute_year(gps->date.year), 
             gps->date.month, gps->date.day,
             gps->tim.hour, gps->tim.minute, gps->tim.second, 
             gps->tim.thousand);

    char data_line[CSV_BUFFER_SIZE];
    int len;

    if (data->ble_data.is_ble_device) {
        // BLE device format - matches WiGLE Bluetooth format
        len = snprintf(data_line, CSV_BUFFER_SIZE,
            "%s,%s,%d,%s,%.6f,%.6f,%.1f,%.1f,%s\n",
            data->ble_data.ble_mac,
            data->ble_data.ble_name[0] ? data->ble_data.ble_name : "[Unknown]",
            data->ble_data.ble_rssi,
            timestamp,
            data->latitude,
            data->longitude,
            data->altitude,
            data->accuracy,
            "BLE");  // Fixed type for BLE devices
    } else {
        // WiFi device format
        int frequency = data->channel > 14 ? 
            5000 + (data->channel * 5) : 2407 + (data->channel * 5);
            
        len = snprintf(data_line, CSV_BUFFER_SIZE,
            "%s,%s,%s,%s,%d,%d,%d,%.6f,%.6f,%.1f,%.1f,WIFI\n",
            data->bssid,
            data->ssid,
            data->encryption_type,
            timestamp,
            data->channel,
            frequency,
            data->rssi,
            data->latitude,
            data->longitude,
            data->altitude,
            data->accuracy);
    }

    if (len < 0 || len >= CSV_BUFFER_SIZE) {
        ESP_LOGE(CSV_TAG, "Buffer overflow prevented");
        return ESP_ERR_NO_MEM;
    }

    // Check if buffer needs flushing
    if (buffer_offset + len >= BUFFER_SIZE) {
        esp_err_t err = csv_flush_buffer_to_file();
        if (err != ESP_OK) {
            return err;
        }
        buffer_offset = 0;
    }

    // For BLE entries, ensure we're past the headers
    if (data->ble_data.is_ble_device && buffer_offset == 0) {
        // Skip to Bluetooth section if this is the first entry after a flush
        const char* ble_section = "# Bluetooth\n";
        size_t section_len = strlen(ble_section);
        memcpy(csv_buffer, ble_section, section_len);
        buffer_offset = section_len;
    }

    memcpy(csv_buffer + buffer_offset, data_line, len);
    buffer_offset += len;

    return ESP_OK;
}

esp_err_t csv_flush_buffer_to_file() {
    if (csv_file == NULL) {
        printf("Storage: No SD card found, logging to serial\n");
        TERMINAL_VIEW_ADD_TEXT("Storage: No SD card found, logging to serial\n");
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
        TERMINAL_VIEW_ADD_TEXT("Failed to write buffer to file.\n");
        return ESP_FAIL;
    }

    printf("Flushed %zu bytes to CSV file.\n", buffer_offset);
    TERMINAL_VIEW_ADD_TEXT("Flushed %zu bytes to CSV file.\n", buffer_offset);
    buffer_offset = 0;

    return ESP_OK;
}

void csv_file_close() {
    if (csv_file != NULL) {
        if (buffer_offset > 0) {
            printf("Flushing remaining buffer before closing file.\n");
            TERMINAL_VIEW_ADD_TEXT("Flushing remaining buffer before closing file.\n");
            csv_flush_buffer_to_file();
        }
        fclose(csv_file);
        csv_file = NULL;
        printf("CSV file closed.\n");
        TERMINAL_VIEW_ADD_TEXT("CSV file closed.\n");
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

void populate_gps_quality_data(wardriving_data_t *data, const gps_t *gps) {
    if (!data || !gps) return;
    
    data->gps_quality.satellites_used = gps->sats_in_use;
    data->gps_quality.hdop = gps->dop_h;
    data->gps_quality.speed = gps->speed;
    data->gps_quality.course = gps->cog;
    data->gps_quality.fix_quality = gps->fix;
    data->gps_quality.magnetic_var = gps->variation;
    data->gps_quality.has_valid_fix = gps->valid;
    
    // Calculate accuracy (existing method)
    data->accuracy = gps->dop_h * 5.0;
    
    // Copy basic GPS data (existing fields)
    data->latitude = gps->latitude;
    data->longitude = gps->longitude;
    data->altitude = gps->altitude;
}

const char* get_gps_quality_string(const wardriving_data_t *data) {
    if (!data->gps_quality.has_valid_fix) {
        return "No Fix";
    }
    
    if (data->gps_quality.hdop <= 1.0) {
        return "Excellent";
    } else if (data->gps_quality.hdop <= 2.0) {
        return "Good";
    } else if (data->gps_quality.hdop <= 5.0) {
        return "Moderate";
    } else if (data->gps_quality.hdop <= 10.0) {
        return "Fair";
    } else {
        return "Poor";
    }
}

static const char* get_cardinal_direction(float course) {
    const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                               "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    int index = (int)((course + 11.25f) / 22.5f) % 16;
    return directions[index];
}

static const char* get_fix_type_str(uint8_t fix) {
    switch(fix) {
        case GPS_FIX_INVALID: return "No Fix";
        case GPS_FIX_GPS: return "GPS";
        case GPS_FIX_DGPS: return "DGPS";
        default: return "Unknown";
    }
}

static void format_coordinates(double lat, double lon, char* lat_str, char* lon_str) {
    int lat_deg = (int)fabs(lat);
    double lat_min = (fabs(lat) - lat_deg) * 60;
    int lon_deg = (int)fabs(lon);
    double lon_min = (fabs(lon) - lon_deg) * 60;
    
    sprintf(lat_str, "%d°%.4f'%c", lat_deg, lat_min, lat >= 0 ? 'N' : 'S');
    sprintf(lon_str, "%d°%.4f'%c", lon_deg, lon_min, lon >= 0 ? 'E' : 'W');
}

float get_accuracy_percentage(float hdop) {
    // HDOP ranges from 1 (best) to 20+ (worst)
    // Let's consider HDOP of 1 as 100% and HDOP of 20 as 0%
    
    if (hdop <= 1.0f) return 100.0f;
    if (hdop >= 20.0f) return 0.0f;
    
    // Linear interpolation between 1 and 20
    return (20.0f - hdop) * (100.0f / 19.0f);
}

void gps_info_display_task(void *pvParameters) {
    const TickType_t delay = pdMS_TO_TICKS(5000);
    char output_buffer[512] = {0}; 
    char lat_str[20] = {0}, lon_str[20] = {0};
    static wardriving_data_t gps_data = {0};
    
    ESP_LOGI(GPS_TAG, "GPS info display task started");
    
    while(1) {
        // Add null check for nmea_hdl
        if (!nmea_hdl) {
            ESP_LOGW(GPS_TAG, "NMEA handle is null");
            vTaskDelay(delay);
            continue;
        }

        ESP_LOGD(GPS_TAG, "Getting GPS structure");
        gps_t* gps = &((esp_gps_t*)nmea_hdl)->parent;
        
        if (!gps) {
            ESP_LOGW(GPS_TAG, "GPS structure is null");
            vTaskDelay(delay);
            continue;
        }
        
        // Only populate GPS data, don't trigger CSV operations
        if (gps->valid) {
            ESP_LOGD(GPS_TAG, "Populating GPS data");
            populate_gps_quality_data(&gps_data, gps);
        }
        
        // Build complete string before sending to terminal
        if (!gps->valid || !is_valid_date(&gps->date)) {
            ESP_LOGD(GPS_TAG, "GPS not valid, showing searching message");
            strncpy(output_buffer, "Searching sats...\n", sizeof(output_buffer) - 1);
            output_buffer[sizeof(output_buffer) - 1] = '\0';
        } else {
            ESP_LOGD(GPS_TAG, "Formatting GPS coordinates");
            format_coordinates(gps_data.latitude, gps_data.longitude, lat_str, lon_str);
            const char* direction = get_cardinal_direction(gps_data.gps_quality.course);
            
            // Use safer snprintf with size checking
            int written = snprintf(output_buffer, sizeof(output_buffer),
                    "GPS Info\n"
                    "Sats: %d/%d\n"
                    "Lat: %s\n"
                    "Long: %s\n"
                    "Direction: %d° %s\n",
                    gps_data.gps_quality.satellites_used, 
                    GPS_MAX_SATELLITES_IN_USE,
                    lat_str,
                    lon_str,
                    (int)gps_data.gps_quality.course,
                    direction ? direction : "Unknown");

            // Verify the write was successful
            if (written < 0 || written >= sizeof(output_buffer)) {
                ESP_LOGE(GPS_TAG, "Buffer overflow prevented in GPS info formatting");
                strncpy(output_buffer, "GPS Data Error\n", sizeof(output_buffer) - 1);
                output_buffer[sizeof(output_buffer) - 1] = '\0';
            }
        }
        
        // Safely send to terminal with single call
        terminal_view_add_text(output_buffer);
        
        vTaskDelay(delay);
    }
}