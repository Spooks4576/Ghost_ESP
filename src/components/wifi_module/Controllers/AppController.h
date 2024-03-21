#pragma once
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include "../Services/IRemoteService.h"
#include <Arduino.h>

enum class HandlerType {
    Base,
    YoutubeController,
    NetflixController,
    RokuController
};

inline String UserAgent = F("Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Mobile Safari/537.36");

class AppController {
public:

  WiFiClient client;


  virtual ~AppController() = default;


  virtual void launchApp(const String& appUrl) = 0;

  virtual HandlerType getType() const { return HandlerType::Base; }


  virtual int checkAppStatus(const String& appUrl, Device& device) = 0;

  void extractIPAndPort(const String& appUrl, IPAddress& ip, uint16_t& port) {
    int portStartIndex = appUrl.lastIndexOf(':');
    if (portStartIndex != -1 && portStartIndex < appUrl.length() - 1) {
        String portStr = appUrl.substring(portStartIndex + 1);
        port = portStr.toInt();
    }

    String ipStr = appUrl.substring(7, portStartIndex);
    if (!ip.fromString(ipStr)) {
        Serial.println(F("Failed to parse IP"));
    }
  }
};
