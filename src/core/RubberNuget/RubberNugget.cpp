#include "Arduino.h"
#include <WiFi.h>

#include "RubberNugget.h"
#include "utils.h"

// ESPTinyUSB libraries
#include "cdcusb.h"
#include "mscusb.h"
#include "flashdisk.h"
#include "hidkeyboard.h"

HIDkeyboard keyboard;
CDCusb CDCUSBSerial;
FlashUSB fat1;

char *l1 = "ffat";
String payloadPath = "";

/*-----------------------------------------------------------------*/

class MyCDCCallbacks : public CDCCallbacks {
    void onCodingChange(cdc_line_coding_t const* p_line_coding)
    {
        int bitrate = CDCUSBSerial.getBitrate();
        Serial.printf("new bitrate: %d\n", bitrate);
    }

    bool onConnect(bool dtr, bool rts)
    {
        Serial.printf("connection state changed, dtr: %d, rts: %d\n", dtr, rts);
        return true;  // allow to persist reset, when Arduino IDE is trying to enter bootloader mode
    }

    void onData()
    {
        int len = CDCUSBSerial.available();
        Serial.printf("\nnew data, len %d\n", len);
        uint8_t buf[len] = {};
        CDCUSBSerial.read(buf, len);
        Serial.write(buf, len);
    }
};

class Device: public USBCallbacks {
    void onMount() { Serial.println("Mount"); }
    void onUnmount() { Serial.println("Unmount"); }
    void onSuspend(bool remote_wakeup_en) { Serial.println("Suspend"); }
    void onResume() { Serial.println("Resume"); }
};

class MyHIDCallbacks : public HIDCallbacks
{
    void onData(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
    {
        Serial.printf("ID: %d, type: %d, size: %d\n", report_id, (int)report_type, bufsize);
        for (size_t i = 0; i < bufsize; i++)
        {
            Serial.printf("%d\n", buffer[i]);
        }
    }
};

void echo_all(char c) {
    CDCUSBSerial.write(c);
    Serial.write(c);
}

/*-----------------------------------------------------------------*/


void RubberNugget::init() {
  // Mount FAT fs
  if (fat1.init("/fat1", "ffat")) {
        //disable this on startup
        if (fat1.begin()) {
            Serial.println("MSC lun 1 begin");
        }
        else {
            log_e("LUN 1 failed");
        }
  }
  if (!CDCUSBSerial.begin())
      Serial.println("Failed to start CDC USB stack");
  CDCUSBSerial.setCallbacks(new MyCDCCallbacks());
  EspTinyUSB::registerDeviceCallbacks(new Device());

  // Read config from settings file
  NuggetConfig c = getConfig();

  // Init AP
  WiFi.softAP(c.network.c_str(), c.password.c_str());
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Setup keyboard
  keyboard.deviceID(c.vid,c.pid);
  keyboard.setBaseEP(3);
  keyboard.begin();
  keyboard.setCallbacks(new MyHIDCallbacks());
}

// newFileList returns a paginated list of strings representing the files
// available at the given path. resultsPerPage will be set to number of results
// returned. On error, it returns nullptr and numFiles is set to -1.
FILINFO* newFileList(const char* path, int& numFiles) {
  uint currentCapacity = 4; // arbitrary, will grow as needed
  uint filesSoFar = 0;
  bool errored = false;
  FILINFO* fileList = new FILINFO[currentCapacity];

  FRESULT res;
  FF_DIR dir;
  FILINFO fno;

  res = f_opendir(&dir, path);
  if (res != FR_OK) {
    Serial.printf("[newFileList] f_opendir error: %d\n", res);
    goto onError;
  }

  for (;;) {
      res = f_readdir(&dir, &fno); // Read a directory item
      if (res != FR_OK) {
        errored = true;
        break;
      }
      if (fno.fname[0] == 0) { // End of dir
        break;
      }
      if (fno.fname[0] == '.') { // Don't show files starting with '.'
        continue;
      }
      // Found a file/dir. Store it, doubling capacity first if needed. 
      if (filesSoFar >= currentCapacity) {
        FILINFO* newFileList = new FILINFO[currentCapacity*2];
        for (int i = 0; i < currentCapacity; i++){
          newFileList[i] = fileList[i];
        }
        currentCapacity *= 2;
        delete[] fileList;
        fileList = newFileList;
      }
      fileList[filesSoFar] = fno;
      filesSoFar++; 
  }
  if (errored) {
    Serial.printf("[newFileList] f_readdir error: %d\n", res);
    goto onError;
  }
  res = f_closedir(&dir);
  if (res != FR_OK) {
    Serial.printf("[newFileList] f_closedir error: %d\n", res);
    goto onError;
  }
  if (filesSoFar < 1){
    Serial.println("[newFileList] directory empty");
    goto onError;
  }
  numFiles = filesSoFar;
  return fileList;

onError:
  numFiles = -1;
  delete[] fileList;
  return nullptr;
}

// allPayloadPaths returns a comma seperated list of paths to all payloads.
String* RubberNugget::allPayloadPaths(const char* path) {
  int numFiles = 0;
  FILINFO* files = newFileList(path, numFiles);

  // DFS on fs
  // TODO: measure max stack usage; if cutting it close reimplement
  // non-recursively.
  if (numFiles < 1) {
    return nullptr;
  }
  String* ret = new String;
  for(int i = 0; i < numFiles; i++) {
    if (files[i].fattrib & AM_DIR) { // Directory; recurse
      String recursivePath(path);
      if (recursivePath.length()>1){ // non-root
        recursivePath += "/";
      }
      recursivePath += files[i].fname;
      String* subDirFiles = allPayloadPaths(recursivePath.c_str());
      if (subDirFiles) {
        (*ret) += (*subDirFiles);
        delete subDirFiles;
      }
    } else { // file, append to list
      // TODO: escape commas in file names 
      String pathString(path);
      if (pathString.length() > 1) { // non-root
        pathString += "/";
      }
      (*ret) += pathString;
      (*ret) += files[i].fname;
      (*ret) += ",";
    }
  }
  delete[] files;
  return ret;
}

NuggetConfig getConfig() {
  NuggetConfig conf;
  conf.locale = "EN";
  conf.network = "Nugget AP";
  conf.password = "nugget123";
  conf.pid = 0x20b;
  conf.vid = 0x05ac;

  fileOp configRead = readFile(".usbnugget.conf");
  if (!configRead.ok) {
    Serial.printf("config file could not be read: %s\n", configRead.result);
    return conf;
  }

  int lineStart = 0;
  String currentLine;
  String currentLineKeyValue;
  while (lineStart < configRead.result.length()) {
    int lineEnd = configRead.result.indexOf('\n', lineStart);
    if (lineEnd == -1){
      lineEnd = configRead.result.length();
      break;
    }

    // process line
    currentLine = configRead.result.substring(lineStart, lineEnd);
    currentLine.trim();
    int valueStart = currentLine.indexOf("\"");
    int valueEnd = currentLine.lastIndexOf("\"");
    if (valueStart == -1 || valueStart==valueEnd) {
      // couldn't find both quotes
      goto nextLine;
    }
    currentLineKeyValue = currentLine.substring(valueStart+1, valueEnd);

    if (currentLine.indexOf("network = \"") == 0) {
      conf.network = currentLineKeyValue;
    } else
    if (currentLine.indexOf("password = \"") == 0) {
      if (currentLineKeyValue.length() < 8) {
        goto nextLine;
      }
      conf.password = currentLineKeyValue;
    } else
    if (currentLine.indexOf("pid = \"") == 0) {
        char hex[currentLineKeyValue.length() + 1];
        strcpy(hex, currentLineKeyValue.c_str());
        char* ptr;
        conf.pid = strtoul(hex, &ptr, 16);
    } else
    if (currentLine.indexOf("vid = \"") == 0) {
        char hex[currentLineKeyValue.length() + 1];
        strcpy(hex, currentLineKeyValue.c_str());
        char* ptr;
        conf.vid = strtoul(hex, &ptr, 16);
    }
nextLine:
    // update line
    lineStart = lineEnd+1;
  }
  return conf;
}
