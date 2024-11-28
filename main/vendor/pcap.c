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

static const char *PCAP_TAG = "PCAP";


esp_err_t pcap_write_global_header(FILE* f) {
    pcap_global_header_t global_header;
    global_header.magic_number = 0xa1b2c3d4;
    global_header.version_major = 2;
    global_header.version_minor = 4;
    global_header.thiszone = 0;  // UTC
    global_header.sigfigs = 0;
    global_header.snaplen = 4096;  // Max packet length
    global_header.network = 105;   // DLT_IEEE802_11 for Wi-Fi

    if (f == NULL)
    {
        const char* mark_begin = "[BUF/BEGIN]";
        const size_t mark_begin_len = strlen(mark_begin);
        const char* mark_close = "[BUF/CLOSE]";
        const size_t mark_close_len = strlen(mark_close);

        uart_write_bytes(UART_NUM_0, mark_begin, mark_begin_len);

        
        uart_write_bytes(UART_NUM_0, (const char*)&global_header, sizeof(global_header));

        
        uart_write_bytes(UART_NUM_0, mark_close, mark_close_len);

        
        const char* newline = "\n";
        uart_write_bytes(UART_NUM_0, newline, 1);
        return ESP_OK;
    }
    else 
    {
        size_t written = fwrite(&global_header, 1, sizeof(global_header), f);
        return (written == sizeof(global_header)) ? ESP_OK : ESP_FAIL;
    }
}

void get_next_pcap_file_name(char *file_name_buffer, const char* base_name) {
    int next_index = get_next_pcap_file_index(base_name);
    snprintf(file_name_buffer, MAX_FILE_NAME_LENGTH, "/mnt/ghostesp/pcaps/%s_%d.pcap", base_name, next_index);
}

esp_err_t pcap_file_open(const char* base_file_name) {
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

    struct timeval tv;
    gettimeofday(&tv, NULL);
    pcap_packet_header_t packet_header;

    packet_header.ts_sec = tv.tv_sec;
    packet_header.ts_usec = tv.tv_usec;
    packet_header.incl_len = length;
    packet_header.orig_len = length;

    size_t total_packet_size = sizeof(packet_header) + length;
    
    // Validate total size
    if (total_packet_size > BUFFER_SIZE) {
        ESP_LOGE(PCAP_TAG, "Packet too large for buffer: %zu", total_packet_size);
        return ESP_ERR_NO_MEM;
    }

    // If buffer doesn't have space, flush first
    if (buffer_offset + total_packet_size > BUFFER_SIZE) {
        ESP_LOGI(PCAP_TAG, "Buffer full, flushing %zu bytes", buffer_offset);
        esp_err_t ret = pcap_flush_buffer_to_file();
        if (ret != ESP_OK) {
            ESP_LOGE(PCAP_TAG, "Buffer flush failed");
            return ret;
        }
    }

    // Double check we have space after flush
    if (buffer_offset + total_packet_size > BUFFER_SIZE) {
        ESP_LOGE(PCAP_TAG, "Packet too large for buffer: %zu", total_packet_size);
        return ESP_FAIL;
    }

    // Write header and packet atomically
    memcpy(pcap_buffer + buffer_offset, &packet_header, sizeof(packet_header));
    buffer_offset += sizeof(packet_header);
    memcpy(pcap_buffer + buffer_offset, packet, length);
    buffer_offset += length;

    ESP_LOGD(PCAP_TAG, "Added packet: size=%zu, buffer at: %zu", length, buffer_offset);
    
    return ESP_OK;
}


esp_err_t pcap_flush_buffer_to_file() {
    if (buffer_offset == 0) {
        return ESP_OK;  // Nothing to flush
    }

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
        return ESP_OK;
    }

    // Validate buffer contains at least one complete packet
    if (buffer_offset < sizeof(pcap_packet_header_t)) {
        ESP_LOGE(PCAP_TAG, "Buffer contains incomplete packet header");
        return ESP_FAIL;
    }

    // Write entire buffer
    size_t written = fwrite(pcap_buffer, 1, buffer_offset, pcap_file);
    if (written != buffer_offset) {
        ESP_LOGE(PCAP_TAG, "Failed to write buffer: %zu of %zu written", written, buffer_offset);
        return ESP_FAIL;
    }

    // Force flush to disk
    if (fflush(pcap_file) != 0) {
        ESP_LOGE(PCAP_TAG, "Failed to flush file buffer");
        return ESP_FAIL;
    }

    ESP_LOGI(PCAP_TAG, "Flushed %zu bytes to file", written);
    memset(pcap_buffer, 0, BUFFER_SIZE);
    buffer_offset = 0;
    return ESP_OK;
}


void pcap_file_close() {
    if (pcap_file != NULL) {
        if (buffer_offset > 0) {
            ESP_LOGI(PCAP_TAG, "Flushing remaining buffer before closing file.");
            pcap_flush_buffer_to_file();
        }

        // Close the file
        fclose(pcap_file);
        pcap_file = NULL;
        ESP_LOGI(PCAP_TAG, "PCAP file closed.");
    }
}