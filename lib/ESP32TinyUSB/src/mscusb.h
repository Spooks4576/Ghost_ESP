
#include "esptinyusb.h"

#pragma once
#if CFG_TUD_MSC

class MSCCallbacks {
public:
    ~MSCCallbacks();
    virtual void onInquiry(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) = 0;
    virtual bool onReady(uint8_t lun) = 0;
    virtual void onCapacity(uint8_t lun, uint32_t* block_count, uint16_t* block_size) = 0;
    virtual bool onStop(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) = 0; // { return false; }
    virtual int32_t onRead(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) = 0; // { return 0; }
    virtual int32_t onWrite(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) = 0 ; //{ return 0; }
};

class MSCusb : public EspTinyUSB
{
public:
    MSCusb();
    // virtual ~MSCusb() = 0;
    bool begin(char* str = nullptr);

    int available(void) { return -1; }
    int peek(void) { return -1; }
    int read(void) { return -1; }
    size_t read(uint8_t *buffer, size_t size) { return 0; }
    void flush(void) { return; }
    size_t write(uint8_t) { return 0; }
    size_t write(const uint8_t *buffer, size_t size) { return 0; }
    void setBaseEP(uint8_t);
    void setCallbacks(MSCCallbacks*);
    uint8_t _EPNUM_MSC;

    friend TU_ATTR_WEAK bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject);
    friend TU_ATTR_WEAK int32_t tud_msc_read10_cb (uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);
    friend TU_ATTR_WEAK int32_t tud_msc_write10_cb (uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize);
    friend TU_ATTR_WEAK void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]);
    friend TU_ATTR_WEAK void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size);
    friend TU_ATTR_WEAK int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize);
    friend TU_ATTR_WEAK bool tud_msc_test_unit_ready_cb(uint8_t lun);

protected:
    MSCCallbacks* m_callbacks;
    uint8_t m_lun;
};

#endif
