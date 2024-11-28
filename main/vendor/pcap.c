#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sys/time.h"
#include "vendor/pcap.h"
#include "driver/uart.h"
#include <errno.h>
#include "core/utils.h"
#include <sys/stat.h>
#include <arpa/inet.h>
#include "managers/sd_card_manager.h"

#define RADIOTAP_HEADER_LEN 8

static const char *PCAP_TAG = "PCAP";

esp_err_t pcap_init(void) {
    if (pcap_mutex != NULL) {
        // Already initialized
        return ESP_OK;
    }
    
    pcap_mutex = xSemaphoreCreateMutex();
    if (pcap_mutex == NULL) {
        ESP_LOGE(PCAP_TAG, "Failed to create PCAP mutex");
        return ESP_FAIL;
    }
    
    ESP_LOGI(PCAP_TAG, "PCAP mutex initialized successfully");
    return ESP_OK;
}

esp_err_t pcap_write_global_header(FILE* f) {
    pcap_global_header_t header = {
        .magic_number = 0xa1b2c3d4,
        .version_major = 2,
        .version_minor = 4,
        .thiszone = 0,
        .sigfigs = 0,
        .snaplen = 65535,
        .network = 127  // DLT_IEEE802_11_RADIO
    };

    if (f == NULL)
    {
        const char* mark_begin = "[BUF/BEGIN]";
        const size_t mark_begin_len = strlen(mark_begin);
        const char* mark_close = "[BUF/CLOSE]";
        const size_t mark_close_len = strlen(mark_close);

        uart_write_bytes(UART_NUM_0, mark_begin, mark_begin_len);

        
        uart_write_bytes(UART_NUM_0, (const char*)&header, sizeof(header));

        
        uart_write_bytes(UART_NUM_0, mark_close, mark_close_len);

        
        const char* newline = "\n";
        uart_write_bytes(UART_NUM_0, newline, 1);
        return ESP_OK;
    }
    else 
    {
        size_t written = fwrite(&header, 1, sizeof(header), f);
        return (written == sizeof(header)) ? ESP_OK : ESP_FAIL;
    }
}

void get_next_pcap_file_name(char *file_name_buffer, const char* base_name) {
    int next_index = get_next_pcap_file_index(base_name);
    snprintf(file_name_buffer, MAX_FILE_NAME_LENGTH, "/mnt/ghostesp/pcaps/%s_%d.pcap", base_name, next_index);
}

esp_err_t pcap_file_open(const char* base_file_name) {
    // First ensure PCAP is initialized
    esp_err_t init_ret = pcap_init();
    if (init_ret != ESP_OK) {
        ESP_LOGE(PCAP_TAG, "Failed to initialize PCAP");
        return init_ret;
    }

    char file_name[MAX_FILE_NAME_LENGTH];
    
    if (sd_card_exists("/mnt/ghostesp/pcaps"))
    {
        get_next_pcap_file_name(file_name, base_file_name);
        pcap_file = fopen(file_name, "wb");
    }
    
    esp_err_t ret = pcap_write_global_header(pcap_file);
    if (ret != ESP_OK) {
        ESP_LOGE(PCAP_TAG, "Failed to write PCAP global header.");
        fclose(pcap_file);
        pcap_file = NULL;
        return ret;
    }

    ESP_LOGI(PCAP_TAG, "PCAP file %s opened and global header written.", file_name);
    return ESP_OK;
}


esp_err_t pcap_write_packet_to_buffer(const void* packet, size_t length) {
    if (packet == NULL || length == 0) {
        ESP_LOGE(PCAP_TAG, "Invalid packet data");
        return ESP_ERR_INVALID_ARG;
    }

    if (pcap_mutex == NULL) {
        ESP_LOGE(PCAP_TAG, "PCAP mutex not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(pcap_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(PCAP_TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    pcap_packet_header_t packet_header;

    // Add radiotap length to packet length
    size_t total_length = length + RADIOTAP_HEADER_LEN;
    packet_header.ts_sec = tv.tv_sec;
    packet_header.ts_usec = tv.tv_usec;
    packet_header.incl_len = total_length;
    packet_header.orig_len = total_length;

    size_t total_packet_size = sizeof(packet_header) + total_length;
    
    // Validate total size
    if (total_packet_size > BUFFER_SIZE) {
        xSemaphoreGive(pcap_mutex);
        ESP_LOGE(PCAP_TAG, "Packet too large for buffer: %zu", total_packet_size);
        return ESP_ERR_NO_MEM;
    }

    // If buffer doesn't have space, flush first
    if (buffer_offset + total_packet_size > BUFFER_SIZE) {
        ESP_LOGI(PCAP_TAG, "Buffer full, flushing %zu bytes", buffer_offset);
        esp_err_t ret = pcap_flush_buffer_to_file();
        if (ret != ESP_OK) {
            xSemaphoreGive(pcap_mutex);
            ESP_LOGE(PCAP_TAG, "Buffer flush failed");
            return ret;
        }
    }

    // Write packet header
    memcpy(pcap_buffer + buffer_offset, &packet_header, sizeof(packet_header));
    buffer_offset += sizeof(packet_header);

    // Write minimal radiotap header
    uint8_t radiotap_header[RADIOTAP_HEADER_LEN] = {
        0x00, 0x00,             // Version 0
        0x08, 0x00,             // Header length (8 bytes)
        0x00, 0x00, 0x00, 0x00  // Present flags (none)
    };
    memcpy(pcap_buffer + buffer_offset, radiotap_header, RADIOTAP_HEADER_LEN);
    buffer_offset += RADIOTAP_HEADER_LEN;

    // Write actual packet
    memcpy(pcap_buffer + buffer_offset, packet, length);
    buffer_offset += length;

    ESP_LOGD(PCAP_TAG, "Added packet: size=%zu, buffer at: %zu", length, buffer_offset);
    
    xSemaphoreGive(pcap_mutex);
    return ESP_OK;
}


esp_err_t pcap_flush_buffer_to_file() {
    if (buffer_offset == 0) {
        return ESP_OK;  // Nothing to flush
    }

    bool needs_mutex = (xTaskGetCurrentTaskHandle() != xSemaphoreGetMutexHolder(pcap_mutex));
    
    if (needs_mutex) {
        if (xSemaphoreTake(pcap_mutex, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(PCAP_TAG, "Failed to take mutex");
            return ESP_ERR_TIMEOUT;
        }
    }

    esp_err_t ret = ESP_OK;
    
    if (pcap_file == NULL) {
        ESP_LOGE(PCAP_TAG, "PCAP file is not open. Flushing to Serial...");
        const char* mark_begin = "[BUF/BEGIN]";
        const size_t mark_begin_len = strlen(mark_begin);
        const char* mark_close = "[BUF/CLOSE]";
        const size_t mark_close_len = strlen(mark_close);

        uart_write_bytes(UART_NUM_0, mark_begin, mark_begin_len);
        uart_write_bytes(UART_NUM_0, (const char*)pcap_buffer, buffer_offset);
        uart_write_bytes(UART_NUM_0, mark_close, mark_close_len);
        uart_write_bytes(UART_NUM_0, "\n", 1);
        
        buffer_offset = 0;
        goto exit;
    }

    // Validate buffer contains at least one complete packet
    if (buffer_offset < sizeof(pcap_packet_header_t)) {
        ESP_LOGE(PCAP_TAG, "Buffer contains incomplete packet header");
        ret = ESP_FAIL;
        goto exit;
    }

    // Write entire buffer
    size_t written = fwrite(pcap_buffer, 1, buffer_offset, pcap_file);
    if (written != buffer_offset) {
        ESP_LOGE(PCAP_TAG, "Failed to write buffer: %zu of %zu written", written, buffer_offset);
        ret = ESP_FAIL;
        goto exit;
    }

    // Force flush to disk
    if (fflush(pcap_file) != 0) {
        ESP_LOGE(PCAP_TAG, "Failed to flush file buffer");
        ret = ESP_FAIL;
        goto exit;
    }

    ESP_LOGI(PCAP_TAG, "Flushed %zu bytes to file", written);

    memset(pcap_buffer, 0, BUFFER_SIZE);
    buffer_offset = 0;

exit:
    xSemaphoreGive(pcap_mutex);
    return ret;
}


void pcap_file_close() {
    if (pcap_file != NULL) {
        if (xSemaphoreTake(pcap_mutex, portMAX_DELAY) == pdTRUE) {
            if (buffer_offset > 0) {
                ESP_LOGI(PCAP_TAG, "Flushing remaining buffer before closing file.");
                pcap_flush_buffer_to_file();
            }

            fclose(pcap_file);
            pcap_file = NULL;
            ESP_LOGI(PCAP_TAG, "PCAP file closed.");
            xSemaphoreGive(pcap_mutex);
        }
    }
}