#pragma once

#ifdef HAS_GPS
#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#endif
#include <core/system_manager.h>


class gps_module
{
public:
#ifdef HAS_GPS
    TinyGPSPlus gps;
#endif
    File MapData;
    bool Initilized;
    bool Stop;
    unsigned long lastUpdate = 0;
    long updateInterval = 5000;

    void setup();
    void streetloop();
    String findClosestStreet(double lat, double lng);
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    double degToRad(double degrees);
};

