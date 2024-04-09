#include "hidkeyboard.h"
#define EPNUM_HID 0x02
#if CFG_TUD_HID

HIDkeyboard::HIDkeyboard(uint8_t reportid)
{
  report_id = reportid;
  enableHID = true;
  _EPNUM_HID = EPNUM_HID;
}

bool HIDkeyboard::begin(char *str)
{
  uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(report_id))};
  // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
  uint8_t hid[] = {TUD_HID_DESCRIPTOR(ifIdx++, 6, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), (uint8_t)(_EPNUM_HID | 0x80), CFG_TUD_HID_BUFSIZE, 1)};
  memcpy(&desc_configuration[total], hid, sizeof(hid));
  total += sizeof(hid);
  count++;

  memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, sizeof(desc_hid_report));
  EspTinyUSB::hid_report_desc_len += TUD_HID_DESC_LEN;
  log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

  return EspTinyUSB::begin(str, 6);
}

bool HIDkeyboard::sendKey(uint8_t _keycode, uint8_t modifier)
{
  /*------------- Keyboard -------------*/
  if (tud_hid_ready())
  {
    if(sendPress(_keycode, modifier)) {
      delay(2);
      return sendRelease();
    }
  }
  return false;
}

bool HIDkeyboard::sendChar(uint8_t _keycode)
{
  return sendKey(keymap[_keycode].usage, keymap[_keycode].modifier);
}

bool HIDkeyboard::sendPress(uint8_t _keycode, uint8_t modifier)
{
  uint8_t keycode[6] = {0};
  keycode[0] = _keycode;

  return tud_hid_keyboard_report(report_id, modifier, keycode);
}

bool HIDkeyboard::sendRelease()
{
  // send empty key report if previously has key pressed
  return tud_hid_keyboard_report(report_id, 0, NULL);
}

bool HIDkeyboard::sendString(const char* _text)
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

bool HIDkeyboard::sendString(String text)
{
  return sendString(text.c_str());
}

#endif
