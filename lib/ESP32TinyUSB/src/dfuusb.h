
#include "esptinyusb.h"

#pragma once

#if CFG_TUD_DFU_RUNTIME

class DFUusb : public EspTinyUSB
{
public:
    DFUusb();
    bool begin(char* str = nullptr);
    int available(void) { return -1; }
    int peek(void) { return -1; }
    int read(void) { return -1; }
    size_t read(uint8_t *buffer, size_t size) { return 0; }
    void flush(void) { return; }
    size_t write(uint8_t) { return 0; }
    size_t write(const uint8_t *buffer, size_t size) { return 0; }
    void setBaseEP(uint8_t) { return; }
};

#endif
