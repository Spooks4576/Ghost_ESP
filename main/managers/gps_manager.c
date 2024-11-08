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

    if (!gps)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(data->ssid) <= 2)
    {
        return ESP_OK;
    }

    if (gps->date.year <= 2000)
    {
        if (rand() % 20 == 0)
        {
            printf("No Valid GPS Signal\n");
        }
        return ESP_OK;
    }
    
    esp_err_t ret = csv_write_data_to_buffer(data);
    if (ret != ESP_OK) {
        printf("Failed to write wardriving data to CSV buffer.");
        return ret;
    }

    if (rand() % 20 == 0)
    {
        printf("Wrote to the buffer with %u Satellites \n", gps->sats_in_view);
    }
    
    return ret;
}