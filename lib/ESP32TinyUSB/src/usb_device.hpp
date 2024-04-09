#pragma once
#include "esp_err.h"
#include "usb/usb_host.h"
#include "usb_host.hpp"

// class USBhost;
class USBhostDevice
{
protected:
    USBhost* _host;
    uint8_t itf_num;

    usb_transfer_t *xfer_ctrl = NULL;    //Must be large enough to contain CBW and MSC reset control transfer
    usb_transfer_t *xfer_write = NULL;    //Must be large enough to contain CBW and MSC reset control transfer
    usb_transfer_t *xfer_read = NULL;    //Must be large enough to contain CBW and MSC reset control transfer
    usb_transfer_t *xfer_out[2] = {};    //Must be large enough to contain CBW and MSC reset control transfer
    usb_transfer_t *xfer_in = nullptr;     //Must be large enough to contain CSW and Data

public:
    USBhostDevice();
    ~USBhostDevice();

    virtual bool init() = 0;
    esp_err_t allocate(size_t);

};

