#pragma once

#include "board_config.h" // Include the main configuration header

#ifdef SD_CARD_CS_PIN // Only include this class if the SD card is supported

#include <SD.h>

class SDCardModule {
public:
    SDCardModule();
    bool init();
    bool writeFile(const char *path, const char *message);
    bool readFile(const char *path);
    bool appendFile(const char *path, const char *message);
    bool deleteFile(const char *path);
    bool logMessage(const char *logFileName, const char* foldername, const char *message);

private:
    int csPin;
    int BootNum;
    bool Initlized;
};

#endif // SD_CARD_CS_PIN