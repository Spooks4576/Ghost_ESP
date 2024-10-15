#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include "esp_err.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_types.h"
#include <stdbool.h>


typedef struct {
    sdmmc_card_t *card;
    bool is_initialized;
    int clkpin;
    int cmdpin;
    int d0pin;
    int d1pin;
    int d2pin;
    int d3pin;

    // SPI
    int spi_cs_pin;
    int spi_miso_pin;
    int spi_mosi_pin;
    int spi_clk_pin;
} sd_card_manager_t;


extern sd_card_manager_t sd_card_manager;


esp_err_t sd_card_init();
void sd_card_unmount(void);
esp_err_t sd_card_append_file(const char* path, const void* data, size_t size);
esp_err_t sd_card_write_file(const char* path, const void* data, size_t size);
esp_err_t sd_card_read_file(const char* path);
esp_err_t sd_card_create_directory(const char* path);
bool sd_card_exists(const char* path);
esp_err_t sd_card_setup_directory_structure();


#endif // SD_CARD_MANAGER_H