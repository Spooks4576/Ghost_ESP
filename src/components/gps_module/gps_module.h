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
    long updateInterval = 1000;

    void WarDrivingLoop();
    void setup();
    void streetloop();
    String GetDateAndTime();
    String findClosestStreet(double lat, double lng);
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);
    double degToRad(double degrees);
};

