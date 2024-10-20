#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <nvs_flash.h>
#include "core/utils.h"
#include <nvs.h>

// Enum for RGB Modes
typedef enum {
    RGB_MODE_STEALTH = 0,
    RGB_MODE_NORMAL = 1,
    RGB_MODE_RAINBOW = 2
} RGBMode;

typedef enum {
    ALIGNMENT_CM, // Center Middle
    ALIGNMENT_TL, // Top Left
    ALIGNMENT_TR, // Top Right
    ALIGNMENT_BR, // Bottom Right
    ALIGNMENT_BL  // Bottom Left
} PrinterAlignment;

// Enum for Supported Boards
typedef enum {
    FLIPPER_DEV_BOARD = 0,
    AWOK_DUAL_MINI = 1,
    AWOK_DUAL = 2,
    MARAUDER_V6 = 3,
    CARDPUTER = 4,
    DEVBOARD_PRO = 5,
    CUSTOM = 6
} SupportedBoard;

// Struct for advanced pin configuration
typedef struct {
    int8_t neopixel_pin;
    int8_t sd_card_spi_miso;
    int8_t sd_card_spi_mosi;
    int8_t sd_card_spi_clk;
    int8_t sd_card_spi_cs;
    int8_t sd_card_mmc_cmd;
    int8_t sd_card_mmc_clk;
    int8_t sd_card_mmc_d0;
    int8_t gps_tx_pin;
    int8_t gps_rx_pin;
} PinConfig;

// Struct for settings
typedef struct {
    RGBMode rgb_mode;
    float channel_delay;
    uint16_t broadcast_speed;
    char ap_ssid[33];       // Max SSID length is 32 bytes + null terminator
    char ap_password[65];   // Max password length is 64 bytes + null terminator
    uint8_t rgb_speed;

    // Evil Portal settings
    char portal_url[129];         // URL or file path for offline mode
    char portal_ssid[33];         // SSID for the Evil Portal
    char portal_password[65];     // Password for the Evil Portal
    char portal_ap_ssid[33];      // AP SSID for the Evil Portal
    char portal_domain[65];       // Domain for the Evil Portal
    bool portal_offline_mode;     // Toggle for offline/online mode

    // Power Printer settings
    char printer_ip[16];          // Printer IP address (IPv4)
    char printer_text[257];       // Last printed text (max 256 characters + null terminator)
    uint8_t printer_font_size;    // Font size for printing
    PrinterAlignment printer_alignment; // Text alignment
    bool printer_connected;       // Whether the printer is connected or not

    // Pin settings
    SupportedBoard board_type;    // Type of the selected board
    PinConfig custom_pin_config;  // Custom pin configuration for "Custom" board type
} FSettings;

// Function declarations
void settings_init(FSettings* settings);
void settings_deinit(void);
void settings_load(FSettings* settings);
void settings_save(const FSettings* settings);
void settings_set_defaults(FSettings* settings);

// Getters and Setters for core settings
void settings_set_rgb_mode(FSettings* settings, RGBMode mode);
RGBMode settings_get_rgb_mode(const FSettings* settings);

void settings_set_channel_delay(FSettings* settings, float delay_ms);
float settings_get_channel_delay(const FSettings* settings);

void settings_set_broadcast_speed(FSettings* settings, uint16_t speed);
uint16_t settings_get_broadcast_speed(const FSettings* settings);

void settings_set_ap_ssid(FSettings* settings, const char* ssid);
const char* settings_get_ap_ssid(const FSettings* settings);

void settings_set_ap_password(FSettings* settings, const char* password);
const char* settings_get_ap_password(const FSettings* settings);

void settings_set_rgb_speed(FSettings* settings, uint8_t speed);
uint8_t settings_get_rgb_speed(const FSettings* settings);

// Getters and Setters for Evil Portal
void settings_set_portal_url(FSettings* settings, const char* url);
const char* settings_get_portal_url(const FSettings* settings);

void settings_set_portal_ssid(FSettings* settings, const char* ssid);
const char* settings_get_portal_ssid(const FSettings* settings);

void settings_set_portal_password(FSettings* settings, const char* password);
const char* settings_get_portal_password(const FSettings* settings);

void settings_set_portal_ap_ssid(FSettings* settings, const char* ap_ssid);
const char* settings_get_portal_ap_ssid(const FSettings* settings);

void settings_set_portal_domain(FSettings* settings, const char* domain);
const char* settings_get_portal_domain(const FSettings* settings);

void settings_set_portal_offline_mode(FSettings* settings, bool offline_mode);
bool settings_get_portal_offline_mode(const FSettings* settings);

// Getters and Setters for Power Printer
void settings_set_printer_ip(FSettings* settings, const char* ip);
const char* settings_get_printer_ip(const FSettings* settings);

void settings_set_printer_text(FSettings* settings, const char* text);
const char* settings_get_printer_text(const FSettings* settings);

void settings_set_printer_font_size(FSettings* settings, uint8_t font_size);
uint8_t settings_get_printer_font_size(const FSettings* settings);

void settings_set_printer_alignment(FSettings* settings, PrinterAlignment alignment);
PrinterAlignment settings_get_printer_alignment(const FSettings* settings);

void settings_set_printer_connected(FSettings* settings, bool connected);
bool settings_get_printer_connected(const FSettings* settings);

// Getters and Setters for Pin Configurations
void settings_set_board_type(FSettings* settings, SupportedBoard board);
SupportedBoard settings_get_board_type(const FSettings* settings);

void settings_set_custom_pin_config(FSettings* settings, PinConfig pin_config);
PinConfig settings_get_custom_pin_config(const FSettings* settings);

void settings_apply_board_pin_config(FSettings* settings);
static void settings_set_board_pin_defaults(FSettings* settings, SupportedBoard board);

static nvs_handle_t nvsHandle;

FSettings G_Settings;

#endif // SETTINGS_MANAGER_H