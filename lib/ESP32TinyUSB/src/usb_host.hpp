#pragma once
#include "usb/usb_host.h"

class USBhost
{
    friend void _client_event_callback(const usb_host_client_event_msg_t *event_msg, void *arg);

protected:
    usb_device_info_t dev_info;
    usb_host_client_handle_t client_hdl;
    usb_device_handle_t dev_hdl;
    
    usb_host_client_event_cb_t _client_event_cb = nullptr;
    uint8_t _dev_addr;
    uint8_t _configs;
    uint8_t _itfs;

public:
    USBhost();
    ~USBhost();


    bool init(bool create_tasks = true);
    bool open(const usb_host_client_event_msg_t *event_msg);
    usb_device_info_t getDeviceInfo();
    const usb_device_desc_t* getDeviceDescriptor();
    const usb_config_desc_t* getConfigurationDescriptor();

    uint8_t getConfiguration();
    bool setConfiguration(uint8_t);
    void parseConfig();

    usb_host_client_handle_t clientHandle();
    usb_device_handle_t deviceHandle();
    
    void registerClientCb(usb_host_client_event_cb_t cb) { _client_event_cb = cb; }

};
