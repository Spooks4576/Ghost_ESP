#include "webusb.h"

#if CFG_TUD_VENDOR

static bool web_serial_connected = false;
static WebUSB *_webUSB = NULL;

WebUSB::WebUSB(uint8_t itf)
{
    _itf = 0;  // hardcoded due to limitations in arduino tinyusb
    _webUSB = this;
    enableVENDOR = true;
    String url = String("www.tinyusb.org/examples/webusb-serial");
    landingPageURI(url, true);
    _EPNUM_VENDOR = EPNUM_VENDOR;
}

void WebUSB::setBaseEP(uint8_t ep)
{
  _EPNUM_VENDOR = ep;
}

bool WebUSB::begin(char* str, const char *url, bool ssl)
{
    if (url != nullptr)
        landingPageURI(url, ssl);

    // Interface number, string index, EP Out & IN address, EP size
    uint8_t vendor[] = {TUD_VENDOR_DESCRIPTOR(ifIdx++, 7, _EPNUM_VENDOR, (uint8_t)(0x80 | _EPNUM_VENDOR), 64)};
    memcpy(&desc_configuration[total], vendor, sizeof(vendor));
    total += sizeof(vendor);
    count++;
    _bcdUSB = 0x210;
    return EspTinyUSB::begin(str, 7);
}

int WebUSB::available()
{
    return tud_vendor_n_available(_itf);
}

int WebUSB::peek()
{
    int pos;
    uint8_t buffer[1];
    if (web_serial_connected)
    {
        tud_vendor_n_peek(_itf, buffer);
        return buffer[0];
    }
    else
    {
        return -1;
    }
}

int WebUSB::read()
{
    if (web_serial_connected)
    {
        char c;
        if (tud_vendor_n_available(_itf))
        {
            uint32_t count = tud_vendor_n_read(_itf, &c, 1);
            return c;
        }
    }

    return -1;
}

size_t WebUSB::read(uint8_t *buffer, size_t size)
{
    if (web_serial_connected)
    {
        if (tud_vendor_n_available(_itf))
        {
            uint32_t count = tud_vendor_n_read(_itf, buffer, size);
            return count;
        }
    }

    return -1;
}

size_t WebUSB::write(uint8_t c)
{
    return write(&c, 1);
}

size_t WebUSB::write(const uint8_t *buffer, size_t size)
{
    uint32_t sent = tud_vendor_n_write(_itf, buffer, size);
    return sent;
}

void WebUSB::flush() {}

WebUSB::operator bool() const
{
    return web_serial_connected;
}

void WebUSB::landingPageURI(String url, bool ssl)
{
    landingPageURI((const char *)url.c_str(), ssl);
}

void WebUSB::landingPageURI(const char *url, bool ssl)
{
    if (_url != NULL)
    {
        free(_url);
    }
    uint8_t size = strlen(url);
    _url = (uint8_t *)calloc(1, size + 3);
    memcpy(&_url[3], url, size);
    _url[0] = size + 3;
    _url[1] = 3;
    _url[2] = ssl;
}

void WebUSB::setCallbacks(WebUSBCallbacks* cb)
{
    m_callbacks = cb;
}

void tud_vendor_rx_cb(uint8_t itf)
{
    if(itf && _webUSB->m_callbacks)
        _webUSB->m_callbacks->onData();
}

//--------------------------------------------------------------------+
// WebUSB use vendor class
//--------------------------------------------------------------------+
extern "C"
{
#define BOS_TOTAL_LEN \
    (TUD_BOS_DESC_LEN + TUD_BOS_WEBUSB_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

    uint8_t const desc_bos[] = {
        // total length, number of device caps
        TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 2),

        // Vendor Code, iLandingPage
        TUD_BOS_WEBUSB_DESCRIPTOR(VENDOR_REQUEST_WEBUSB, 1),

        // Microsoft OS 2.0 descriptor
        TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MICROSOFT)};

    uint8_t const *tud_descriptor_bos_cb(void)
    {
        if(_webUSB == NULL) return nullptr;

        return desc_bos;
    }

    // Invoked when received VENDOR control request
    bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
    {
        if(_webUSB == NULL) return false;
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP ) return true;

    switch (request->bRequest)
    {
        case VENDOR_REQUEST_WEBUSB:
        // match vendor request in BOS descriptor
        // Get landing page url
            if (!_webUSB->_url)
                return false;
            return tud_control_xfer(rhport, request, (void *)_webUSB->_url, _webUSB->_url[0]);

        case VENDOR_REQUEST_MICROSOFT:
            if (request->wIndex == 7)
            {
                // Get Microsoft OS 2.0 compatible descriptor
                uint16_t total_len;
                memcpy(&total_len, desc_ms_os_20 + 8, 2);

                return tud_control_xfer(rhport, request, (void *)desc_ms_os_20, total_len);
            }
            else
            {
                return false;
            }
            break;
        case 0x22:
            // Webserial simulate the CDC_REQUEST_SET_CONTROL_LINE_STATE (0x22) to
            // connect and disconnect.
            web_serial_connected = (request->wValue != 0);

            if(_webUSB->m_callbacks)
                _webUSB->m_callbacks->onConnect(web_serial_connected);

            // response with status OK
            return tud_control_status(rhport, request);

        default:
            // stall unknown request
            return false;
        }

        return true;
    }

}

#endif

