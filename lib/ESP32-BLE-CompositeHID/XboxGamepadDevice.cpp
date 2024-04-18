#include "XboxGamepadDevice.h"
#include "BleCompositeHID.h"


#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "XboxGamepadDevice"
#else
#include "esp_log.h"
static const char *LOG_TAG = "XboxGamepadDevice";
#endif

XboxGamepadCallbacks::XboxGamepadCallbacks(XboxGamepadDevice* device) : _device(device)
{
}

void XboxGamepadCallbacks::onWrite(NimBLECharacteristic* pCharacteristic)
{    
    // An example packet we might receive from XInput might look like 0x0300002500ff00ff
    XboxGamepadOutputReportData vibrationData = pCharacteristic->getValue<uint64_t>();
    
    ESP_LOGD(LOG_TAG, "XboxGamepadCallbacks::onWrite, Size: %d, DC enable: %d, magnitudeWeak: %d, magnitudeStrong: %d, duration: %d, start delay: %d, loop count: %d", 
        pCharacteristic->getValue().size(),
        vibrationData.dcEnableActuators, 
        vibrationData.weakMotorMagnitude, 
        vibrationData.strongMotorMagnitude, 
        vibrationData.duration, 
        vibrationData.startDelay, 
        vibrationData.loopCount
    );
}

void XboxGamepadCallbacks::onRead(NimBLECharacteristic* pCharacteristic)
{
    ESP_LOGD(LOG_TAG, "XboxGamepadCallbacks::onRead");
}

void XboxGamepadCallbacks::onNotify(NimBLECharacteristic* pCharacteristic)
{
    ESP_LOGD(LOG_TAG, "XboxGamepadCallbacks::onNotify");
}

void XboxGamepadCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code)
{
    ESP_LOGD(LOG_TAG, "XboxGamepadCallbacks::onStatus, status: %d, code: %d", status, code);
}

XboxGamepadDevice::XboxGamepadDevice() :
    _config(new XboxOneSControllerDeviceConfiguration()),
    _extra_input(nullptr),
    _callbacks(nullptr)
{
}

// XboxGamepadDevice methods
XboxGamepadDevice::XboxGamepadDevice(XboxGamepadDeviceConfiguration* config) :
    _config(config),
    _extra_input(nullptr),
    _callbacks(nullptr)
{
}

XboxGamepadDevice::~XboxGamepadDevice() {
    if (getOutput() && _callbacks){
        getOutput()->setCallbacks(nullptr);
        delete _callbacks;
        _callbacks = nullptr;
    }

    if(_extra_input){
        delete _extra_input;
        _extra_input = nullptr;
    }

    if(_config){
        delete _config;
        _config = nullptr;
    }
}

void XboxGamepadDevice::init(NimBLEHIDDevice* hid) {
    /// Create input characteristic to send events to the computer
    auto input = hid->inputReport(XBOX_INPUT_REPORT_ID);
    //_extra_input = hid->inputReport(XBOX_EXTRA_INPUT_REPORT_ID);

    // Create output characteristic to handle events coming from the computer
    auto output = hid->outputReport(XBOX_OUTPUT_REPORT_ID);
    _callbacks = new XboxGamepadCallbacks(this);
    output->setCallbacks(_callbacks);

    setCharacteristics(input, output);
}

const BaseCompositeDeviceConfiguration* XboxGamepadDevice::getDeviceConfig() const {
    // Return the device configuration
    return _config;
}

void XboxGamepadDevice::resetInputs() {
    std::lock_guard<std::mutex> lock(_mutex);
    memset(&_inputReport, 0, sizeof(XboxGamepadInputReportData));
}

void XboxGamepadDevice::press(uint16_t button) {
    // Avoid double presses
    if (!isPressed(button))
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.buttons |= button;
            ESP_LOGD(LOG_TAG, "XboxGamepadDevice::press, button: %d", button);
        }

        if (_config->getAutoReport())
        {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::release(uint16_t button) {
    // Avoid double presses
    if (isPressed(button))
    {   
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.buttons ^= button;
            ESP_LOGD(LOG_TAG, "XboxGamepadDevice::release, button: %d", button);
        }

        if (_config->getAutoReport())
        {
            sendGamepadReport();
        }
    }
}

bool XboxGamepadDevice::isPressed(uint16_t button) {
    std::lock_guard<std::mutex> lock(_mutex);
    return (bool)((_inputReport.buttons & button) == button);
}

void XboxGamepadDevice::setLeftThumb(int16_t x, int16_t y) {
    x = constrain(x, XBOX_STICK_MIN, XBOX_STICK_MAX);
    y = constrain(y, XBOX_STICK_MIN, XBOX_STICK_MAX);

    if(_inputReport.x != x || _inputReport.y != y){
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.x = (uint16_t)(x + 0x8000);
            _inputReport.y = (uint16_t)(y + 0x8000);
        }

        if (_config->getAutoReport())
        {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::setRightThumb(int16_t z, int16_t rZ) {
    z = constrain(z, XBOX_STICK_MIN, XBOX_STICK_MAX);
    rZ = constrain(rZ, XBOX_STICK_MIN, XBOX_STICK_MAX);

    if(_inputReport.z != z || _inputReport.rz != rZ){
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.z = (uint16_t)(z + 0x8000);
            _inputReport.rz = (uint16_t)(rZ+ 0x8000);
        }

        if (_config->getAutoReport())
        {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::setLeftTrigger(uint16_t value) {
    value = constrain(value, XBOX_TRIGGER_MIN, XBOX_TRIGGER_MAX);

    if (_inputReport.brake != value) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.brake = value;
        }

        if (_config->getAutoReport()) {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::setRightTrigger(uint16_t value) {
    value = constrain(value, XBOX_TRIGGER_MIN, XBOX_TRIGGER_MAX);

    if (_inputReport.accelerator != value) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.accelerator = value;
        }

        if (_config->getAutoReport()) {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::setTriggers(uint16_t left, uint16_t right) {
    left = constrain(left, XBOX_TRIGGER_MIN, XBOX_TRIGGER_MAX);
    right = constrain(right, XBOX_TRIGGER_MIN, XBOX_TRIGGER_MAX);

    if (_inputReport.brake != left || _inputReport.accelerator != right) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.brake = left;
            _inputReport.accelerator = right;
        }
        if (_config->getAutoReport()) {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::pressDPadDirection(uint8_t direction) {

    // Avoid double presses
    if (!isDPadPressed(direction))
    {
        ESP_LOGD(LOG_TAG, "Pressing dpad direction %s", dPadDirectionName(direction).c_str());
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.hat = direction;
        }

        if (_config->getAutoReport())
        {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::pressDPadDirectionFlag(XboxDpadFlags direction) {
    // Filter opposite button presses
    if((direction & (XboxDpadFlags::NORTH | XboxDpadFlags::SOUTH)) == (XboxDpadFlags::NORTH | XboxDpadFlags::SOUTH)){
        ESP_LOGD(LOG_TAG, "Filtering opposite button presses - up down");
        direction = (XboxDpadFlags)(direction ^ (uint8_t)(XboxDpadFlags::NORTH | XboxDpadFlags::SOUTH));
    }
    if((direction & (XboxDpadFlags::EAST | XboxDpadFlags::WEST)) == (XboxDpadFlags::EAST | XboxDpadFlags::WEST)){
        ESP_LOGD(LOG_TAG, "Filtering opposite button presses - left right");
        direction = (XboxDpadFlags)(direction ^ (uint8_t)(XboxDpadFlags::EAST | XboxDpadFlags::WEST));
    }

    pressDPadDirection(dPadDirectionToValue(direction));
}

void XboxGamepadDevice::releaseDPad() {
    pressDPadDirection(XBOX_BUTTON_DPAD_NONE);
}

bool XboxGamepadDevice::isDPadPressed(uint8_t direction) {
    std::lock_guard<std::mutex> lock(_mutex);
    // Serial.print("Internal hat value:");
    // Serial.println(_inputReport.hat, HEX);
    return _inputReport.hat == direction;
    //return (bool)((_inputReport.hat & direction) == direction);
}

bool XboxGamepadDevice::isDPadPressedFlag(XboxDpadFlags direction) {
    std::lock_guard<std::mutex> lock(_mutex);

    if(direction == XboxDpadFlags::NORTH){
        return _inputReport.hat == XBOX_BUTTON_DPAD_NORTH;
    } else if(direction == (XboxDpadFlags::NORTH & XboxDpadFlags::EAST)){
        return _inputReport.hat == XBOX_BUTTON_DPAD_NORTHEAST;
    } else if(direction == XboxDpadFlags::EAST){
        return _inputReport.hat == XBOX_BUTTON_DPAD_EAST;
    } else if(direction == (XboxDpadFlags::SOUTH & XboxDpadFlags::EAST)){
        return _inputReport.hat == XBOX_BUTTON_DPAD_SOUTHEAST;
    } else if(direction == XboxDpadFlags::SOUTH){
        return _inputReport.hat == XBOX_BUTTON_DPAD_SOUTH;
    } else if(direction == (XboxDpadFlags::SOUTH & XboxDpadFlags::WEST)){
        return _inputReport.hat == XBOX_BUTTON_DPAD_SOUTHWEST;
    } else if(direction == XboxDpadFlags::WEST){
        return _inputReport.hat == XBOX_BUTTON_DPAD_WEST;
    } else if(direction == (XboxDpadFlags::NORTH & XboxDpadFlags::WEST)){
        return _inputReport.hat == XBOX_BUTTON_DPAD_NORTHWEST;
    }
    return false;
}


void XboxGamepadDevice::pressShare() {
    // Avoid double presses
    if (!(_inputReport.share & XBOX_BUTTON_SHARE))
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.share |= XBOX_BUTTON_SHARE;
        }

        if (_config->getAutoReport())
        {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::releaseShare() {
    if (_inputReport.share & XBOX_BUTTON_SHARE)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.share ^= XBOX_BUTTON_SHARE;
        }

        if (_config->getAutoReport())
        {
            sendGamepadReport();
        }
    }
}

void XboxGamepadDevice::sendGamepadReport(bool defer) {
    if(defer || _config->getAutoDefer()){
        queueDeferredReport(std::bind(&XboxGamepadDevice::sendGamepadReportImpl, this));
    } else {
        sendGamepadReportImpl();
    }
}

void XboxGamepadDevice::sendGamepadReportImpl(){
    auto input = getInput();
    auto parentDevice = this->getParent();

    if (!input || !parentDevice)
        return;

    if(!parentDevice->isConnected())
        return;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        size_t packedSize = sizeof(_inputReport);
        ESP_LOGD(LOG_TAG, "Sending gamepad report, size: %d", packedSize);
        input->setValue((uint8_t*)&_inputReport, packedSize);
    }
    input->notify();
}
