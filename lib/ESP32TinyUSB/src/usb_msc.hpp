#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb_device.hpp"
#include "usb_requests.hpp"

typedef int host_event_t;

class USBmscDevice : public USBhostDevice
{
    friend void usb_transfer_cb(usb_transfer_t *transfer);

private:
    const usb_ep_desc_t * ep_in;
    const usb_ep_desc_t * ep_out;
    
    uint8_t* temp_buffer;               // pointer to buffer in read10, write10

    bool _in = false;
    TaskHandle_t xTaskToNotify = NULL;  // notify 
    uint32_t block_count[4];
    uint32_t block_size[4] = {4096};
    uint8_t _luns;                  // max lun number
    uint8_t _lun;                   // current handled lun
    int event = -1;                 // current handled event

    static USBmscDevice* instance;      // class is treat as singleton, it is used in ff_vfs
    msc_transfer_cb_t callbacks = {};   // user callbacks

public:
    USBmscDevice(const usb_config_desc_t* config_desc, USBhost*);
    ~USBmscDevice();

    bool            init();
    void            reset();
    void            format();
    void            mount(char*, uint8_t lun = 0);
    uint8_t         getMaxLUN();
    uint32_t        getBlockCount(uint8_t lun = 0);
    uint16_t        getBlockSize(uint8_t lun = 0);
    void            registerCallbacks(msc_transfer_cb_t);
    void            onEvent();


public: // POSIX

    // int  open(char* name);
    // void close(int);
    // void read(char* name, uint8_t* buffer, size_t len);
    // void write(char* name, uint8_t* buffer, size_t len);
    // void delete(char* name);


public: // public to use from ff_diskio
    static USBmscDevice* getInstance();
    esp_err_t _read10 (uint8_t lun, int offset, int num_sectors, uint8_t* buff);
    esp_err_t _write10(uint8_t lun, int offset, int num_sectors, uint8_t* buff);

private:
    void inquiry();
    esp_err_t unitReady();
    void _getCapacity(uint8_t);
    void _setCapacity(uint32_t, uint32_t);
    void _emitEvent(host_event_t event, usb_transfer_t* data);
    void _getMaxLUN();
    void csw();
    void csw(usb_transfer_cb_t);

    esp_err_t allocate(size_t);
    esp_err_t dealocate(uint8_t);    


private: // internal callbacks
    void onCSW(usb_transfer_t *transfer);
    void onCBW(usb_transfer_t *transfer);
    void onData(usb_transfer_t *transfer);
};

