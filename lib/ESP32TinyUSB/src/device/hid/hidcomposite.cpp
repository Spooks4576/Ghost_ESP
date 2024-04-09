#include "hidcomposite.h"
#define EPNUM_HID 0x03
#if CFG_TUD_HID

HIDcomposite::HIDcomposite(uint8_t id)
{
    report_mouse = id;
    report_keyboard = id + 1;
    enableHID = true;
    _EPNUM_HID = EPNUM_HID;
}

bool HIDcomposite::begin(char *str)
{
    uint8_t const desc_hid_report[] = {
        TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(report_mouse)),
        TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID((uint8_t)(report_keyboard)))    
    };
    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    uint8_t hid[] = {TUD_HID_DESCRIPTOR(ifIdx++, 6, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), (uint8_t)(_EPNUM_HID | 0x80), CFG_TUD_HID_BUFSIZE, 1)};
    memcpy(&desc_configuration[total], hid, sizeof(hid));
    total += sizeof(hid);
    count++;

    memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, sizeof(desc_hid_report));
    EspTinyUSB::hid_report_desc_len += TUD_HID_DESC_LEN;
    log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

    return EspTinyUSB::begin(str, 6);
}

/*------------- MOUSE ----------------*/
void HIDcomposite::buttons(uint8_t bt)
{
    button = bt;
    if (tud_hid_ready())
    {
        // uint8_t report_id, uint8_t buttons, int8_t x, int8_t y, int8_t vertical, int8_t horizontal
        tud_hid_mouse_report(report_mouse, button, 0, 0, 0, 0);
    }
}

void HIDcomposite::move(int8_t x, int8_t y)
{
    if (tud_hid_ready())
    {
        // uint8_t report_id, uint8_t buttons, int8_t x, int8_t y, int8_t vertical, int8_t horizontal
        tud_hid_mouse_report(report_mouse, button, x, y, 0, 0);
    }
}

void HIDcomposite::wheel(int8_t vertical, int8_t horizontal)
{
    if (tud_hid_ready())
    {
        // uint8_t report_id, uint8_t buttons, int8_t x, int8_t y, int8_t vertical, int8_t horizontal
        tud_hid_mouse_report(report_mouse, button, 0, 0, vertical, horizontal);
    }
}

void HIDcomposite::pressLeft()
{
    buttons(LEFT_BTN);
    delay(10);
    buttons(0);
}

void HIDcomposite::pressMiddle()
{
    buttons(MIDDLE_BTN);
    delay(10);
    buttons(0);
}

void HIDcomposite::pressRight()
{
    buttons(RIGHT_BTN);
    delay(10);
    buttons(0);
}

void HIDcomposite::doublePressLeft()
{
    pressLeft();
    delay(80);
    pressLeft();
}

void HIDcomposite::backwardBtn()
{
    buttons(BACK_BTN);
    delay(10);
    buttons(0);
}

void HIDcomposite::forwardBtn()
{
    buttons(FORWARD_BTN);
    delay(10);
    buttons(0);
}

void HIDcomposite::scrollUp(uint8_t val)
{
    wheel(val, 0);
}

void HIDcomposite::scrollDown(uint8_t val)
{
    wheel(-1 * val, 0);
}
/*------------- MOUSE ----------------*/

/*------------- Keyboard -------------*/
bool HIDcomposite::sendKey(uint8_t _keycode, uint8_t modifier)
{
  if (tud_hid_ready())
  {
    if(sendPress(_keycode, modifier)) {
      delay(2);
      return sendRelease();
    }
  }
  return false;
}

bool HIDcomposite::sendChar(uint8_t _keycode)
{
  return sendKey(keymap[_keycode].usage, keymap[_keycode].modifier);
}

bool HIDcomposite::sendPress(uint8_t _keycode, uint8_t modifier)
{
  uint8_t keycode[6] = {0};
  keycode[0] = _keycode;

  return tud_hid_keyboard_report(report_keyboard, modifier, keycode);
}

bool HIDcomposite::sendRelease()
{
  // send empty key report if previously has key pressed
  return tud_hid_keyboard_report(report_keyboard, 0, NULL);
}

bool HIDcomposite::sendString(const char* _text)
{
  size_t len = strlen(_text);
  uint8_t keycode;
  for(size_t i = 0; i < len; i++) {
    keycode = (uint8_t) _text[i];
    if(!sendKey(keymap[keycode].usage, keymap[keycode].modifier)) return false;
    delay(2);
  }

  return true;
}

bool HIDcomposite::sendString(String text)
{
  return sendString(text.c_str());
}
/*------------- Keyboard -------------*/

#endif
