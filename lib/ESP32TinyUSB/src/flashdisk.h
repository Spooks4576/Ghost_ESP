#pragma once
#include "mscusb.h"
#include "esp_vfs_fat.h"

#if CFG_TUD_MSC

class FlashUSB : public MSCusb {
    const char *base_path = "/fatfs";
    const char *partition_label = NULL;
    wl_handle_t wl_handle;

public:
    FlashUSB(bool aes = false);
    bool begin(char* str = nullptr);
    bool init(char* path = "/fatfs",char* label = NULL);
    void setCallbacks(MSCCallbacks*);
    void setCapacity(uint32_t count, uint32_t size);
    bool isReady();

    MSCCallbacks* m_private;
    uint32_t block_count = 0;
    uint32_t block_size = 0;
    bool sdcardReady;
    bool encrypted;
};

#endif
