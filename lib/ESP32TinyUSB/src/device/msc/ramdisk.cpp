#include "ramdisk.h"

#ifdef CFG_TUD_MSC

class RAMCallbacks : public MSCCallbacks {
    USBramdisk* m_parent;
public:
    RAMCallbacks(USBramdisk* ram) { m_parent = ram; }
    ~RAMCallbacks() { }
    void onInquiry(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) 
    {
        if (m_parent->m_private)
        {
            m_parent->m_private->onInquiry(lun, vendor_id, product_id, product_rev);
        } else {
            const char vid[] = "ESP32-S2";
            const char pid[] = "RAM disk";
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
            return true; // RAM disk is always ready
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
                log_v("default load");
            }else
            {
                // unload disk storage
                log_v("default unload");
            }
        } else {
            if (start)
            {
                // load disk storage
                log_v("default start");
            }else
            {
                // unload disk storage
                log_v("default stop");
            }
        }

        return true;
    }
    int32_t onRead(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
    {
        log_v("default onread");
        (void) lun;
        uint8_t* addr = &m_parent->ram_disk[lba * 512] + offset;
        memcpy(buffer, addr, bufsize);

        return bufsize;
    }
    int32_t onWrite(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
    {
        log_v("default onwrite");
        (void) lun;
        uint8_t* addr = &m_parent->ram_disk[lba * 512] + offset;
        memcpy(addr, buffer, bufsize);

        return bufsize;
    }
};

USBramdisk::USBramdisk( )
{
    MSCusb::setCallbacks(new RAMCallbacks(this));
}

bool USBramdisk::begin(char* str)
{
    assert(block_count);
    assert(block_size);
    if(ram_disk == nullptr){
        log_e("NO disk");
        if (psramFound()){
            ram_disk = (uint8_t*)heap_caps_calloc(1, block_count * block_size, MALLOC_CAP_SPIRAM);
            log_e("init ram disk from SPIRAM: %d", sizeof(ram_disk));
        } else {
            ram_disk = (uint8_t*)heap_caps_calloc(1, block_count * block_size, MALLOC_CAP_INTERNAL);
            log_e("init ram disk from internal memory: %d", sizeof(ram_disk));
        }
        if(ram_disk == nullptr) return false;
    }
    if(set_demo_content){
        setContent(&ram_disk_demo[0][0], sizeof(ram_disk_demo));
        log_e("init ram disk size: %d", sizeof(ram_disk)/sizeof(*ram_disk));
        ram_disk[20] = (uint8_t)(block_count  >> 8);
        ram_disk[19] = (uint8_t)(block_count  & 0xff);        
    }
    return MSCusb::begin(str);
}

void USBramdisk::setCapacity(uint32_t count, uint32_t size)
{
    block_count = count;
    block_size = size;
}

void USBramdisk::setDiskMemory(uint8_t* memory, bool demo)
{
    ram_disk = memory;
    set_demo_content = demo;
}

void USBramdisk::setContent(uint8_t* content, size_t size)
{
    memcpy(ram_disk, content, size);
}

void USBramdisk::setCallbacks(MSCCallbacks* cb)
{
    m_private = cb;
}

#endif
