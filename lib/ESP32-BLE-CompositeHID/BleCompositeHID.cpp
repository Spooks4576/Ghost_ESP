#include "BleCompositeHID.h"
#include "BleConnectionStatus.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include <NimBLEHIDDevice.h>

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "BLECompositeHID"
#else
#include "esp_log.h"
static const char *LOG_TAG = "BLECompositeHID";
#endif

#define SERVICE_UUID_DEVICE_INFORMATION        "180A"      // Service - Device information

#define CHARACTERISTIC_UUID_SYSTEM_ID          "2A23"      // Characteristic - System ID 0x2A23
#define CHARACTERISTIC_UUID_MODEL_NUMBER       "2A24"      // Characteristic - Model Number String - 0x2A24
#define CHARACTERISTIC_UUID_SOFTWARE_REVISION  "2A28"      // Characteristic - Software Revision String - 0x2A28
#define CHARACTERISTIC_UUID_SERIAL_NUMBER      "2A25"      // Characteristic - Serial Number String - 0x2A25
#define CHARACTERISTIC_UUID_FIRMWARE_REVISION  "2A26"      // Characteristic - Firmware Revision String - 0x2A26
#define CHARACTERISTIC_UUID_HARDWARE_REVISION  "2A27"      // Characteristic - Hardware Revision String - 0x2A27

uint16_t vidSource;
uint16_t vid;
uint16_t pid;
uint16_t guidVersion;
std::string modelNumber;
std::string softwareRevision;
std::string serialNumber;
std::string firmwareRevision;
std::string hardwareRevision;
std::string systemID;

std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
  std::stringstream ss;

  ss << std::hex << std::setfill('0');

  for (int i = 0; i < s; i++) {
    ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
  }

  return ss.str();
}

BleCompositeHID::BleCompositeHID(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) : _hid(nullptr)
{
    this->deviceName = deviceName.substr(0, CONFIG_BT_NIMBLE_GAP_DEVICE_NAME_MAX_LEN - 1);
    this->deviceManufacturer = deviceManufacturer;
    this->batteryLevel = batteryLevel;
    this->_connectionStatus = new BleConnectionStatus();
}

BleCompositeHID::~BleCompositeHID()
{
    delete this->_connectionStatus;
}

void BleCompositeHID::begin()
{
    this->begin(BLEHostConfiguration());
}

void BleCompositeHID::begin(const BLEHostConfiguration& config)
{
    _configuration = config; // we make a copy, so the user can't change actual values midway through operation, without calling the begin function again

    modelNumber = _configuration.getModelNumber();
    softwareRevision = _configuration.getSoftwareRevision();
    serialNumber = _configuration.getSerialNumber();
    firmwareRevision = _configuration.getFirmwareRevision();
    hardwareRevision = _configuration.getHardwareRevision();
    systemID = _configuration.getSystemID();

    vidSource = _configuration.getVidSource();
	vid = _configuration.getVid();
	pid = _configuration.getPid();
	guidVersion = _configuration.getGuidVersion();

	uint8_t high = highByte(vid);
	uint8_t low = lowByte(vid);

	vid = low << 8 | high;

	high = highByte(pid);
	low = lowByte(pid);

	pid = low << 8 | high;
	
	high = highByte(guidVersion);
	low = lowByte(guidVersion);
	guidVersion = low << 8 | high;
    
    // Start BLE server
    xTaskCreate(this->taskServer, "server", 20000, (void *)this, 5, NULL);
}

void BleCompositeHID::end(void)
{
    vTaskDelete(this->_autoSendTaskHandle);
}

void BleCompositeHID::timedSendDeferredReports(void *pvParameter)
{
    BleCompositeHID *BleCompositeHIDInstance = (BleCompositeHID *)pvParameter;

    if (BleCompositeHIDInstance->_hid)
    {
        std::function<void()> reportFunc;
        while(BleCompositeHIDInstance->_deferredReports.ConsumeSync(reportFunc)){
            reportFunc();
            if(BleCompositeHIDInstance->_configuration.getQueueSendRate() > 0)
                vTaskDelay((1000 / BleCompositeHIDInstance->_configuration.getQueueSendRate()) / portTICK_PERIOD_MS);
        }
    }
}

void BleCompositeHID::addDevice(BaseCompositeDevice *device)
{
    device->_parent = this;
    _devices.push_back(device);
}

bool BleCompositeHID::isConnected()
{
    return this->_connectionStatus->connected;
}

void BleCompositeHID::setBatteryLevel(uint8_t level)
{
    this->batteryLevel = level;
    if (this->_hid)
    {
        this->_hid->setBatteryLevel(this->batteryLevel);

        if (this->isConnected())
        {
            this->_hid->batteryLevel()->notify();
        }
		
        // if (this->_configuration.getAutoReport())
        // {
        //     // TODO: Send report?
        // }
    }
}

void BleCompositeHID::queueDeviceDeferredReport(std::function<void()> && reportFunc)
{
    this->_deferredReports.Produce(std::forward<std::function<void()>>(reportFunc));
}

void BleCompositeHID::sendDeferredReports()
{
    if (this->_hid)
    {
        std::function<void()> reportFunc;
        while(this->_deferredReports.Consume(reportFunc)){
            reportFunc();
        }
    }
}

void BleCompositeHID::taskServer(void *pvParameter)
{
    BleCompositeHID *BleCompositeHIDInstance = (BleCompositeHID *)pvParameter; // static_cast<BleCompositeHID *>(pvParameter);

    // Use the procedure below to set a custom Bluetooth MAC address
    // Compiler adds 0x02 to the last value of board's base MAC address to get the BT MAC address, so take 0x02 away from the value you actually want when setting
    //uint8_t newMACAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF - 0x02};
    //esp_base_mac_addr_set(&newMACAddress[0]); // Set new MAC address 
    NimBLEDevice::init(BleCompositeHIDInstance->deviceName);
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(BleCompositeHIDInstance->_connectionStatus);

    BleCompositeHIDInstance->_hid = new NimBLEHIDDevice(pServer);
    
    // Setup the HID descriptor buffers
    size_t totalBufferSize = 2048;
    uint8_t tempHidReportDescriptor[totalBufferSize];
    int hidReportDescriptorSize = 0;
    ESP_LOGD(LOG_TAG, "About to init devices");
    
    // Setup child devices to build the HID report descriptor
    for(auto device : BleCompositeHIDInstance->_devices){
        ESP_LOGD(LOG_TAG, "Before device %s init", device->getDeviceConfig()->getDeviceName());
        device->init(BleCompositeHIDInstance->_hid);
        ESP_LOGD(LOG_TAG, "After device %s init", device->getDeviceConfig()->getDeviceName());
        
        auto config = device->getDeviceConfig();
        size_t reportSize = config->makeDeviceReport(tempHidReportDescriptor + hidReportDescriptorSize, totalBufferSize);
        
        if(reportSize >= BLE_ATT_ATTR_MAX_LEN){
            ESP_LOGE(LOG_TAG, "Device report size %d is larger than max buffer size %d", reportSize, BLE_ATT_ATTR_MAX_LEN);
            return;
        } else if(reportSize == 0){
            ESP_LOGE(LOG_TAG, "Device report size is 0");
            return;
        } else if(reportSize < 0){
            ESP_LOGE(LOG_TAG, "Error creating report for device %s", config->getDeviceName());
            return;
        } else {
            ESP_LOGD(LOG_TAG, "Created device %s with report size %d", config->getDeviceName(), reportSize);
        }
        hidReportDescriptorSize += reportSize;
    }
    ESP_LOGD(LOG_TAG, "Final hidReportDescriptorSize: %d", hidReportDescriptorSize);

    // Set the report map
    uint8_t customHidReportDescriptor[hidReportDescriptorSize];
    memcpy(&customHidReportDescriptor, tempHidReportDescriptor, hidReportDescriptorSize);
    BleCompositeHIDInstance->_hid->reportMap(&customHidReportDescriptor[0], hidReportDescriptorSize);

    // Set manufacturer info
    BleCompositeHIDInstance->_hid->manufacturer()->setValue(BleCompositeHIDInstance->deviceManufacturer);

    // Create device UUID
    NimBLEService *pService = pServer->getServiceByUUID(SERVICE_UUID_DEVICE_INFORMATION);
	
    // Create characteristics
	BLECharacteristic* pCharacteristic_Model_Number = pService->createCharacteristic(
      CHARACTERISTIC_UUID_MODEL_NUMBER,
      NIMBLE_PROPERTY::READ
    );
    pCharacteristic_Model_Number->setValue(modelNumber);
	
	BLECharacteristic* pCharacteristic_Software_Revision = pService->createCharacteristic(
      CHARACTERISTIC_UUID_SOFTWARE_REVISION,
      NIMBLE_PROPERTY::READ
    );
    pCharacteristic_Software_Revision->setValue(softwareRevision);
	
	BLECharacteristic* pCharacteristic_Serial_Number = pService->createCharacteristic(
      CHARACTERISTIC_UUID_SERIAL_NUMBER,
      NIMBLE_PROPERTY::READ
    );
    pCharacteristic_Serial_Number->setValue(serialNumber);
	
	BLECharacteristic* pCharacteristic_Firmware_Revision = pService->createCharacteristic(
      CHARACTERISTIC_UUID_FIRMWARE_REVISION,
      NIMBLE_PROPERTY::READ
    );
    pCharacteristic_Firmware_Revision->setValue(firmwareRevision);
	
	BLECharacteristic* pCharacteristic_Hardware_Revision = pService->createCharacteristic(
      CHARACTERISTIC_UUID_HARDWARE_REVISION,
      NIMBLE_PROPERTY::READ
    );
    pCharacteristic_Hardware_Revision->setValue(hardwareRevision);

    // BLECharacteristic* pCharacteristic_System_ID = pService->createCharacteristic(
    //   CHARACTERISTIC_UUID_SYSTEM_ID,
    //   NIMBLE_PROPERTY::READ
    // );
    // pCharacteristic_Hardware_Revision->setValue();

    // Set PnP IDs
    BleCompositeHIDInstance->_hid->pnp(vidSource, vid, pid, guidVersion);
    BleCompositeHIDInstance->_hid->hidInfo(0x00, 0x01);

    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);  //BLE_SM_PAIR_AUTHREQ_SC

    // Start BLE server
    BleCompositeHIDInstance->_hid->startServices();
    BleCompositeHIDInstance->onStarted(pServer);

    // Start BLE advertisement
    NimBLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(GENERIC_HID);
    pAdvertising->addServiceUUID(BleCompositeHIDInstance->_hid->hidService()->getUUID());
    pAdvertising->start();
    ESP_LOGD(LOG_TAG, "Advertising started!");

    // Update battery
    BleCompositeHIDInstance->_hid->setBatteryLevel(BleCompositeHIDInstance->batteryLevel);

    // Start timed auto send for deferred reports
    if(BleCompositeHIDInstance->_configuration.getQueuedSending()){
        xTaskCreate(BleCompositeHIDInstance->timedSendDeferredReports, "autoSend", 20000, (void *)BleCompositeHIDInstance, 5, &BleCompositeHIDInstance->_autoSendTaskHandle);
    }

    // Wait to let the server start up
    vTaskDelay(portMAX_DELAY);
}
