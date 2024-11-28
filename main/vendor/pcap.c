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

static size_t calculate_wifi_frame_length(const uint8_t* frame, size_t max_len) {
    if (max_len < 2) return 0;
    
    uint16_t frame_control = frame[0] | (frame[1] << 8);
    uint8_t type = (frame_control >> 2) & 0x3;
    uint8_t subtype = (frame_control >> 4) & 0xF;
    uint8_t to_ds = (frame_control >> 8) & 0x1;
    uint8_t from_ds = (frame_control >> 9) & 0x1;
    
    // Start with MAC header
    size_t length = 24;  // Basic MAC header length
    
    // Handle different frame types
    switch (type) {
        case 0x0:  // Management frames
            // Add fixed parameters for beacons and probe responses
            if (subtype == 0x8 || subtype == 0x5) {  // Beacon or Probe Response
                length += 12;  // timestamp(8) + beacon_interval(2) + capability_info(2)
            }
            
            // Parse tagged parameters if we have enough data
            if (max_len > length + 2) {
                size_t pos = length;
                while (pos + 2 <= max_len) {
                    uint8_t tag_len = frame[pos + 1];
                    
                    // Check if we can safely read this tag
                    if (pos + 2 + tag_len > max_len) {
                        break;
                    }
                    
                    pos += 2 + tag_len;
                    
                    // Check for padding or end of tags
                    if (tag_len == 0) break;
                }
                length = pos;
            }
            break;
            
        case 0x1:  // Control frames
            switch (subtype) {
                case 0xB:  // RTS
                    length = 16;
                    break;
                case 0xC:  // CTS
                case 0xD:  // ACK
                    length = 10;
                    break;
                default:
                    length = 16;  // Default for other control frames
            }
            break;
            
        case 0x2:  // Data frames
            // Determine address field length based on To/From DS flags
            if (to_ds && from_ds) {
                length = 30;  // 24 + 6 (four address fields)
            }
            
            // Check for QoS data frames (subtypes 0x8 to 0xF)
            if ((subtype & 0x8) != 0) {
                length += 2;  // Add QoS Control field
            }
            
            // If there's a frame body, include it
            if (max_len > length) {
                // Data frames must have at least 8 bytes for LLC/SNAP
                size_t data_len = max_len - length;
                if (data_len >= 8) {  // Minimum LLC/SNAP header
                    length = max_len;  // Include the entire frame body
                }
            }
            break;
    }
    
    // Ensure we don't return a length longer than the actual packet
    return (length <= max_len) ? length : max_len;
}

esp_err_t pcap_write_packet_to_buffer(const void* packet, size_t length) {
    if (packet == NULL || length < 2) {
        ESP_LOGE(PCAP_TAG, "Invalid packet data");
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(pcap_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(PCAP_TAG, "Failed to take mutex");
        return ESP_ERR_TIMEOUT;
    }

    const uint8_t* frame = (const uint8_t*)packet;
    size_t actual_length = calculate_wifi_frame_length(frame, length);
    
    if (actual_length == 0) {
        xSemaphoreGive(pcap_mutex);
        ESP_LOGE(PCAP_TAG, "Invalid frame length calculated");
        return ESP_ERR_INVALID_ARG;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    pcap_packet_header_t packet_header;

    // Add radiotap header length to packet length
    size_t total_length = actual_length + RADIOTAP_HEADER_LEN;
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

    // Write radiotap header
    uint8_t radiotap_header[RADIOTAP_HEADER_LEN] = {
        0x00, 0x00,  // Version 0
        0x08, 0x00,  // Header length
        0x00, 0x00, 0x00, 0x00  // Present flags
    };
    memcpy(pcap_buffer + buffer_offset, radiotap_header, RADIOTAP_HEADER_LEN);
    buffer_offset += RADIOTAP_HEADER_LEN;

    // Write actual packet data
    memcpy(pcap_buffer + buffer_offset, packet, actual_length);
    buffer_offset += actual_length;

    xSemaphoreGive(pcap_mutex);
    
    ESP_LOGD(PCAP_TAG, "Added packet: size=%zu, buffer at: %zu", actual_length, buffer_offset);
    
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