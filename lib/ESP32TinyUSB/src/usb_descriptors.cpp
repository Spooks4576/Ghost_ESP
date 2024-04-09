// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esptinyusb.h"
#include "usb_descriptors.h"
#include "esp_log.h"

#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#if CONFIG_TINYUSB_ENABLED

#define USB_TUSB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) | _PID_MAP(DFU_RT, 5))

uint8_t* EspTinyUSB::descriptor_config = NULL;
uint8_t* EspTinyUSB::descriptor_config_if = NULL;


uint8_t* EspTinyUSB::getConfigurationDescriptor()
{
    int CONFIG_TOTAL_LEN = TUD_CONFIG_DESC_LEN + 
                        (int)enableCDC * TUD_CDC_DESC_LEN + (int)enableMSC * TUD_MSC_DESC_LEN +
                        (int)enableHID * EspTinyUSB::hid_report_desc_len + (int)enableVENDOR * TUD_VENDOR_DESC_LEN + 
                        (int)enableMIDI * TUD_MIDI_DESC_LEN + (int)enableDFU * TUD_DFU_RT_DESC_LEN;


    // interface count, string index, total length, attribute, power in mA
    uint8_t dcd[TUD_CONFIG_DESC_LEN] = {TUD_CONFIG_DESCRIPTOR(1, count, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500)};
    memcpy(&desc_configuration[0], dcd, sizeof(dcd));

    if(EspTinyUSB::descriptor_config_if != NULL) {
        free(EspTinyUSB::descriptor_config_if);
    }
    EspTinyUSB::descriptor_config_if = (uint8_t*)calloc(1, total);
    memcpy(EspTinyUSB::descriptor_config_if, desc_configuration, total);
    log_d("descriptor length: %d\n", total);

    return EspTinyUSB::descriptor_config_if;
}

void EspTinyUSB::setDeviceDescriptorStrings()
{
    descriptor_str_config[0] = strings.langId;       // 0: supported language is English (0x0409)
    descriptor_str_config[1] = strings.manufacturer; // 1: Manufacturer
    descriptor_str_config[2] = strings.product;      // 2: Product
    descriptor_str_config[3] = strings.serial;       // 3: Serials, should use chip ID

    descriptor_str_config[4] = strings.cdc;          // 4: CDC Interface
    descriptor_str_config[5] = strings.msc;          // 5: MSC Interface
    descriptor_str_config[6] = strings.hid;          // 6: HIDs
    descriptor_str_config[7] = strings.vendor;       // 7: Vendor
    descriptor_str_config[8] = strings.midi;         // 8: MIDI
    descriptor_str_config[9] = strings.dfu;          // 9: DFU
}

/**
 * Device descriptor
 */
tusb_desc_device_t EspTinyUSB::getDeviceDescriptor()
{
    /**** Kconfig driven Descriptor ****/
    tusb_desc_device_t _descriptor_config = {
        .bLength = sizeof(_descriptor_config),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = _bcdUSB,

        .bDeviceClass = 0x00,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = _VID,
        .idProduct = _PID,

        .bcdDevice = _revision,

        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,

        .bNumConfigurations = 0x01,
    };


    if (enableCDC) {
        // Use Interface Association Descriptor (IAD) for CDC
        // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
        _descriptor_config.bDeviceClass = TUSB_CLASS_MISC;
        _descriptor_config.bDeviceSubClass = MISC_SUBCLASS_COMMON;
        _descriptor_config.bDeviceProtocol = MISC_PROTOCOL_IAD;
    } 


    if(descriptor_config != NULL) {
        free(descriptor_config);
    }
    descriptor_config = (uint8_t *)calloc(1, sizeof(_descriptor_config));

    memcpy(descriptor_config, &_descriptor_config, sizeof(_descriptor_config));

    return _descriptor_config;
}

// =============================================================================
// Driver functions
// =============================================================================
/**
 * @brief Invoked when received GET DEVICE DESCRIPTOR.
 * Application returns pointer to descriptor
 *
 * @return uint8_t const*
 */
uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const*)EspTinyUSB::descriptor_config;
}

/**
 * @brief Invoked when received GET CONFIGURATION DESCRIPTOR.
 * Descriptor contents must exist long enough for transfer to complete
 *
 * @param index
 * @return uint8_t const* Application return pointer to descriptor
 */
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index; // for multiple configurations
    return (uint8_t const*)EspTinyUSB::descriptor_config_if;
}

static uint16_t _desc_str[32];

/**
 * @brief Invoked when received GET STRING DESCRIPTOR request.
 * Application returns pointer to descriptor, whose contents must exist long
 * enough for transfer to complete
 *
 * @param index
 * @return uint16_t const*
 */
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    uint8_t chr_count = 0;
    if (index == 0)
    {
        memcpy(&_desc_str[1], descriptor_str_config[0], 2);
        chr_count = 1;
    }
    else
    {
        // Convert ASCII string into UTF-16
        if (index >= sizeof(descriptor_str_config) /
                         sizeof(descriptor_str_config[0]))
        {
            return NULL;
        }
        const char *str = descriptor_str_config[index];
        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31)
        {
            chr_count = 31;
        }
        for (uint8_t i = 0; i < chr_count; i++)
        {
            _desc_str[1 + i] = str[i];
        }
    }

    // first byte is len, second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}

#endif
#endif
