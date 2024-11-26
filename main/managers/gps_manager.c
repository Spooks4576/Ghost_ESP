#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "managers/gps_manager.h"
#include "esp_log.h"
#include "sys/time.h"
#include "driver/uart.h"
#include "core/callbacks.h"
#include "vendor/GPS/MicroNMEA.h"
#include "vendor/GPS/gps_logger.h"
#include "managers/settings_manager.h"
#include <managers/views/terminal_screen.h>


static const char *GPS_TAG = "GPS";

nmea_parser_handle_t nmea_hdl;

gps_date_t cacheddate = {0};

static bool is_valid_date(const gps_date_t* date) {
    if (!date) return false;
    
    // Check year
    if (!gps_is_valid_year(date->year)) return false;
    
    // Check month
    if (date->month < 1 || date->month > 12) return false;
    
    // Check day
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

void gps_manager_init(GPSManager* manager) {
    
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();

    uint8_t current_rx_pin = settings_get_gps_rx_pin(&G_Settings);

    if (current_rx_pin != 0)
    {   
        config.uart.rx_pin = current_rx_pin;
    }
        
#ifdef CONFIG_IS_GHOST_BOARD
    config.uart.rx_pin = 2;
#endif

    nmea_hdl = nmea_parser_init(&config);

    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

    manager->isinitilized = true;
    
    if (csv_file_open("gps_data") == ESP_OK) {
        printf("CSV file opened for GPS data logging.");
    } else {
        printf("Failed to open CSV file for GPS data logging.");
    }
}

void gps_manager_deinit(GPSManager* manager) {
    if (manager->isinitilized) {
        nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
        nmea_parser_deinit(nmea_hdl);
        csv_file_close();
        printf("CSV file closed for GPS data logging.");

        manager->isinitilized = false;
    }
}

#define GPS_STATUS_MESSAGE "GPS: %s\nSats: %u/%u\nSpeed: %.1f km/h\nAccuracy: %s\n"
#define GPS_UPDATE_INTERVAL 4  // Show status every 4th update (25% chance)

#define MIN_SPEED_THRESHOLD 0.1     // Minimum 0.1 m/s (~0.36 km/h)
#define MAX_SPEED_THRESHOLD 340.0   // Maximum 340 m/s (~1224 km/h)

esp_err_t gps_manager_log_wardriving_data(wardriving_data_t* data) {
    if (!data || !gps || !gps->valid || strlen(data->ssid) <= 2) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate GPS data
    if (!is_valid_date(&gps->date) || 
        gps->tim.hour > 23 || gps->tim.minute > 59 || gps->tim.second > 59 ||
        gps->latitude < -90.0 || gps->latitude > 90.0 || 
        gps->longitude < -180.0 || gps->longitude > 180.0 ||
        gps->speed < 0.0 || gps->speed > 340.0 ||
        gps->dop_h < 0.0 || gps->dop_h > 50.0) {
        return ESP_OK;  // Skip invalid data silently
    }

    if (!gps->valid) {
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(data->ssid) <= 2) {
        return ESP_OK;
    }

    // First, validate the current GPS date
    if (!is_valid_date(&gps->date)) {
        // Only show warning if we have a truly valid fix
        if (gps->valid && 
            gps->fix >= GPS_FIX_GPS && 
            gps->fix_mode >= GPS_MODE_2D && 
            gps->sats_in_use >= 3 && 
            gps->sats_in_use <= GPS_MAX_SATELLITES_IN_USE && // Should be ≤ 12
            rand() % 100 == 0) {
            printf("Warning: GPS date is out of range despite good fix: %04d-%02d-%02d "
                   "(Fix: %d, Mode: %d, Sats: %d)\n",
                gps_get_absolute_year(gps->date.year), 
                gps->date.month, gps->date.day,
                gps->fix, gps->fix_mode, gps->sats_in_use);
        }
        return ESP_OK;
    }

    // Then, only if we don't have a cached date and the current date is valid, cache it
    if (cacheddate.year <= 0) {
        cacheddate = gps->date;
    }

    if (gps->tim.hour > 23 || gps->tim.minute > 59 || gps->tim.second > 59) {
        if (rand() % 20 == 0) {
        printf("Warning: GPS time is invalid: %02d:%02d:%02d\n",
               gps->tim.hour, gps->tim.minute, gps->tim.second);
        }
        return ESP_OK;
    }

    
    if (gps->latitude < -90.0 || gps->latitude > 90.0 || 
        gps->longitude < -180.0 || gps->longitude > 180.0) {
        if (rand() % 20 == 0) {
            printf("GPS Error: Invalid location detected (Lat: %f, Lon: %f)\n",
                gps->latitude, gps->longitude);
        }
        return ESP_OK;
    }

    
    if (gps->speed < 0.0 || gps->speed > 340.0) {
        if (rand() % 20 == 0) {
            printf("Warning: GPS speed is out of range: %f m/s\n", gps->speed);
        }
        return ESP_OK;
    }

    
    if (gps->dop_h < 0.0 || gps->dop_p < 0.0 || gps->dop_v < 0.0 || 
        gps->dop_h > 50.0 || gps->dop_p > 50.0 || gps->dop_v > 50.0) {
        if (rand() % 20 == 0) {
            printf("Warning: GPS DOP values are out of range: HDOP: %f, PDOP: %f, VDOP: %f\n",
                gps->dop_h, gps->dop_p, gps->dop_v);
        }
        return ESP_OK;
    }

    esp_err_t ret = csv_write_data_to_buffer(data);
    if (ret != ESP_OK) {
        ESP_LOGE(GPS_TAG, "Failed to write wardriving data to CSV buffer");
        return ret;
    }

    // Update display periodically
    if (rand() % GPS_UPDATE_INTERVAL == 0) {
        // Determine GPS fix status
        char fix_status[10];
        if (!gps->valid || gps->fix == GPS_FIX_INVALID) {
            strcpy(fix_status, "No Fix");
        } else if (gps->fix_mode == GPS_MODE_2D) {
            strcpy(fix_status, "Basic");
        } else if (gps->fix_mode == GPS_MODE_3D) {
            strcpy(fix_status, "Locked");
        } else {
            strcpy(fix_status, "Unknown");
        }

        // Validate satellite counts
        uint8_t sats_in_use = gps->sats_in_use;

        // Ensure count is non-negative and within limits
        if (sats_in_use > GPS_MAX_SATELLITES_IN_USE || sats_in_use < 0) {
            sats_in_use = 0;
        }

        // Determine accuracy based on HDOP
        char accuracy[10];
        if (gps->dop_h < 0.0 || gps->dop_h > 50.0) {
            strcpy(accuracy, "Invalid");
        } else if (gps->dop_h <= 1.0) {
            strcpy(accuracy, "Perfect");
        } else if (gps->dop_h <= 2.0) {
            strcpy(accuracy, "High");
        } else if (gps->dop_h <= 5.0) {
            strcpy(accuracy, "Good");
        } else if (gps->dop_h <= 10.0) {
            strcpy(accuracy, "Okay");
        } else {
            strcpy(accuracy, "Poor");
        }

        // Convert speed from m/s to km/h for display with validation
        float speed_kmh = 0.0;
        if (gps->valid && gps->fix >= GPS_FIX_GPS) {  // Only trust speed with a valid fix
            if (gps->speed >= MIN_SPEED_THRESHOLD && gps->speed <= MAX_SPEED_THRESHOLD) {
                speed_kmh = gps->speed * 3.6;  // Convert m/s to km/h
            } else if (gps->speed < MIN_SPEED_THRESHOLD && gps->speed >= 0.0) {
                speed_kmh = 0.0;  // Show as stopped if below threshold but not negative
            }
            // Speeds above MAX_SPEED_THRESHOLD remain at 0.0
        }

        // Add newline before status update for better readability
        printf("\n");
        printf(GPS_STATUS_MESSAGE, 
               fix_status, sats_in_use, GPS_MAX_SATELLITES_IN_USE, 
               speed_kmh, accuracy);
        TERMINAL_VIEW_ADD_TEXT(GPS_STATUS_MESSAGE,
                              fix_status, sats_in_use, GPS_MAX_SATELLITES_IN_USE, 
                              speed_kmh, accuracy);
    }

    return ret;
}