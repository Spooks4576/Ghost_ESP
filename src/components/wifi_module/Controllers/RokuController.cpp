#include "RokuController.h"

RokuController::RokuController() {
}


void RokuController::launchApp(const String& appUrl) {

  IPAddress extractedIp;
  uint16_t extractedPort;

  extractIPAndPort(appUrl, extractedIp, extractedPort);

  HttpClient httpc(client, extractedIp, extractedPort);

  httpc.beginRequest();

  String urlPath("/launch/" + String(AppIDToLaunch));

  httpc.post(urlPath.c_str());
  httpc.sendHeader("User-Agent", UserAgent.c_str());
  httpc.endRequest();

  Serial.println("Tried to Launch App");
}

bool RokuController::isRokuDevice(const char* appURL) {
  IPAddress extractedIp;
  uint16_t extractedPort;

  extractIPAndPort(appURL, extractedIp, extractedPort);

  HttpClient httpc(client, extractedIp, extractedPort);

  String path = "/query/device-info";

  httpc.beginRequest();

  httpc.get(path.c_str());
  httpc.sendHeader("User-Agent", UserAgent.c_str());

  httpc.endRequest();

  int httpResponseCode = httpc.responseStatusCode();

  Serial.println("Response IsRokuDevice: " + String(httpResponseCode));

  return (httpResponseCode == 200);
}

void RokuController::ExecuteKeyCommand(const char* AppUrl) {
  const char* CommandStr;

  IPAddress extractedIp;
  uint16_t extractedPort;

  extractIPAndPort(AppUrl, extractedIp, extractedPort);

  HttpClient httpc(client, extractedIp, extractedPort);

  unsigned long startTime = millis();

  while (millis() - startTime < 15000) {

    httpc.beginRequest();

    RokuKeyPress key = getRandomRokuKeyPress();

    switch (key) {
      case RokuKeyPress_UP:
        CommandStr = "up";
        break;
      case RokuKeyPress_DOWN:
        CommandStr = "down";
        break;
      case RokuKeyPress_LEFT:
        CommandStr = "left";
        break;
      case RokuKeyPress_RIGHT:
        CommandStr = "right";
        break;
      case RokuKeyPress_HOME:
        CommandStr = "home";
        break;
      case RokuKeyPress_PLAY:
        CommandStr = "Play";
        break;
    }


    String urlPath("/keypress/" + String(CommandStr));

    httpc.post(urlPath.c_str());
    httpc.sendHeader("User-Agent", UserAgent.c_str());
    httpc.endRequest();

    delay(50);
  }
}


int RokuController::checkAppStatus(const String& appUrl, Device& device) {
  return 0;
}