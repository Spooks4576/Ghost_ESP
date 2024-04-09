#pragma once
#include "mscusb.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#if CFG_TUD_MSC

class SDCard2USB : public MSCusb {
    // fs::SDFS &_sd;

public:
    SDCard2USB();
    bool begin(char* str = nullptr);
    bool initSD(uint8_t ssPin=SS, SPIClass &spi=SPI, uint32_t frequency=4000000, const char * mountpoint="/sd", uint8_t max_files=5);
    bool initSD(int8_t sck, int8_t miso, int8_t mosi, int8_t ss);
    void setCallbacks(MSCCallbacks*);
    void setCapacity(uint32_t count, uint32_t size);
    void ready(bool ready);
    bool isReady();

    MSCCallbacks* m_private;
    uint32_t block_count = 0;
    uint32_t block_size = 512;
    bool sdcardReady;

};

#endif
