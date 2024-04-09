#include "Arduino.h"
#include "esptinyusb.h"
#include "cdcusb.h"

#if CFG_TUD_CDC

#define EPNUM_CDC   0x02

static CDCusb* _CDCusb[2] = {};
enum { CDC_LINE_IDLE, CDC_LINE_1, CDC_LINE_2, CDC_LINE_3 };

CDCusb::CDCusb(uint8_t itf)
{
    _itf = itf;
    _CDCusb[_itf] = this;
    enableCDC = true;
    _EPNUM_CDC = EPNUM_CDC;
}

void CDCusb::setBaseEP(uint8_t ep)
{
  _EPNUM_CDC = ep;
}

bool CDCusb::begin(char* str)
{
    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    uint8_t cdc[TUD_CDC_DESC_LEN] = {TUD_CDC_DESCRIPTOR(ifIdx, 4, (uint8_t)(0x80 | (_EPNUM_CDC - 1)), 8, (uint8_t)_EPNUM_CDC, (uint8_t)(0x80 | _EPNUM_CDC), 64)};
    memcpy(&desc_configuration[total], cdc, sizeof(cdc));
    total += sizeof(cdc);
    ifIdx += 2;
    count += 2;

    if(!EspTinyUSB::begin(str, 4)) return false;
    return true;
}

int CDCusb::available()
{
    return tud_cdc_n_available(_itf);
}

int CDCusb::peek()
{
    if (tud_cdc_n_connected(_itf))
    {
        uint8_t buffer;
        tud_cdc_n_peek(_itf, &buffer);
        return buffer;
    }
    else
    {
        return -1;
    }
}

int CDCusb::read()
{
    if (tud_cdc_n_connected(_itf))
    {
        if (tud_cdc_n_available(_itf))
        {
            char c;
            uint32_t count = tud_cdc_n_read(_itf, &c, 1);
            return c;
        }
    }

    return -1;
}

size_t CDCusb::read(uint8_t *buffer, size_t size)
{
    if (tud_cdc_n_connected(_itf))
    {
        if (tud_cdc_n_available(_itf))
        {
            uint32_t count = tud_cdc_n_read(_itf, buffer, size);
            return count;
        }
    }

    return 0;
}

size_t CDCusb::write(uint8_t buffer)
{
    uint8_t c = buffer;
    if (tud_cdc_n_connected(_itf))
    {
        uint32_t d = tud_cdc_n_write(_itf, &c, 1);
        tud_cdc_n_write_flush(_itf);
        return d;
    }
    else
    {
        return 0;
    }
}

size_t CDCusb::write(const uint8_t *buffer, size_t size)
{
    if (tud_cdc_n_connected(_itf))
    {
        uint32_t d = 0;
        do{
            d += tud_cdc_n_write(_itf, buffer + d, size <= 64 ? size: size - d);
        }while(size - d > 0 || d == 0);
        tud_cdc_n_write_flush(_itf);
        return d;
    }
    else
    {
        return 0;
    }
}

void CDCusb::flush()
{
    tud_cdc_n_read_flush(_itf);
    tud_cdc_n_write_flush(_itf);
}

CDCusb::operator bool() const
{
    return tud_cdc_n_connected(_itf);
}

void CDCusb::setWantedChar(char c)
{
    tud_cdc_n_set_wanted_char(0, c);
}

void CDCusb::setCallbacks(CDCCallbacks* cb)
{
    m_callbacks = cb;
}

uint32_t CDCusb::getBitrate()
{
    return coding.bit_rate;
}

uint8_t CDCusb::getParity()
{
    return coding.parity;
}

uint8_t CDCusb::getDataBits()
{
    return coding.data_bits;
}

uint8_t CDCusb::getStopBits()
{
    return coding.stop_bits;
}

// Invoked when received new data
void tud_cdc_rx_cb(uint8_t itf)
{
    if(_CDCusb[itf]->m_callbacks)
        _CDCusb[itf]->m_callbacks->onData();
}

// void tud_cdc_n_set_wanted_char (uint8_t itf, char wanted);
// Invoked when received `wanted_char`
void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char) {
    if(_CDCusb[itf]->m_callbacks)
        _CDCusb[itf]->m_callbacks->onWantedChar(wanted_char);
}

// Invoked when line state DTR & RTS are changed via SET_CONTROL_LINE_STATE
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    static uint8_t lineState = CDC_LINE_IDLE;
    bool reset = true;

    if(_CDCusb[itf]->m_callbacks)
        reset = _CDCusb[itf]->m_callbacks->onConnect(dtr, rts);

    if(!dtr && rts){
        if(lineState == CDC_LINE_IDLE){
            lineState++;
        } else {
            lineState = CDC_LINE_IDLE;
        }
    } else if(dtr && rts){
        if(lineState == CDC_LINE_1){
            lineState++;
        } else {
            lineState = CDC_LINE_IDLE;
        }
    } else if(dtr && !rts){
        if(lineState == CDC_LINE_2){
            lineState++;
        } else {
            lineState = CDC_LINE_IDLE;
        }
    } else if(!dtr && !rts){
        if(lineState == CDC_LINE_3){
            if (reset)
            {
                _CDCusb[itf]->persistentReset(RESTART_BOOTLOADER);
            }
        } else {
            lineState = CDC_LINE_IDLE;
        }
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
    if(p_line_coding->bit_rate == 1200){
        esp_restart();
    }
    memcpy(&_CDCusb[itf]->coding, p_line_coding, sizeof(cdc_line_coding_t));
    if(_CDCusb[itf]->m_callbacks)
        _CDCusb[itf]->m_callbacks->onCodingChange(p_line_coding);
}

#endif
