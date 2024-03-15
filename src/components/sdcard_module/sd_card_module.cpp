#include "sd_card_module.h"

#ifdef SD_CARD_CS_PIN

SDCardModule::SDCardModule() : csPin(SD_CARD_CS_PIN) {}

bool SDCardModule::init() {
    if (!SD.begin(csPin)) {
        Serial.println("SD Card initialization failed!");
        return false;
    }
    Serial.println("SD Card initialized.");
    return true;
}

bool SDCardModule::writeFile(const char *path, const char *message) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }
    if (file.println(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
    return true;
}

bool SDCardModule::readFile(const char *path) {
    File file = SD.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }
    Serial.print("Read from file: ");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
    return true;
}

bool SDCardModule::appendFile(const char *path, const char *message) {
    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return false;
    }
    if (file.println(message)) {
        Serial.println("File appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
    return true;
}

bool SDCardModule::deleteFile(const char *path) {
    if (SD.remove(path)) {
        Serial.println("File deleted");
        return true;
    } else {
        Serial.println("Delete failed");
        return false;
    }
}

#endif // SD_CARD_CS_PIN