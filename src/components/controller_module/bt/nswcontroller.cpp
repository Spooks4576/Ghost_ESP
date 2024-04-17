#include "nswcontroller.h"

#ifdef HAS_BT

void NSWController::onConnect(NimBLEServer* pServer) {
  Connected = true;
  Serial.println("Client Connected...");
}

void NSWController::onDisconnect(NimBLEServer* pServer)
{
    Connected = false;
    Serial.println("Client Disconnected...");
    advertising->start();
}

void NSWController::onWrite(NimBLECharacteristic* Characteristic) {

}

void NSWController::connect()
{
    DeviceName = "NSW Wireless Pro Controller";
    NimBLEDevice::init(DeviceName);

    this->vid = 0x20d6;
    this->pid = 0xa713;

    NimBLEServer* pServer = NimBLEDevice::createServer();
    hid = new NimBLEHIDDevice(pServer);

    inputcontroller = hid->inputReport(0x01);
    inputreport = hid->inputReport(0x02);
    hid->pnp(0x02, vid, pid, 0x0210);

    pServer->setCallbacks(this, true);

    NimBLEDevice::setSecurityAuth(false, false, false);

    hid->reportMap((uint8_t*)NSW_HID_REPORT_DESCRIPTOR, sizeof(NSW_HID_REPORT_DESCRIPTOR));
    hid->startServices();

    advertising = pServer->getAdvertising();
    advertising->setAppearance(HID_GAMEPAD);
    advertising->addServiceUUID(hid->hidService()->getUUID());
    advertising->setScanResponse(false);
    advertising->start();

    report = (hid_nsw_report_t*)malloc(sizeof(hid_nsw_report_t));

    Serial.println("Advertising Started...");
    Initilized = true;
}


void NSWController::sendreport()
{
    if (inputreport && advertising)
    {
        inputreport->setValue((uint8_t*)report, sizeof(hid_nsw_report_t));
        inputreport->notify();
        delay(10);
    }
}

void NSWController::SetInputState(SwitchButton state)
{
    if (state == NONE) {
        memset(report, 0, sizeof(hid_nsw_report_t)); // Reset the entire structure
        report->leftStickX = 0x80; // Neutral X for left stick
        report->leftStickY = 0x80; // Neutral Y for left stick
        report->rightStickX = 0x80; // Neutral X for right stick
        report->rightStickY = 0x80; // Neutral Y for right stick
        report->dpad = 0x0F; // Neutral for D-pad
        return;
    }

    switch (state) {
        case BUTTON_Y:
            report->buttonCombos |= 0x01;
            break;
        case BUTTON_B:
            report->buttonCombos |= 0x02;
            break;
        case BUTTON_A:
            report->buttonCombos |= 0x04;
            break;
        case BUTTON_X:
            report->buttonCombos |= 0x08;
            break;
        case BUTTON_MINUS:
            report->additionalButtons |= 0x01;
            break;
        case BUTTON_PLUS:
            report->additionalButtons |= 0x02;
            break;
        case BUTTON_HOME:
            report->additionalButtons |= 0x10;
            break;
        case BUTTON_SHARE:
            report->additionalButtons |= 0x20;
            break;
        case BUTTON_LBUMPER:
            report->buttonCombos |= 0x10;
            break;
        case BUTTON_RBUMPER:
            report->buttonCombos |= 0x20;
            break;
        case BUTTON_LTRIGGER:
            report->buttonCombos |= 0x40;
            break;
        case BUTTON_RTRIGGER:
            report->buttonCombos |= 0x80;
            break;
        case BUTTON_LTHUMBSTICK:
            report->additionalButtons |= 0x04;
            break;
        case BUTTON_RTHUMBSTICK:
            report->additionalButtons |= 0x08;
            break;
        case DPAD_UP:
            report->dpad = 0x00;
            break;
        case DPAD_RIGHT:
            report->dpad = 0x02;
            break;
        case DPAD_DOWN:
            report->dpad = 0x04;
            break;
        case DPAD_LEFT:
            report->dpad = 0x06;
            break;
        default:
            break;
    }
    Serial.println("Sent Input");
}

#endif