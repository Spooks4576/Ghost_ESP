#include "managers/settings_manager.h"
#include "managers/rgb_manager.h"
#include <string.h>
#include <esp_log.h>

#define S_TAG "SETTINGS"

// NVS Keys
static const char* NVS_RGB_MODE_KEY = "rgb_mode";
static const char* NVS_CHANNEL_DELAY_KEY = "channel_delay";
static const char* NVS_BROADCAST_SPEED_KEY = "broadcast_speed";
static const char* NVS_AP_SSID_KEY = "ap_ssid";
static const char* NVS_AP_PASSWORD_KEY = "ap_password";
static const char* NVS_RGB_SPEED_KEY = "rgb_speed";
static const char* NVS_PORTAL_URL_KEY = "portal_url";
static const char* NVS_PORTAL_SSID_KEY = "portal_ssid";
static const char* NVS_PORTAL_PASSWORD_KEY = "portal_password";
static const char* NVS_PORTAL_AP_SSID_KEY = "portal_ap_ssid";
static const char* NVS_PORTAL_DOMAIN_KEY = "portal_domain";
static const char* NVS_PORTAL_OFFLINE_KEY = "portal_offline";
static const char* NVS_PRINTER_IP_KEY = "printer_ip";
static const char* NVS_PRINTER_TEXT_KEY = "printer_text";
static const char* NVS_PRINTER_FONT_SIZE_KEY = "printer_font_size";
static const char* NVS_PRINTER_ALIGNMENT_KEY = "printer_alignment";
static const char* NVS_PRINTER_CONNECTED_KEY = "printer_connected";
static const char* NVS_BOARD_TYPE_KEY = "board_type";
static const char* NVS_CUSTOM_PIN_CONFIG_KEY = "custom_pin_config";

void settings_init(FSettings* settings) {
    settings_set_defaults(settings);
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    err = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    if (err == ESP_OK) {
        settings_load(settings);
        ESP_LOGI(S_TAG, "Settings loaded successfully.");
    } else {
        ESP_LOGE(S_TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
    }
}

void settings_deinit(void) {
    nvs_close(nvsHandle);
}

void settings_set_defaults(FSettings* settings) {
    settings->rgb_mode = RGB_MODE_NORMAL;
    settings->channel_delay = 1.0f;
    settings->broadcast_speed = 500;
    strcpy(settings->ap_ssid, "GhostNet");
    strcpy(settings->ap_password, "GhostNet");
    settings->rgb_speed = 50;

    // Evil Portal defaults
    strcpy(settings->portal_url, "/default/path");
    strcpy(settings->portal_ssid, "EvilPortal");
    strcpy(settings->portal_password, "EvilPortalPass");
    strcpy(settings->portal_ap_ssid, "EvilAP");
    strcpy(settings->portal_domain, "portal.local");
    settings->portal_offline_mode = false;

    // Power Printer defaults
    strcpy(settings->printer_ip, "192.168.1.100");
    strcpy(settings->printer_text, "Default Text");
    settings->printer_font_size = 12;
    settings->printer_alignment = ALIGNMENT_CM;
}

void settings_load(FSettings* settings) {
    esp_err_t err;
    uint8_t value_u8;
    uint16_t value_u16;
    float value_float;
    size_t str_size;

    // Load RGB Mode
    err = nvs_get_u8(nvsHandle, NVS_RGB_MODE_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->rgb_mode = (RGBMode)value_u8;
    }

    size_t required_size = sizeof(value_float); // Set the size of the buffer
    err = nvs_get_blob(nvsHandle, NVS_CHANNEL_DELAY_KEY, &value_float, &required_size);
    if (err == ESP_OK) {
        settings->channel_delay = value_float;
    } else {
        ESP_LOGE(S_TAG, "Failed to load Channel Delay: %s", esp_err_to_name(err));
    }

    // Load Broadcast Speed
    err = nvs_get_u16(nvsHandle, NVS_BROADCAST_SPEED_KEY, &value_u16);
    if (err == ESP_OK) {
        settings->broadcast_speed = value_u16;
    }

    // Load AP SSID
    str_size = sizeof(settings->ap_ssid);
    err = nvs_get_str(nvsHandle, NVS_AP_SSID_KEY, settings->ap_ssid, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load AP SSID");
    }

    // Load AP Password
    str_size = sizeof(settings->ap_password);
    err = nvs_get_str(nvsHandle, NVS_AP_PASSWORD_KEY, settings->ap_password, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load AP Password");
    }

    // Load RGB Speed
    err = nvs_get_u8(nvsHandle, NVS_RGB_SPEED_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->rgb_speed = value_u8;
    }

    // Load Evil Portal settings
    str_size = sizeof(settings->portal_url);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_URL_KEY, settings->portal_url, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load Portal URL");
    }

    str_size = sizeof(settings->portal_ssid);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_SSID_KEY, settings->portal_ssid, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load Portal SSID");
    }

    str_size = sizeof(settings->portal_password);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_PASSWORD_KEY, settings->portal_password, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load Portal Password");
    }

    str_size = sizeof(settings->portal_ap_ssid);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_AP_SSID_KEY, settings->portal_ap_ssid, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load Portal AP SSID");
    }

    str_size = sizeof(settings->portal_domain);
    err = nvs_get_str(nvsHandle, NVS_PORTAL_DOMAIN_KEY, settings->portal_domain, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load Portal Domain");
    }

    err = nvs_get_u8(nvsHandle, NVS_PORTAL_OFFLINE_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->portal_offline_mode = value_u8;
    }

    // Load Power Printer settings
    str_size = sizeof(settings->printer_ip);
    err = nvs_get_str(nvsHandle, NVS_PRINTER_IP_KEY, settings->printer_ip, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load Printer IP");
    }

    str_size = sizeof(settings->printer_text);
    err = nvs_get_str(nvsHandle, NVS_PRINTER_TEXT_KEY, settings->printer_text, &str_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to load Printer Text");
    }

    err = nvs_get_u8(nvsHandle, NVS_PRINTER_FONT_SIZE_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->printer_font_size = value_u8;
    }

    err = nvs_get_u8(nvsHandle, NVS_PRINTER_ALIGNMENT_KEY, &value_u8);
    if (err == ESP_OK) {
        settings->printer_alignment = (PrinterAlignment)value_u8;
    }

    ESP_LOGI(S_TAG, "Settings loaded from NVS.");
}

void settings_save(const FSettings* settings) {
    esp_err_t err;

    // Save RGB Mode
    err = nvs_set_u8(nvsHandle, NVS_RGB_MODE_KEY, (uint8_t)settings->rgb_mode);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save RGB Mode");
    }

    // Save Channel Delay
    err = nvs_set_blob(nvsHandle, NVS_CHANNEL_DELAY_KEY, &settings->channel_delay, sizeof(settings->channel_delay));
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Channel Delay");
    }

    // Save Broadcast Speed
    err = nvs_set_u16(nvsHandle, NVS_BROADCAST_SPEED_KEY, settings->broadcast_speed);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Broadcast Speed");
    }

    // Save AP SSID
    err = nvs_set_str(nvsHandle, NVS_AP_SSID_KEY, settings->ap_ssid);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save AP SSID");
    }

    // Save AP Password
    err = nvs_set_str(nvsHandle, NVS_AP_PASSWORD_KEY, settings->ap_password);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save AP Password");
    }

    // Save RGB Speed
    err = nvs_set_u8(nvsHandle, NVS_RGB_SPEED_KEY, settings->rgb_speed);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save RGB Speed");
    }

    // Save Evil Portal settings
    err = nvs_set_str(nvsHandle, NVS_PORTAL_URL_KEY, settings->portal_url);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Portal URL");
    }

    err = nvs_set_str(nvsHandle, NVS_PORTAL_SSID_KEY, settings->portal_ssid);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Portal SSID");
    }

    err = nvs_set_str(nvsHandle, NVS_PORTAL_PASSWORD_KEY, settings->portal_password);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Portal Password");
    }

    err = nvs_set_str(nvsHandle, NVS_PORTAL_AP_SSID_KEY, settings->portal_ap_ssid);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Portal AP SSID");
    }

    err = nvs_set_str(nvsHandle, NVS_PORTAL_DOMAIN_KEY, settings->portal_domain);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Portal Domain");
    }

    err = nvs_set_u8(nvsHandle, NVS_PORTAL_OFFLINE_KEY, settings->portal_offline_mode);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Portal Offline Mode");
    }

    // Save Power Printer settings
    err = nvs_set_str(nvsHandle, NVS_PRINTER_IP_KEY, settings->printer_ip);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Printer IP");
    }

    err = nvs_set_str(nvsHandle, NVS_PRINTER_TEXT_KEY, settings->printer_text);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Printer Text");
    }

    err = nvs_set_u8(nvsHandle, NVS_PRINTER_FONT_SIZE_KEY, settings->printer_font_size);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Printer Font Size");
    }

    err = nvs_set_u8(nvsHandle, NVS_PRINTER_ALIGNMENT_KEY, (uint8_t)settings->printer_alignment);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to save Printer Alignment");
    }


    if (settings->rgb_mode == 0)
    {
        if (rgb_effect_task_handle != NULL)
        {
            vTaskDelete(&rgb_effect_task_handle);
        }
        rgb_manager_set_color(&rgb_manager, 1, 0, 0, 0, false);
    }
    else 
    {
        if (rgb_effect_task_handle == NULL)
        {
            xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
        }
    }

    // Commit all changes
    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
        ESP_LOGE(S_TAG, "Failed to commit NVS changes");
    } else {
        ESP_LOGI(S_TAG, "Settings saved to NVS.");
    }
}

// Core Settings Getters and Setters
void settings_set_rgb_mode(FSettings* settings, RGBMode mode) {
    settings->rgb_mode = mode;
}

RGBMode settings_get_rgb_mode(const FSettings* settings) {
    return settings->rgb_mode;
}

void settings_set_channel_delay(FSettings* settings, float delay_ms) {
    settings->channel_delay = delay_ms;
}

float settings_get_channel_delay(const FSettings* settings) {
    return settings->channel_delay;
}

void settings_set_broadcast_speed(FSettings* settings, uint16_t speed) {
    settings->broadcast_speed = speed;
}

uint16_t settings_get_broadcast_speed(const FSettings* settings) {
    return settings->broadcast_speed;
}

void settings_set_ap_ssid(FSettings* settings, const char* ssid) {
    strncpy(settings->ap_ssid, ssid, sizeof(settings->ap_ssid) - 1);
    settings->ap_ssid[sizeof(settings->ap_ssid) - 1] = '\0';
}

const char* settings_get_ap_ssid(const FSettings* settings) {
    return settings->ap_ssid;
}

void settings_set_ap_password(FSettings* settings, const char* password) {
    strncpy(settings->ap_password, password, sizeof(settings->ap_password) - 1);
    settings->ap_password[sizeof(settings->ap_password) - 1] = '\0';
}

const char* settings_get_ap_password(const FSettings* settings) {
    return settings->ap_password;
}

void settings_set_rgb_speed(FSettings* settings, uint8_t speed) {
    settings->rgb_speed = speed;
}

uint8_t settings_get_rgb_speed(const FSettings* settings) {
    return settings->rgb_speed;
}

// Evil Portal Getters and Setters
void settings_set_portal_url(FSettings* settings, const char* url) {
    strncpy(settings->portal_url, url, sizeof(settings->portal_url) - 1);
    settings->portal_url[sizeof(settings->portal_url) - 1] = '\0';
}

const char* settings_get_portal_url(const FSettings* settings) {
    return settings->portal_url;
}

void settings_set_portal_ssid(FSettings* settings, const char* ssid) {
    strncpy(settings->portal_ssid, ssid, sizeof(settings->portal_ssid) - 1);
    settings->portal_ssid[sizeof(settings->portal_ssid) - 1] = '\0';
}

const char* settings_get_portal_ssid(const FSettings* settings) {
    return settings->portal_ssid;
}

void settings_set_portal_password(FSettings* settings, const char* password) {
    strncpy(settings->portal_password, password, sizeof(settings->portal_password) - 1);
    settings->portal_password[sizeof(settings->portal_password) - 1] = '\0';
}

const char* settings_get_portal_password(const FSettings* settings) {
    return settings->portal_password;
}

void settings_set_portal_ap_ssid(FSettings* settings, const char* ap_ssid) {
    strncpy(settings->portal_ap_ssid, ap_ssid, sizeof(settings->portal_ap_ssid) - 1);
    settings->portal_ap_ssid[sizeof(settings->portal_ap_ssid) - 1] = '\0';
}

const char* settings_get_portal_ap_ssid(const FSettings* settings) {
    return settings->portal_ap_ssid;
}

void settings_set_portal_domain(FSettings* settings, const char* domain) {
    strncpy(settings->portal_domain, domain, sizeof(settings->portal_domain) - 1);
    settings->portal_domain[sizeof(settings->portal_domain) - 1] = '\0';
}

const char* settings_get_portal_domain(const FSettings* settings) {
    return settings->portal_domain;
}

void settings_set_portal_offline_mode(FSettings* settings, bool offline_mode) {
    settings->portal_offline_mode = offline_mode;
}

bool settings_get_portal_offline_mode(const FSettings* settings) {
    return settings->portal_offline_mode;
}

// Power Printer Getters and Setters
void settings_set_printer_ip(FSettings* settings, const char* ip) {
    strncpy(settings->printer_ip, ip, sizeof(settings->printer_ip) - 1);
    settings->printer_ip[sizeof(settings->printer_ip) - 1] = '\0';
}

const char* settings_get_printer_ip(const FSettings* settings) {
    return settings->printer_ip;
}

void settings_set_printer_text(FSettings* settings, const char* text) {
    strncpy(settings->printer_text, text, sizeof(settings->printer_text) - 1);
    settings->printer_text[sizeof(settings->printer_text) - 1] = '\0';
}

const char* settings_get_printer_text(const FSettings* settings) {
    return settings->printer_text;
}

void settings_set_printer_font_size(FSettings* settings, uint8_t font_size) {
    settings->printer_font_size = font_size;
}

uint8_t settings_get_printer_font_size(const FSettings* settings) {
    return settings->printer_font_size;
}

void settings_set_printer_alignment(FSettings* settings, PrinterAlignment alignment) {
    settings->printer_alignment = alignment;
}

PrinterAlignment settings_get_printer_alignment(const FSettings* settings) {
    return settings->printer_alignment;
}