#pragma once
#include "esptinyusb.h"
#define MS_OS_20_DESC_LEN 0xB2
#define EPNUM_VENDOR 0x04
#define _vendor  "Vendor class (webUSB)"
#if CFG_TUD_VENDOR

// https://developers.google.com/web/fundamentals/native-hardware/build-for-webusb#microsoft_os_compatibility_descriptors
const uint8_t desc_ms_os_20[] = {
    // Set header: length, type, windows version, total length
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

    // Configuration subset header: length, type, configuration index, reserved,
    // configuration total length
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
    0, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),

    // Function Subset header: length, type, first interface, reserved, subset
    // length
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
    0 /*itf num*/, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),

    // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub
    // compatible ID
    U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W',
    'I', 'N', 'U', 'S', 'B', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // sub-compatible

    // MS OS 2.0 Registry property descriptor: length, type
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY), U16_TO_U8S_LE(0x0007),
    U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength and
                           // PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00,
    'n', 0x00, 't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00,
    'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00,
    0x00,
    U16_TO_U8S_LE(0x0050), // wPropertyDataLength
    // bPropertyData: “{975F44D9-0D08-43FD-8B3E-127CA8AFFF9D}”.
    '{', 0x00, '9', 0x00, '7', 0x00, '5', 0x00, 'F', 0x00, '4', 0x00, '4', 0x00,
    'D', 0x00, '9', 0x00, '-', 0x00, '0', 0x00, 'D', 0x00, '0', 0x00, '8', 0x00,
    '-', 0x00, '4', 0x00, '3', 0x00, 'F', 0x00, 'D', 0x00, '-', 0x00, '8', 0x00,
    'B', 0x00, '3', 0x00, 'E', 0x00, '-', 0x00, '1', 0x00, '2', 0x00, '7', 0x00,
    'C', 0x00, 'A', 0x00, '8', 0x00, 'A', 0x00, 'F', 0x00, 'F', 0x00, 'F', 0x00,
    '9', 0x00, 'D', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00};

enum
{
  VENDOR_REQUEST_WEBUSB = 1,
  VENDOR_REQUEST_MICROSOFT = 2
};

class WebUSBCallbacks : public USBCallbacks {
public:
  virtual ~WebUSBCallbacks() { }
  virtual void onConnect(bool state) { }
  virtual void onData() { }
};

class WebUSB : public EspTinyUSB
{
public:
  WebUSB(uint8_t itf = 0);
  bool begin(char* vendor = nullptr, const char* url = nullptr, bool ssl = true);
  int available(void);
  int peek(void);
  int read(void);
  size_t read(uint8_t *buffer, size_t size);
  void flush(void);
  size_t write(uint8_t);
  size_t write(const uint8_t *buffer, size_t size);

  operator bool() const;

  void setCallbacks(WebUSBCallbacks*);
  void landingPageURI(String url, bool ssl);
  void landingPageURI(const char* url, bool ssl);
  void setBaseEP(uint8_t);
  WebUSBCallbacks* m_callbacks;

private:

  friend bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);

  uint8_t* _url;
  uint8_t _EPNUM_VENDOR;

};

#endif
