#pragma once
#include "../Controllers/AppController.h"
#include "../Controllers/NetflixController.h"
#include "../Controllers/YoutubeController.h"
#include "../Controllers/RokuController.h"
#include <ArduinoHttpClient.h>
#include <set>
#include <vector>

class DIALClient {
public:
    DIALClient(const char* YUrl, const char* ssid, const char* password, AppController* appHandler)
        : ssid(ssid), password(password), appHandler(appHandler), YTurl(YUrl) {}

    void connectWiFi();
    void Execute();
    String getDialApplicationUrl(const String& locationUrl);
    std::vector<Device> discoverDevices();
    void exploreNetwork();
    bool parseSSDPResponse(const String& response, Device& device);
    String concatenatePaths(const String& base, const String& appUrl);
    String extractApplicationURL(HttpClient &httpc);
    bool fetchScreenIdWithRetries(const String& applicationUrl, Device& device, YoutubeController* YTController);
public:
    String ssid, password, YTurl;
    AppController* appHandler;
    WiFiUDP multicastClient;
    bool ShouldRokuKeySpam;
};