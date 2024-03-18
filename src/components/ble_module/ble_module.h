#ifndef BLE_MODULE_H
#define BLE_MODULE_H

#include "board_config.h"

#ifdef HAS_BT
#include <NimBLEDevice.h>
#endif
#include <esp_mac.h>

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
    NimBLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType type);
    #endif
    void executeSpam(EBLEPayloadType type);
    void generateRandomMac(uint8_t* mac);
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
    #endif
};
#endif