#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sys/time.h"
#include "vendor/pcap.h"
#include <errno.h>
#include "core/utils.h"
#include <sys/stat.h>
#include <arpa/inet.h>

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

    size_t written = fwrite(&global_header, 1, sizeof(global_header), f);
    return (written == sizeof(global_header)) ? ESP_OK : ESP_FAIL;
}

void get_next_pcap_file_name(char *file_name_buffer, const char* base_name) {
    int next_index = get_next_pcap_file_index(base_name);
    snprintf(file_name_buffer, MAX_FILE_NAME_LENGTH, "/mnt/ghostesp/pcaps/%s_%d.pcap", base_name, next_index);
}

esp_err_t pcap_file_open(const char* base_file_name) {
    char file_name[MAX_FILE_NAME_LENGTH];

    
    get_next_pcap_file_name(file_name, base_file_name);


    pcap_file = fopen(file_name, "wb");
    if (pcap_file == NULL) {
        ESP_LOGE(PCAP_TAG, "fopen failed for file: %s, errno: %d (%s)", file_name, errno, strerror(errno));
        return ESP_FAIL;
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
    if (pcap_file == NULL) {
        ESP_LOGE(PCAP_TAG, "PCAP file is not open.");
        return ESP_FAIL;
    }

    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pcap_packet_header_t packet_header;


    packet_header.ts_sec = tv.tv_sec;
    packet_header.ts_usec = tv.tv_usec;
    packet_header.incl_len = length;
    packet_header.orig_len = length;

    
    size_t total_packet_size = sizeof(packet_header) + length;
    if (buffer_offset + total_packet_size > BUFFER_SIZE) {
        // Buffer is full, flush to file
        ESP_LOGI(PCAP_TAG, "Buffer full, flushing to file.");
        esp_err_t ret = pcap_flush_buffer_to_file();
        if (ret != ESP_OK) {
            return ret;
        }
    }

    
    memcpy(pcap_buffer + buffer_offset, &packet_header, sizeof(packet_header));
    buffer_offset += sizeof(packet_header);

    
    memcpy(pcap_buffer + buffer_offset, packet, length);
    buffer_offset += length;

    return ESP_OK;
}


esp_err_t pcap_flush_buffer_to_file() {
    if (pcap_file == NULL) {
        ESP_LOGE(PCAP_TAG, "PCAP file is not open.");
        return ESP_FAIL;
    }

    
    size_t written = fwrite(pcap_buffer, 1, buffer_offset, pcap_file);
    if (written != buffer_offset) {
        ESP_LOGE(PCAP_TAG, "Failed to write buffer to file.");
        return ESP_FAIL;
    }

    ESP_LOGI(PCAP_TAG, "Flushed %zu bytes to PCAP file.", buffer_offset);

    
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