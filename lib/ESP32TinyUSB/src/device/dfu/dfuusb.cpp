#include "Arduino.h"
#include "soc/rtc_cntl_reg.h"

#include "dfuusb.h"
#if CFG_TUD_DFU_RUNTIME

DFUusb* _DFU = NULL;

DFUusb::DFUusb()
{
    enableDFU = true;
    _DFU = this;
}

bool DFUusb::begin(char* str)
{
    // Interface number, string index, attributes, detach timeout, transfer size
    uint8_t dfu[] = {TUD_DFU_RT_DESCRIPTOR(ifIdx++, 9, 0x0f, 1000, 1024)};
    memcpy(&desc_configuration[total], dfu, sizeof(dfu));
    total += sizeof(dfu);
    count++;
    if (!EspTinyUSB::begin(str, 9)) return false;
    return true;
}

/**
 * enter dfu bootload mode
 */
void tud_dfu_rt_reboot_to_dfu(void)
{
    _DFU->persistentReset(RESTART_BOOTLOADER_DFU);
}

#endif
