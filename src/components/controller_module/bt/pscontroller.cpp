#include "pscontroller.h"

#ifdef HAS_BT

void PSController::onConnect(NimBLEServer* pServer) {
  Connected = true;
  Serial.println("Client Connected...");
}

void PSController::onDisconnect(NimBLEServer* pServer)
{
    Connected = false;
    Serial.println("Client Disconnected...");
    advertising->start();
}

void PSController::onWrite(NimBLECharacteristic* Characteristic) {

}

void PSController::connect()
{
    DeviceName = "Dualshock Wireless Controller";
    NimBLEDevice::init(DeviceName);

    this->vid = 0x054C;
    this->pid = 0x05C4;

    NimBLEServer* pServer = NimBLEDevice::createServer();
    hid = new NimBLEHIDDevice(pServer);

    pServer->setCallbacks(this, true);

    inputcontroller = hid->inputReport(0x01);
    inputreport = hid->inputReport(0x02);
    hid->pnp(0x02, vid, pid, 0x0210);
    hid->hidInfo(0x00, 0x01);

    NimBLEDevice::setSecurityAuth(false, false, false);

    hid->reportMap((uint8_t*)ps5btreportdescriptor, sizeof(ps5btreportdescriptor));
    hid->startServices();

    
    advertising = pServer->getAdvertising();
    advertising->setAppearance(HID_GAMEPAD);
    advertising->addServiceUUID(hid->hidService()->getUUID());
    advertising->setScanResponse(true);
    advertising->start();

    Serial.println("Advertising Started...");

    report = (ps_hid_bt_report*)malloc(sizeof(ps_hid_bt_report));

    Initilized = true;
}

void PSController::sendreport()
{
    if (inputreport && advertising && Connected)
    {
        inputcontroller->setValue((uint8_t*)report, sizeof(ps_hid_bt_report));
        inputcontroller->notify();
        delay(10);
    }
}

void PSController::SetInputState(PSButton state)
{
    if (state == PS_NONE) {
        memset(report, 0, sizeof(ps_hid_bt_report));  // Clear structure
        report->x = 0x7D;         // Neutral X
        report->y = 0x7E;         // Neutral Y
        report->z = 0x83;         // Neutral Z
        report->rz = 0x82;        // Neutral Rz
        report->buttons1 = 0x08;  // Set hat switch to neutral (0x08)
        return;
    }

    switch (state) {
        case PS_BUTTON_TRIANGLE:
            report->buttons1 |= (1 << 7); // Set bit for Triangle button
            break;
        case PS_BUTTON_CIRCLE:
            report->buttons1 |= (1 << 6); // Set bit for Circle button
            break;
        case PS_BUTTON_CROSS:
            report->buttons1 |= (1 << 5); // Set bit for Cross button
            break;
        case PS_BUTTON_SQUARE:
            report->buttons1 |= (1 << 4); // Set bit for Square button
            break;
        case PS_BUTTON_SHARE:
            report->buttons2 |= (1 << 4); // Set bit for Share button
            break;
        case PS_BUTTON_OPTIONS:
            report->buttons2 |= (1 << 5); // Set bit for Options button
            break;
        case PS_BUTTON_PS:
            report->buttons3 |= (1 << 0); // Set bit for PS button
            break;
        case PS_BUTTON_TOUCHPAD:
            report->buttons3 |= (1 << 1); // Set bit for Touchpad button
            break;
        case PS_BUTTON_L1:
            report->buttons2 |= (1 << 0); // Set bit for L1 button
            break;
        case PS_BUTTON_R1:
            report->buttons2 |= (1 << 1); // Set bit for R1 button
            break;
        case PS_BUTTON_L2:
            report->buttons2 |= (1 << 2); // Set bit for L2 button
            break;
        case PS_BUTTON_R2:
            report->buttons2 |= (1 << 3); // Set bit for R2 button
            break;
        case PS_BUTTON_L3:
            report->buttons2 |= (1 << 6); // Set bit for L3 button
            break;
        case PS_BUTTON_R3:
            report->buttons2 |= (1 << 7); // Set bit for R3 button
            break;
        case PS_DPAD_UP:
            report->buttons1 = (report->buttons1 & 0xF0) | 0x00; // Set bits for D-Pad Up
            break;
        case PS_DPAD_DOWN:
            report->buttons1 = (report->buttons1 & 0xF0) | 0x04; // Set bits for D-Pad Down
            break;
        case PS_DPAD_LEFT:
            report->buttons1 = (report->buttons1 & 0xF0) | 0x06; // Set bits for D-Pad Left
            break;
        case PS_DPAD_RIGHT:
            report->buttons1 = (report->buttons1 & 0xF0) | 0x02; // Set bits for D-Pad Right
            break;
        case PS_LSTICK_UP:
            report->y = 0; // Move left stick up
            break;
        case PS_LSTICK_DOWN:
            report->y = 255; // Move left stick down
            break;
        case PS_LSTICK_LEFT:
            report->x = 0; // Move left stick left
            break;
        case PS_LSTICK_RIGHT:
            report->x = 255; // Move left stick right
            break;
        case PS_RSTICK_UP:
            report->rz = 0; // Move right stick up
            break;
        case PS_RSTICK_DOWN:
            report->rz = 255; // Move right stick down
            break;
        case PS_RSTICK_LEFT:
            report->z = 0; // Move right stick left
            break;
        case PS_RSTICK_RIGHT:
            report->z = 255; // Move right stick right
            break;
        default:
            break;
    }

    Serial.println("Sent Input");
}


#endif