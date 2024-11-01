#ifndef MICRONMEA_H
#define MICRONMEA_H

#include <stdbool.h>
#include <stdint.h>

#define EXP10(b) ((b) ? 10 * EXP10((b)-1) : 1)

// Struct definition for MicroNMEA
typedef struct {
    char talkerID;
    char messageID[4];
    char* buffer;
    int bufferLen;
    char* ptr;
    char navSystem;
    bool isValid;
    bool altitudeValid;
    long latitude;
    long longitude;
    long altitude;
    long speed;
    long course;
    unsigned int numSat;
    unsigned int hdop;
    unsigned int year, month, day;
    unsigned int hour, minute, second, hundredths;
} MicroNMEA;

// Function prototypes
void microNMEA_init(MicroNMEA* nmea, char* buf, uint8_t len);
void microNMEA_clear(MicroNMEA* nmea);
const char* microNMEA_skipField(const char* s);
unsigned int microNMEA_parseUnsignedInt(const char *s, uint8_t len);
long microNMEA_parseFloat(const char* s, uint8_t log10Multiplier, const char** eptr);
long microNMEA_parseDegreeMinute(const char* s, uint8_t degWidth, const char** eptr);
bool microNMEA_process(MicroNMEA* nmea, char c);
const char* microNMEA_generateChecksum(const char* s, char* checksum);
bool microNMEA_testChecksum(const char* s);
const char* microNMEA_parseTime(MicroNMEA* nmea, const char* s);
const char* microNMEA_parseDate(MicroNMEA* nmea, const char* s);
bool microNMEA_processGGA(MicroNMEA* nmea, const char* s);
bool microNMEA_processRMC(MicroNMEA* nmea, const char* s);

#endif // MICRONMEA_H