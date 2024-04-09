
#pragma once
#include "Arduino.h"
#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#if CONFIG_TINYUSB_ENABLED

#include "common/tusb_common.h"
#include "tusb.h"

#include "soc/rtc_cntl_reg.h"
#include "soc/usb_struct.h"
#include "soc/usb_reg.h"
#include "soc/usb_wrap_reg.h"
#include "soc/usb_wrap_struct.h"
#if CONFIG_IDF_TARGET_ESP32S2
  #include "esp32s2/rom/usb/usb_persist.h"
  #include "esp32s2/rom/usb/usb_dc.h"
  #include "esp32s2/rom/usb/chip_usb_dw_wrapper.h"
#endif
#if CONFIG_IDF_TARGET_ESP32S3
  #include "esp32s3/rom/usb/usb_persist.h"
  #include "esp32s3/rom/usb/usb_dc.h"
  #include "esp32s3/rom/usb/chip_usb_dw_wrapper.h"
#endif


#include "usb_descriptors.h"
/*
 * USB Persistence API
 * */
typedef enum {
    RESTART_NO_PERSIST,
    RESTART_PERSIST,
    RESTART_BOOTLOADER,
    RESTART_BOOTLOADER_DFU,
    RESTART_TYPE_MAX
} restart_type_t;

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+
class USBCallbacks {
public:
    virtual ~USBCallbacks() { }
    virtual void onMount() { }
    virtual void onUnmount() { }
    virtual void onSuspend(bool remote_wakeup_en) { }
    virtual void onResume() { }
};

typedef struct
{
    char langId[2];
    const char *manufacturer;
    const char *product;
    const char *serial;

    const char *cdc;
    const char *dfu;
    const char *hid;
    const char *midi;
    const char *msc;
    const char *vendor;
} descriptor_strings_t;

static const char *descriptor_str_config[11];


class EspTinyUSB : public Stream
{
public:
    EspTinyUSB(bool extPhy = false);
    bool begin(char* str, uint8_t n);
    static void registerDeviceCallbacks(USBCallbacks* cb);
    void persistentReset(restart_type_t usb_persist_mode);

    static size_t hid_report_desc_len;

    tusb_desc_device_t getDeviceDescriptor();
    void setDeviceDescriptorStrings();
    uint8_t *getConfigurationDescriptor();
    void deviceID(uint16_t, uint16_t);
    void deviceID(uint16_t *, uint16_t *);
    void useDFU(bool);
    void useMSC(bool);
    static void manufacturer(char*);
    static void product(char*);
    static void serial(char*);
    static void revision(uint16_t);
    virtual void setBaseEP(uint8_t) = 0;


    friend uint8_t const *tud_descriptor_device_cb(void);
    friend uint8_t const *tud_descriptor_configuration_cb(uint8_t index);
    friend uint8_t const *tud_descriptor_device_cb(void);
    friend uint8_t const *tud_descriptor_configuration_cb(uint8_t index);
    friend uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid);
    friend tusb_desc_device_t *tusb_get_active_desc(void);
    friend char **tusb_get_active_str_desc(void);
    friend void tusb_clear_descriptor(void);
    friend void tud_mount_cb(void);
    friend void tud_umount_cb(void);
    friend void tud_suspend_cb(bool remote_wakeup_en);
    friend void tud_resume_cb(void);

protected:
    static uint8_t *descriptor_config;
    static uint8_t *descriptor_config_if;
    uint8_t _itf;
    static USBCallbacks* m_callbacks;

    xTaskHandle usbTaskHandle;

    static bool enableCDC;
    static bool enableMSC;
    static bool enableMIDI;
    static bool enableHID;
    static bool enableVENDOR;
    static bool enableDFU;

    static descriptor_strings_t strings;
    static uint8_t desc_configuration[1500];

    char langId[2];
    static uint16_t _VID;
    static uint16_t _PID;
    bool isEnabled = false;
    static uint8_t ifIdx;
    static int total;
    static uint8_t count;
    static uint16_t _revision;
    static uint16_t _bcdUSB;
};

#endif
#endif
