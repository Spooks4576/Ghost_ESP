#include "hidnswusb.h"
#include "byteswap.h"
#define EPNUM_HID   0x03
#if CFG_TUD_HID

HIDNSWUSB::HIDNSWUSB(uint8_t id)
{
  report_id = 0;
  enableHID = true;
  _EPNUM_HID = EPNUM_HID;
}

bool HIDNSWUSB::begin(char* str)
{
    _VID = 0x20d6;
    _PID = 0xa713;

    strings.manufacturer = "Nintendo";
    strings.product = "Nintendo Switch Pro Controller";
    strings.serial = "4d1e55b2-f16f-11cf-88cb-001111000030";


    uint8_t const desc_hid_report[] = {NSW_HID_REPORT_DESCRIPTOR};
    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    uint8_t hid[] = {TUD_HID_DESCRIPTOR(ifIdx++, 6, HID_ITF_PROTOCOL_NONE, 94, (uint8_t)(_EPNUM_HID | 0x80), CFG_TUD_HID_BUFSIZE, 10)};
    memcpy(&desc_configuration[total], hid, sizeof(hid));
    total += sizeof(hid);
    count++;

    memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, 94);
    EspTinyUSB::hid_report_desc_len += TUD_HID_DESC_LEN;
    log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

    Initialized = true;
    return EspTinyUSB::begin(str, 6);
}

void HIDNSWUSB::sendReport()
{
    if(tud_hid_ready()){
        int ret = write((uint8_t*)&report, sizeof(hid_nsw_report_t));
        if(-1 == ret) log_e("error: %i", ret);
    }
}

void HIDNSWUSB::SetInputState(SwitchButton state)
{
    if (state == NONE) {
        memset(&report, 0, sizeof(hid_nsw_report_t)); // Reset the entire structure
        report.leftStickX = 0x80; // Neutral X for left stick
        report.leftStickY = 0x80; // Neutral Y for left stick
        report.rightStickX = 0x80; // Neutral X for right stick
        report.rightStickY = 0x80; // Neutral Y for right stick
        report.dpad = 0x0F; // Neutral for D-pad
        return;
    }

    switch (state) {
        case BUTTON_Y:
            report.buttonCombos |= 0x01;
            break;
        case BUTTON_B:
            report.buttonCombos |= 0x02;
            break;
        case BUTTON_A:
            report.buttonCombos |= 0x04;
            break;
        case BUTTON_X:
            report.buttonCombos |= 0x08;
            break;
        case BUTTON_MINUS:
            report.additionalButtons |= 0x01;
            break;
        case BUTTON_PLUS:
            report.additionalButtons |= 0x02;
            break;
        case BUTTON_HOME:
            report.additionalButtons |= 0x10;
            break;
        case BUTTON_SHARE:
            report.additionalButtons |= 0x20;
            break;
        case BUTTON_LBUMPER:
            report.buttonCombos |= 0x10;
            break;
        case BUTTON_RBUMPER:
            report.buttonCombos |= 0x20;
            break;
        case BUTTON_LTRIGGER:
            report.buttonCombos |= 0x40;
            break;
        case BUTTON_RTRIGGER:
            report.buttonCombos |= 0x80;
            break;
        case BUTTON_LTHUMBSTICK:
            report.additionalButtons |= 0x04;
            break;
        case BUTTON_RTHUMBSTICK:
            report.additionalButtons |= 0x08;
            break;
        case DPAD_UP:
            report.dpad = 0x00;
            break;
        case DPAD_RIGHT:
            report.dpad = 0x02;
            break;
        case DPAD_DOWN:
            report.dpad = 0x04;
            break;
        case DPAD_LEFT:
            report.dpad = 0x06;
            break;
        default:
            break;
    }
}

#endif
