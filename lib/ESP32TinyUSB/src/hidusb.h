
#include "esptinyusb.h"

#pragma once
#if CFG_TUD_HID

class HIDCallbacks
{
public:
    virtual ~HIDCallbacks() { }
    virtual void onData(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) { }
};

class HIDusb : public EspTinyUSB
{
public:
    HIDusb(uint8_t reportid = 0);
    virtual bool begin(char* str = nullptr) = 0;
    void begin(uint8_t* desc_hid_report, size_t len1, uint8_t* hid, size_t len2);
    int available(void) { return -1; }
    int peek(void) { return -1; }
    int read(void) { return -1; }
    size_t read(uint8_t *buffer, size_t size) { return 0; }
    void flush(void) { return; }
    size_t write(uint8_t);
    size_t write(const uint8_t *buffer, size_t size);
    size_t write(char);
    size_t write(const char *buffer, size_t size);
    void setBaseEP(uint8_t);

    void setCallbacks(HIDCallbacks* cb);

    uint8_t _EPNUM_HID;
    uint8_t report_id;
    static uint8_t hid_report_desc[500];
    static size_t hid_report_desc_len;
    HIDCallbacks* m_callbacks;
};

#endif
