/**
 * Simple MSC device, use as ram disk
 * author: chegewara
 */
#include "ramdisk.h"
//#define AUTO_ALLOCATE_DISK 
#define BLOCK_COUNT 2 * 100
#define BLOCK_SIZE 512
#if CFG_TUD_MSC

USBramdisk dev;

void setup()
{
    Serial.begin(115200);

#ifndef AUTO_ALLOCATE_DISK
    uint8_t* disk = (uint8_t*)ps_calloc(BLOCK_COUNT, BLOCK_SIZE);
    dev.setDiskMemory(disk, true); // pass pointer to allocated ram disk and initialize it with demo FAT12 (true)
#endif
    dev.setCapacity(BLOCK_COUNT, BLOCK_SIZE); // if PSRAM is enableb, then ramdisk will be initialized in it

    if(dev.begin()) {
      Serial.println("MSC lun 1 begin");
    } else log_e("LUN 1 failed");

}

void loop()
{
  delay(1000);
}

#endif
