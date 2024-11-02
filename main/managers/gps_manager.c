#include <stdio.h>
#include <string.h>
#include "managers/gps_manager.h"
#include "esp_log.h"
#include "sys/time.h"
#include "driver/uart.h"
#include "vendor/GPS/gps_logger.h"


static const char *GPS_TAG = "GPS";

void gps_manager_init(GPSManager* manager) {
    microNMEA_init(&manager->nmea, manager->nmeaBuffer, sizeof(manager->nmeaBuffer));
    manager->isinitilized = true;

    
    if (csv_file_open("gps_data") == ESP_OK) {
        ESP_LOGI(GPS_TAG, "CSV file opened for GPS data logging.");
    } else {
        ESP_LOGE(GPS_TAG, "Failed to open CSV file for GPS data logging.");
    }
}

void gps_manager_deinit(GPSManager* manager) {
    if (manager->isinitilized) {
        csv_file_close();
        ESP_LOGI(GPS_TAG, "CSV file closed for GPS data logging.");

        manager->isinitilized = false;
    }
}

void gps_manager_process_char(GPSManager* manager, char c) {
    if (manager->isinitilized) {
        microNMEA_process(&manager->nmea, c);
    }
}

esp_err_t gps_manager_log_wardriving_data(wardriving_data_t* data) {
    if (!data) {
        ESP_LOGE(GPS_TAG, "Invalid data pointer.");
        return ESP_ERR_INVALID_ARG;
    }

    // Log data to CSV buffer
    esp_err_t ret = csv_write_data_to_buffer(data);
    if (ret != ESP_OK) {
        ESP_LOGE(GPS_TAG, "Failed to write wardriving data to CSV buffer.");
    }

    return ret;
}

void gps_manager_log_values(GPSManager* manager) {
    MicroNMEA* nmea = &manager->nmea;
    printf("Latitude: %ld (degrees * 10^6)\n", nmea->latitude);
    printf("Longitude: %ld (degrees * 10^6)\n", nmea->longitude);
    printf("Altitude: %ld meters\n", nmea->altitude);
    printf("Speed: %ld knots\n", nmea->speed);
    printf("Number of Satellites: %u\n", nmea->numSat);
    printf("----------\n");
}