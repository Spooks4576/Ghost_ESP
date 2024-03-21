#pragma once

#include <WiFiClientSecure.h>
#include <ESP_SSLClient.h>
#include <ArduinoJson.h>
#include "CastSerializer.h"
#include <ArduinoHttpClient.h>
#include "../Controllers/YoutubeController.h"
#include "ESPmDNSHelper.h"
#include <functional>

#define MAX_BUFFER_SIZE 1024

int threshold = 50;
const uint8_t MAX_STRING_SIZE = 100;

byte hexCharToByte(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

bool isValidSessionId(const String& sessionId) {
  return (sessionId.length() == 36) && (sessionId[8] == '-') && (sessionId[13] == '-') && (sessionId[18] == '-') && (sessionId[23] == '-');
}

String bufferToHexString(const uint8_t* buffer, uint16_t length) {
  String result = "";
  for (uint16_t i = 0; i < length; i++) {
    char hex[3];  // Two characters for the byte and one for the null terminator
    sprintf(hex, "%02X", buffer[i]);
    result += hex;
  }
  return result;
}


void hexStringToBytes(const String& hex, byte* buffer, int bufferSize) {
  int byteCount = 0;
  for (int i = 0; i < hex.length() && byteCount < bufferSize; i += 2) {
    char c1 = hex.charAt(i);
    char c2 = hex.charAt(i + 1);
    buffer[byteCount] = hexCharToByte(c1) * 16 + hexCharToByte(c2);
    byteCount++;
  }
}

uint16_t hexStringToBytes_U(const String& hexString, uint8_t* buffer, uint16_t bufferSize) {
  uint16_t byteCount = 0;
  for (uint16_t i = 0; i < hexString.length() && byteCount < bufferSize; i += 2) {
    char c1 = hexString[i];
    char c2 = hexString[i + 1];
    buffer[byteCount] = (hexCharToByte(c1) << 4) | hexCharToByte(c2);
    byteCount++;
  }
  return byteCount;
}

class Channel {
private:
  BSSL_TCP_Client& client;
  WiFiClient unsecureclient;
  WiFiClientSecure secureClient;
  String sourceId;
  String destinationId;
  String namespace_;
  String encoding;
  ESPmDNSHelper* Parent;

public:
  String YTUrl;
  Channel(BSSL_TCP_Client& clientRef, String srcId, String destId, String ns, String enc = "", ESPmDNSHelper* InParent = nullptr)
    : client(clientRef), sourceId(srcId), destinationId(destId), namespace_(ns), encoding(enc), Parent(InParent) {}

  String Deserialize_Internal(String serializedData) {
    uint8_t* buffer = new uint8_t[1000];
    uint16_t byteCount = hexStringToBytes_U(serializedData, buffer, 1000);

    char* payloadUtf8 = new char[1000];
    ExpandedCastMessageSerializer::DeserializationResult result = ExpandedCastMessageSerializer::deserialize(buffer, byteCount, payloadUtf8);

    String returnString = "";
    if (result == ExpandedCastMessageSerializer::DESERIALIZATION_SUCCESS) {
      returnString = String(payloadUtf8);
    } else {
      Serial.println("Failed to Deserialize");
    }

    delete[] buffer;
    delete[] payloadUtf8;

    return returnString;
  }



  String Seralize_Internal(String Data) {

    ExpandedCastMessageSerializer::CastMessage message;
    message.protocol_version = ExpandedCastMessageSerializer::CASTV2_1_0;
    strncpy(message.source_id, sourceId.c_str(), MAX_STRING_SIZE - 1);
    message.source_id[MAX_STRING_SIZE - 1] = '\0';  // Ensure null-termination
    strncpy(message.destination_id, destinationId.c_str(), MAX_STRING_SIZE - 1);
    message.destination_id[MAX_STRING_SIZE - 1] = '\0';  // Ensure null-termination
    strncpy(message.namespace_, namespace_.c_str(), MAX_STRING_SIZE - 1);
    message.namespace_[MAX_STRING_SIZE - 1] = '\0';  // Ensure null-termination
    strncpy(message.payload_utf8, Data.c_str(), 500 - 1);
    message.payload_utf8[500 - 1] = '\0';  // Ensure null-termination
    message.payload_type = ExpandedCastMessageSerializer::STRING;
    memset(message.payload_binary, 0, 100);
    message.payload_binary_size = 0;

    Serial.println("Input Data length: " + String(Data.length()));

    uint8_t buffer[1000];
    uint16_t index = 0;
    ExpandedCastMessageSerializer::SerializationResult result = ExpandedCastMessageSerializer::serialize(message, buffer, index, sizeof(buffer));

    if (result == ExpandedCastMessageSerializer::SUCCESS) {

      String hex = bufferToHexString(buffer, index);
      return hex;
    } else {
      Serial.println("Serialization Error Occured");
      return "";
    }
  }

  void checkForMessages() {

    if (client.available() > 0) {
      const int maxBytesToRead = 500;
      byte buffer[maxBytesToRead];
      int bytesRead = client.read(buffer, min(client.available(), maxBytesToRead));

      String serializedData = "";

      for (int i = 0; i < bytesRead; i++) {
        if (buffer[i] < 16) serializedData += '0';
        serializedData += String(buffer[i], HEX);
      }

      if (bytesRead > 0) {
        Serial.println("Received data: " + serializedData);
        String jsonData = Deserialize_Internal(serializedData);

        onMessage(sourceId, destinationId, namespace_, jsonData);

        if (jsonData != "") {
          Serial.println("Deserialized data: " + jsonData);
        } else {
          Serial.println(F("Failed to deserialize data or received empty response."));
        }
      } else {
        Serial.println(F("Received data length is not appropriate."));
      }
    }
  }


  void onMessage(String srcId, String destId, String ns, String data) {
    // Check if the data is a valid session ID first
    if (data != "" && isValidSessionId(data)) {
      Serial.println("Session ID: " + data);
      Serial.println("About to Do Message Callback");
      Parent->HandleMessage(data, data);
      Serial.println("Finished Callback Call");
    }

    else if (data != "") {
      // Parse the JSON data
      DynamicJsonDocument doc(700);
      DeserializationError error = deserializeJson(doc, data);


      if (!error && doc["data"].containsKey("screenId")) {
        String screenId = doc["data"]["screenId"].as<String>();
        Serial.println("Found screenId: " + screenId);

        Parent->HandleCloseConnection();

        delay(500);  // short delay to make sure the connection actually closes

        client.stop();

        YoutubeController* YtController = new YoutubeController();

        YtController->YTService.secureClient.setInsecure();

        Device ccdevice;

        String Token = YtController->YTService.getToken(screenId);

        ccdevice.screenID = screenId;
        ccdevice.YoutubeToken = Token;

        YtController->YTService.BindSessionID(ccdevice);

        String url = YTUrl.isEmpty() ? "JWjoL5EgWms" : YTUrl;

        YtController->YTService.sendCommand("setPlaylist", url.c_str(), ccdevice);

        delay(500);

        delete YtController;

      } else {
        Serial.println("Other valid data received: " + data);
      }
    } else {
      Serial.println(F("deviceId not found in nested JSON"));
    }
  }

  void send(String Data) {
    String serializedData = Seralize_Internal(Data);
    byte buffer[4 + serializedData.length() / 2];


    uint32_t dataLength = serializedData.length() / 2;
    buffer[0] = (dataLength >> 24) & 0xFF;
    buffer[1] = (dataLength >> 16) & 0xFF;
    buffer[2] = (dataLength >> 8) & 0xFF;
    buffer[3] = dataLength & 0xFF;


    hexStringToBytes(serializedData, buffer + 4, dataLength);

    client.write(buffer, sizeof(buffer));
  }
};