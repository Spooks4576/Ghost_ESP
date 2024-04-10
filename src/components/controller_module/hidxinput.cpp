#include "hidxinput.h"
#include "byteswap.h"
#define EPNUM_HID   0x03
#if CFG_TUD_HID

HIDXInputUSB::HIDXInputUSB(uint8_t id)
{
  report_id = XBOX_INPUT_REPORT_ID;
  enableHID = true;
  _EPNUM_HID = EPNUM_HID;
}

bool HIDXInputUSB::begin(char* str)
{
    _VID = 0x045E; 
    _PID = 0x02D1;
    uint8_t desc_hid_report[283];

    for(size_t i = 0; i < 283; ++i) {
        desc_hid_report[i] = pgm_read_byte_near(XboxOneS_1914_HIDDescriptor + i);
    }
    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    uint8_t hid[] = {TUD_HID_DESCRIPTOR(ifIdx++, 6, HID_ITF_PROTOCOL_NONE, 283, (uint8_t)(_EPNUM_HID | 0x80), CFG_TUD_HID_BUFSIZE, 10)};
    memcpy(&desc_configuration[total], hid, sizeof(hid));
    total += sizeof(hid);
    count++;

    memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, 283);
    EspTinyUSB::hid_report_desc_len += TUD_HID_DESC_LEN;
    log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

    Initialized = true;
    return EspTinyUSB::begin(str, 6);
}

void HIDXInputUSB::sendReport()
{
    if(tud_hid_ready()){
        int ret = write((uint8_t*)&report, sizeof(XboxGamepadInputReportData));
        if(-1 == ret) log_e("error: %i", ret);
    }
}

void HIDXInputUSB::SetInputState(XinputButton state)
{
    switch (state) {
        case XboxButton_Y:
            report.buttons |= XBOX_BUTTON_Y;
            break;
        case XboxButton_B:
            report.buttons |= XBOX_BUTTON_B;
            break;
        case XboxButton_A:
            report.buttons |= XBOX_BUTTON_A;
            break;
        case XboxButton_X:
            report.buttons |= XBOX_BUTTON_X;
            break;
        case XboxButton_Start:
            report.buttons |= XBOX_BUTTON_START;
            break;
        case XboxButton_Back:
            report.buttons |= XBOX_BUTTON_SELECT;
            break;
        case XboxButton_Home:
            report.buttons |= XBOX_BUTTON_HOME;
            break;
        case XboxButton_LB:
            report.buttons |= XBOX_BUTTON_LB;
            break;
        case XboxButton_RB:
            report.buttons |= XBOX_BUTTON_RB;
            break;
        case XboxButton_LT:
            report.brake = 1023; // Maximum value for left trigger
            break;
        case XboxButton_RT:
            report.accelerator = 1023; // Maximum value for right trigger
            break;
        case XboxButton_DPad_Up:
            report.hat = XBOX_BUTTON_DPAD_NORTH; // D-pad up
            break;
        case XboxButton_DPad_Right:
            report.hat = XBOX_BUTTON_DPAD_EAST; // D-pad right
            break;
        case XboxButton_DPad_Down:
            report.hat = XBOX_BUTTON_DPAD_SOUTH; // D-pad down
            break;
        case XboxButton_DPad_Left:
            report.hat = XBOX_BUTTON_DPAD_WEST; // D-pad left
            break;
        case Xbox_NONE:
            memset(&report, 0, sizeof(XboxGamepadInputReportData));
        default:
            break;
    }
}

#endif