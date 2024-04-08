#pragma once

#include "board_config.h" 

#ifdef SD_CARD_CS_PIN

#include <Arduino.h>
#include <SD.h>
#include <SD_MMC.h>


struct pcap_global_header {
    uint32_t magic_number;   // Magic number
    uint16_t version_major;  // Major version number
    uint16_t version_minor;  // Minor version number
    int32_t thiszone;        // GMT to local correction
    uint32_t sigfigs;        // Accuracy of timestamps
    uint32_t snaplen;        // Max length of captured packets, in octets
    uint32_t network;        // Data link type
};

// Pcap Packet Header Format
struct pcap_packet_header {
    uint32_t ts_sec;         // Timestamp seconds
    uint32_t ts_usec;        // Timestamp microseconds
    uint32_t incl_len;       // Number of octets of packet saved in file
    uint32_t orig_len;       // Actual length of packet
};

class SDCardModule {
public:
    SDCardModule();
    bool init();
    bool writeFile(const char *path, const char *message);
    bool readFile(const char *path);
    bool appendFile(const char *path, const char *message);
    bool deleteFile(const char *path);
    bool logMessage(const char *logFileName, const char* foldername, String message);
    bool startPcapLogging(const char *path,bool bluetooth = false);
    bool logPacket(const uint8_t *packet, uint32_t length);
    void flushLog();
    void stopPcapLogging();
    FS* SDI;
private:
    int csPin;
    int BootNum;
    bool Initlized;
    bool IsMMCCard;
    int PcapFileIndex;
    File logFile;
    unsigned long bufferLength = 0;
    const unsigned long maxBufferLength = 512;
    pcap_global_header pcapHeader = {
        0xa1b2c3d4, // Magic number for pcap
        2, 4, // Version major and minor
        0, 0, // thiszone, sigfigs
        65535, // snaplen
        1 // Network - assuming Ethernet
    };
    FS* createFileSystem(bool isMMCCard) {
        if (isMMCCard) {
            return &SD_MMC;
        } else {
            return &SD;
        }
    }
};

#endif // SD_CARD_CS_PIN