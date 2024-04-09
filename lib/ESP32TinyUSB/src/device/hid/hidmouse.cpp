#include "hidmouse.h"
#define EPNUM_HID 0x03
#if CFG_TUD_HID

HIDmouse::HIDmouse(uint8_t id)
{
    report_id = id;
    enableHID = true;
    _EPNUM_HID = EPNUM_HID;
}

bool HIDmouse::begin(char *str)
{
    uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(report_id))};
    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    uint8_t hid[] = {TUD_HID_DESCRIPTOR(ifIdx++, 6, HID_ITF_PROTOCOL_MOUSE, sizeof(desc_hid_report), (uint8_t)(_EPNUM_HID | 0x80), CFG_TUD_HID_BUFSIZE, 10)};
    memcpy(&desc_configuration[total], hid, sizeof(hid));
    total += sizeof(hid);
    count++;

    memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, sizeof(desc_hid_report));
    EspTinyUSB::hid_report_desc_len += TUD_HID_DESC_LEN;
    log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

    return EspTinyUSB::begin(str, 6);
}

void HIDmouse::buttons(uint8_t bt)
{
    button = bt;
    if (tud_hid_ready())
    {
        // uint8_t report_id, uint8_t buttons, int8_t x, int8_t y, int8_t vertical, int8_t horizontal
        tud_hid_mouse_report(report_id, button, 0, 0, 0, 0);
    }
}

void HIDmouse::move(int8_t x, int8_t y)
{
    if (tud_hid_ready())
    {
        // uint8_t report_id, uint8_t buttons, int8_t x, int8_t y, int8_t vertical, int8_t horizontal
        tud_hid_mouse_report(report_id, button, x, y, 0, 0);
    }
}

void HIDmouse::wheel(int8_t vertical, int8_t horizontal)
{
    if (tud_hid_ready())
    {
        // uint8_t report_id, uint8_t buttons, int8_t x, int8_t y, int8_t vertical, int8_t horizontal
        tud_hid_mouse_report(report_id, button, 0, 0, vertical, horizontal);
    }
}

void HIDmouse::pressLeft()
{
    buttons(LEFT_BTN);
    delay(10);
    buttons(0);
}

void HIDmouse::pressMiddle()
{
    buttons(MIDDLE_BTN);
    delay(10);
    buttons(0);
}

void HIDmouse::pressRight()
{
    buttons(RIGHT_BTN);
    delay(10);
    buttons(0);
}

void HIDmouse::doublePressLeft()
{
    pressLeft();
    delay(80);
    pressLeft();
}

void HIDmouse::backwardBtn()
{
    buttons(BACK_BTN);
    delay(10);
    buttons(0);
}

void HIDmouse::forwardBtn()
{
    buttons(FORWARD_BTN);
    delay(10);
    buttons(0);
}

void HIDmouse::scrollUp(uint8_t val)
{
    wheel(val, 0);
}

void HIDmouse::scrollDown(uint8_t val)
{
    wheel(-1 * val, 0);
}

#endif
