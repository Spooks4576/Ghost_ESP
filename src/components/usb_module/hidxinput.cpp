#include "hidxinput.h"
#include "byteswap.h"
#define EPNUM_HID   0x03
#if CFG_TUD_HID

HIDXInputUSB::HIDXInputUSB(uint8_t id)
{
  report_id = 0;
  enableHID = true;
  _EPNUM_HID = EPNUM_HID;
}

bool HIDXInputUSB::begin(char* str)
{
    _VID = 0x045E; 
    _PID = 0x02D1;
    uint8_t const desc_hid_report[] = {XINPUT_HID_REPORT_DESCRIPTOR};
    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    uint8_t hid[] = {TUD_HID_DESCRIPTOR(ifIdx++, 6, HID_ITF_PROTOCOL_NONE, 140, (uint8_t)(_EPNUM_HID | 0x80), CFG_TUD_HID_BUFSIZE, 10)};
    memcpy(&desc_configuration[total], hid, sizeof(hid));
    total += sizeof(hid);
    count++;

    memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, 140);
    EspTinyUSB::hid_report_desc_len += TUD_HID_DESC_LEN;
    log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

    Initialized = true;
    return EspTinyUSB::begin(str, 6);
}

void HIDXInputUSB::sendReport()
{
    if(tud_hid_ready()){
        int ret = write((uint8_t*)&report, sizeof(XinputReport_t));
        if(-1 == ret) log_e("error: %i", ret);
    }
}

void HIDXInputUSB::SetInputState(XinputButton state)
{
    switch (state) {
        case XboxButton_Y:
            report.BTN_GamePadButton1 = 1;
            break;
        case XboxButton_B:
            report.BTN_GamePadButton2 = 1;
            break;
        case XboxButton_A:
            report.BTN_GamePadButton3 = 1;
            break;
        case XboxButton_X:
            report.BTN_GamePadButton4 = 1;
            break;
        case XboxButton_Start:
            report.BTN_GamePadButton5 = 1;
            break;
        case XboxButton_Back:
            report.BTN_GamePadButton6 = 1;
            break;
        case XboxButton_Home:
            report.BTN_GamePadButton7 = 1;
            break;
        case XboxButton_LB:
            report.BTN_GamePadButton9 = 1;
            break;
        case XboxButton_RB:
            report.BTN_GamePadButton10 = 1;
            break;
        case XboxButton_LT:
            report.GD_GamePadZ = 1023; // Maximum value for left trigger
            break;
        case XboxButton_RT:
            report.GD_GamePadRz = 1023; // Maximum value for right trigger
            break;
        case XboxButton_LeftStick:
            report.BTN_GamePadButton1 = 1; // For left thumbstick button
            break;
        case XboxButton_RightStick:
            report.BTN_GamePadButton3 = 1; // For right thumbstick button
            break;
        case XboxButton_DPad_Up:
            report.GD_GamePadHatSwitch = 1; // D-pad up
            break;
        case XboxButton_DPad_Right:
            report.GD_GamePadHatSwitch = 2; // D-pad right
            break;
        case XboxButton_DPad_Down:
            report.GD_GamePadHatSwitch = 4; // D-pad down
            break;
        case XboxButton_DPad_Left:
            report.GD_GamePadHatSwitch = 8; // D-pad left
            break;
        case Xbox_NONE:
            memset(&report, 0, sizeof(XinputReport_t));
            report.GD_GamePadX = 0x8000; // Neutral X for left stick
            report.GD_GamePadY = 0x8000; // Neutral Y for left stick
            report.GD_GamePadRx = 0x8000; // Neutral X for right stick
            report.GD_GamePadRy = 0x8000; // Neutral Y for right stick
            report.GD_GamePadHatSwitch = 8; // Neutral for D-pad (center position)
        default:
            break;
    }
}

#endif