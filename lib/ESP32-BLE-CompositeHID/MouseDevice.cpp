#include "MouseDevice.h"
#include "BleCompositeHID.h"

MouseDevice::MouseDevice():
    _config(MouseConfiguration()), // Use default config
    _mouseButtons(),
    _mouseX(0),
    _mouseY(0),
    _mouseWheel(0),
    _mouseHWheel(0)
{
    this->resetButtons();
}

MouseDevice::MouseDevice(const MouseConfiguration& config):
    _config(config), // Copy config to avoid modification
    _mouseButtons(),
    _mouseX(0),
    _mouseY(0),
    _mouseWheel(0),
    _mouseHWheel(0)
{
    this->resetButtons();
}

void MouseDevice::init(NimBLEHIDDevice* hid)
{
    setCharacteristics(hid->inputReport(_config.getReportId()), nullptr);
}

const BaseCompositeDeviceConfiguration* MouseDevice::getDeviceConfig() const
{
    return &_config;
}

void MouseDevice::resetButtons()
{
    std::lock_guard<std::mutex> lock(_mutex);
    memset(&_mouseButtons, 0, sizeof(_mouseButtons));
}

// Mouse
void MouseDevice::mouseClick(uint8_t button)
{
    // No-op
    // TODO: Send two reports one after the other for convience? Can't be both pressed and not pressed in a bitflag
}

void MouseDevice::mousePress(uint8_t button)
{
    uint8_t index = (button - 1) / 8;
    uint8_t bit = (button - 1) % 8;
    uint8_t bitmask = (1 << bit);

    uint8_t result = _mouseButtons[index] | bitmask;

    if (result != _mouseButtons[index])
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _mouseButtons[index] = result;
    }

    if (_config.getAutoReport())
    {
        sendMouseReport();
    }
}

void MouseDevice::mouseRelease(uint8_t button)
{
    uint8_t index = (button - 1) / 8;
    uint8_t bit = (button - 1) % 8;
    uint8_t bitmask = (1 << bit);

    uint64_t result = _mouseButtons[index] & ~bitmask;

    if (result != _mouseButtons[index])
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _mouseButtons[index] = result;
    }

    if (_config.getAutoReport())
    {
        sendMouseReport();
    }
}

void MouseDevice::mouseMove(signed char x, signed char y, signed char scrollX, signed char scrollY)
{
    if (x == -127)
    {
        x = -126;
    }
    if (y == -127)
    {
        y = -126;
    }
    if (scrollX == -127)
    {
        scrollX = -126;
    }
    if (scrollY == -127)
    {
        scrollY = -126;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _mouseX = x;
        _mouseY = y;
        _mouseWheel = scrollY;
        _mouseHWheel = scrollX;
    }

    if (_config.getAutoReport())
    {
        sendMouseReport();
    }
}

void MouseDevice::sendMouseReport(bool defer)
{
    if (defer || _config.getAutoDefer())
    {
        queueDeferredReport(std::bind(&MouseDevice::sendMouseReportImpl, this));
    }
    else
    {
        sendMouseReportImpl();
    }
}

void MouseDevice::sendMouseReportImpl()
{
    auto input = getInput();
    auto parentDevice = this->getParent();

    if (!input || !parentDevice)
        return;
    
    if(!parentDevice->isConnected())
        return;

    uint8_t mouse_report[_config.getDeviceReportSize()];
    uint8_t currentReportIndex = 0;

    { 
        std::lock_guard<std::mutex> lock(_mutex);
        
        memset(&mouse_report, 0, sizeof(mouse_report));
        memcpy(&mouse_report, &_mouseButtons, sizeof(_mouseButtons));
        currentReportIndex += _config.getMouseButtonNumBytes();

        // TODO: Make dynamic based on axis counts
        if (_config.getMouseAxisCount() > 0)
        {
            mouse_report[currentReportIndex++] = _mouseX;
            mouse_report[currentReportIndex++] = _mouseY;
            mouse_report[currentReportIndex++] = _mouseWheel;
            mouse_report[currentReportIndex++] = _mouseHWheel;
        }
    }

    input->setValue(mouse_report, sizeof(mouse_report));
    input->notify();
}