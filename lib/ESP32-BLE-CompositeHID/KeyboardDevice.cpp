#include "KeyboardDevice.h"
#include "KeyboardDescriptors.h"
#include "BleCompositeHID.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "KeyboardDevice"
#else
#include "esp_log.h"
static const char *LOG_TAG = "KeyboardDevice";
#endif

KeyboardCallbacks::KeyboardCallbacks(KeyboardDevice* device) :
    _device(device)
{
}

void KeyboardCallbacks::onWrite(NimBLECharacteristic* pCharacteristic)
{
    // An example packet we might receive from XInput might look like 0x0300002500ff00ff
    KeyboardOutputReport ledReport = pCharacteristic->getValue<uint8_t>();
    ESP_LOGD(LOG_TAG, "KeyboardDevice::onWrite - LED Report: %d", ledReport);
}

void KeyboardCallbacks::onRead(NimBLECharacteristic* pCharacteristic)
{
}

void KeyboardCallbacks::onNotify(NimBLECharacteristic* pCharacteristic)
{
}

void KeyboardCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code)
{
}

KeyboardDevice::KeyboardDevice() :
    _config(KeyboardConfiguration(KEYBOARD_REPORT_ID)),
    _input(),
    _output(),
    _inputReport(),
    _callbacks(nullptr)
{
    resetKeys();
}

KeyboardDevice::KeyboardDevice(const KeyboardConfiguration& config) :
    _config(config),
    _input(),
    _output(),
    _inputReport(),
    _callbacks(nullptr)
{
    resetKeys();
}

KeyboardDevice::~KeyboardDevice()
{
    if (getOutput() && _callbacks){
        getOutput()->setCallbacks(nullptr);
        delete _callbacks;
        _callbacks = nullptr;
    }
}

void KeyboardDevice::init(NimBLEHIDDevice* hid)
{
    _input = hid->inputReport(_config.getReportId());
    _mediaInput = hid->inputReport(MEDIA_KEYS_REPORT_ID);
    _output = hid->outputReport(_config.getReportId());
    _callbacks = new KeyboardCallbacks(this);
    _output->setCallbacks(_callbacks);

    setCharacteristics(_input, _output);
}

const BaseCompositeDeviceConfiguration* KeyboardDevice::getDeviceConfig() const
{
    return &_config;
}

void KeyboardDevice::resetKeys()
{
    std::lock_guard<std::mutex> lock(_mutex);
    memset(&_inputReport, KEY_NONE, sizeof(_inputReport));
    _mediaKeyInputReport.keys = 0x000000;
}

void KeyboardDevice::modifierKeyPress(uint8_t modifier)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _inputReport.modifiers |= modifier;
    }

    if (_config.getAutoReport())
    {
        sendKeyReport();
    }
}

void KeyboardDevice::modifierKeyRelease(uint8_t modifier)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _inputReport.modifiers ^= modifier;
    }

    if (_config.getAutoReport())
    {
        sendKeyReport();
    }
}

void KeyboardDevice::mediaKeyPress(uint32_t mediaKey)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _mediaKeyInputReport.keys |= mediaKey;
    }

    if (_config.getAutoReport())
    {
        sendMediaKeyReport();
    }
}

void KeyboardDevice::mediaKeyRelease(uint32_t mediaKey)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _mediaKeyInputReport.keys ^= mediaKey;
    }

    if (_config.getAutoReport())
    {
        sendMediaKeyReport();
    }
}

void KeyboardDevice::keyPress(uint8_t keyCode)
{
    // Find the first empty slot
    bool full = true;
    for (int slotIdx = 0; slotIdx < 6; slotIdx++)
    {
        if (_inputReport.keys[slotIdx] == 0x00)
        {
            full = false;
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.keys[slotIdx] = keyCode;
            break;
        }
    }

    // If no slots are free, set the overflow flag
    if(full){
        std::lock_guard<std::mutex> lock(_mutex);
        memset(_inputReport.keys, KEY_ERR_OVF, sizeof(_inputReport.keys));
    }

    if (_config.getAutoReport())
    {
        sendKeyReport();
    }
}

void KeyboardDevice::keyRelease(uint8_t keyCode)
{
    for (int slotIdx = 0; slotIdx < 6; slotIdx++)
    {
        if (_inputReport.keys[slotIdx] == keyCode)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.keys[slotIdx] = 0x00;
            break;
        }
    }

    if (_config.getAutoReport())
    {
        sendKeyReport();
    }
}

void KeyboardDevice::sendKeyReport(bool defer)
{
    if(defer || _config.getAutoDefer()){
        queueDeferredReport(std::bind(&KeyboardDevice::sendKeyReportImpl, this));
    } else {
        sendKeyReportImpl();
    }
}

void KeyboardDevice::sendKeyReportImpl()
{
    auto input = getInput();
    auto parentDevice = this->getParent();

    if (!input || !parentDevice)
        return;

    if(!parentDevice->isConnected())
        return;

    uint8_t currentReportIndex = 0;
    uint8_t m[_config.getDeviceReportSize()];
    memset(&m, 0, sizeof(m));

    // Copy key input report into buffer
    {
        std::lock_guard<std::mutex> lock(_mutex);
        memcpy(&m[currentReportIndex], &_inputReport, sizeof(_inputReport));
        input->setValue((uint8_t*)&_inputReport, sizeof(_inputReport));
    }
    input->notify();
}

void KeyboardDevice::sendMediaKeyReport(bool defer)
{
    if(defer || _config.getAutoDefer()){
        queueDeferredReport(std::bind(&KeyboardDevice::sendMediaKeyReportImpl, this));
    } else {
        sendMediaKeyReportImpl();
    }
}


void KeyboardDevice::sendMediaKeyReportImpl()
{
   auto input = getInput();
    auto parentDevice = this->getParent();

    if (!input || !parentDevice)
        return;

    if(!parentDevice->isConnected())
        return;

    uint8_t m[3];
    memset(&m, 0, sizeof(m));

    // Copy key input report into buffer
    {
        std::lock_guard<std::mutex> lock(_mutex);
        m[0] = _mediaKeyInputReport.keys & 0xFF;
        m[1] = (_mediaKeyInputReport.keys >> 8) & 0xFF;
        m[2] = (_mediaKeyInputReport.keys >> 16) & 0xFF;

        _mediaInput->setValue((uint8_t*)&m, sizeof(m));
    }
    _mediaInput->notify();
}