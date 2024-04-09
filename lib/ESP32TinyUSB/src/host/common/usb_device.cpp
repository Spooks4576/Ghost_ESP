#include "esp_log.h"
#include "usb_device.hpp"



USBhostDevice::USBhostDevice()
{
}

USBhostDevice::~USBhostDevice()
{
}

esp_err_t USBhostDevice::allocate(size_t _size)
{
    size_t out_worst_case_size = 50 + _size;
    ESP_LOGI("", "allocate with new size: %d [%d]", _size, out_worst_case_size);

    esp_err_t err = 0;
    for (size_t i = 0; i < 2; i++)
    {
        err = usb_host_transfer_alloc(64, 0, &xfer_out[i]);
        if (ESP_OK == err)
        {
            usb_device_handle_t handle = _host->deviceHandle();
            xfer_out[i]->device_handle = handle;
            xfer_out[i]->context = this;
        }
    }

    err = usb_host_transfer_alloc(out_worst_case_size, 0, &xfer_in);
    xfer_in->device_handle = _host->deviceHandle();
    xfer_in->context = this;

    err = usb_host_transfer_alloc(out_worst_case_size, 0, &xfer_write);
    xfer_write->device_handle = _host->deviceHandle();
    xfer_write->context = this;

    err = usb_host_transfer_alloc(out_worst_case_size, 0, &xfer_read);
    xfer_read->device_handle = _host->deviceHandle();
    xfer_read->context = this;

    err = usb_host_transfer_alloc(64, 0, &xfer_ctrl);
    xfer_ctrl->device_handle = _host->deviceHandle();
    xfer_ctrl->context = this;
    xfer_ctrl->bEndpointAddress = 0;

    return err;
}


