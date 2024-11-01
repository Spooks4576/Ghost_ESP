#include "vendor/GPS/MicroNMEA.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

// Initialize NMEA instance
void microNMEA_init(MicroNMEA* nmea, char* buf, uint8_t len) {
    nmea->talkerID = '\0';
    memset(nmea->messageID, 0, sizeof(nmea->messageID));
    nmea->buffer = buf;
    nmea->bufferLen = len;
    nmea->ptr = nmea->buffer;
    microNMEA_clear(nmea);
}

// Clear NMEA data
void microNMEA_clear(MicroNMEA* nmea) {
    nmea->navSystem = '\0';
    nmea->numSat = 0;
    nmea->hdop = 255;
    nmea->isValid = false;
    nmea->latitude = 999000000L;
    nmea->longitude = 999000000L;
    nmea->altitude = LONG_MIN;
    nmea->speed = LONG_MIN;
    nmea->course = LONG_MIN;
    nmea->altitudeValid = false;
    nmea->year = nmea->month = nmea->day = 0;
    nmea->hour = nmea->minute = nmea->second = 99;
    nmea->hundredths = 0;
}

// Skip field helper
const char* microNMEA_skipField(const char* s) {
    while (*s && *s != ',' && *s != '*') s++;
    return (*s == ',' || *s == '*') ? ++s : NULL;
}

// Parse unsigned int
unsigned int microNMEA_parseUnsignedInt(const char *s, uint8_t len) {
    unsigned int r = 0;
    while (len--) r = 10 * r + (*s++ - '0');
    return r;
}

// Parse float
long microNMEA_parseFloat(const char* s, uint8_t log10Multiplier, const char** eptr) {
    long r = 0, frac = 0;
    int neg = (*s == '-') ? -1 : 1;
    if (*s == '-' || *s == '+') s++;
    while (isdigit(*s)) r = 10 * r + (*s++ - '0');
    r *= EXP10(log10Multiplier);

    if (*s == '.') {
        s++;
        while (isdigit(*s) && log10Multiplier) {
            frac = 10 * frac + (*s++ - '0');
            log10Multiplier--;
        }
        frac *= EXP10(log10Multiplier);
    }
    r += frac;
    r *= neg;
    *eptr = microNMEA_skipField(s);
    return r;
}

// Parse degree minute
long microNMEA_parseDegreeMinute(const char* s, uint8_t degWidth, const char** eptr) {
    long r = microNMEA_parseUnsignedInt(s, degWidth) * 1000000L;
    s += degWidth;
    r += microNMEA_parseFloat(s, 6, eptr) / 60;
    return r;
}

// Generate checksum
const char* microNMEA_generateChecksum(const char* s, char* checksum) {
    uint8_t c = 0;
    if (*s == '$') s++;
    while (*s && *s != '*') c ^= *s++;
    checksum[0] = (c >> 4) + '0';
    checksum[1] = (c & 0xF) + '0';
    return s;
}

// Test checksum
bool microNMEA_testChecksum(const char* s) {
    char checksum[2];
    const char* p = microNMEA_generateChecksum(s, checksum);
    return *p == '*' && p[1] == checksum[0] && p[2] == checksum[1];
}

// Parse time
const char* microNMEA_parseTime(MicroNMEA* nmea, const char* s) {
    nmea->hour = microNMEA_parseUnsignedInt(s, 2);
    nmea->minute = microNMEA_parseUnsignedInt(s + 2, 2);
    nmea->second = microNMEA_parseUnsignedInt(s + 4, 2);
    nmea->hundredths = microNMEA_parseUnsignedInt(s + 7, 2);
    return microNMEA_skipField(s + 9);
}

// Parse date
const char* microNMEA_parseDate(MicroNMEA* nmea, const char* s) {
    nmea->day = microNMEA_parseUnsignedInt(s, 2);
    nmea->month = microNMEA_parseUnsignedInt(s + 2, 2);
    nmea->year = microNMEA_parseUnsignedInt(s + 4, 2) + 2000;
    return microNMEA_skipField(s + 6);
}

// Process GGA sentence
bool microNMEA_processGGA(MicroNMEA* nmea, const char* s) {
    s = microNMEA_parseTime(nmea, s);
    if (!s) return false;
    nmea->latitude = microNMEA_parseDegreeMinute(s, 2, &s);
    if (!s) return false;
    if (*s == 'S') nmea->latitude *= -1;
    s += 2;
    nmea->longitude = microNMEA_parseDegreeMinute(s, 3, &s);
    if (*s == 'W') nmea->longitude *= -1;
    s += 2;
    nmea->isValid = (*s >= '1' && *s <= '5');
    s += 2;
    nmea->numSat = microNMEA_parseFloat(s, 0, &s);
    nmea->hdop = microNMEA_parseFloat(s, 1, &s);
    nmea->altitude = microNMEA_parseFloat(s, 3, &s);
    nmea->altitudeValid = true;
    return true;
}

// Process RMC sentence
bool microNMEA_processRMC(MicroNMEA* nmea, const char* s) {
    s = microNMEA_parseTime(nmea, s);
    if (!s) return false;
    nmea->isValid = (*s == 'A');
    s += 2;
    nmea->latitude = microNMEA_parseDegreeMinute(s, 2, &s);
    if (*s == 'S') nmea->latitude *= -1;
    s += 2;
    nmea->longitude = microNMEA_parseDegreeMinute(s, 3, &s);
    if (*s == 'W') nmea->longitude *= -1;
    s += 2;
    nmea->speed = microNMEA_parseFloat(s, 3, &s);
    nmea->course = microNMEA_parseFloat(s, 3, &s);
    s = microNMEA_parseDate(nmea, s);
    return true;
}

// Process input character by character
bool microNMEA_process(MicroNMEA* nmea, char c) {
    if (!nmea->buffer || !nmea->bufferLen) return false;
    if (c == '\n' || c == '\r') {
        *nmea->ptr = '\0';
        nmea->ptr = nmea->buffer;

        if (*nmea->buffer == '$' && microNMEA_testChecksum(nmea->buffer)) {
            const char* data;
            if (nmea->buffer[1] == 'G') {
                nmea->talkerID = nmea->buffer[2];
                data = nmea->buffer + 3;
            } else {
                nmea->talkerID = '\0';
                data = nmea->buffer + 1;
            }

            if (strncmp(data, "GGA", 3) == 0)
                return microNMEA_processGGA(nmea, data);
            else if (strncmp(data, "RMC", 3) == 0)
                return microNMEA_processRMC(nmea, data);
        }
        return true;
    } else {
        *nmea->ptr = c;
        if (nmea->ptr < nmea->buffer + nmea->bufferLen - 1)
            nmea->ptr++;
    }
    return false;
}