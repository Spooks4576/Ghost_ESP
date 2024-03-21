#include "ESPmDNSHelper.h"
#include "CastChannel.h"

ESPmDNSHelper::ESPmDNSHelper(const char* inSsid, const char* inpaSsword, const char* Target, const char* url, const char* Appid) {
  mdns = new MDNSResponder();
  ssid = inSsid;
  password = inpaSsword;
  if (Target != "") {
    TargetIP = Target;
  } else {
    TargetIP = "";
  }

  if (url != "") {
    TargetURL = url;
  } else {
    TargetURL = "";
  }

  if (Appid != "") {
    AppID = Appid;
  }
  connectWiFi();
}

void LoadMedia(Channel MediaChannel, String contentId, String contentType, String title, String imageUrl, bool autoplay = false, float currentTime = 0.0, String repeatMode = "REPEAT_OFF") {

  StaticJsonDocument<600> mediaDoc;
  mediaDoc["contentId"] = contentId.c_str();
  mediaDoc["contentType"] = contentType.c_str();
  mediaDoc["streamType"] = "BUFFERED";
  JsonObject metadata = mediaDoc.createNestedObject("metadata");
  metadata["type"] = 0;
  metadata["metadataType"] = 0;
  metadata["title"] = title.c_str();
  JsonArray images = metadata.createNestedArray("images");
  JsonObject image1 = images.createNestedObject();
  image1["url"] = imageUrl.c_str();

  StaticJsonDocument<700> loadDoc;
  loadDoc["type"] = "LOAD";
  loadDoc["autoplay"] = autoplay;
  loadDoc["currentTime"] = currentTime;
  loadDoc["repeatMode"] = repeatMode.c_str();
  loadDoc["media"] = mediaDoc;
  loadDoc["requestId"] = 1;
  loadDoc.createNestedArray("activeTrackIds");

  String loadString;
  serializeJson(loadDoc, loadString);
  Serial.println(loadString);
  MediaChannel.send(loadString);
}

void ESPmDNSHelper::HandleCloseConnection() {
  Channel ConnectChannel(SSLClient, "sender-0", CurrentSessionID, "urn:x-cast:com.google.cast.tp.connection", "JSON", this);

  StaticJsonDocument<200> doc2;
  String CloseString;
  doc2["type"] = "CLOSE";
  serializeJson(doc2, CloseString);

  ConnectChannel.send(CloseString);
}

void ESPmDNSHelper::HandleMessage(String Session, String Data) {
  SessionIDIndex++;
  if (!Session.isEmpty()) {

    CurrentSessionID = Session;

    Channel ConnectChannel(SSLClient, "sender-0", Session, "urn:x-cast:com.google.cast.tp.connection", "JSON", this);
    Channel MediaChannel(SSLClient, "sender-0", Session, "urn:x-cast:com.google.youtube.mdx", "JSON", this);

    StaticJsonDocument<200> doc1;

    doc1["type"] = "CONNECT";

    String CONNECTSTRING;
    serializeJson(doc1, CONNECTSTRING);

    ConnectChannel.send(CONNECTSTRING);


    delay(1000);


    StaticJsonDocument<200> doc2;
    String CONNECTSTRING2;
    doc2["type"] = "getMdxSessionStatus";
    serializeJson(doc2, CONNECTSTRING2);

    MediaChannel.send(CONNECTSTRING2);

    SessionIDIndex = 0;
  }
}

ESPmDNSHelper::~ESPmDNSHelper() {
}

bool ESPmDNSHelper::initialize(const char* hostName) {
  if (mdns->begin(hostName)) {
    Serial.println(F("MDNS responder started"));
    return true;
  }
  return false;
}

void ESPmDNSHelper::queryServices(const char* serviceType, const char* proto, uint32_t timeout) {
  int n = mdns->queryService(serviceType, proto);
  if (n == 0) {
    Serial.println(F("no services found"));
  } else {
    for (int i = 0; i < n; ++i) {
      String serviceName = mdns->hostname(i);
      IPAddress serviceIP = mdns->IP(i);
      uint16_t servicePort = mdns->port(i);

      Serial.println("Service Name: " + serviceName);
      Serial.println("Service IP: " + serviceIP.toString());
      Serial.println("Service Port: " + String(servicePort));
      Serial.println("----------------------------");

      if (TargetIP != "") {
        if (initializeClient(TargetIP, servicePort)) {
          SendAuth();
          break;
        }
      } else {
        if (initializeClient(serviceIP.toString().c_str(), servicePort)) {
          SendAuth();
        }
      }
    }
  }
  delay(timeout);
}

void ESPmDNSHelper::connectWiFi() {
  Serial.println("[connectWiFi] Initiating WiFi connection.");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(F("[connectWiFi] Connecting to WiFi..."));
  }
  Serial.println(F("[connectWiFi] Connected to WiFi."));

  if (initialize("ESP32S2-Device")) {
    queryServices("_googlecast", "_tcp", 2000);
  }
}

void ESPmDNSHelper::SendAuth() {
  Channel ConnectChannel(SSLClient, "sender-0", "receiver-0", "urn:x-cast:com.google.cast.tp.connection", "JSON", this);
  Channel HeartBeat(SSLClient, "sender-0", "receiver-0", "urn:x-cast:com.google.cast.tp.heartbeat", "JSON", this);
  Channel RecieverChannel(SSLClient, "sender-0", "receiver-0", "urn:x-cast:com.google.cast.receiver", "JSON", this);
  StaticJsonDocument<200> Heartbeatdoc;

  Heartbeatdoc["type"] = "PING";

  String HeartBeatString;
  serializeJson(Heartbeatdoc, HeartBeatString);


  StaticJsonDocument<200> doc1;

  doc1["type"] = "CONNECT";

  String CONNECTSTRING;
  serializeJson(doc1, CONNECTSTRING);

  ConnectChannel.send(CONNECTSTRING);


  HeartBeat.send(HeartBeatString);

  StaticJsonDocument<200> doc2;
  doc2["type"] = "LAUNCH";
  doc2["appId"] = AppID;
  doc2["requestId"] = 1;
  String LaunchString;
  serializeJson(doc2, LaunchString);
  RecieverChannel.send(LaunchString);


  delay(2000);

  StaticJsonDocument<200> doc3;
  doc3["type"] = "SET_VOLUME";
  JsonObject volume = doc3.createNestedObject("volume");


  volume["level"] = 1;

  doc3["requestId"] = 1;

  String VolumeString;
  serializeJson(doc3, VolumeString);

  RecieverChannel.send(VolumeString);


  if (TargetURL != "") {
    RecieverChannel.YTUrl = String(TargetURL);
  } else {
    RecieverChannel.YTUrl = "dQw4w9WgXcQ";
  }


  while (SSLClient) {
    RecieverChannel.checkForMessages();
    delay(700);
  }

  Serial.println(F("Finished checking for messages."));
}



bool ESPmDNSHelper::initializeClient(const char* _host, uint16_t _port) {
  host = _host;
  port = _port;

  SSLClient.setClient(&unsecureclient);

  SSLClient.allowSelfSignedCerts();

  SSLClient.setDebugLevel(4);

  if (SSLClient.connect(host.c_str(), port)) {

    Serial.println("Connected");

    SSLClient.setInsecure();

    SSLClient.connectSSL();

    return true;
  } else {
    Serial.println(F("Failed to connect securely. Entirely"));
    return false;
  }
}