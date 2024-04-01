#ifndef BLE_MODULE_H
#define BLE_MODULE_H

#include "board_config.h"

#ifdef HAS_BT
#include <NimBLEDevice.h>
#endif
#include <esp_mac.h>
#include <Arduino.h>

inline struct {
    uint32_t value;
    const char* name;
} buds_models[] = {
    {0xEE7A0C, "Fallback Buds"},
    {0x9D1700, "Fallback Dots"},
    {0x39EA48, "Light Purple Buds2"},
    {0xA7C62C, "Bluish Silver Buds2"},
    {0x850116, "Black Buds Live"},
    {0x3D8F41, "Gray & Black Buds2"},
    {0x3B6D02, "Bluish Chrome Buds2"},
    {0xAE063C, "Gray Beige Buds2"},
    {0xB8B905, "Pure White Buds"},
    {0xEAAA17, "Pure White Buds2"},
    {0xD30704, "Black Buds"},
    {0x9DB006, "French Flag Buds"},
    {0x101F1A, "Dark Purple Buds Live"},
    {0x859608, "Dark Blue Buds"},
    {0x8E4503, "Pink Buds"},
    {0x2C6740, "White & Black Buds2"},
    {0x3F6718, "Bronze Buds Live"},
    {0x42C519, "Red Buds Live"},
    {0xAE073A, "Black & White Buds2"},
    {0x011716, "Sleek Black Buds2"},
};

#define NUM_MODELS (sizeof(buds_models) / sizeof(buds_models[0]))

enum EBLEPayloadType
{
    Microsoft,
    Apple,
    Samsung,
    Google
};

struct BLEData
{
    #ifdef HAS_BT
    NimBLEAdvertisementData AdvData;
    NimBLEAdvertisementData ScanData;
    #endif
};

struct WatchModel
{
    uint8_t value;
    const char *name;
};

inline WatchModel* watch_models = new WatchModel[26] 
{
    {0x1A, "Fallback Watch"},
    {0x01, "White Watch4 Classic 44m"},
    {0x02, "Black Watch4 Classic 40m"},
    {0x03, "White Watch4 Classic 40m"},
    {0x04, "Black Watch4 44mm"},
    {0x05, "Silver Watch4 44mm"},
    {0x06, "Green Watch4 44mm"},
    {0x07, "Black Watch4 40mm"},
    {0x08, "White Watch4 40mm"},
    {0x09, "Gold Watch4 40mm"},
    {0x0A, "French Watch4"},
    {0x0B, "French Watch4 Classic"},
    {0x0C, "Fox Watch5 44mm"},
    {0x11, "Black Watch5 44mm"},
    {0x12, "Sapphire Watch5 44mm"},
    {0x13, "Purpleish Watch5 40mm"},
    {0x14, "Gold Watch5 40mm"},
    {0x15, "Black Watch5 Pro 45mm"},
    {0x16, "Gray Watch5 Pro 45mm"},
    {0x17, "White Watch5 44mm"},
    {0x18, "White & Black Watch5"},
    {0x1B, "Black Watch6 Pink 40mm"},
    {0x1C, "Gold Watch6 Gold 40mm"},
    {0x1D, "Silver Watch6 Cyan 44mm"},
    {0x1E, "Black Watch6 Classic 43m"},
    {0x20, "Green Watch6 Classic 43m"},
};

struct PayloadInfo {
    int count;
    unsigned long firstSeenTime;
    String Mac;
};

#ifdef HAS_BT
class FlipperFinderCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override;
};

class BleSpamDetectorCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override;
    std::map<String, PayloadInfo> payloadInfoMap;
};

#endif

#ifdef HAS_BT
static void scanCompleteCB(BLEScanResults scanResults);
#endif
class BLEModule
{
public:
    const char* generateRandomName();
    #ifdef HAS_BT
    NimBLEScan* pBLEScan;
    NimBLEAdvertising* pAdvertising;
    BLEData GetUniversalAdvertisementData(EBLEPayloadType type);
    #endif
    void executeSpam(EBLEPayloadType type, bool Initlized);
    void generateRandomMac(uint8_t* mac);
    void executeSpamAll();
    void esp_fill_random(uint8_t* target, size_t size);

    void findtheflippers();
    void BleSpamDetector();

    bool BLEInitilized;
    #ifdef HAS_BT
    bool shutdownBLE()
    {
        BLEInitilized = false; // Stop While Loops
        pAdvertising->stop();
        pBLEScan->stop();
        
        pBLEScan->clearResults();
        NimBLEDevice::deinit();
        return true;
    }
    void init()
    {  // Defining here due to linker errors
        NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
        NimBLEDevice::setScanDuplicateCacheSize(200);
        NimBLEDevice::init("");
        pBLEScan = NimBLEDevice::getScan(); //create new scan
        this->shutdownBLE();
    }



    void fill_samsungbud_byte(uint8_t *array) {
        int randomIndex = rand() % NUM_MODELS;
        uint32_t value = buds_models[randomIndex].value;
        array[17] = (value >> 24) & 0xFF; // 17th byte
        array[18] = (value >> 16) & 0xFF;  // 18th byte
        array[20] = (value >> 8) & 0xFF; // 20th byte, note the change in order due to byte significance
    }

    #endif
};
#endif