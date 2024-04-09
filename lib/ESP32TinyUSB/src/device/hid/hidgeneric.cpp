#include "hidgeneric.h"
#define EPNUM_HID   0x03
#define REPORT_ID   1
#if CFG_TUD_HID

HIDgeneric::HIDgeneric(uint8_t reportid)
{
  report_id = reportid;
  enableHID = true;
  _EPNUM_HID = EPNUM_HID;
}

bool HIDgeneric::begin(char* str)
{
  uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC_GENERIC_INOUT(CFG_TUD_HID_BUFSIZE, HID_REPORT_ID(report_id))};
  // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
  uint8_t hid[] = {TUD_HID_INOUT_DESCRIPTOR(ifIdx++, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), _EPNUM_HID, (uint8_t)(0x80 | _EPNUM_HID), CFG_TUD_HID_BUFSIZE, 10)};
  memcpy(&desc_configuration[total], hid, sizeof(hid));
  total += sizeof(hid);
  count++;

  memcpy(&hid_report_desc[EspTinyUSB::hid_report_desc_len], (uint8_t *)desc_hid_report, sizeof(desc_hid_report));
  EspTinyUSB::hid_report_desc_len += TUD_HID_INOUT_DESC_LEN;
  log_d("begin len: %d", EspTinyUSB::hid_report_desc_len);

  return EspTinyUSB::begin(str, 6);
}

#endif
