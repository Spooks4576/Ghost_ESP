#include "YoutubeController.h"
#include "core/globals.h"


void YoutubeController::launchApp(const String& appUrl) {
  int startPos = appUrl.indexOf('/', 7);
  String basePath = (startPos != -1) ? appUrl.substring(startPos) : "/";

  String youtubePath = basePath + "YouTube";

  IPAddress extractedIp;
  uint16_t extractedPort;
  extractIPAndPort(appUrl, extractedIp, extractedPort);

  HttpClient httpc(client, extractedIp, extractedPort);

  httpc.beginRequest();

  int httpCode = httpc.post(youtubePath);
  httpc.sendHeader("User-Agent", UserAgent.c_str());
  httpc.sendHeader("Origin", "https://www.youtube.com");
  httpc.endRequest();

  if (httpCode == 201) {
    Serial.println(F("Successfully launched the YouTube app."));
    LOG_MESSAGE_TO_SD("Successfully launched the YouTube app.");
  } else {
    Serial.println("Failed to launch the YouTube app. HTTP Response Code: " + String(httpCode));
    LOG_MESSAGE_TO_SD("Failed to launch the YouTube app. HTTP Response Code: " + String(httpCode));
  }
}

int YoutubeController::checkAppStatus(const String& appUrl, Device& device_I) {
    IPAddress extractedIp;
    uint16_t extractedPort;
    extractIPAndPort(appUrl, extractedIp, extractedPort);

    int startPos = appUrl.indexOf('/', 7);
    String basePath = (startPos != -1) ? appUrl.substring(startPos) : "/";
    String youtubePath = basePath + "YouTube";

    Serial.println("[checkYouTubeAppStatus] Connecting to IP: " + extractedIp.toString() + " Port: " + String(extractedPort));
    LOG_MESSAGE_TO_SD("[checkYouTubeAppStatus] Connecting to IP: " + extractedIp.toString() + " Port: " + String(extractedPort));
    Serial.println("[checkYouTubeAppStatus] Trying to access: " + youtubePath);
    LOG_MESSAGE_TO_SD("[checkYouTubeAppStatus] Trying to access: " + youtubePath);

    HttpClient httpc(client, extractedIp, extractedPort);
    httpc.beginRequest();

    int resultCode = httpc.get(youtubePath);
    httpc.sendHeader("User-Agent", UserAgent.c_str());
    httpc.sendHeader("Origin", "https://www.youtube.com");
    httpc.endRequest();

    if (resultCode == HTTP_SUCCESS) {
        Serial.println(F("Successfully connected and received a response."));
        LOG_MESSAGE_TO_SD("Successfully connected and received a response.");
        int responseCode = httpc.responseStatusCode();
        Serial.println("[checkYouTubeAppStatus] HTTP Response Code: " + String(responseCode));
        LOG_MESSAGE_TO_SD("[checkYouTubeAppStatus] HTTP Response Code: " + String(responseCode));
        
        if (responseCode == 200) {
            Serial.println("Got Sucessful Response");
            LOG_MESSAGE_TO_SD("Got Sucessful Response");

            String responseBody = httpc.responseBody();
            if (responseBody.indexOf("<state>running</state>") != -1) {
                device_I.screenID = extractScreenId(responseBody);
                Serial.println("YouTube app is running.");
                LOG_MESSAGE_TO_SD("YouTube app is running.");
                return responseCode;
            } else if (responseBody.indexOf("<state>stopped</state>") != -1) {
                Serial.println(F("YouTube app is not running."));
                LOG_MESSAGE_TO_SD("YouTube app is not running.");
                responseCode = 404;
                return responseCode;
            } else {
                Serial.println(F("Unable to determine the status of the YouTube app."));
                LOG_MESSAGE_TO_SD("Unable to determine the status of the YouTube app.");
                responseCode = 500;
                return responseCode;
            }
        } else if (responseCode == 404) {
            Serial.println(F("YouTube app is not running."));
            LOG_MESSAGE_TO_SD("YouTube app is not running.");
            return responseCode;
        } else {
            Serial.println("Received unexpected HTTP Response Code: " + String(responseCode));
            LOG_MESSAGE_TO_SD("Received unexpected HTTP Response Code: " + String(responseCode));
            return responseCode;
        }
    } else {
        Serial.println("Failed to establish a connection or send the request. Result code: " + String(resultCode));
        LOG_MESSAGE_TO_SD("Failed to establish a connection or send the request. Result code: " + String(resultCode));
        return resultCode;
    }
}