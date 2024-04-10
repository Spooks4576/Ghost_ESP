#ifndef ESP32_GAMEPAD_DEVICE_H
#define ESP32_GAMEPAD_DEVICE_H

#include <NimBLECharacteristic.h>
#include <GamepadConfiguration.h>
#include <BaseCompositeDevice.h>
#include <mutex>

// Forwards
class GamepadDevice;


class GamepadCallbacks : public NimBLECharacteristicCallbacks
{
public:
    GamepadCallbacks(GamepadDevice* device);

    void onWrite(NimBLECharacteristic* pCharacteristic) override;
    void onRead(NimBLECharacteristic* pCharacteristic) override;
    void onNotify(NimBLECharacteristic* pCharacteristic) override;
    void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) override;

private:
    GamepadDevice* _device;
};


class GamepadDevice : public BaseCompositeDevice
{
    friend class GamepadCallbacks;
private:
    GamepadConfiguration _config;

    // Gamepad
    uint8_t _buttons[16]; // 8 bits x 16 --> 128 bits
    uint8_t _specialButtons;
    int16_t _x;
    int16_t _y;
    int16_t _z;
    int16_t _rZ;
    int16_t _rX;
    int16_t _rY;
    int16_t _slider1;
    int16_t _slider2;
    int16_t _rudder;
    int16_t _throttle;
    int16_t _accelerator;
    int16_t _brake;
    int16_t _steering;
    int16_t _hat1;
    int16_t _hat2;
    int16_t _hat3;
    int16_t _hat4;

    GamepadCallbacks* _callbacks;

public:
    GamepadDevice();
    GamepadDevice(const GamepadConfiguration& config);
    ~GamepadDevice();

    void init(NimBLEHIDDevice* hid) override;
    const BaseCompositeDeviceConfiguration* getDeviceConfig() const override;

    void setAxes(int16_t x = 0, int16_t y = 0, int16_t z = 0, int16_t rZ = 0, int16_t rX = 0, int16_t rY = 0, int16_t slider1 = 0, int16_t slider2 = 0);
    void press(uint8_t b = BUTTON_1);   // press BUTTON_1 by default
    void release(uint8_t b = BUTTON_1); // release BUTTON_1 by default
    void pressSpecialButton(uint8_t b);
    void releaseSpecialButton(uint8_t b);
    void pressStart();
    void releaseStart();
    void pressSelect();
    void releaseSelect();
    void pressMenu();
    void releaseMenu();
    void pressHome();
    void releaseHome();
    void pressBack();
    void releaseBack();
    void pressVolumeInc();
    void releaseVolumeInc();
    void pressVolumeDec();
    void releaseVolumeDec();
    void pressVolumeMute();
    void releaseVolumeMute();
    void setLeftThumb(int16_t x = 0, int16_t y = 0);
    void setRightThumb(int16_t z = 0, int16_t rZ = 0);
    void setLeftTrigger(int16_t rX = 0);
    void setRightTrigger(int16_t rY = 0);
    void setTriggers(int16_t rX = 0, int16_t rY = 0);
    void setHats(signed char hat1 = 0, signed char hat2 = 0, signed char hat3 = 0, signed char hat4 = 0);
    void setHat(signed char hat = 0);
    void setHat1(signed char hat1 = 0);
    void setHat2(signed char hat2 = 0);
    void setHat3(signed char hat3 = 0);
    void setHat4(signed char hat4 = 0);
    void setX(int16_t x = 0);
    void setY(int16_t y = 0);
    void setZ(int16_t z = 0);
    void setRZ(int16_t rZ = 0);
    void setRX(int16_t rX = 0);
    void setRY(int16_t rY = 0);
    void setSliders(int16_t slider1 = 0, int16_t slider2 = 0);
    void setSlider(int16_t slider = 0);
    void setSlider1(int16_t slider1 = 0);
    void setSlider2(int16_t slider2 = 0);
    void setRudder(int16_t rudder = 0);
    void setThrottle(int16_t throttle = 0);
    void setAccelerator(int16_t accelerator = 0);
    void setBrake(int16_t brake = 0);
    void setSteering(int16_t steering = 0);
    void setSimulationControls(int16_t rudder = 0, int16_t throttle = 0, int16_t accelerator = 0, int16_t brake = 0, int16_t steering = 0);
    
    bool isPressed(uint8_t b = BUTTON_1); // check BUTTON_1 by default
    bool isConnected(void);
    void resetButtons();

    void sendGamepadReport(bool defer = false);
private:
    void sendGamepadReportImp();


    // Output properties
    uint8_t getPlayerIndicator();


private:
    uint8_t specialButtonBitPosition(uint8_t specialButton);

    // Output properties
    uint8_t playerIndicator;

    // Threaded access
    std::mutex _mutex;

    NimBLECharacteristic* _setEffectCharacteristic;
    NimBLECharacteristic* _setEnvelopeCharacteristic;
    NimBLECharacteristic* _setConditionCharacteristic;
    NimBLECharacteristic* _setPeriodicCharacteristic;
    NimBLECharacteristic* _setConstantCharacteristic;
    NimBLECharacteristic* _setRampCharacteristic;
    NimBLECharacteristic* _setCustomForceCharacteristic;
    NimBLECharacteristic* _downloadForceCharacteristic;
    NimBLECharacteristic* _effectOperationCharacteristic;
    NimBLECharacteristic* _pidDeviceControlCharacteristic;
    NimBLECharacteristic* _deviceGainCharacteristic;
    NimBLECharacteristic* _customForceCharacteristic;
    NimBLECharacteristic* _pidState;
    NimBLECharacteristic* _createNewEffect;
    NimBLECharacteristic* _pidBlockLoad;
    NimBLECharacteristic* _pidPool;
};

#endif