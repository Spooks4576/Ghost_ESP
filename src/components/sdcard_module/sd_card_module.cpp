#include "sd_card_module.h"

SDCardModule::SDCardModule(){}

bool SDCardModule::init() {

    ECardType CardType;

#ifdef SD_CARD_CS_PIN
    csPin = SD_CARD_CS_PIN;
    Initlized = SD.begin(csPin);
    CardType = ECardType::SDI;
#elif SUPPORTS_MMC
    Initlized = SD_MMC.begin();
    IsMMCCard = true;
    CardType = ECardType::MMC;
#else
CardType = ECardType::Serial;
#endif

    if (!Initlized) {
        Serial.println("SD Card initialization failed!");
        return false;
    }

    SDI = createFileSystem(CardType);

    Serial.println("SD Card initialized.");

    const char* dirPath = "/logs";
    if (!SDI->exists(dirPath)) {
        if (SDI->mkdir(dirPath)) {
        } else {
        Serial.println("Directory creation failed");
        }
    } else {
        Serial.println("Directory already exists");
    }

    dirPath = "/pcaps";
    if (!SDI->exists(dirPath)) {
        if (SDI->mkdir(dirPath)) {
        } else {
        Serial.println("Directory creation failed");
        }
    } else {
        Serial.println("Directory already exists");
    }
    
    dirPath = "/pcaps/raw";
    if (!SDI->exists(dirPath)) {
        if (SDI->mkdir(dirPath)) {
        } else {
        Serial.println("Directory creation failed");
        }
    } else {
        Serial.println("Directory already exists");
    }

    dirPath = "/pcaps/eapol";
    if (!SDI->exists(dirPath)) {
        if (SDI->mkdir(dirPath)) {
        } else {
        Serial.println("Directory creation failed");
        }
    } else {
        Serial.println("Directory already exists");
    }

    dirPath = "/pcaps/pwn";
    if (!SDI->exists(dirPath)) {
        if (SDI->mkdir(dirPath)) {
        } else {
        Serial.println("Directory creation failed");
        }
    } else {
        Serial.println("Directory already exists");
    }

    dirPath = "/pcaps/probe";
    if (!SDI->exists(dirPath)) {
        if (SDI->mkdir(dirPath)) {
        } else {
        Serial.println("Directory creation failed");
        }
    } else {
        Serial.println("Directory already exists");
    }

    unsigned int maxNum = 0;
    File root = SDI->open("/logs");
    
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }
        if (strstr(entry.name(), "boot_") == entry.name()) {
            unsigned int fileNum;
            int scanned = sscanf(entry.name(), "boot_%u_", &fileNum);
            if (scanned == 1) {
                maxNum = max(maxNum, fileNum);
            }
        }
        entry.close();
    }
    root.close();

    BootNum = maxNum + 1;

    char newLogFileName[128];
    sprintf(newLogFileName, "/logs/boot_%u_%s.txt", BootNum, "GhostESP");

    writeFile(newLogFileName, "Begin Ghost ESP Log");
    return true;
}

bool SDCardModule::logMessage(const char *logFileName, const char* foldername, String message)
{
    if (Initlized)
    {
        unsigned long timeSinceBoot = millis();
        unsigned long hours = (timeSinceBoot / (3600000UL)) % 24;
        unsigned long minutes = (timeSinceBoot / (60000UL)) % 60;
        unsigned long seconds = (timeSinceBoot / 1000UL) % 60;

        char timeString[64];
        sprintf(timeString, "[%02lu:%02lu:%02lu] ", hours, minutes, seconds);
        
        char newLogFileName[128];
        sprintf(newLogFileName, "/%s/boot_%u_%s", foldername, BootNum, logFileName);

        String fullMessage = message;

        if (!SDI->exists("/" + String(foldername)))
        {
            SDI->mkdir("/" + String(foldername));
        }

        if (!SDI->exists(newLogFileName))
        {
            writeFile(newLogFileName, "");
        }
        
        return appendFile(newLogFileName, fullMessage.c_str());
    }
    else
    {
        return false;
    }
}

bool SDCardModule::startPcapLogging(const char *path, bool bluetooth) {
    if (!Initlized)
    {
        if (bluetooth)
        {
            pcapHeader.network = 251;
        }
        else 
        {
            pcapHeader.network = 105;
        }
        return false;
    }

    String CutPath = path;
    CutPath.replace(".pcap", "");
    CutPath.toLowerCase();
    
    if (!SDI->exists("/pcaps/" + CutPath))
    {
        SDI->mkdir("/pcaps/" + CutPath);
    }

    unsigned int maxNum = 0;
    File root = SDI->open("/pcaps/" + CutPath);
    
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }
        if (strstr(entry.name(), "boot_") == entry.name()) {
            unsigned int fileNum;
            int scanned = sscanf(entry.name(), "boot_%u_", &fileNum);
            if (scanned == 1) {
                maxNum = max(maxNum, fileNum);
            }
        }
        entry.close();
    }
    root.close();

    PcapFileIndex = maxNum;

    char newLogFileName[128];
    sprintf(newLogFileName, "/%s/%s/boot_%u_%s", "pcaps", CutPath.c_str(), PcapFileIndex += 1, path);
    
    logFile = SDI->open(newLogFileName, FILE_WRITE);
    if (!logFile) {
        return false;
    }

    if (bluetooth)
    {
        pcapHeader.network = 251;
    }
    else 
    {
        pcapHeader.network = 105;
    }

    logFile.write((const uint8_t *)&pcapHeader, sizeof(pcapHeader));
    bufferLength = 0;
    return true;
}

void SDCardModule::flushBufferToSerial() {
    if (bufferLength > 0) {
        const char* mark_begin = "[BUF/BEGIN]";
        const size_t mark_begin_len = strlen(mark_begin);
        const char* mark_close = "[BUF/CLOSE]";
        const size_t mark_close_len = strlen(mark_close);

        uint8_t* buf = (uint8_t*)malloc(mark_begin_len + bufferLength + mark_close_len);
        if (!buf) {
            Serial.println("Failed to allocate memory for buffer flush.");
            return;
        }

        uint8_t* it = buf;

        memcpy(it, mark_begin, mark_begin_len);
        it += mark_begin_len;

        memcpy(it, buffer, bufferLength);
        it += bufferLength;

        memcpy(it, mark_close, mark_close_len);
        it += mark_close_len;

        Serial.write(buf, it - buf);

        free(buf);
        bufferLength = 0;
        BufferFull = false;

        Serial.println("Buffer flushed to Serial successfully.");
    } else {
        Serial.println("Buffer is empty. Nothing to flush.");
    }
}

bool SDCardModule::logPacket(const uint8_t *packet, uint32_t length) {

    if (BufferFull)
    {
        return false;
    }

    if (!Initlized || !logFile) {
        if (buffer == nullptr) {
            // Allocate initial memory for buffer
            buffer = (uint8_t*)malloc(maxBufferLength);
            if (!buffer) {
                Serial.println("Failed to allocate memory for buffer.");
                return false;
            }
            pcapHeader.network = 105;
            memcpy(buffer, &pcapHeader, sizeof(pcapHeader));
            bufferLength = sizeof(pcapHeader);
        }

        if (bufferLength + sizeof(pcap_packet_header) + length > maxBufferLength) {
           BufferFull = true;
           flushBufferToSerial();
           return false;
        }

        uint32_t ts_sec = millis();
        uint32_t ts_usec = micros();

        pcap_packet_header packetHeader = {
            ts_sec, ts_usec, length, length
        };

        // Copy packet header and packet to buffer
        memcpy(buffer + bufferLength, &packetHeader, sizeof(packetHeader));
        bufferLength += sizeof(packetHeader);

        memcpy(buffer + bufferLength, packet, length);
        bufferLength += length;
        return true;
    }

    // Debugging: Log that SD card is initialized and file is available
    Serial.println("SD card initialized and log file available. Logging to SD card.");

    // Normal logging to SD card
    uint32_t ts_sec = millis();
    uint32_t ts_usec = micros();

    pcap_packet_header packetHeader = {
        ts_sec, ts_usec, length, length
    };

    if (bufferLength + sizeof(packetHeader) + length > maxBufferLength) {
        Serial.println("Buffer limit reached. Flushing log to SD card.");
        flushLog();
    }

    logFile.write((const uint8_t *)&packetHeader, sizeof(packetHeader));
    logFile.write(packet, length);
    bufferLength += sizeof(packetHeader) + length;

    Serial.println("Packet logged to SD card successfully.");
    return true;
}

void SDCardModule::stopPcapLogging() {
    if (logFile) {
        flushLog();
        logFile.close();
    }
    flushBufferToSerial();
}

void SDCardModule::flushLog() {
    if (logFile) {
        logFile.flush();
        bufferLength = 0;
    }
}

bool SDCardModule::writeFile(const char *path, const char *message) {
    File file = SDI->open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }
    if (file.println(message)) {

    } else {
        Serial.println("Write failed");
    }
    file.close();
    return true;
}

File SDCardModule::readFile(const char *path) {
    File file = SDI->open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return file;
    }
    Serial.print("Read from file: ");
    Serial.println(path);
    return file;
}

bool SDCardModule::appendFile(const char *path, const char *message) {
    File file = SDI->open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return false;
    }
    if (file.println(message)) {

    } else {
        Serial.println("Append failed");
    }
    file.close();
    return true;
}

bool SDCardModule::deleteFile(const char *path) {
    if (SDI->remove(path)) {
        return true;
    } else {
        Serial.println("Delete failed");
        return false;
    }
}