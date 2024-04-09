#include "esp_log.h"
#include "string.h"

#include "usb_acm.hpp"

void IRAM_ATTR usb_ctrl_cb(usb_transfer_t *transfer)
{
    USBacmDevice *dev = (USBacmDevice *)transfer->context;

    if (transfer->data_buffer[0] == SET_VALUE && transfer->data_buffer[1] == SET_LINE_CODING) // set line coding
    {
        dev->_callback(CDC_CTRL_SET_LINE_CODING, transfer);
    }
    else if (transfer->data_buffer[0] == GET_VALUE && transfer->data_buffer[1] == GET_LINE_CODING) // get line coding
    {
        dev->_callback(CDC_CTRL_GET_LINE_CODING, transfer);
    }
    else if (transfer->data_buffer[0] == SET_VALUE && transfer->data_buffer[1] == SET_CONTROL_LINE_STATE) // set line coding
    {
        dev->_callback(CDC_CTRL_SET_CONTROL_LINE_STATE, transfer);
    }
}

void IRAM_ATTR usb_read_cb(usb_transfer_t *transfer)
{
    USBacmDevice *dev = (USBacmDevice *)transfer->context;
    dev->_callback(CDC_DATA_IN, transfer);
}

void IRAM_ATTR usb_write_cb(usb_transfer_t *transfer)
{
    USBacmDevice *dev = (USBacmDevice *)transfer->context;
    dev->_callback(CDC_DATA_OUT, transfer);
}

USBacmDevice::USBacmDevice(const usb_config_desc_t *config_desc, USBhost *host)
{
    _host = host;
    int offset = 0;
    for (size_t n = 0; n < config_desc->bNumInterfaces; n++)
    {
        const usb_intf_desc_t *intf = usb_parse_interface_descriptor(config_desc, n, 0, &offset);
        const usb_ep_desc_t *ep = nullptr;

        if (intf->bInterfaceClass == 0x02)
        {
            if (intf->bNumEndpoints != 1)
                return;
            int _offset = 0;
            ep = usb_parse_endpoint_descriptor_by_index(intf, 0, config_desc->wTotalLength, &_offset);
            ep_int = ep;
            ESP_LOGI("", "EP CDC comm.");

            printf("EP num: %d/%d, len: %d, ", 1, intf->bNumEndpoints, config_desc->wTotalLength);
            if (ep)
                printf("address: 0x%02x, EP max size: %d, dir: %s\n", ep->bEndpointAddress, ep->wMaxPacketSize, (ep->bEndpointAddress & 0x80) ? "IN" : "OUT");
            else
                ESP_LOGW("", "error to parse endpoint by index; EP num: %d/%d, len: %d", 1, intf->bNumEndpoints, config_desc->wTotalLength);

            esp_err_t err = usb_host_interface_claim(_host->clientHandle(), _host->deviceHandle(), n, 0);
            ESP_LOGI("", "interface claim status: %d", err);
            itf_num = 0;
        }
        else if (intf->bInterfaceClass == 0x0a)
        {
            if (intf->bNumEndpoints != 2)
                return;
            ESP_LOGI("", "EP CDC data.");
            for (size_t i = 0; i < intf->bNumEndpoints; i++)
            {
                int _offset = 0;
                ep = usb_parse_endpoint_descriptor_by_index(intf, i, config_desc->wTotalLength, &_offset);
                if (ep->bEndpointAddress & 0x80)
                {
                    ep_in = ep;
                }
                else
                {
                    ep_out = ep;
                }

                printf("EP num: %d/%d, len: %d, ", i + 1, intf->bNumEndpoints, config_desc->wTotalLength);
                if (ep)
                    printf("address: 0x%02x, EP max size: %d, dir: %s\n", ep->bEndpointAddress, ep->wMaxPacketSize, (ep->bEndpointAddress & 0x80) ? "IN" : "OUT");
                else
                    ESP_LOGW("", "error to parse endpoint by index; EP num: %d/%d, len: %d", i + 1, intf->bNumEndpoints, config_desc->wTotalLength);
            }
            esp_err_t err = usb_host_interface_claim(_host->clientHandle(), _host->deviceHandle(), n, 0);
            ESP_LOGI("", "interface claim status: %d", err);
        }
    }
}

USBacmDevice::~USBacmDevice()
{
}

bool USBacmDevice::init()
{
    usb_device_info_t info = _host->getDeviceInfo();

    esp_err_t err = usb_host_transfer_alloc(64, 0, &xfer_write);
    xfer_write->device_handle = _host->deviceHandle();
    xfer_write->context = this;
    xfer_write->callback = usb_write_cb;
    xfer_write->bEndpointAddress = ep_out->bEndpointAddress;

    err = usb_host_transfer_alloc(64, 0, &xfer_read);
    xfer_read->device_handle = _host->deviceHandle();
    xfer_read->context = this;
    xfer_read->callback = usb_read_cb;
    xfer_read->bEndpointAddress = ep_in->bEndpointAddress;

    err = usb_host_transfer_alloc(info.bMaxPacketSize0, 0, &xfer_ctrl);
    xfer_ctrl->device_handle = _host->deviceHandle();
    xfer_ctrl->context = this;
    xfer_ctrl->callback = usb_ctrl_cb;
    xfer_ctrl->bEndpointAddress = 0;

    return true;
}

void USBacmDevice::setControlLine(bool dtr, bool rts)
{
    USB_CTRL_REQ_CDC_SET_CONTROL_LINE_STATE((usb_setup_packet_t *)xfer_ctrl->data_buffer, 0, dtr, rts);
    xfer_ctrl->num_bytes = sizeof(usb_setup_packet_t) + ((usb_setup_packet_t *)xfer_ctrl->data_buffer)->wLength;
    esp_err_t err = usb_host_transfer_submit_control(_host->clientHandle(), xfer_ctrl);
}

void USBacmDevice::setLineCoding(uint32_t bitrate, uint8_t cf, uint8_t parity, uint8_t bits)
{
    line_coding_t data;
    data.dwDTERate = bitrate;
    data.bCharFormat = cf;
    data.bParityType = parity;
    data.bDataBits = bits;
    USB_CTRL_REQ_CDC_SET_LINE_CODING((usb_setup_packet_t *)xfer_ctrl->data_buffer, 0, bitrate, cf, parity, bits);
    memcpy(xfer_ctrl->data_buffer + sizeof(usb_setup_packet_t), &data, sizeof(line_coding_t));
    xfer_ctrl->num_bytes = sizeof(usb_setup_packet_t) + 7;
    esp_err_t err = usb_host_transfer_submit_control(_host->clientHandle(), xfer_ctrl);
}

void USBacmDevice::getLineCoding()
{
    USB_CTRL_REQ_CDC_GET_LINE_CODING((usb_setup_packet_t *)xfer_ctrl->data_buffer, 0);
    xfer_ctrl->num_bytes = sizeof(usb_setup_packet_t) + ((usb_setup_packet_t *)xfer_ctrl->data_buffer)->wLength;
    esp_err_t err = usb_host_transfer_submit_control(_host->clientHandle(), xfer_ctrl);
}

void USBacmDevice::INDATA()
{
    if (!connected)
        return;
    xfer_read->num_bytes = 64;

    esp_err_t err = usb_host_transfer_submit(xfer_read);
    if (err)
    {
        ESP_LOGW("", "test read data: 0x%02x", err);
    }
}

void USBacmDevice::OUTDATA(uint8_t *data, size_t len)
{
    if (!connected)
        return;
    if (!len)
        return;
    xfer_write->num_bytes = len;
    memcpy(xfer_write->data_buffer, data, len);

    esp_err_t err = usb_host_transfer_submit(xfer_write);
    if (err)
    {
        ESP_LOGW("", "test write data: 0x%02x", err);
    }
}

bool USBacmDevice::isConnected()
{
    return connected;
}

void USBacmDevice::onEvent(cdc_event_cb_t _cb)
{
    event_cb = _cb;
}

void USBacmDevice::_callback(int event, usb_transfer_t *transfer)
{
    switch (event)
    {
    case CDC_DATA_IN:
        if (event_cb)
            event_cb(event, transfer->data_buffer, transfer->actual_num_bytes);
        break;
    case CDC_DATA_OUT:
        if (event_cb)
            event_cb(event, transfer->data_buffer, transfer->actual_num_bytes);
        break;

    default:
        if (event_cb)
            event_cb(event, NULL, transfer->actual_num_bytes);
        connected = true;
        break;
    }
}
