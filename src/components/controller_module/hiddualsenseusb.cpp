#include "hiddualsenseusb.h"
#include "byteswap.h"
#define EPNUM_HID   0x03
#if CFG_TUD_HID

HIDDualSenseUSB::HIDDualSenseUSB(uint8_t id)
{
  report_id = 1;
  enableHID = true;
  _EPNUM_HID = EPNUM_HID;
}

bool HIDDualSenseUSB::begin(char* str)
{
    _VID = 0x054C;
    _PID = 0x0CE6;
    strings.manufacturer = "Sony Interactive Entertainment";
    strings.product = "DualSense Wireless Controller";
    strings.serial = "4d1e55b2-f16f-11cf-88cb-001111000030";

    uint8_t desc_hid_report[PS5_REPORT_DESCRIPTOR_SIZE];

    for(size_t i = 0; i < PS5_REPORT_DESCRIPTOR_SIZE; ++i) {
        desc_hid_report[i] = pgm_read_byte_near(ps5reportdescriptor + i);
    }
    
    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    uint8_t hid[] = {TUD_HID_DESCRIPTOR(ifIdx++, 6, HID_ITF_PROTOCOL_NONE, PS5_REPORT_DESCRIPTOR_SIZE, (uint8_t)(_EPNUM_HID | 0x80), CFG_TUD_HID_BUFSIZE, 10)};
    memcpy(&desc_configuration[total], hid, sizeof(hid));
    total += sizeof(hid);
    count++;

    memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, PS5_REPORT_DESCRIPTOR_SIZE);
    EspTinyUSB::hid_report_desc_len += TUD_HID_DESC_LEN;
    log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

    Initialized = true;
    return EspTinyUSB::begin(str, 6);
}

void HIDDualSenseUSB::sendReport()
{
    if(tud_hid_ready()){
        int ret = write((uint8_t*)&report, sizeof(ps5_hid_report));
        if(-1 == ret) log_e("error: %i", ret);
    }
}

void HIDDualSenseUSB::SetInputState(PS5Button state) {
    if (state == PS5_NONE) {
        memset(&report, 0, sizeof(ps5_hid_report)); // Reset the entire structure
        report.axes0 = 128; // Left stick X axis neutral position
        report.axes1 = 128; // Left stick Y axis neutral position
        report.axes2 = 128; // Right stick X axis neutral position
        report.axes3 = 128; // Right stick Y axis neutral position
        report.buttons0 = (report.buttons0 & 0xF0) | 0x08;
        return;
    }

    switch (state) {
        case PS5_BUTTON_TRIANGLE:
            report.buttons0 |= (1 << 7); // Triangle button is bit 7 of buttons0
            break;
        case PS5_BUTTON_CIRCLE:
            report.buttons0 |= (1 << 6); // Circle button is bit 6 of buttons0
            break;
        case PS5_BUTTON_CROSS:
            report.buttons0 |= (1 << 5); // Cross button is bit 5 of buttons0
            break;
        case PS5_BUTTON_SQUARE:
            report.buttons0 |= (1 << 4); // Square button is bit 4 of buttons0
            break;
        case PS5_BUTTON_SHARE: // Share maps to Create
            report.buttons1 |= (1 << 4); // Create (Share) button is bit 4 of buttons1
            break;
        case PS5_BUTTON_OPTIONS:
            report.buttons1 |= (1 << 5); // Options button is bit 5 of buttons1
            break;
        case PS5_BUTTON_PS:
            report.buttons2 |= (1 << 0); // PS button is bit 0 of buttons2
            break;
        case PS5_BUTTON_TOUCHPAD:
            report.buttons2 |= (1 << 1); // Touchpad click is bit 1 of buttons2
            break;
        case PS5_BUTTON_L1:
            report.buttons1 |= (1 << 0); // L1 button is bit 0 of buttons1
            break;
        case PS5_BUTTON_R1:
            report.buttons1 |= (1 << 1); // R1 button is bit 1 of buttons1
            break;
        case PS5_BUTTON_L2:
            report.buttons1 |= (1 << 2); // L2 button is bit 2 of buttons1
            report.axes4 = 0xFF; // Fully pressed L2 analog value
            break;
        case PS5_BUTTON_R2:
            report.buttons1 |= (1 << 3); // R2 button is bit 3 of buttons1
            report.axes5 = 0xFF; // Fully pressed R2 analog value
            break;
        case PS5_BUTTON_L3:
            report.buttons1 |= (1 << 6); // L3 button is bit 6 of buttons1
            break;
        case PS5_BUTTON_R3:
            report.buttons1 |= (1 << 7); // R3 button is bit 7 of buttons1
            break;
        case PS5_DPAD_UP:
        report.buttons0 = (report.buttons0 & 0xF0) | 0x0; // Up
        break;
        case PS5_DPAD_DOWN:
            report.buttons0 = (report.buttons0 & 0xF0) | 0x4; // Down
            break;
        case PS5_DPAD_LEFT:
            report.buttons0 = (report.buttons0 & 0xF0) | 0x6; // Left
            break;
        case PS5_DPAD_RIGHT:
            report.buttons0 = (report.buttons0 & 0xF0) | 0x2; // Right
            break;

        // Left Stick Movements
        case PS5_LSTICK_UP:
            report.axes0 = 128; // X remains neutral
            report.axes1 = 0;   // Y at minimum for up
            break;
        case PS5_LSTICK_DOWN:
            report.axes0 = 128; // X remains neutral
            report.axes1 = 255; // Y at maximum for down
            break;
        case PS5_LSTICK_LEFT:
            report.axes0 = 0;   // X at minimum for left
            report.axes1 = 128; // Y remains neutral
            break;
        case PS5_LSTICK_RIGHT:
            report.axes0 = 255; // X at maximum for right
            report.axes1 = 128; // Y remains neutral
            break;

        // Right Stick Movements
        case PS5_RSTICK_UP:
            report.axes2 = 128; // X remains neutral
            report.axes3 = 0;   // Y at minimum for up
            break;
        case PS5_RSTICK_DOWN:
            report.axes2 = 128; // X remains neutral
            report.axes3 = 255; // Y at maximum for down
            break;
        case PS5_RSTICK_LEFT:
            report.axes2 = 0;   // X at minimum for left
            report.axes3 = 128; // Y remains neutral
            break;
        case PS5_RSTICK_RIGHT:
            report.axes2 = 255; // X at maximum for right
            report.axes3 = 128; // Y remains neutral
            break;
            default:
            break;
        }
}

#endif