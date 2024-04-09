#include "sdusb.h"

#ifdef CFG_TUD_MSC

class SDCallbacks : public MSCCallbacks {
    SDCard2USB* m_parent;
public:
    SDCallbacks(SDCard2USB* ram) { m_parent = ram; }
    ~SDCallbacks() { }
    void onInquiry(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) 
    {
        if (m_parent->m_private)
        {
            m_parent->m_private->onInquiry(lun, vendor_id, product_id, product_rev);
        } else {
            const char vid[] = "ESP32-S2";
            const char pid[] = "SD card";
            const char rev[] = "1.0";

            memcpy(vendor_id  , vid, strlen(vid));
            memcpy(product_id , pid, strlen(pid));
            memcpy(product_rev, rev, strlen(rev));
            log_v("default onInquiry");
        }
    }
    bool onReady(uint8_t lun) {
        if (m_parent->m_private)
        {
            log_v("custom RAM disk onready");
            return m_parent->m_private->onReady(lun);
        } else {
            log_v("RAM disk always ready");
            return m_parent->sdcardReady; // RAM disk is always ready
        }
    }
    void onCapacity(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
    {
        (void) lun;
        *block_count = m_parent->block_count;
        *block_size = m_parent->block_size;
        log_v("ram disk block count: %d, block size: %d", *block_count, *block_size);
    }
    bool onStop(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
    {
        (void) lun;
        (void) power_condition;

        if ( load_eject )
        {
            if (start)
            {
                // load disk storage
                log_v("default start/stop load");
            }else
            {
                // unload disk storage
                log_v("default start/stop unload");
            }
        }

        return true;
    }
    int32_t onRead(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
    {
        log_v("default onread");
        (void) lun;
        SD.readRAW((uint8_t*)buffer, lba);

        return bufsize;
    }
    int32_t onWrite(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
    {
        log_v("default onwrite");
        (void) lun;
        SD.writeRAW((uint8_t*)buffer, lba);

        return bufsize;
    }
};

SDCard2USB::SDCard2USB( )
{
    MSCusb::setCallbacks(new SDCallbacks(this));
}

bool SDCard2USB::begin(char* str)
{
    assert(block_count);
    assert(block_size);

    return MSCusb::begin(str);
}

bool SDCard2USB::initSD(uint8_t ssPin, SPIClass &spi, uint32_t frequency, const char * mountpoint, uint8_t max_files)
{
    if(!SD.begin(ssPin, spi, frequency, mountpoint, max_files)){
        Serial.println("Card Mount Failed");
        return false;
    }

    return true;
}

bool SDCard2USB::initSD(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
{

    static SPIClass* spi = NULL;
    spi = new SPIClass(FSPI);
    spi->begin(sck, miso, mosi, ss);
    if(!SD.begin(ss, *spi, 40000000)){
        Serial.println("Card Mount Failed");
        return false;
    }
    
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return false;
    }

    block_count = SD.cardSize() / block_size;
    sdcardReady = true;
    return true;
}

void SDCard2USB::setCapacity(uint32_t count, uint32_t size)
{
    block_count = count;
    block_size = size;
}

void SDCard2USB::setCallbacks(MSCCallbacks* cb)
{
    m_private = cb;
}

void SDCard2USB::ready(bool ready)
{

}

bool SDCard2USB::isReady()
{
    return sdcardReady;
}

#endif
