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


static const char *GPS_TAG = "GPS";

nmea_parser_handle_t nmea_hdl;

void gps_manager_init(GPSManager* manager) {
    
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();

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

esp_err_t gps_manager_log_wardriving_data(wardriving_data_t* data) {
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!gps) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!gps->valid) {
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(data->ssid) <= 2) {
        return ESP_OK;
    }

    
    if (gps->date.year < 2000 || gps->date.year > 2100 || 
        gps->date.month < 1 || gps->date.month > 12 || 
        gps->date.day < 1 || gps->date.day > 31) {
        if (rand() % 20 == 0) {
            printf("Warning: GPS date is out of range: %04d-%02d-%02d\n",
               gps->date.year, gps->date.month, gps->date.day);
        }
        return ESP_OK;
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
            printf("Warning: GPS coordinates are out of range: Lat: %f, Lon: %f\n",
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
        printf("Failed to write wardriving data to CSV buffer.\n");
        return ret;
    }

    if (rand() % 20 == 0) {
        printf("Wrote to the buffer with %u Satellites\n", gps->sats_in_view);
    }

    return ret;
}