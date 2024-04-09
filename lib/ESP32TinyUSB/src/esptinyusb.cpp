#include "Arduino.h"
#include "hal/usb_hal.h"
#include "soc/usb_periph.h"
#include "driver/periph_ctrl.h"
#include "driver/gpio.h"
#if CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/gpio.h"
#endif
#if CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/gpio.h"
#endif
#include "esp_log.h"

#include "esptinyusb.h"
#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#if CONFIG_TINYUSB_ENABLED

#define _manufacturer  "Espressif"
#define _product  "ESP32 Arduino Device"
#define _serial  "1234-5678"

#define _cdc "CDC class"
#define _dfu  "DFU class"
#define _hid  "HID class"
#define _midi  "MIDI class"
#define _msc  "MSC class"
#define _vendor  "Vendor class (webUSB)"

#define TAG __func__
static EspTinyUSB* _device = NULL;
bool EspTinyUSB::enableCDC;
bool EspTinyUSB::enableMSC;
bool EspTinyUSB::enableMIDI;
bool EspTinyUSB::enableHID;
bool EspTinyUSB::enableVENDOR;
bool EspTinyUSB::enableDFU;
uint8_t EspTinyUSB::ifIdx = 0;
int EspTinyUSB::total = 9;
uint8_t EspTinyUSB::count = 0;
uint8_t EspTinyUSB::desc_configuration[1500] = {0};
uint16_t EspTinyUSB::_VID = 0x303a;
uint16_t EspTinyUSB::_PID = 0x0002;
descriptor_strings_t EspTinyUSB::strings;
uint16_t EspTinyUSB::_revision;
uint16_t EspTinyUSB::_bcdUSB;
size_t EspTinyUSB::hid_report_desc_len = 0;
USBCallbacks* EspTinyUSB::m_callbacks;
static void IRAM_ATTR usb_persist_shutdown_handler(void);

static void esptinyusbtask(void *p)
{
    (void)p;
    ESP_LOGD(TAG, "USB tud_task created");
    while (1)
    {
        // tinyusb device task
        tud_task();
    }
}

EspTinyUSB::EspTinyUSB(bool extPhy)
{
    if (!isEnabled)
    {

        bool usb_did_persist = (USB_WRAP.date.val == USBDC_PERSIST_ENA);

        if(usb_did_persist){
            // Enable USB/IO_MUX peripheral reset, if coming from persistent reboot
            REG_CLR_BIT(RTC_CNTL_USB_CONF_REG, RTC_CNTL_IO_MUX_RESET_DISABLE);
            REG_CLR_BIT(RTC_CNTL_USB_CONF_REG, RTC_CNTL_USB_RESET_DISABLE);
        }

        if(1){
            // Reset USB module
            periph_module_reset((periph_module_t)PERIPH_USB_MODULE);
            periph_module_enable((periph_module_t)PERIPH_USB_MODULE);
        }
        

        // Hal init
        usb_hal_context_t hal = {
            .use_external_phy = extPhy};
        usb_hal_init(&hal);
        /* usb_periph_iopins currently configures USB_OTG as USB Device.
        * Introduce additional parameters in usb_hal_context_t when adding support
        * for USB Host.
        */
        for (const usb_iopin_dsc_t *iopin = usb_periph_iopins; iopin->pin != -1; ++iopin)
        {
            if ((iopin->ext_phy_only == 0))
            {
                gpio_pad_select_gpio(iopin->pin);
                if (iopin->is_output)
                {
                    gpio_matrix_out(iopin->pin, iopin->func, false, false);
                }
                else
                {
                    gpio_matrix_in(iopin->pin, iopin->func, false);
                    gpio_pad_input_enable(iopin->pin);
                }
                gpio_pad_unhold(iopin->pin);
            }
        }
        gpio_set_drive_capability((gpio_num_t)USBPHY_DP_NUM, GPIO_DRIVE_CAP_3);
        gpio_set_drive_capability((gpio_num_t)USBPHY_DM_NUM, GPIO_DRIVE_CAP_3);
        isEnabled = true;
        _device = this;
        _revision = 0x100;

        strings.langId[0] = 0x09;
        strings.langId[1] = 0x04;
        strings.manufacturer = _manufacturer;
        strings.product = _product;
        strings.serial = _serial;
        strings.cdc = _cdc;
        strings.dfu = _dfu;
        strings.hid = _hid;
        strings.midi = _midi;
        strings.msc = _msc;
        strings.vendor = _vendor;

        _bcdUSB = 0x200;
        esp_register_shutdown_handler(usb_persist_shutdown_handler);
    }
}

void EspTinyUSB::manufacturer(char* str)
{
    strings.manufacturer = str;
}

void EspTinyUSB::product(char* str)
{
    strings.product = str;
}

void EspTinyUSB::serial(char* str)
{
    strings.serial = str;
}

void EspTinyUSB::revision(uint16_t val)
{
    _revision = val;
}

bool EspTinyUSB::begin(char* str, uint8_t n)
{
    switch (n)
    {
        case 4:
            if(str != nullptr)
                strings.cdc = str;
            break;
        
        case 5:
            if(str != nullptr)
                strings.msc = str;
            break;
        
        case 6:
            if(str != nullptr)
                strings.hid = str;
            break;
        
        case 7:
            if(str != nullptr)
                strings.vendor = str;
            break;
        
        case 8:
            if(str != nullptr)
                strings.midi = str;
            break;
        
        case 9:
            if(str != nullptr)
                strings.dfu = str;
            break;
        
        default:
            break;
    }

    getDeviceDescriptor();
    setDeviceDescriptorStrings();
    getConfigurationDescriptor();

    if (tusb_inited()) {
        return  true;
    }

    if(!tusb_init()) {
        ESP_LOGE("TAG", "failed to init");
        return false;
    }

    if (usbTaskHandle != nullptr) {
        return true;
    }

    return xTaskCreate(esptinyusbtask, "espUSB", 4 * 1024, NULL, 24, &usbTaskHandle) == pdTRUE;
}

void EspTinyUSB::registerDeviceCallbacks(USBCallbacks* cb)
{
    m_callbacks = cb;
}


void EspTinyUSB::deviceID(uint16_t vid, uint16_t pid)
{
    _VID = vid;
    _PID = pid;
}

void EspTinyUSB::deviceID(uint16_t* vid, uint16_t* pid)
{
    *vid = _VID;
    *pid = _PID;
}

void EspTinyUSB::useDFU(bool en)
{
    enableDFU = en;
}

void EspTinyUSB::useMSC(bool en)
{
    enableMSC = en;
}

int usb_persist_mode = RESTART_BOOTLOADER;
void EspTinyUSB::persistentReset(restart_type_t _usb_persist_mode)
{
    usb_persist_mode = _usb_persist_mode;
    if(usb_persist_mode != RESTART_NO_PERSIST){
        if (usb_persist_mode == RESTART_BOOTLOADER) {
            REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        } else if (usb_persist_mode == RESTART_BOOTLOADER_DFU) {
            //DFU Download
            chip_usb_set_persist_flags(USBDC_BOOT_DFU);
            REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        } else {
            //USB Persist reboot
            // chip_usb_set_persist_flags(USBDC_PERSIST_ENA);
        }
    }
}

static void IRAM_ATTR usb_persist_shutdown_handler(void)
{
    if(usb_persist_mode != RESTART_NO_PERSIST){
        int usb_persist_enabled = 0;
        if (usb_persist_enabled) {
            usb_dc_prepare_persist();
        }
        if (usb_persist_mode == RESTART_BOOTLOADER) {
            //USB CDC Download
            if (usb_persist_enabled) {
                chip_usb_set_persist_flags(USBDC_PERSIST_ENA);
            } else {
                periph_module_reset(PERIPH_USB_MODULE);
                periph_module_enable(PERIPH_USB_MODULE);
            }
            REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        } else if (usb_persist_mode == RESTART_BOOTLOADER_DFU) {
            //DFU Download
            // Reset USB Core
            USB0.grstctl |= USB_CSFTRST;
            while ((USB0.grstctl & USB_CSFTRST) == USB_CSFTRST){}
            chip_usb_set_persist_flags(USBDC_BOOT_DFU);
            REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        } else if (usb_persist_enabled) {
            //USB Persist reboot
            chip_usb_set_persist_flags(USBDC_PERSIST_ENA);
        }
    }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    if (EspTinyUSB::m_callbacks)
    {
        EspTinyUSB::m_callbacks->onMount();
    }
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    if (EspTinyUSB::m_callbacks)
    {
        EspTinyUSB::m_callbacks->onUnmount();
    }
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    if (EspTinyUSB::m_callbacks)
    {
        EspTinyUSB::m_callbacks->onSuspend(remote_wakeup_en);
    }
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    if (EspTinyUSB::m_callbacks)
    {
        EspTinyUSB::m_callbacks->onResume();
    }
}

#endif
#endif
