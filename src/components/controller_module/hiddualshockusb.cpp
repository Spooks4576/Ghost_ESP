#include "hiddualshockusb.h"
#include "byteswap.h"
#define EPNUM_HID   0x03
#if CFG_TUD_HID

HIDDualShockUSB::HIDDualShockUSB(uint8_t id)
{
  report_id = 1;
  enableHID = true;
  _EPNUM_HID = EPNUM_HID;
}

bool HIDDualShockUSB::begin(char* str)
{
    _VID = 0x054C;
    _PID = 0x05C4;

    uint8_t desc_hid_report[PS4_REPORT_DESCRIPTOR_SIZE];

    for(size_t i = 0; i < PS4_REPORT_DESCRIPTOR_SIZE; ++i) {
        desc_hid_report[i] = pgm_read_byte_near(ps4_report_descriptor + i);
    }
    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    uint8_t hid[] = {TUD_HID_DESCRIPTOR(ifIdx++, 6, HID_ITF_PROTOCOL_NONE, PS4_REPORT_DESCRIPTOR_SIZE, (uint8_t)(_EPNUM_HID | 0x80), CFG_TUD_HID_BUFSIZE, 10)};
    memcpy(&desc_configuration[total], hid, sizeof(hid));
    total += sizeof(hid);
    count++;

    memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, PS4_REPORT_DESCRIPTOR_SIZE);
    EspTinyUSB::hid_report_desc_len += TUD_HID_DESC_LEN;
    log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

    Initialized = true;
    return EspTinyUSB::begin(str, 6);
}

void HIDDualShockUSB::sendReport()
{
    if(tud_hid_ready()){
        int ret = write((uint8_t*)&report, sizeof(ps4_hid_report));
        if(-1 == ret) log_e("error: %i", ret);
    }
}

void HIDDualShockUSB::SetInputState(PS4Button state) {
    if (state == PS4_NONE) {
        memset(&report, 0, sizeof(ps4_hid_report)); // Reset the entire structure
        report.leftStickX = 128;
        report.leftStickY = 128;
        report.rightStickX = 128;
        report.rightStickY = 128; // Prevent Stick Drift
        report.buttons1 = (report.buttons1 & 0xF0) | 0x08;
        return;
    }

    switch (state) {
        case PS4_BUTTON_TRIANGLE:
            report.buttons1 |= (1 << 7); // Triangle button is bit 7 of buttons1
            break;
        case PS4_BUTTON_CIRCLE:
            report.buttons1 |= (1 << 6); // Circle button is bit 6 of buttons1
            break;
        case PS4_BUTTON_CROSS:
            report.buttons1 |= (1 << 5); // Cross button is bit 5 of buttons1
            break;
        case PS4_BUTTON_SQUARE:
            report.buttons1 |= (1 << 4); // Square button is bit 4 of buttons1
            break;
        case PS4_BUTTON_SHARE:
            report.buttons2 |= (1 << 4); // Share button is bit 4 of buttons2
            break;
        case PS4_BUTTON_OPTIONS:
            report.buttons2 |= (1 << 5); // Options button is bit 5 of buttons2
            break;
        case PS4_BUTTON_PS:
            report.buttons3 |= (1 << 0); // PS button is bit 0 of buttons3
            break;
        case PS4_BUTTON_TOUCHPAD:
            report.buttons3 |= (1 << 1); // Touchpad click is bit 1 of buttons3
            break;
        case PS4_BUTTON_L1:
            report.buttons2 |= (1 << 0); // L1 button is bit 0 of buttons2
            break;
        case PS4_BUTTON_R1:
            report.buttons2 |= (1 << 1); // R1 button is bit 1 of buttons2
            break;
        case PS4_BUTTON_L2:
            report.buttons2 |= (1 << 2); // L2 button is bit 2 of buttons2, also consider analog value
            report.l2Trigger = 0xFF; // Fully pressed
            break;
        case PS4_BUTTON_R2:
            report.buttons2 |= (1 << 3); // R2 button is bit 3 of buttons2, also consider analog value
            report.r2Trigger = 0xFF; // Fully pressed
            break;
        case PS4_BUTTON_L3:
            report.buttons2 |= (1 << 6); // L3 button is bit 6 of buttons2
            break;
        case PS4_BUTTON_R3:
            report.buttons2 |= (1 << 7); // R3 button is bit 7 of buttons2
            break;
        case PS4_DPAD_UP:
            report.buttons1 = (report.buttons1 & 0xF0) | 0x00; // North
            break;
        case PS4_DPAD_DOWN:
            report.buttons1 = (report.buttons1 & 0xF0) | 0x04; // South
            break;
        case PS4_DPAD_LEFT:
            report.buttons1 = (report.buttons1 & 0xF0) | 0x06; // West
            break;
        case PS4_DPAD_RIGHT:
            report.buttons1 = (report.buttons1 & 0xF0) | 0x02; // East
            break;
        case PS4_LSTICK_UP:
            report.leftStickY = 0; // Move left stick up
            break;
        case PS4_LSTICK_DOWN:
            report.leftStickY = 255; // Move left stick down
            break;
        case PS4_LSTICK_LEFT:
            report.leftStickX = 0; // Move left stick left
            break;
        case PS4_LSTICK_RIGHT:
            report.leftStickX = 255; // Move left stick right
            break;
        case PS4_RSTICK_UP:
            report.rightStickY = 0; // Move right stick up
            break;
        case PS4_RSTICK_DOWN:
            report.rightStickY = 255; // Move right stick down
            break;
        case PS4_RSTICK_LEFT:
            report.rightStickX = 0; // Move right stick left
            break;
        case PS4_RSTICK_RIGHT:
            report.rightStickX = 255; // Move right stick right
            break;
        default:
            break;
    }
}

#endif