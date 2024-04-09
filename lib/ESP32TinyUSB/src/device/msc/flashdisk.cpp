#include "flashdisk.h"
#include "ffconf.h"
#include "ff.h"
#include "diskio.h"
#ifdef CFG_TUD_MSC
static uint8_t _buf[4 * 1024] = {};

class FlashCallbacks : public MSCCallbacks {
    FlashUSB* m_parent;
    wl_handle_t wl_handle_thandle;
    int s_disk_block_size = 0;
    int s_pdrv = 0;
    bool eject = true;

public:
    FlashCallbacks(FlashUSB* ram, wl_handle_t handle, int pdrv) { m_parent = ram; wl_handle_thandle = handle; s_pdrv = pdrv; }
    ~FlashCallbacks() { }
    void onInquiry(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) 
    {
        if (m_parent->m_private)
        {
            m_parent->m_private->onInquiry(lun, vendor_id, product_id, product_rev);
        } else {
            const char vid[] = "ESP32-S2";
            const char pid[] = "FLASH";
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
            return m_parent->m_private->onReady(lun);
        } else {
            return m_parent->sdcardReady;
        }
    }
    void onCapacity(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
    {
        if (m_parent->m_private)
        {
            return m_parent->m_private->onCapacity(lun, block_count, block_size);
        } else {
            (void) lun;
            disk_ioctl(s_pdrv, GET_SECTOR_COUNT, block_count);
            disk_ioctl(s_pdrv, GET_SECTOR_SIZE, block_size);
            s_disk_block_size = *block_size;
            m_parent->block_count = *block_count;
            m_parent->block_size = *block_size;
            log_d("disk block count: %d, block size: %d", *block_count, *block_size);
        }
    }
    bool onStop(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
    {
        if (m_parent->m_private)
        {
            return m_parent->m_private->onStop(lun, power_condition, start, load_eject);
        } else {
            (void) lun;
            (void) power_condition;

            if ( load_eject )
            {
                if (!start) {
                    // Eject but first flush.
                    if (disk_ioctl(s_pdrv, CTRL_SYNC, NULL) != RES_OK) {
                        eject = false;
                        return false;
                    } else {
                        eject = true;
                    }
                } else {
                    // We can only load if it hasn't been ejected.
                    return !eject;
                }
            } else {
                if (!start) {
                    // Stop the unit but don't eject.
                    if (disk_ioctl(s_pdrv, CTRL_SYNC, NULL) != RES_OK) {
                        return false;
                    }
                }
            }

            return true;
        }
    }
    int32_t onRead(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
    {
        if (m_parent->m_private)
        {
            return m_parent->m_private->onRead(lun, lba, offset, buffer, bufsize);
        } else {
            log_v("default onread");
            (void) lun;
            if(CONFIG_WL_SECTOR_SIZE > CONFIG_TINYUSB_MSC_BUFSIZE){
                disk_read(s_pdrv, _buf, lba, 1);
                memcpy(buffer, &_buf[offset], bufsize);
            } else {
                disk_read(s_pdrv, (BYTE*)buffer, lba, 1);
            }

            return bufsize;
        }
    }

    int32_t onWrite(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
    {
        if (m_parent->m_private)
        {
            return m_parent->m_private->onWrite(lun, lba, offset, buffer, bufsize);
        } else {
            log_v("default onwrite; lba: %d, off: %d, size: %d", lba, offset, bufsize);
            (void) lun;
            if(CONFIG_WL_SECTOR_SIZE > CONFIG_TINYUSB_MSC_BUFSIZE){
                disk_read(s_pdrv, _buf, lba, 1);
                memcpy(&_buf[offset], buffer, bufsize);
                disk_write(s_pdrv, _buf, lba, 1);
            } else {
                disk_write(s_pdrv, (BYTE*)buffer, lba, 1);
            }
            log_v("lba: %d, offset: %d, buffsize: %d, s_disk: %d, count: %d ", lba, offset, bufsize, s_disk_block_size, 1);

            return bufsize;
        }
    }
};

FlashUSB::FlashUSB(bool _aes)
{
    static int pdrv = 0;

    MSCusb::setCallbacks(new FlashCallbacks(this, wl_handle, pdrv));
    pdrv++;
    encrypted = _aes;
}

bool FlashUSB::begin(char* str)
{
    assert(block_count);
    assert(block_size);

    return MSCusb::begin(str);
}

bool FlashUSB::init(char* path, char* label)
{
    esp_vfs_fat_mount_config_t mount_config = 
    {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };

    esp_err_t err = esp_vfs_fat_spiflash_mount(path, label, &mount_config, &wl_handle);
    if(!err){
        setCapacity(wl_size(wl_handle)/wl_sector_size(wl_handle), wl_sector_size(wl_handle));
        sdcardReady = true;
    }

    return err == ESP_OK;
}

void FlashUSB::setCapacity(uint32_t count, uint32_t size)
{
    block_count = count;
    block_size = size;
}

void FlashUSB::setCallbacks(MSCCallbacks* cb)
{
    m_private = cb;
}

bool FlashUSB::isReady()
{
    return sdcardReady;
}

#endif
