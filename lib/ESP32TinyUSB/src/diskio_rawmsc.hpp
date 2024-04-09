#pragma once

#include <string.h>
#include "diskio_impl.h"
#include "ffconf.h"
#include "ff.h"
#include "esp_log.h"
#include "diskio_rawflash.h"
#include "esp_compiler.h"

esp_err_t ff_msc_register_raw_partition(BYTE pdrv, USBmscDevice* part_handle);
BYTE ff_msc_get_pdrv_raw(const void* part_handle);
esp_err_t vfs_fat_rawmsc_mount(const char* base_path,
    const esp_vfs_fat_mount_config_t* mount_config, uint8_t lun);

