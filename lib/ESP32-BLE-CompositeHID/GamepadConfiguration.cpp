#include "GamepadConfiguration.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "GamepadConfiguration"
#else
#include "esp_log.h"
static const char *LOG_TAG = "GamepadConfiguration";
#endif

GamepadConfiguration::GamepadConfiguration() : 
    BaseCompositeDeviceConfiguration(GAMEPAD_REPORT_ID),
    _controllerType(CONTROLLER_TYPE_GAMEPAD),
    _buttonCount(16),
    _hatSwitchCount(1),
    _whichSpecialButtons{false, false, false, false, false, false, false, false},
    _whichAxes{true, true, true, true, true, true, true, true},
    _whichSimulationControls{false, false, false, false, false},
    _axesMin(0x0000),
    _axesMax(0x7FFF),
    _simulationMin(0x0000),
    _simulationMax(0x7FFF),
    _includeRumble(false),
    _includePlayerIndicators(false)
{
}

const char* GamepadConfiguration::getDeviceName() const 
{
    return GAMEPAD_DEVICE_NAME;
}

uint8_t GamepadConfiguration::getDeviceReportSize() const
{
    uint8_t numOfButtonBytes = getButtonNumBytes();
    uint8_t numOfSpecialButtonBytes = getSpecialButtonNumBytes();
    uint8_t numOfAxisBytes = this->getAxisCount() * 2;
    uint8_t numOfSimulationBytes = this->getSimulationCount() * 2;

    uint8_t reportSize = numOfButtonBytes + numOfSpecialButtonBytes + numOfAxisBytes + numOfSimulationBytes + this->getHatSwitchCount();
    return reportSize;
}

size_t GamepadConfiguration::makeDeviceReport(uint8_t* buffer, size_t bufferSize) const
{
    // Report description START -------------------------------------------------

    uint8_t tempHidReportDescriptor[BLE_ATT_ATTR_MAX_LEN];
    size_t reportSize = 0;

    // USAGE_PAGE (Generic Desktop)
    tempHidReportDescriptor[reportSize++] = USAGE_PAGE(1); //0x05;
    tempHidReportDescriptor[reportSize++] = 0x01; //Generic Desktop

    // USAGE (Joystick - 0x04; Gamepad - 0x05; Multi-axis Controller - 0x08)
    tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
    tempHidReportDescriptor[reportSize++] = this->getControllerType();

    // COLLECTION (Application)
    tempHidReportDescriptor[reportSize++] = COLLECTION(1); //0xa1;
    tempHidReportDescriptor[reportSize++] = 0x01;
    {
        // REPORT_ID (Gamepad)
        tempHidReportDescriptor[reportSize++] = REPORT_ID(1);
        tempHidReportDescriptor[reportSize++] = this->getReportId();

        if (this->getButtonCount() > 0)
        {
            // USAGE_PAGE (Button)
            tempHidReportDescriptor[reportSize++] = USAGE_PAGE(1); //0x05;
            tempHidReportDescriptor[reportSize++] = 0x09;

            // LOGICAL_MINIMUM (0)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MINIMUM(1);//0x15;
            tempHidReportDescriptor[reportSize++] = 0x00;

            // LOGICAL_MAXIMUM (1)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MAXIMUM(1); //0x25;
            tempHidReportDescriptor[reportSize++] = 0x01;

            // REPORT_SIZE (1)
            tempHidReportDescriptor[reportSize++] = REPORT_SIZE(1); //0x75;
            tempHidReportDescriptor[reportSize++] = 0x01;

            // USAGE_MINIMUM (Button 1)
            tempHidReportDescriptor[reportSize++] = USAGE_MINIMUM(1);//0x19;
            tempHidReportDescriptor[reportSize++] = 0x01;

            // USAGE_MAXIMUM (Up to 128 buttons possible)
            tempHidReportDescriptor[reportSize++] = USAGE_MAXIMUM(1);//0x29;
            tempHidReportDescriptor[reportSize++] = this->getButtonCount();

            // REPORT_COUNT (# of buttons)
            tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); //0x95;
            tempHidReportDescriptor[reportSize++] = this->getButtonCount();

            // INPUT (Data,Var,Abs)
            tempHidReportDescriptor[reportSize++] = HIDINPUT(1); //0x81;
            tempHidReportDescriptor[reportSize++] = 0x02;

            uint8_t buttonPaddingBits = getButtonNumPaddingBits();
            if (buttonPaddingBits > 0)
            {
                // REPORT_SIZE (1)
                tempHidReportDescriptor[reportSize++] = REPORT_SIZE(1); //0x75;
                tempHidReportDescriptor[reportSize++] = 0x01;

                // REPORT_COUNT (# of padding bits)
                tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); //0x95;
                tempHidReportDescriptor[reportSize++] = buttonPaddingBits;

                // INPUT (Const,Var,Abs)
                tempHidReportDescriptor[reportSize++] = HIDINPUT(1); //0x81;
                tempHidReportDescriptor[reportSize++] = 0x03;

            } // Padding Bits Needed

        } // Buttons

        if (this->getTotalSpecialButtonCount() > 0)
        {
            // LOGICAL_MINIMUM (0)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MINIMUM(1); //0x15;
            tempHidReportDescriptor[reportSize++] = 0x00;

            // LOGICAL_MAXIMUM (1)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MAXIMUM(1); //;0x25;
            tempHidReportDescriptor[reportSize++] = 0x01;

            // REPORT_SIZE (1)
            tempHidReportDescriptor[reportSize++] = REPORT_SIZE(1); //0x75;
            tempHidReportDescriptor[reportSize++] = 0x01;

            if (this->getDesktopSpecialButtonCount() > 0)
            {
                // USAGE_PAGE (Generic Desktop)
                tempHidReportDescriptor[reportSize++] = USAGE_PAGE(1); // 0x05;
                tempHidReportDescriptor[reportSize++] = 0x01;

                // REPORT_COUNT
                tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); //0x95;
                tempHidReportDescriptor[reportSize++] = this->getDesktopSpecialButtonCount();
                if (this->getIncludeStart())
                {
                    // USAGE (Start)
                    tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                    tempHidReportDescriptor[reportSize++] = 0x3D;
                }

                if (this->getIncludeSelect())
                {
                    // USAGE (Select)
                    tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                    tempHidReportDescriptor[reportSize++] = 0x3E;
                }

                if (this->getIncludeMenu())
                {
                    // USAGE (App Menu)
                    tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                    tempHidReportDescriptor[reportSize++] = 0x86;
                }

                // INPUT (Data,Var,Abs)
                tempHidReportDescriptor[reportSize++] = HIDINPUT(1); //0x81;
                tempHidReportDescriptor[reportSize++] = 0x02;
            }

            if (this->getConsumerSpecialButtonCount() > 0)
            {
                // USAGE_PAGE (Consumer Page)
                tempHidReportDescriptor[reportSize++] = USAGE_PAGE(1); //0x05;
                tempHidReportDescriptor[reportSize++] = 0x0C;

                // REPORT_COUNT
                tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); //0x95;
                tempHidReportDescriptor[reportSize++] = this->getConsumerSpecialButtonCount();

                if (this->getIncludeHome())
                {
                    // USAGE (Home)
                    tempHidReportDescriptor[reportSize++] = USAGE(2); //0x0A;
                    tempHidReportDescriptor[reportSize++] = 0x23;
                    tempHidReportDescriptor[reportSize++] = 0x02;
                }

                if (this->getIncludeBack())
                {
                    // USAGE (Back)
                    tempHidReportDescriptor[reportSize++] = USAGE(2); //0x0A;
                    tempHidReportDescriptor[reportSize++] = 0x24;
                    tempHidReportDescriptor[reportSize++] = 0x02;
                }

                if (this->getIncludeVolumeInc())
                {
                    // USAGE (Volume Increment)
                    tempHidReportDescriptor[reportSize++] =  USAGE(1); //0x09;
                    tempHidReportDescriptor[reportSize++] = 0xE9;
                }

                if (this->getIncludeVolumeDec())
                {
                    // USAGE (Volume Decrement)
                    tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                    tempHidReportDescriptor[reportSize++] = 0xEA;
                }

                if (this->getIncludeVolumeMute())
                {
                    // USAGE (Mute)
                    tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                    tempHidReportDescriptor[reportSize++] = 0xE2;
                }

                // INPUT (Data,Var,Abs)
                tempHidReportDescriptor[reportSize++] = HIDINPUT(1); //0x81;
                tempHidReportDescriptor[reportSize++] = 0x02;
            }

            uint8_t specialButtonPaddingBits = getSpecialButtonNumPaddingBits();
            if (specialButtonPaddingBits > 0)
            {
                // REPORT_SIZE (1)
                tempHidReportDescriptor[reportSize++] = REPORT_SIZE(1); //0x75;
                tempHidReportDescriptor[reportSize++] = 0x01;

                // REPORT_COUNT (# of padding bits)
                tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); //0x95;
                tempHidReportDescriptor[reportSize++] = specialButtonPaddingBits;

                // INPUT (Const,Var,Abs)
                tempHidReportDescriptor[reportSize++] = HIDINPUT(1); //0x81;
                tempHidReportDescriptor[reportSize++] = 0x03;

            } // Padding Bits Needed

        } // Special Buttons

        if (this->getAxisCount() > 0)
        {
            // USAGE_PAGE (Generic Desktop)
            tempHidReportDescriptor[reportSize++] = USAGE_PAGE(1); 0x05;
            tempHidReportDescriptor[reportSize++] = 0x01; // Generic desktop controls

            // USAGE (Pointer)
            tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
            tempHidReportDescriptor[reportSize++] = 0x01; 

            // LOGICAL_MINIMUM (-32767)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MINIMUM(2); //0x16;
            tempHidReportDescriptor[reportSize++] = lowByte(this->getAxesMin());
            tempHidReportDescriptor[reportSize++] = highByte(this->getAxesMin());
            //tempHidReportDescriptor[reportSize++] = 0x00;		// Use these two lines for 0 min
            //tempHidReportDescriptor[reportSize++] = 0x00;
                //tempHidReportDescriptor[reportSize++] = 0x01;	// Use these two lines for -32767 min
            //tempHidReportDescriptor[reportSize++] = 0x80;

            // LOGICAL_MAXIMUM (+32767)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MAXIMUM(2);//0x26;
            tempHidReportDescriptor[reportSize++] = lowByte(this->getAxesMax());
            tempHidReportDescriptor[reportSize++] = highByte(this->getAxesMax());
            //tempHidReportDescriptor[reportSize++] = 0xFF;	// Use these two lines for 255 max
            //tempHidReportDescriptor[reportSize++] = 0x00;
                //tempHidReportDescriptor[reportSize++] = 0xFF;	// Use these two lines for +32767 max
            //tempHidReportDescriptor[reportSize++] = 0x7F;

            // REPORT_SIZE (16)
            tempHidReportDescriptor[reportSize++] = REPORT_SIZE(1); //0x75;
            tempHidReportDescriptor[reportSize++] = 0x10;

            // REPORT_COUNT (this->getAxisCount())
            tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); //0x95;
            tempHidReportDescriptor[reportSize++] = this->getAxisCount();

            // COLLECTION (Physical)
            tempHidReportDescriptor[reportSize++] = COLLECTION(1); //0xA1;
            tempHidReportDescriptor[reportSize++] = 0x00;

            if (this->getIncludeXAxis())
            {
                // USAGE (X)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0x30;
            }

            if (this->getIncludeYAxis())
            {
                // USAGE (Y)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0x31;
            }

            if (this->getIncludeZAxis())
            {
                // USAGE (Z)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0x32;
            }

            if (this->getIncludeRzAxis())
            {
                // USAGE (Rz)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0x35;
            }

            if (this->getIncludeRxAxis())
            {
                // USAGE (Rx)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0x33;
            }

            if (this->getIncludeRyAxis())
            {
                // USAGE (Ry)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0x34;
            }

            if (this->getIncludeSlider1())
            {
                // USAGE (Slider)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0x36;
            }

            if (this->getIncludeSlider2())
            {
                // USAGE (Slider)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0x36;
            }

            // INPUT (Data,Var,Abs)
            tempHidReportDescriptor[reportSize++] = HIDINPUT(1); //0x81;
            tempHidReportDescriptor[reportSize++] = 0x02;

            // END_COLLECTION (Physical)
            tempHidReportDescriptor[reportSize++] = END_COLLECTION(0); //0xc0;

        } // X, Y, Z, Rx, Ry, and Rz Axis

        if (this->getSimulationCount() > 0)
        {
            // USAGE_PAGE (Simulation Controls)
            tempHidReportDescriptor[reportSize++] = USAGE_PAGE(1); //0x05;
            tempHidReportDescriptor[reportSize++] = 0x02;

            // LOGICAL_MINIMUM (-32767)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MINIMUM(2); //0x16;
            tempHidReportDescriptor[reportSize++] = lowByte(this->getSimulationMin());
            tempHidReportDescriptor[reportSize++] = highByte(this->getSimulationMin());
            //tempHidReportDescriptor[reportSize++] = 0x00;		// Use these two lines for 0 min
            //tempHidReportDescriptor[reportSize++] = 0x00;
            //tempHidReportDescriptor[reportSize++] = 0x01;	    // Use these two lines for -32767 min
            //tempHidReportDescriptor[reportSize++] = 0x80;

            // LOGICAL_MAXIMUM (+32767)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MAXIMUM(2); //0x26;
            tempHidReportDescriptor[reportSize++] = lowByte(this->getSimulationMax());
            tempHidReportDescriptor[reportSize++] = highByte(this->getSimulationMax());
            //tempHidReportDescriptor[reportSize++] = 0xFF;	    // Use these two lines for 255 max
            //tempHidReportDescriptor[reportSize++] = 0x00;
            //tempHidReportDescriptor[reportSize++] = 0xFF;		// Use these two lines for +32767 max
            //tempHidReportDescriptor[reportSize++] = 0x7F;

            // REPORT_SIZE (16)
            tempHidReportDescriptor[reportSize++] = REPORT_SIZE(1); //0x75;
            tempHidReportDescriptor[reportSize++] = 0x10;

            // REPORT_COUNT (this->getSimulationCount())
            tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); //0x95;
            tempHidReportDescriptor[reportSize++] = this->getSimulationCount();

            // COLLECTION (Physical)
            tempHidReportDescriptor[reportSize++] = COLLECTION(1); //0xA1;
            tempHidReportDescriptor[reportSize++] = 0x00;

            if (this->getIncludeRudder())
            {
                // USAGE (Rudder)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0xBA;
            }

            if (this->getIncludeThrottle())
            {
                // USAGE (Throttle)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0xBB;
            }

            if (this->getIncludeAccelerator())
            {
                // USAGE (Accelerator)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0xC4;
            }

            if (this->getIncludeBrake())
            {
                // USAGE (Brake)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0xC5;
            }

            if (this->getIncludeSteering())
            {
                // USAGE (Steering)
                tempHidReportDescriptor[reportSize++] = USAGE(1); //0x09;
                tempHidReportDescriptor[reportSize++] = 0xC8;
            }

            // INPUT (Data,Var,Abs)
            tempHidReportDescriptor[reportSize++] = HIDINPUT(1); 0x81;
            tempHidReportDescriptor[reportSize++] = 0x02;

            // END_COLLECTION (Physical)
            tempHidReportDescriptor[reportSize++] = END_COLLECTION(0); //0xc0;

        } // Simulation Controls

        if (this->getHatSwitchCount() > 0)
        {

            // COLLECTION (Physical)
            tempHidReportDescriptor[reportSize++] = COLLECTION(1); //0xA1;
            tempHidReportDescriptor[reportSize++] = 0x00;

            // USAGE_PAGE (Generic Desktop)
            tempHidReportDescriptor[reportSize++] = USAGE_PAGE(1);
            tempHidReportDescriptor[reportSize++] = 0x01;

            // USAGE (Hat Switch)
            for (int currentHatIndex = 0; currentHatIndex < this->getHatSwitchCount(); currentHatIndex++)
            {
                tempHidReportDescriptor[reportSize++] = USAGE(1);
                tempHidReportDescriptor[reportSize++] = 0x39;
            }

            // Logical Min (1)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MINIMUM(1); //0x15;
            tempHidReportDescriptor[reportSize++] = 0x01;

            // Logical Max (8)
            tempHidReportDescriptor[reportSize++] = LOGICAL_MAXIMUM(1); //0x25;
            tempHidReportDescriptor[reportSize++] = 0x08;

            // Physical Min (0)
            tempHidReportDescriptor[reportSize++] = PHYSICAL_MINIMUM(1); //0x35;
            tempHidReportDescriptor[reportSize++] = 0x00;

            // Physical Max (315)
            tempHidReportDescriptor[reportSize++] = PHYSICAL_MAXIMUM(2); //0x46;
            tempHidReportDescriptor[reportSize++] = 0x3B;
            tempHidReportDescriptor[reportSize++] = 0x01;

            // Unit (SI Rot : Ang Pos)
            tempHidReportDescriptor[reportSize++] = UNIT(1); //0x65;
            tempHidReportDescriptor[reportSize++] = 0x12;

            // Report Size (8)
            tempHidReportDescriptor[reportSize++] = REPORT_SIZE(1); //0x75;
            tempHidReportDescriptor[reportSize++] = 0x08;

            // Report Count (4)
            tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); //0x95;
            tempHidReportDescriptor[reportSize++] = this->getHatSwitchCount();

            // Input (Data, Variable, Absolute)
            tempHidReportDescriptor[reportSize++] = HIDINPUT(1); //0x81;
            tempHidReportDescriptor[reportSize++] = 0x42;

            // END_COLLECTION (Physical)
            tempHidReportDescriptor[reportSize++] = END_COLLECTION(0); //0xc0;
        }

        if(this->getIncludeRumble()){
            ESP_LOGI(LOG_TAG, "Start offset: %d, Bytes to copy: %d, Final size: %d", reportSize, sizeof(pidReportDescriptor), reportSize + sizeof(pidReportDescriptor));
            memcpy(&tempHidReportDescriptor[reportSize], &pidReportDescriptor[0], sizeof(pidReportDescriptor));
            reportSize += sizeof(pidReportDescriptor) / sizeof(pidReportDescriptor[0]);
        }

        if(this->getIncludePlayerIndicators()){
            tempHidReportDescriptor[reportSize++] = COLLECTION(1); // Physical collection
            tempHidReportDescriptor[reportSize++] = 0x00;

            tempHidReportDescriptor[reportSize++] = REPORT_ID(1); // Report ID
            tempHidReportDescriptor[reportSize++] = this->getReportId();

            tempHidReportDescriptor[reportSize++] = USAGE_PAGE(1); // Usage page - LED usage page
            tempHidReportDescriptor[reportSize++] = 0x08;

            tempHidReportDescriptor[reportSize++] = USAGE_MINIMUM(1); // Usage minimum
            tempHidReportDescriptor[reportSize++] = 0x61;

            tempHidReportDescriptor[reportSize++] = USAGE_MAXIMUM(1); // Usage maximum
            tempHidReportDescriptor[reportSize++] = 0x68;

            tempHidReportDescriptor[reportSize++] = LOGICAL_MINIMUM(1); // Logical minimum
            tempHidReportDescriptor[reportSize++] = 0x00;

            tempHidReportDescriptor[reportSize++] = LOGICAL_MAXIMUM(1); // Logical maximum
            tempHidReportDescriptor[reportSize++] = 0x01;

            tempHidReportDescriptor[reportSize++] = REPORT_COUNT(1); // Report count - 8 bits
            tempHidReportDescriptor[reportSize++] = 0x08;

            tempHidReportDescriptor[reportSize++] = REPORT_SIZE(1);  // Report size
            tempHidReportDescriptor[reportSize++] = 0x01;

            tempHidReportDescriptor[reportSize++] = HIDOUTPUT(1); // Output
            tempHidReportDescriptor[reportSize++] = 0x02;

            tempHidReportDescriptor[reportSize++] = END_COLLECTION(0); // End physical collection
        }
    }
    
    // End gamepad collection
    tempHidReportDescriptor[reportSize++] = END_COLLECTION(0); //0xc0;
    
    if(reportSize < bufferSize){
        memcpy(buffer, tempHidReportDescriptor, reportSize);
    } else {
        return -1;
    }

    return reportSize;
}


uint8_t GamepadConfiguration::getTotalSpecialButtonCount() const
{
    int count = 0;
    for (int i = 0; i < POSSIBLESPECIALBUTTONS; i++)
    {
        count += (int)_whichSpecialButtons[i];
    }

    return count;
}

uint8_t GamepadConfiguration::getDesktopSpecialButtonCount() const
{
    int count = 0;
    for (int i = 0; i < 3; i++)
    {
        count += (int)_whichSpecialButtons[i];
    }

    return count;
}

uint8_t GamepadConfiguration::getConsumerSpecialButtonCount() const
{
    int count = 0;
    for (int i = 3; i < 8; i++)
    {
        count += (int)_whichSpecialButtons[i];
    }

    return count;
}

uint8_t GamepadConfiguration::getAxisCount() const
{
    int count = 0;
    for (int i = 0; i < POSSIBLEAXES; i++)
    {
        count += (int)_whichAxes[i];
    }

    return count;
}

uint8_t GamepadConfiguration::getSimulationCount() const
{
    int count = 0;
    for (int i = 0; i < POSSIBLESIMULATIONCONTROLS; i++)
    {
        count += (int)_whichSimulationControls[i];
    }

    return count;
}

uint16_t GamepadConfiguration::getButtonCount() const { return _buttonCount; }
uint8_t GamepadConfiguration::getHatSwitchCount() const { return _hatSwitchCount; }
int16_t GamepadConfiguration::getAxesMin() const { return _axesMin; }
int16_t GamepadConfiguration::getAxesMax() const { return _axesMax; }
int16_t GamepadConfiguration::getSimulationMin() const { return _simulationMin; }
int16_t GamepadConfiguration::getSimulationMax() const { return _simulationMax; }
uint8_t GamepadConfiguration::getControllerType() const { return _controllerType; }
bool GamepadConfiguration::getIncludeStart() const { return _whichSpecialButtons[START_BUTTON]; }
bool GamepadConfiguration::getIncludeSelect() const { return _whichSpecialButtons[SELECT_BUTTON]; }
bool GamepadConfiguration::getIncludeMenu() const { return _whichSpecialButtons[MENU_BUTTON]; }
bool GamepadConfiguration::getIncludeHome() const { return _whichSpecialButtons[HOME_BUTTON]; }
bool GamepadConfiguration::getIncludeBack() const { return _whichSpecialButtons[BACK_BUTTON]; }
bool GamepadConfiguration::getIncludeVolumeInc() const { return _whichSpecialButtons[VOLUME_INC_BUTTON]; }
bool GamepadConfiguration::getIncludeVolumeDec() const { return _whichSpecialButtons[VOLUME_DEC_BUTTON]; }
bool GamepadConfiguration::getIncludeVolumeMute() const { return _whichSpecialButtons[VOLUME_MUTE_BUTTON]; }
const bool *GamepadConfiguration::getWhichSpecialButtons() const { return _whichSpecialButtons; }
bool GamepadConfiguration::getIncludeXAxis() const { return _whichAxes[X_AXIS]; }
bool GamepadConfiguration::getIncludeYAxis() const { return _whichAxes[Y_AXIS]; }
bool GamepadConfiguration::getIncludeZAxis() const { return _whichAxes[Z_AXIS]; }
bool GamepadConfiguration::getIncludeRxAxis() const { return _whichAxes[RX_AXIS]; }
bool GamepadConfiguration::getIncludeRyAxis() const { return _whichAxes[RY_AXIS]; }
bool GamepadConfiguration::getIncludeRzAxis() const { return _whichAxes[RZ_AXIS]; }
bool GamepadConfiguration::getIncludeSlider1() const { return _whichAxes[SLIDER1]; }
bool GamepadConfiguration::getIncludeSlider2() const { return _whichAxes[SLIDER2]; }
const bool *GamepadConfiguration::getWhichAxes() const { return _whichAxes; }
bool GamepadConfiguration::getIncludeRudder() const { return _whichSimulationControls[RUDDER]; }
bool GamepadConfiguration::getIncludeThrottle() const { return _whichSimulationControls[THROTTLE]; }
bool GamepadConfiguration::getIncludeAccelerator() const { return _whichSimulationControls[ACCELERATOR]; }
bool GamepadConfiguration::getIncludeBrake() const { return _whichSimulationControls[BRAKE]; }
bool GamepadConfiguration::getIncludeSteering() const { return _whichSimulationControls[STEERING]; }
const bool *GamepadConfiguration::getWhichSimulationControls() const { return _whichSimulationControls; }
bool GamepadConfiguration::getIncludeRumble() const { return _includeRumble; }
bool GamepadConfiguration::getIncludePlayerIndicators() const { return _includePlayerIndicators; }

void GamepadConfiguration::setIncludeRumble(bool value) { _includeRumble = value; }
void GamepadConfiguration::setIncludePlayerIndicators(bool value) { _includePlayerIndicators = value; }

void GamepadConfiguration::setWhichSpecialButtons(bool start, bool select, bool menu, bool home, bool back, bool volumeInc, bool volumeDec, bool volumeMute)
{
    _whichSpecialButtons[START_BUTTON] = start;
    _whichSpecialButtons[SELECT_BUTTON] = select;
    _whichSpecialButtons[MENU_BUTTON] = menu;
    _whichSpecialButtons[HOME_BUTTON] = home;
    _whichSpecialButtons[BACK_BUTTON] = back;
    _whichSpecialButtons[VOLUME_INC_BUTTON] = volumeInc;
    _whichSpecialButtons[VOLUME_DEC_BUTTON] = volumeDec;
    _whichSpecialButtons[VOLUME_MUTE_BUTTON] = volumeMute;
}

void GamepadConfiguration::setWhichAxes(bool xAxis, bool yAxis, bool zAxis, bool rxAxis, bool ryAxis, bool rzAxis, bool slider1, bool slider2)
{
    _whichAxes[X_AXIS] = xAxis;
    _whichAxes[Y_AXIS] = yAxis;
    _whichAxes[Z_AXIS] = zAxis;
    _whichAxes[RX_AXIS] = rxAxis;
    _whichAxes[RY_AXIS] = ryAxis;
    _whichAxes[RZ_AXIS] = rzAxis;
    _whichAxes[SLIDER1] = slider1;
    _whichAxes[SLIDER2] = slider2;
}

void GamepadConfiguration::setWhichSimulationControls(bool rudder, bool throttle, bool accelerator, bool brake, bool steering)
{
    _whichSimulationControls[RUDDER] = rudder;
    _whichSimulationControls[THROTTLE] = throttle;
    _whichSimulationControls[ACCELERATOR] = accelerator;
    _whichSimulationControls[BRAKE] = brake;
    _whichSimulationControls[STEERING] = steering;
}

void GamepadConfiguration::setControllerType(uint8_t value) { _controllerType = value; }

void GamepadConfiguration::setButtonCount(uint16_t value) { _buttonCount = value; }
void GamepadConfiguration::setHatSwitchCount(uint8_t value) { _hatSwitchCount = value; }
void GamepadConfiguration::setIncludeStart(bool value) { _whichSpecialButtons[START_BUTTON] = value; }
void GamepadConfiguration::setIncludeSelect(bool value) { _whichSpecialButtons[SELECT_BUTTON] = value; }
void GamepadConfiguration::setIncludeMenu(bool value) { _whichSpecialButtons[MENU_BUTTON] = value; }
void GamepadConfiguration::setIncludeHome(bool value) { _whichSpecialButtons[HOME_BUTTON] = value; }
void GamepadConfiguration::setIncludeBack(bool value) { _whichSpecialButtons[BACK_BUTTON] = value; }
void GamepadConfiguration::setIncludeVolumeInc(bool value) { _whichSpecialButtons[VOLUME_INC_BUTTON] = value; }
void GamepadConfiguration::setIncludeVolumeDec(bool value) { _whichSpecialButtons[VOLUME_DEC_BUTTON] = value; }
void GamepadConfiguration::setIncludeVolumeMute(bool value) { _whichSpecialButtons[VOLUME_MUTE_BUTTON] = value; }
void GamepadConfiguration::setIncludeXAxis(bool value) { _whichAxes[X_AXIS] = value; }
void GamepadConfiguration::setIncludeYAxis(bool value) { _whichAxes[Y_AXIS] = value; }
void GamepadConfiguration::setIncludeZAxis(bool value) { _whichAxes[Z_AXIS] = value; }
void GamepadConfiguration::setIncludeRxAxis(bool value) { _whichAxes[RX_AXIS] = value; }
void GamepadConfiguration::setIncludeRyAxis(bool value) { _whichAxes[RY_AXIS] = value; }
void GamepadConfiguration::setIncludeRzAxis(bool value) { _whichAxes[RZ_AXIS] = value; }
void GamepadConfiguration::setIncludeSlider1(bool value) { _whichAxes[SLIDER1] = value; }
void GamepadConfiguration::setIncludeSlider2(bool value) { _whichAxes[SLIDER2] = value; }
void GamepadConfiguration::setIncludeRudder(bool value) { _whichSimulationControls[RUDDER] = value; }
void GamepadConfiguration::setIncludeThrottle(bool value) { _whichSimulationControls[THROTTLE] = value; }
void GamepadConfiguration::setIncludeAccelerator(bool value) { _whichSimulationControls[ACCELERATOR] = value; }
void GamepadConfiguration::setIncludeBrake(bool value) { _whichSimulationControls[BRAKE] = value; }
void GamepadConfiguration::setIncludeSteering(bool value) { _whichSimulationControls[STEERING] = value; }
void GamepadConfiguration::setAxesMin(int16_t value) { _axesMin = value; }
void GamepadConfiguration::setAxesMax(int16_t value) { _axesMax = value; }
void GamepadConfiguration::setSimulationMin(int16_t value) { _simulationMin = value; }
void GamepadConfiguration::setSimulationMax(int16_t value) { _simulationMax = value; }


uint8_t GamepadConfiguration::getButtonNumBytes() const
{
    uint8_t buttonNumBytes = this->getButtonCount() / 8;
    if (getButtonNumPaddingBits() > 0){
        buttonNumBytes++;
    }

    return buttonNumBytes;
}

uint8_t GamepadConfiguration::getButtonNumPaddingBits() const
{
    uint8_t buttonPaddingBits = 8 - (this->getButtonCount() % 8);
    if (buttonPaddingBits == 8)
    {
        buttonPaddingBits = 0;
    }
    return buttonPaddingBits;
}

uint8_t GamepadConfiguration::getSpecialButtonNumBytes() const
{
    uint8_t specialButtonNumBytes = this->getTotalSpecialButtonCount() / 8;
    if (getSpecialButtonNumPaddingBits() > 0){
        specialButtonNumBytes++;
    }

    return specialButtonNumBytes;
}

uint8_t GamepadConfiguration::getSpecialButtonNumPaddingBits() const
{
    uint8_t specialButtonPaddingBits = 8 - (this->getTotalSpecialButtonCount() % 8);
    if (specialButtonPaddingBits == 8)
    {
        specialButtonPaddingBits = 0;
    }
    return specialButtonPaddingBits;
}
