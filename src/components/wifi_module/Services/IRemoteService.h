#pragma once
#include <WiFiClientSecure.h>
#include <Arduino.h>


struct Device {
  String uniqueServiceName;
  String location;
  String applicationUrl;
  String friendlyName;
  String wakeup;
  String screenID;
  String YoutubeToken;
  String gsession;
  String SID;
  String UUID;
  String listID;
  IPAddress DeviceIP;
  uint16_t Port;
};

class IRemoteService {
public:
  virtual void BindSessionID(Device& device) = 0;
  virtual void sendCommand(const String& command, const String& videoId, const Device& device) = 0;
  virtual String getToken(const String& id) = 0;

public:
  WiFiClientSecure secureClient;

  bool connectToServer(const char* serverAddress, const int port) {
    if (!secureClient.connect(serverAddress, port)) {
      Serial.println("Connection failed!");
      return false;
    }
    return true;
  }

  void sendHeaders(const char* serverAddress, const String& endpoint, const String& data, const String& URLParams, const char* contentType) {
    secureClient.print("POST " + String(endpoint) + "?" + URLParams + " HTTP/1.1\r\n");
    secureClient.print("Host: " + String(serverAddress) + "\r\n");
    secureClient.print(F("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.45 Safari/537.36\r\n"));
    secureClient.print("Content-Type: " + String(contentType) + "\r\n");
    secureClient.print("Content-Length: " + String(data.length()) + "\r\n");
    secureClient.print(F("Origin: https://www.youtube.com\r\n"));
    secureClient.print("\r\n");
    secureClient.print(data);
  }

  String readResponse() {
    while (!secureClient.available()) {
      delay(10);
    }
    return secureClient.readString();
  }
};
