#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include <sys/dirent.h>
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "driver/sdmmc_types.h"
#include "managers/sd_card_manager.h"
#include "driver/gpio.h"
#include "esp_heap_trace.h"
#include "core/utils.h"
#include "vendor/pcap.h"

static const char *SD_TAG = "SD_Card_Manager";


sd_card_manager_t sd_card_manager = { // Change this based on board config
    .card = NULL,
    .is_initialized = false,
    .clkpin = 19,
    .cmdpin = 18,
    .d0pin = 20,
    .d1pin = 21,
    .d2pin = 22,
    .d3pin = 23,
    .spi_cs_pin = 23,
    .spi_clk_pin = 19,
    .spi_miso_pin = 20,
    .spi_mosi_pin = 18
};

static void get_next_pcap_file_name(char *file_name_buffer, const char* base_name) {
    int next_index = get_next_pcap_file_index(base_name);
    snprintf(file_name_buffer, 128, "/mnt/ghostesp/pcaps/%s_%d.pcap", base_name, next_index);
}

void list_files_recursive(const char *dirname, int level) {
    DIR *dir = opendir(dirname);
    if (!dir) {
        ESP_LOGE("File List", "Failed to open directory: %s", dirname);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char path[512];
        int written = snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        if (written < 0 || written >= sizeof(path)) {
            ESP_LOGW(SD_TAG, "Path was truncated: %s/%s", dirname, entry->d_name);
            continue;
        }

        struct stat statbuf;
        if (stat(path, &statbuf) == 0) {
            for (int i = 0; i < level; i++) {
                printf("  ");
            }

            if (S_ISDIR(statbuf.st_mode)) {
                printf("[Dir] %s/\n", entry->d_name);
                list_files_recursive(path, level + 1);
            } else {
                printf("[File] %s\n", entry->d_name);
            }
        }
    }
    closedir(dir);
}


static void sdmmc_card_print_info(const sdmmc_card_t* card) {
    if (card == NULL) {
        ESP_LOGE(SD_TAG, "Card is NULL");
        return;
    }

    ESP_LOGI(SD_TAG, "Name: %s", card->cid.name);
    ESP_LOGI(SD_TAG, "Type: %s", (card->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC");
    ESP_LOGI(SD_TAG, "Capacity: %lluMB", ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
    ESP_LOGI(SD_TAG, "Sector size: %dB", card->csd.sector_size);
    ESP_LOGI(SD_TAG, "Speed: %s", (card->csd.tr_speed > 25000000) ? "high speed" : "default speed");
    
    if (card->is_mem) {
        ESP_LOGI(SD_TAG, "Card is memory card");
        ESP_LOGI(SD_TAG, "CSD version: %d", card->csd.csd_ver);
        ESP_LOGI(SD_TAG, "Manufacture ID: %02x", card->cid.mfg_id);
        ESP_LOGI(SD_TAG, "Serial number: %08x", card->cid.serial);
    } else {
        ESP_LOGI(SD_TAG, "Card is not a memory card");
    }
}

esp_err_t sd_card_init(void) {
    esp_err_t ret;
#if USING_MMC

    ESP_LOGI(SD_TAG, "Initializing SD card in SDMMC mode (4-bit)...");

    
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    
    slot_config.clk = sd_card_manager.clkpin;
    slot_config.cmd = sd_card_manager.cmdpin;  // SDMMC_CMD -> GPIO 16
    slot_config.d0  = sd_card_manager.d0pin;  // SDMMC_D0  -> GPIO 14
    slot_config.d1  = sd_card_manager.d1pin;  // SDMMC_D1  -> GPIO 17
    slot_config.d2  = sd_card_manager.d2pin;  // SDMMC_D2  -> GPIO 21
    slot_config.d3  = sd_card_manager.d3pin;  // SDMMC_D3  -> GPIO 18

    
    host.flags = SDMMC_HOST_FLAG_4BIT;


    gpio_set_pull_mode(sd_card_manager.clkpin, GPIO_PULLUP_ONLY);  // CLK
    gpio_set_pull_mode(sd_card_manager.cmdpin, GPIO_PULLUP_ONLY);  // CMD
    gpio_set_pull_mode(sd_card_manager.d0pin, GPIO_PULLUP_ONLY);  // D0
    gpio_set_pull_mode(sd_card_manager.d1pin, GPIO_PULLUP_ONLY);  // D1
    gpio_set_pull_mode(sd_card_manager.d2pin, GPIO_PULLUP_ONLY);  // D2
    gpio_set_pull_mode(sd_card_manager.d3pin, GPIO_PULLUP_ONLY);  // D3

    slot_config.gpio_cd = GPIO_NUM_NC;  // Disable Card Detect pin
    slot_config.gpio_wp = GPIO_NUM_NC;  // Disable Write Protect pin
   
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

   
    ret = esp_vfs_fat_sdmmc_mount("/mnt", &host, &slot_config, &mount_config, &sd_card_manager.card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SD_TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(SD_TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ret;
    }

    
    sd_card_manager.is_initialized = true;
    sdmmc_card_print_info(sd_card_manager.card);
    ESP_LOGI(SD_TAG, "SD card initialized successfully");

    sd_card_setup_directory_structure();
#elif USING_SPI

    ESP_LOGI(SD_TAG, "Initializing SD card in SPI mode...");


    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_config;

    memset(&bus_config, 0, sizeof(spi_bus_config_t));

    bus_config.miso_io_num = sd_card_manager.spi_miso_pin;
    bus_config.mosi_io_num = sd_card_manager.spi_mosi_pin;
    bus_config.sclk_io_num = sd_card_manager.spi_clk_pin;

    ret = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(SD_TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = sd_card_manager.spi_cs_pin;
    slot_config.host_id = SPI2_HOST;

    ret = esp_vfs_fat_sdspi_mount("/mnt", &host, &slot_config, &mount_config, &sd_card_manager.card);
    if (ret != ESP_OK) {
        ESP_LOGE(SD_TAG, "Failed to mount filesystem: %s", esp_err_to_name(ret));
        spi_bus_free(host.slot);
        return ret;
    }

    sd_card_manager.is_initialized = true;
    sdmmc_card_print_info(sd_card_manager.card);
    ESP_LOGI(SD_TAG, "SD card initialized successfully in SPI mode.");

    sd_card_setup_directory_structure();

#endif
    return ESP_OK;
}


void sd_card_unmount(void) {
#if SOC_SDMMC_HOST_SUPPORTED && SOC_SDMMC_USE_GPIO_MATRIX
    if (sd_card_manager.is_initialized) {
        esp_vfs_fat_sdmmc_unmount();
        ESP_LOGI(SD_TAG, "SD card unmounted");
        sd_card_manager.is_initialized = false;
    }
#else
    if (sd_card_manager.is_initialized) {
        esp_vfs_fat_sdcard_unmount("/mnt", sd_card_manager.card);
        spi_bus_free(SPI2_HOST);
        ESP_LOGI(SD_TAG, "SD card unmounted");
    }
#endif
}

esp_err_t sd_card_append_file(const char* path, const void* data, size_t size) {
    if (!sd_card_manager.is_initialized) {
        ESP_LOGE(SD_TAG, "SD card is not initialized. Cannot append to file.");
        return ESP_FAIL;
    }

    FILE* f = fopen(path, "ab");
    if (f == NULL) {
        ESP_LOGE(SD_TAG, "Failed to open file for appending");
        return ESP_FAIL;
    }
    fwrite(data, 1, size, f);
    fclose(f);
    ESP_LOGI(SD_TAG, "Data appended to file: %s", path);
    return ESP_OK;
}

esp_err_t sd_card_write_file(const char* path, const void* data, size_t size) {
    if (!sd_card_manager.is_initialized) {
        ESP_LOGE(SD_TAG, "SD card is not initialized. Cannot write to file.");
        return ESP_FAIL;
    }

    FILE* f = fopen(path, "wb");
    if (f == NULL) {
        ESP_LOGE(SD_TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fwrite(data, 1, size, f);
    fclose(f);
    ESP_LOGI(SD_TAG, "File written: %s", path);
    return ESP_OK;
}


esp_err_t sd_card_read_file(const char* path) {
    if (!sd_card_manager.is_initialized) {
        ESP_LOGE(SD_TAG, "SD card is not initialized. Cannot read from file.");
        return ESP_FAIL;
    }

    FILE* f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(SD_TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[64];
    while (fgets(line, sizeof(line), f) != NULL) {
        printf("%s", line);
    }
    fclose(f);
    ESP_LOGI(SD_TAG, "File read: %s", path);
    return ESP_OK;
}


static bool has_full_permissions(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if ((st.st_mode & 0777) == 0777) {
            return true;
        }
    }
    return false;
}


esp_err_t sd_card_create_directory(const char* path) {
    if (!sd_card_manager.is_initialized) {
        ESP_LOGE(SD_TAG, "SD card is not initialized. Cannot create directory.");
        return ESP_FAIL;
    }

    if (sd_card_exists(path)) {
        ESP_LOGW(SD_TAG, "Directory already exists: %s", path);


        if (!has_full_permissions(path)) {
            ESP_LOGW(SD_TAG, "Directory %s does not have full permissions. Deleting and recreating.", path);

            
            if (rmdir(path) != 0) {
                ESP_LOGE(SD_TAG, "Failed to remove directory: %s", path);
                return ESP_FAIL;
            }

            int res = mkdir(path, 0777);
            if (res != 0) {
                ESP_LOGE(SD_TAG, "Failed to create directory: %s", path);
                return ESP_FAIL;
            }

            ESP_LOGI(SD_TAG, "Directory created: %s", path);

        } else {
            ESP_LOGI(SD_TAG, "Directory %s has correct permissions.", path);
            return ESP_OK;
        }
        return ESP_OK;
    }

    int res = mkdir(path, 0777);
    if (res != 0) {
        ESP_LOGE(SD_TAG, "Failed to create directory: %s", path);
        return ESP_FAIL;
    }

    ESP_LOGI(SD_TAG, "Directory created: %s", path);
    return ESP_OK;
}


bool sd_card_exists(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return true;
    } else {
        return false;
    }
}


esp_err_t sd_card_setup_directory_structure() {
    const char* root_dir = "/mnt/ghostesp";
    const char* debug_dir = "/mnt/ghostesp/debug";
    const char* pcaps_dir = "/mnt/ghostesp/pcaps";
    const char* scans_dir = "/mnt/ghostesp/scans";


    if (!sd_card_exists(root_dir)) {
        ESP_LOGI(SD_TAG, "Creating directory: %s", root_dir);
        esp_err_t ret = sd_card_create_directory(root_dir);
        if (ret != ESP_OK) {
            ESP_LOGE(SD_TAG, "Failed to create directory %s: %s", root_dir, esp_err_to_name(ret));
            return ret;
        }
    } else {
        ESP_LOGI(SD_TAG, "Directory %s already exists", root_dir);
    }

    
    if (!sd_card_exists(debug_dir)) {
        ESP_LOGI(SD_TAG, "Creating directory: %s", debug_dir);
        esp_err_t ret = sd_card_create_directory(debug_dir);
        if (ret != ESP_OK) {
            ESP_LOGE(SD_TAG, "Failed to create directory %s: %s", debug_dir, esp_err_to_name(ret));
            return ret;
        }
    } else {
        ESP_LOGI(SD_TAG, "Directory %s already exists", debug_dir);
    }

    
    if (!sd_card_exists(pcaps_dir)) {
        ESP_LOGI(SD_TAG, "Creating directory: %s", pcaps_dir);
        esp_err_t ret = sd_card_create_directory(pcaps_dir);
        if (ret != ESP_OK) {
            ESP_LOGE(SD_TAG, "Failed to create directory %s: %s", pcaps_dir, esp_err_to_name(ret));
            return ret;
        }
    } else {
        ESP_LOGI(SD_TAG, "Directory %s already exists", pcaps_dir);
    }

    
    if (!sd_card_exists(scans_dir)) {
        ESP_LOGI(SD_TAG, "Creating directory: %s", scans_dir);
        esp_err_t ret = sd_card_create_directory(scans_dir);
        if (ret != ESP_OK) {
            ESP_LOGE(SD_TAG, "Failed to create directory %s: %s", scans_dir, esp_err_to_name(ret));
            return ret;
        }
    } else {
        ESP_LOGI(SD_TAG, "Directory %s already exists", scans_dir);
    }

    ESP_LOGI(SD_TAG, "Directory structure successfully set up.");
    return ESP_OK;
}