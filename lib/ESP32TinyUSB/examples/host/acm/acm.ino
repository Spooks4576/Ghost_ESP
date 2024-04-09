#include "usb/usb_host.h"

#include "usb_host.hpp"
#include "usb_acm.hpp"

USBhost host;           // host is required to detect any device, before USB class is initialized
USBacmDevice *device;   // when USB class is detected from descriptor

void acm_events(int event, void *data, size_t len)
{
    switch (event)
    {
    case CDC_CTRL_SET_CONTROL_LINE_STATE:
        log_i("CDC_CTRL_SET_CONTROL_LINE_STATE");
        device->setLineCoding(115200, 0, 0, 8);
        break;

    case CDC_DATA_IN:
    {
        device->INDATA();
        char *buf = (char *)data;
        buf[len] = 0;
        printf("%s", (char *)data);
        break;
    }
    case CDC_DATA_OUT:

        break;

    case CDC_CTRL_SET_LINE_CODING:
        log_i("CDC_CTRL_SET_LINE_CODING");
        break;
    }
}

void client_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV)
    {
        host.open(event_msg);
        usb_device_info_t info = host.getDeviceInfo();
        log_i("device speed: %s, device address: %d, max ep_ctrl size: %d, config: %d", info.speed ? "USB_SPEED_FULL" : "USB_SPEED_LOW", info.dev_addr, info.bMaxPacketSize0, info.bConfigurationValue);
        const usb_device_desc_t *dev_desc = host.getDeviceDescriptor();
        int offset = 0;
        for (size_t i = 0; i < dev_desc->bNumConfigurations; i++)
        {
            const usb_config_desc_t *config_desc = host.getConfigurationDescriptor();
            for (size_t n = 0; n < config_desc->bNumInterfaces; n++)
            {
                const usb_intf_desc_t *intf = usb_parse_interface_descriptor(config_desc, n, 0, &offset);
                if (intf->bInterfaceClass == 0x0a) // CDC ACM
                {
                    device = new USBacmDevice(config_desc, &host);
                    n = config_desc->bNumInterfaces;
                    if (device)
                    {
                        device->init();
                        device->onEvent(acm_events);
                        device->setControlLine(1, 1);
                        device->INDATA();
                    }
                }

                printf("config: %d[%d], interface: %d[%d], intf class: %d\n", i, dev_desc->bNumConfigurations, n, config_desc->bNumInterfaces, intf->bInterfaceClass);
            }
        }
    }
    else
    {
        log_w("DEVICE gone event");
    }
}

void setup()
{
    Serial.begin(115200);
    host.registerClientCb(client_event_callback);
    host.init();
}

void loop()
{
    if (device && device->isConnected())
    {
        device->OUTDATA((uint8_t *)"test\n", 5);
    }
    delay(1000);
}
