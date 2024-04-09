// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include "diskio_impl.h"
#include "ff.h"
extern "C" {
    #include "ffconf.h"
}
#include "esp_log.h"
#include "diskio_rawflash.h"
#include "esp_compiler.h"
#include "esp_vfs_fat.h"

#include "usb_msc.hpp"

static const char* TAG = "msc_rawflash";

uint8_t ff_raw_handles[FF_VOLUMES]; // change to lun??


DSTATUS ff_raw_initialize (BYTE pdrv)
{
    ESP_LOGD(TAG, "ff_raw_initialize => %d", pdrv);
    return 0;
}

DSTATUS ff_raw_status (BYTE pdrv)
{
    return 0;
}

DRESULT ff_raw_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    ESP_LOGV(TAG, "ff_raw_read - pdrv=%i, sector=%i, count=%in", (unsigned int)pdrv, (unsigned int)sector, (unsigned int)count);
    uint8_t lun = ff_raw_handles[pdrv];
    USBmscDevice* part = USBmscDevice::getInstance();
    assert(part);
    esp_err_t err = part->_read10(lun, sector, count, buff);
    if (unlikely(err != ESP_OK)) {
        ESP_LOGE(TAG, "msc disk lun failed (0x%x)", err);
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT ff_raw_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    ESP_LOGV(TAG, "ff_raw_write - pdrv=%i, sector=%i, count=%in", (unsigned int)pdrv, (unsigned int)sector, (unsigned int)count);
    uint8_t lun = ff_raw_handles[pdrv];
    USBmscDevice* part = USBmscDevice::getInstance();
    assert(part);
    
    esp_err_t err = part->_write10(lun, sector, count, (uint8_t*)buff);
    if (unlikely(err != ESP_OK)) {
        ESP_LOGE(TAG, "msc disk lun failed (0x%x)", err);
        return RES_ERROR;
    }
    return RES_OK;
    return RES_ERROR;
}

DRESULT ff_raw_ioctl (BYTE pdrv, BYTE cmd, void *buff)
{
    uint8_t lun = ff_raw_handles[pdrv];
    USBmscDevice* part = USBmscDevice::getInstance();
    ESP_LOGV(TAG, "ff_raw_ioctl: cmd=%in", cmd);
    assert(part);
    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *((DWORD *) buff) = part->getBlockCount(lun);
            return RES_OK;
        case GET_SECTOR_SIZE:
            *((WORD *) buff) = part->getBlockSize(lun);
            return RES_OK;
        case GET_BLOCK_SIZE:
            return RES_ERROR;
    }
    return RES_ERROR;
}


esp_err_t ff_msc_register_raw_partition(BYTE pdrv, uint8_t lun)
{
    if (pdrv >= FF_VOLUMES) {
        return ESP_ERR_INVALID_ARG;
    }
    static const ff_diskio_impl_t raw_impl = {
        .init = &ff_raw_initialize,
        .status = &ff_raw_status,
        .read = &ff_raw_read,
        .write = &ff_raw_write,
        .ioctl = &ff_raw_ioctl
    };
    ff_diskio_register(pdrv, &raw_impl);
    ff_raw_handles[pdrv] = lun;
    return ESP_OK;
}


// BYTE ff_msc_get_pdrv_raw(const void* part_handle)
// {
//     for (int i = 0; i < FF_VOLUMES; i++) {
//         if (part_handle == ff_raw_handles[i]) {
//             return i;
//         }
//     }
//     return 0xff;
// }

esp_err_t vfs_fat_rawmsc_mount(const char* base_path,
    const esp_vfs_fat_mount_config_t* mount_config, uint8_t lun)
{
    esp_err_t result = ESP_OK;

    USBmscDevice* part = USBmscDevice::getInstance();
    assert(part);
    if(lun > part->getMaxLUN())
    {
        result = ESP_ERR_INVALID_STATE;
        return result;
    }

    BYTE pdrv = 0xFF;
    if (ff_diskio_get_drive(&pdrv) != ESP_OK) {
        ESP_LOGW(TAG, "the maximum count of volumes is already mounted");
        return ESP_ERR_NO_MEM;
    }
    char drv[3] = {(char)('0' + pdrv), ':', 0};
    ESP_LOGD(TAG, "using pdrv=%i, drv: %s", pdrv, drv);

    result = ff_msc_register_raw_partition(pdrv, lun);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "ff_msc_register_raw_partition failed pdrv=%i, error - 0x(%x)", pdrv, result);
        goto fail;
    }

    FRESULT fresult;
    FATFS *fs;
    result = esp_vfs_fat_register(base_path, drv, mount_config->max_files, &fs);
    if (result == ESP_ERR_INVALID_STATE) {
        // it's okay, already registered with VFS
    } else if (result != ESP_OK) {
        ESP_LOGD(TAG, "esp_vfs_fat_register failed 0x(%x)", result);
        goto fail;
    }

    // Try to mount partition
    fresult = f_mount(fs, drv, 0);
    if (fresult != FR_OK) {
        ESP_LOGW(TAG, "f_mount failed (%d)", fresult);
        result = ESP_FAIL;
        goto fail;
    }
    return ESP_OK;

fail:
    esp_vfs_fat_unregister_path(base_path);
    ff_diskio_unregister(pdrv);
    return result;
}
