#include "YoutubeService.h"


String YouTubeService::zx() {
  String result = "";
  const char* characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const int charactersLength = strlen(characters);
  const int stringLength = 12;

  for (int i = 0; i < stringLength; i++) {
    char randomChar = characters[random(0, charactersLength)];
    result += randomChar;
  }

  return result;
}

String YouTubeService::generateUUID() {
  String uuid = "";


  randomSeed(analogRead(0) + millis());

  for (int i = 0; i < 4; i++) {
    uuid += String(random(0xFFFFFFF), HEX);
  }

  return uuid;
}

String YouTubeService::extractJSON(const String& response) {
  int startIndex = response.indexOf("[[");
  if (startIndex == -1) return "";


  int endIndex = response.lastIndexOf("]");


  return response.substring(startIndex, endIndex + 1);
}

void YouTubeService::BindSessionID(Device& device) {
  if (!connectToServer(ServerAddress, port)) return;

  device.UUID = generateUUID();

  String urlParams = "device=REMOTE_CONTROL";
  urlParams += "&mdx-version=3";
  urlParams += "&ui=1";
  urlParams += "&v=2";
  urlParams += "&name=Flipper_0";
  urlParams += "&app=youtube-desktop";
  urlParams += "&loungeIdToken=" + device.YoutubeToken;
  urlParams += "&id=" + device.UUID;
  urlParams += "&VER=8";
  urlParams += "&CVER=1";
  urlParams += "&zx=" + zx();
  urlParams += "&RID=" + String(rid.next());

  String jsonData = "{count: 0 }";

  sendHeaders(ServerAddress, BindEndpoint, jsonData, urlParams, "application/json");

  String response = readResponse();

  DynamicJsonDocument doc(4096);
  Serial.println(response);
  Serial.println(extractJSON(response));
  DeserializationError error = deserializeJson(doc, extractJSON(response));

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  JsonArray array = doc.as<JsonArray>();
  String gsession;
  String SID;
  String LID;

  for (JsonVariant v : array) {
    if (v[0].as<int>() == 0 && v[1][0].as<String>() == "c") {
      SID = v[1][1].as<String>();
    }
    if (v[0].as<int>() == 1 && v[1][0].as<String>() == "S") {
      gsession = v[1][1].as<String>();
    }
    if (v[0].as<int>() == 3 && v[1][0].as<String>() == "playlistModified") {
      LID = v[1][1]["listId"].as<String>();
    }
  }

  Serial.println("gsession: " + gsession);
  Serial.println("SID: " + SID);

  device.gsession = gsession;
  device.SID = SID;
  device.listID = LID;

  secureClient.stop();
}

String YouTubeService::getToken(const String& screenId) {
  if (!connectToServer(ServerAddress, port)) return "";

  String postData = "screen_ids=" + screenId;
  sendHeaders(ServerAddress, GetLoungeTokenEndpoint, postData, "", "application/x-www-form-urlencoded");

  String entireResponse = readResponse();

  int firstNewline = entireResponse.indexOf('\n');
  String statusLine = entireResponse.substring(0, firstNewline);
  int statusCode = statusLine.substring(9, 12).toInt();
  Serial.println("YouTube API response status: " + String(statusCode));

  String responseBody = "";


  int endOfHeaders = entireResponse.indexOf("\n\r\n");
  if (endOfHeaders != -1) {
    responseBody = entireResponse.substring(endOfHeaders + 3);
    Serial.println("YouTube API response content: " + responseBody);
  } else {
    Serial.println("Failed to parse response headers.");
  }


  int startOfJson = responseBody.indexOf('{');
  if (startOfJson != -1) {
    responseBody = responseBody.substring(startOfJson);
  } else {
    Serial.println("Failed to find start of JSON content.");
    return "";
  }

  firstNewline = entireResponse.indexOf('\n');
  statusLine = entireResponse.substring(0, firstNewline);
  statusCode = statusLine.substring(9, 12).toInt();
  Serial.println("YouTube API response status: " + String(statusCode));

  if (statusCode == 200) {

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, responseBody);
    String loungeToken = doc["screens"][0]["loungeToken"].as<String>();
    Serial.println("Lounge Token: " + loungeToken);
    secureClient.stop();
    return loungeToken;
  } else {
    Serial.println("Failed to retrieve token. HTTP Response Code: " + String(statusCode));
    return "";
  }
}

void YouTubeService::sendCommand(const String& command, const String& videoId, const Device& device) {
    if (!connectToServer(ServerAddress, port)) return;

    String urlParams = "device=REMOTE_CONTROL";
    urlParams += "&loungeIdToken=" + device.YoutubeToken;
    urlParams += "&id=" + device.UUID;
    urlParams += "&VER=8";
    urlParams += "&zx=" + zx();
    urlParams += "&SID=" + device.SID;
    urlParams += "&RID=" + String(rid.next());
    urlParams += "&AID=" + String("5");
    urlParams += "&gsessionid=" + device.gsession;

    String formData;
    formData += "count=1";
    formData += "&ofs=0";
    formData += "&req0__sc=" + command;
    formData += "&req0_videoId=" + videoId;
    formData += "&req0_listId=" + device.listID;

    sendHeaders(ServerAddress, BindEndpoint, formData, urlParams, "application/x-www-form-urlencoded");

    String response = readResponse();

    Serial.println("Set playlist Response");
    Serial.println(response);
}


