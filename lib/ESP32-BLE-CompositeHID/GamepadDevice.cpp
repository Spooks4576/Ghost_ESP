#include "GamepadDevice.h"
#include "BleCompositeHID.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "GamepadDevice"
#else
#include "esp_log.h"
static const char *LOG_TAG = "GamepadDevice";
#endif

GamepadCallbacks::GamepadCallbacks(GamepadDevice* device) : _device(device)
{
}

void GamepadCallbacks::onWrite(NimBLECharacteristic* pCharacteristic)
{
    ESP_LOGD(LOG_TAG, "GamepadCallbacks::onWrite, value: %s", pCharacteristic->getValue().c_str());
    
    uint8_t playerIndicatorBitflags = pCharacteristic->getValue<uint8_t>();
    uint8_t playerIndicator = 0;
    
    // Iterate over each bit position
    for (int i = 0; i < 8; ++i) {
        // Check if the bit is set (1)
        if ((playerIndicatorBitflags & (1 << i)) != 0) {
            // If the bit is set, add the corresponding decimal value
            playerIndicator += (1 << i);
        }
    }

    // Ensure the result is in the range 1 to 8
    playerIndicator = (playerIndicator % 8) + 1;

    _device->playerIndicator = playerIndicator;
}

void GamepadCallbacks::onRead(NimBLECharacteristic* pCharacteristic)
{
    ESP_LOGD(LOG_TAG, "GamepadCallbacks::onRead");
}

void GamepadCallbacks::onNotify(NimBLECharacteristic* pCharacteristic)
{
    ESP_LOGD(LOG_TAG, "GamepadCallbacks::onNotify");
}

void GamepadCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code)
{
    ESP_LOGD(LOG_TAG, "GamepadCallbacks::onStatus, status: %d, code: %d", status, code);
}

GamepadDevice::GamepadDevice() : 
    _config(GamepadConfiguration()), 
    _buttons(),
    _specialButtons(),
    _x(0),
    _y(0),
    _z(0),
    _rZ(0),
    _rX(0),
    _rY(0),
    _slider1(0),
    _slider2(0),
    _rudder(0),
    _throttle(0),
    _accelerator(0),
    _brake(0),
    _steering(0),
    _hat1(0),
    _hat2(0),
    _hat3(0),
    _hat4(0),
    _callbacks(nullptr),
    _setEffectCharacteristic(nullptr),
    _setEnvelopeCharacteristic(nullptr),
    _setConditionCharacteristic(nullptr),
    _setPeriodicCharacteristic(nullptr),
    _setConstantCharacteristic(nullptr),
    _setRampCharacteristic(nullptr),
    _setCustomForceCharacteristic(nullptr),
    _downloadForceCharacteristic(nullptr),
    _effectOperationCharacteristic(nullptr),
    _pidDeviceControlCharacteristic(nullptr),
    _deviceGainCharacteristic(nullptr),
    _pidState(nullptr),
    _createNewEffect(nullptr),
    _pidBlockLoad(nullptr),
    _pidPool(nullptr)
{
    this->resetButtons();
}

GamepadDevice::GamepadDevice(const GamepadConfiguration& config):
    _config(config), 
    _buttons(),
    _specialButtons(),
    _x(0),
    _y(0),
    _z(0),
    _rZ(0),
    _rX(0),
    _rY(0),
    _slider1(0),
    _slider2(0),
    _rudder(0),
    _throttle(0),
    _accelerator(0),
    _brake(0),
    _steering(0),
    _hat1(0),
    _hat2(0),
    _hat3(0),
    _hat4(0),
    _callbacks(nullptr),
    _setEffectCharacteristic(nullptr),
    _setEnvelopeCharacteristic(nullptr),
    _setConditionCharacteristic(nullptr),
    _setPeriodicCharacteristic(nullptr),
    _setConstantCharacteristic(nullptr),
    _setRampCharacteristic(nullptr),
    _setCustomForceCharacteristic(nullptr),
    _downloadForceCharacteristic(nullptr),
    _effectOperationCharacteristic(nullptr),
    _pidDeviceControlCharacteristic(nullptr),
    _deviceGainCharacteristic(nullptr),
    _pidState(nullptr),
    _createNewEffect(nullptr),
    _pidBlockLoad(nullptr),
    _pidPool(nullptr)
{
    this->resetButtons();
}

GamepadDevice::~GamepadDevice()
{
    if (getOutput() && _callbacks){
        getOutput()->setCallbacks(nullptr);
        delete _callbacks;
    }
}

void GamepadDevice::init(NimBLEHIDDevice* hid)
{
    // Create input characteristic to send events to the computer
    auto input = hid->inputReport(_config.getReportId());

    // Create output characteristic to handle events coming from the computer
    auto output = hid->outputReport(_config.getReportId());

    // Set callbacks
    _callbacks = new GamepadCallbacks(this);
    output->setCallbacks(_callbacks);

    setCharacteristics(input, output);
}

const BaseCompositeDeviceConfiguration* GamepadDevice::getDeviceConfig() const
{
    return &_config;
}

void GamepadDevice::resetButtons()
{
    std::lock_guard<std::mutex> lock(_mutex);
    memset(&_buttons, 0, sizeof(_buttons));
}

void GamepadDevice::setAxes(int16_t x, int16_t y, int16_t z, int16_t rZ, int16_t rX, int16_t rY, int16_t slider1, int16_t slider2)
{
    if (x == -32768)
    {
        x = -32767;
    }
    if (y == -32768)
    {
        y = -32767;
    }
    if (z == -32768)
    {
        z = -32767;
    }
    if (rZ == -32768)
    {
        rZ = -32767;
    }
    if (rX == -32768)
    {
        rX = -32767;
    }
    if (rY == -32768)
    {
        rY = -32767;
    }
    if (slider1 == -32768)
    {
        slider1 = -32767;
    }
    if (slider2 == -32768)
    {
        slider2 = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _x = x;
        _y = y;
        _z = z;
        _rZ = rZ;
        _rX = rX;
        _rY = rY;
        _slider1 = slider1;
        _slider2 = slider2;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setSimulationControls(int16_t rudder, int16_t throttle, int16_t accelerator, int16_t brake, int16_t steering)
{
    if (rudder == -32768)
    {
        rudder = -32767;
    }
    if (throttle == -32768)
    {
        throttle = -32767;
    }
    if (accelerator == -32768)
    {
        accelerator = -32767;
    }
    if (brake == -32768)
    {
        brake = -32767;
    }
    if (steering == -32768)
    {
        steering = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _rudder = rudder;
        _throttle = throttle;
        _accelerator = accelerator;
        _brake = brake;
        _steering = steering;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setHats(signed char hat1, signed char hat2, signed char hat3, signed char hat4)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _hat1 = hat1;
        _hat2 = hat2;
        _hat3 = hat3;
        _hat4 = hat4;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setSliders(int16_t slider1, int16_t slider2)
{
    if (slider1 == -32768)
    {
        slider1 = -32767;
    }
    if (slider2 == -32768)
    {
        slider2 = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _slider1 = slider1;
        _slider2 = slider2;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::press(uint8_t b)
{
    uint8_t index = (b - 1) / 8;
    uint8_t bit = (b - 1) % 8;
    uint8_t bitmask = (1 << bit);

    uint8_t result = _buttons[index] | bitmask;

    if (result != _buttons[index])
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _buttons[index] = result;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::release(uint8_t b)
{
    uint8_t index = (b - 1) / 8;
    uint8_t bit = (b - 1) % 8;
    uint8_t bitmask = (1 << bit);

    uint64_t result = _buttons[index] & ~bitmask;

    if (result != _buttons[index])
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _buttons[index] = result;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

uint8_t GamepadDevice::specialButtonBitPosition(uint8_t b)
{
    if (b >= POSSIBLESPECIALBUTTONS)
        throw std::invalid_argument("Index out of range");
    uint8_t bit = 0;
    for (int i = 0; i < b; i++)
    {
        if (_config.getWhichSpecialButtons()[i])
            bit++;
    }
    return bit;
}

void GamepadDevice::pressSpecialButton(uint8_t b)
{
    uint8_t button = specialButtonBitPosition(b);
    uint8_t bit = button % 8;
    uint8_t bitmask = (1 << bit);

    uint64_t result = _specialButtons | bitmask;

    if (result != _specialButtons)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _specialButtons = result;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::releaseSpecialButton(uint8_t b)
{
    uint8_t button = specialButtonBitPosition(b);
    uint8_t bit = button % 8;
    uint8_t bitmask = (1 << bit);

    uint64_t result = _specialButtons & ~bitmask;

    if (result != _specialButtons)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _specialButtons = result;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::pressStart()
{
    pressSpecialButton(START_BUTTON);
}

void GamepadDevice::releaseStart()
{
    releaseSpecialButton(START_BUTTON);
}

void GamepadDevice::pressSelect()
{
    pressSpecialButton(SELECT_BUTTON);
}

void GamepadDevice::releaseSelect()
{
    releaseSpecialButton(SELECT_BUTTON);
}

void GamepadDevice::pressMenu()
{
    pressSpecialButton(MENU_BUTTON);
}

void GamepadDevice::releaseMenu()
{
    releaseSpecialButton(MENU_BUTTON);
}

void GamepadDevice::pressHome()
{
    pressSpecialButton(HOME_BUTTON);
}

void GamepadDevice::releaseHome()
{
    releaseSpecialButton(HOME_BUTTON);
}

void GamepadDevice::pressBack()
{
    pressSpecialButton(BACK_BUTTON);
}

void GamepadDevice::releaseBack()
{
    releaseSpecialButton(BACK_BUTTON);
}

void GamepadDevice::pressVolumeInc()
{
    pressSpecialButton(VOLUME_INC_BUTTON);
}

void GamepadDevice::releaseVolumeInc()
{
    releaseSpecialButton(VOLUME_INC_BUTTON);
}

void GamepadDevice::pressVolumeDec()
{
    pressSpecialButton(VOLUME_DEC_BUTTON);
}

void GamepadDevice::releaseVolumeDec()
{
    releaseSpecialButton(VOLUME_DEC_BUTTON);
}

void GamepadDevice::pressVolumeMute()
{
    pressSpecialButton(VOLUME_MUTE_BUTTON);
}

void GamepadDevice::releaseVolumeMute()
{
    releaseSpecialButton(VOLUME_MUTE_BUTTON);
}

void GamepadDevice::setLeftThumb(int16_t x, int16_t y)
{
    if (x == -32768)
    {
        x = -32767;
    }
    if (y == -32768)
    {
        y = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _x = x;
        _y = y;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setRightThumb(int16_t z, int16_t rZ)
{
    if (z == -32768)
    {
        z = -32767;
    }
    if (rZ == -32768)
    {
        rZ = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _z = z;
        _rZ = rZ;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setLeftTrigger(int16_t rX)
{
    if (rX == -32768)
    {
        rX = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _rX = rX;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setRightTrigger(int16_t rY)
{
    if (rY == -32768)
    {
        rY = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _rY = rY;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setTriggers(int16_t rX, int16_t rY)
{
    if (rX == -32768)
    {
        rX = -32767;
    }
    if (rY == -32768)
    {
        rY = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _rX = rX;
        _rY = rY;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setHat(signed char hat)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _hat1 = hat;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setHat1(signed char hat1)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _hat1 = hat1;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setHat2(signed char hat2)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _hat2 = hat2;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setHat3(signed char hat3)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _hat3 = hat3;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setHat4(signed char hat4)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _hat4 = hat4;
    }


    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setX(int16_t x)
{
    if (x == -32768)
    {
        x = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _x = x;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setY(int16_t y)
{
    if (y == -32768)
    {
        y = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _y = y;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setZ(int16_t z)
{
    if (z == -32768)
    {
        z = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _z = z;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setRZ(int16_t rZ)
{
    if (rZ == -32768)
    {
        rZ = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _rZ = rZ;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setRX(int16_t rX)
{
    if (rX == -32768)
    {
        rX = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _rX = rX;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setRY(int16_t rY)
{
    if (rY == -32768)
    {
        rY = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _rY = rY;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setSlider(int16_t slider)
{
    if (slider == -32768)
    {
        slider = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _slider1 = slider;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setSlider1(int16_t slider1)
{
    if (slider1 == -32768)
    {
        slider1 = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _slider1 = slider1;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setSlider2(int16_t slider2)
{
    if (slider2 == -32768)
    {
        slider2 = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _slider2 = slider2;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setRudder(int16_t rudder)
{
    if (rudder == -32768)
    {
        rudder = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _rudder = rudder;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setThrottle(int16_t throttle)
{
    if (throttle == -32768)
    {
        throttle = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _throttle = throttle;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setAccelerator(int16_t accelerator)
{
    if (accelerator == -32768)
    {
        accelerator = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _accelerator = accelerator;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setBrake(int16_t brake)
{
    if (brake == -32768)
    {
        brake = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _brake = brake;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

void GamepadDevice::setSteering(int16_t steering)
{
    if (steering == -32768)
    {
        steering = -32767;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _steering = steering;
    }

    if (_config.getAutoReport())
    {
        sendGamepadReport();
    }
}

bool GamepadDevice::isPressed(uint8_t b)
{
    uint8_t index = (b - 1) / 8;
    uint8_t bit = (b - 1) % 8;
    uint8_t bitmask = (1 << bit);

    if ((bitmask & _buttons[index]) > 0)
        return true;
    return false;
}

void GamepadDevice::sendGamepadReport(bool defer)
{
    if(defer || _config.getAutoReport()){
        queueDeferredReport(std::bind(&GamepadDevice::sendGamepadReportImp, this));
    } else {
        sendGamepadReportImp();
    }
}

void GamepadDevice::sendGamepadReportImp()
{
    auto input = getInput();
    auto parentDevice = this->getParent();

    if (!input || !parentDevice)
        return;

    if(!parentDevice->isConnected())
        return;

    uint8_t currentReportIndex = 0;
    uint8_t m[_config.getDeviceReportSize()];

    {
        // Lock the device input data
        std::lock_guard<std::mutex> lock(_mutex);

        memset(&m, 0, sizeof(m));
        memcpy(&m, &_buttons, sizeof(_buttons));
        
        currentReportIndex += _config.getButtonNumBytes();

        if (_config.getTotalSpecialButtonCount() > 0)
        {
            m[currentReportIndex++] = _specialButtons;
        }

        if (_config.getIncludeXAxis())
        {
            m[currentReportIndex++] = _x;
            m[currentReportIndex++] = (_x >> 8);
        }
        if (_config.getIncludeYAxis())
        {
            m[currentReportIndex++] = _y;
            m[currentReportIndex++] = (_y >> 8);
        }
        if (_config.getIncludeZAxis())
        {
            m[currentReportIndex++] = _z;
            m[currentReportIndex++] = (_z >> 8);
        }
        if (_config.getIncludeRzAxis())
        {
            m[currentReportIndex++] = _rZ;
            m[currentReportIndex++] = (_rZ >> 8);
        }
        if (_config.getIncludeRxAxis())
        {
            m[currentReportIndex++] = _rX;
            m[currentReportIndex++] = (_rX >> 8);
        }
        if (_config.getIncludeRyAxis())
        {
            m[currentReportIndex++] = _rY;
            m[currentReportIndex++] = (_rY >> 8);
        }

        if (_config.getIncludeSlider1())
        {
            m[currentReportIndex++] = _slider1;
            m[currentReportIndex++] = (_slider1 >> 8);
        }
        if (_config.getIncludeSlider2())
        {
            m[currentReportIndex++] = _slider2;
            m[currentReportIndex++] = (_slider2 >> 8);
        }

        if (_config.getIncludeRudder())
        {
            m[currentReportIndex++] = _rudder;
            m[currentReportIndex++] = (_rudder >> 8);
        }
        if (_config.getIncludeThrottle())
        {
            m[currentReportIndex++] = _throttle;
            m[currentReportIndex++] = (_throttle >> 8);
        }
        if (_config.getIncludeAccelerator())
        {
            m[currentReportIndex++] = _accelerator;
            m[currentReportIndex++] = (_accelerator >> 8);
        }
        if (_config.getIncludeBrake())
        {
            m[currentReportIndex++] = _brake;
            m[currentReportIndex++] = (_brake >> 8);
        }
        if (_config.getIncludeSteering())
        {
            m[currentReportIndex++] = _steering;
            m[currentReportIndex++] = (_steering >> 8);
        }

        if (_config.getHatSwitchCount() > 0)
        {
            signed char hats[4];

            hats[0] = _hat1;
            hats[1] = _hat2;
            hats[2] = _hat3;
            hats[3] = _hat4;

            for (int currentHatIndex = _config.getHatSwitchCount() - 1; currentHatIndex >= 0; currentHatIndex--)
            {
                m[currentReportIndex++] = hats[currentHatIndex];
            }
        }
    }

    // Notify
    input->setValue(m, sizeof(m));
    input->notify();
}
