#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "sys/time.h"
#include "driver/uart.h"
#include <errno.h>
#include <sys/stat.h>
#include "vendor/GPS/gps_logger.h"

static const char *CSV_TAG = "CSV";


#define UART_NUM_0 1
#define CSV_BUFFER_SIZE 512

static FILE *csv_file = NULL;
static char csv_buffer[BUFFER_SIZE];
static size_t buffer_offset = 0;

esp_err_t csv_write_header(FILE* f) {
    const char* header = "BSSID,SSID,Latitude,Longitude,RSSI,Channel,Encryption,Time\n";

    if (f == NULL) {
        const char* mark_begin = "[BUF/BEGIN]";
        const char* mark_close = "[BUF/CLOSE]";
        uart_write_bytes(UART_NUM_0, mark_begin, strlen(mark_begin));
        uart_write_bytes(UART_NUM_0, header, strlen(header));
        uart_write_bytes(UART_NUM_0, mark_close, strlen(mark_close));
        uart_write_bytes(UART_NUM_0, "\n", 1);
        return ESP_OK;
    } else {
        size_t written = fwrite(header, 1, strlen(header), f);
        return (written == strlen(header)) ? ESP_OK : ESP_FAIL;
    }
}

void get_next_csv_file_name(char *file_name_buffer, const char* base_name) {
    int next_index = get_next_csv_file_index(base_name);  // Modify this to be CSV specific
    snprintf(file_name_buffer, MAX_FILE_NAME_LENGTH, "/mnt/ghostesp/logs/%s_%d.csv", base_name, next_index);
}

esp_err_t csv_file_open(const char* base_file_name) {
    char file_name[MAX_FILE_NAME_LENGTH];
    get_next_csv_file_name(file_name, base_file_name);

    csv_file = fopen(file_name, "w");

    esp_err_t ret = csv_write_header(csv_file);
    if (ret != ESP_OK) {
        ESP_LOGE(CSV_TAG, "Failed to write CSV header.");
        fclose(csv_file);
        csv_file = NULL;
        return ret;
    }

    ESP_LOGI(CSV_TAG, "CSV file %s opened and header written.", file_name);
    return ESP_OK;
}

esp_err_t csv_write_data_to_buffer(wardriving_data_t *data) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    char data_line[CSV_BUFFER_SIZE];
    int len = snprintf(data_line, CSV_BUFFER_SIZE, "%s,%s,%lf,%lf,%d,%d,%s,%lld\n",
                       data->bssid, data->ssid, data->latitude, data->longitude,
                       data->rssi, data->channel, data->encryption_type, tv.tv_sec);

    if (buffer_offset + len > BUFFER_SIZE) {
        ESP_LOGI(CSV_TAG, "Buffer full, flushing to file.");
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
        ESP_LOGE(CSV_TAG, "CSV file is not open. Flushing to Serial...");
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
        ESP_LOGE(CSV_TAG, "Failed to write buffer to file.");
        return ESP_FAIL;
    }

    ESP_LOGI(CSV_TAG, "Flushed %zu bytes to CSV file.", buffer_offset);
    buffer_offset = 0;

    return ESP_OK;
}

void csv_file_close() {
    if (csv_file != NULL) {
        if (buffer_offset > 0) {
            ESP_LOGI(CSV_TAG, "Flushing remaining buffer before closing file.");
            csv_flush_buffer_to_file();
        }
        fclose(csv_file);
        csv_file = NULL;
        ESP_LOGI(CSV_TAG, "CSV file closed.");
    }
}