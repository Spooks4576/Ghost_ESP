#pragma once
#include "esptinyusb.h"
#if CFG_TUD_CDC

#include "class/cdc/cdc.h"

class CDCCallbacks : public USBCallbacks {
public:
    virtual ~CDCCallbacks() { }
    virtual bool onConnect(bool dtr, bool rts) { return true; }
    virtual void onData() { }
    virtual void onCodingChange(cdc_line_coding_t const* p_line_coding) { }
    virtual void onWantedChar(char c) { }
};

class CDCusb : public EspTinyUSB
{
public:
    CDCusb(uint8_t itf = 0);
    bool begin(char* str = nullptr);
    int available(void);
    int peek(void);
    int read(void);
    size_t read(uint8_t *buffer, size_t size);
    void flush(void);
    size_t write(uint8_t);
    size_t write(const uint8_t *buffer, size_t size);
    void setBaseEP(uint8_t);
    uint32_t getBitrate();
    uint8_t getParity();
    uint8_t getDataBits();
    uint8_t getStopBits();
    void setWantedChar(char c);

    void setCallbacks(CDCCallbacks*);
    CDCCallbacks* m_callbacks = nullptr;
    operator bool() const;


    uint8_t _EPNUM_CDC;

    friend void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding);

protected:
    cdc_line_coding_t coding;

};

#endif
