#include "Dial.h"

const int MAX_RETRIES = 50;
const int RETRY_DELAY = 500;

String extractPathFromURL(const String& url) {
  int doubleSlashPos = url.indexOf("//");


  if (doubleSlashPos == -1) {
    doubleSlashPos = 0;
  } else {
    doubleSlashPos += 2;
  }

  int slashAfterHost = url.indexOf('/', doubleSlashPos);


  if (slashAfterHost == -1) {
    return "/";
  }

  return url.substring(slashAfterHost);
}

bool DIALClient::fetchScreenIdWithRetries(const String& applicationUrl, Device& device, YoutubeController* YTController) {
  for (int i = 0; i < MAX_RETRIES; i++) {
    YTController->checkAppStatus(applicationUrl, device);

    if (!device.screenID.isEmpty()) {
      String Token = YTController->YTService.getToken(device.screenID);
      device.YoutubeToken = Token;
      YTController->YTService.BindSessionID(device);
      return true;
    } else {
      Serial.println("Screen ID is Empty. Retrying...");
      delay(RETRY_DELAY);
    }
  }

  Serial.println(F("Failed to fetch Screen ID after max retries."));
  return false;
}


void DIALClient::connectWiFi() {
  Serial.println("[connectWiFi] Initiating WiFi connection.");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("[connectWiFi] Connecting to WiFi...");
  }
  Serial.println("[connectWiFi] Connected to WiFi.");

  if (appHandler->getType() == HandlerType::YoutubeController) {
    YoutubeController* YTHandler = static_cast<YoutubeController*>(appHandler);
    YTHandler->YTService.secureClient.setInsecure();
  }

  exploreNetwork();
}


void DIALClient::Execute() {
  connectWiFi();
}


std::vector<Device> DIALClient::discoverDevices() {
  const int MAX_RETRIES = 10;
  const int RETRY_DELAY = 2000;

  std::vector<Device> devices;
  std::set<String> discoveredUSNs;

  const uint8_t mSearchRequest[] = {
    0x4d, 0x2d, 0x53, 0x45, 0x41, 0x52, 0x43, 0x48, 0x20, 0x2a, 0x20, 0x48, 0x54, 0x54, 0x50, 0x2f,
    0x31, 0x2e, 0x31, 0x0d, 0x0a, 0x48, 0x4f, 0x53, 0x54, 0x3a, 0x20, 0x32, 0x33, 0x39, 0x2e, 0x32,
    0x35, 0x35, 0x2e, 0x32, 0x35, 0x35, 0x2e, 0x32, 0x35, 0x30, 0x3a, 0x31, 0x39, 0x30, 0x30, 0x0d,
    0x0a, 0x4d, 0x41, 0x4e, 0x3a, 0x20, 0x22, 0x73, 0x73, 0x64, 0x70, 0x3a, 0x64, 0x69, 0x73, 0x63,
    0x6f, 0x76, 0x65, 0x72, 0x22, 0x0d, 0x0a, 0x53, 0x54, 0x3a, 0x20, 0x75, 0x72, 0x6e, 0x3a, 0x64,
    0x69, 0x61, 0x6c, 0x2d, 0x6d, 0x75, 0x6c, 0x74, 0x69, 0x73, 0x63, 0x72, 0x65, 0x65, 0x6e, 0x2d,
    0x6f, 0x72, 0x67, 0x3a, 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x3a, 0x64, 0x69, 0x61, 0x6c,
    0x3a, 0x31, 0x0d, 0x0a, 0x4d, 0x58, 0x3a, 0x20, 0x33, 0x0d, 0x0a, 0x0d, 0x0a
  };

  int retries = 0;
  while (devices.empty() && retries < MAX_RETRIES) {
    if (retries > 0) {
      Serial.println("[discoverDevices] Retrying device discovery...");
      delay(RETRY_DELAY);
    }

    multicastClient.beginMulticast(IPAddress(239, 255, 255, 250), 1900);


    for (int i = 0; i < 5; i++) {
      multicastClient.beginPacket(IPAddress(239, 255, 255, 250), 1900);
      multicastClient.write((const uint8_t*)mSearchRequest, sizeof(mSearchRequest));
      multicastClient.endPacket();
      delay(500);
    }

    multicastClient.write(mSearchRequest, sizeof(mSearchRequest));

    unsigned long startTime = millis();
    while (millis() - startTime < 5000) {
      if (multicastClient.parsePacket()) {
        int len = multicastClient.available();
        char buffer[len + 1];
        multicastClient.read(buffer, len);
        buffer[len] = '\0';

        String response = String(buffer);

        Device device;
        if (parseSSDPResponse(response, device)) {
          if (discoveredUSNs.find(device.uniqueServiceName) == discoveredUSNs.end()) {

            devices.push_back(device);
            discoveredUSNs.insert(device.uniqueServiceName);
          }
        }
      }
    }
    retries++;
  }


  if (devices.empty()) {
    Serial.println(F("[discoverDevices] No devices found after all retries."));
  }

  return devices;
}


void DIALClient::exploreNetwork() {
  String yturl = YTurl != "" ? YTurl : "dQw4w9WgXcQ";

  const int maxRetries = 3;

  for (int attempt = 0; attempt < maxRetries; ++attempt) {
    std::vector<Device> devices = discoverDevices();

    if (devices.empty()) {
      Serial.println(F("No devices discovered. Retrying..."));
      continue;
    }

    for (Device& device : devices) {
      Serial.println("Discovered Device at URL: " + device.location);
      Serial.println(device.friendlyName.c_str());
      Serial.println(device.wakeup.c_str());


      device.applicationUrl = getDialApplicationUrl(device.location);

      if (!device.applicationUrl.isEmpty()) {
        if (appHandler->getType() == HandlerType::YoutubeController) {
          YoutubeController* YTHandler = static_cast<YoutubeController*>(appHandler);
          if (appHandler->checkAppStatus(device.applicationUrl, device) != 200) {
            Serial.println(F("Launching Youtube App"));
            appHandler->launchApp(device.applicationUrl);
          }

          unsigned long startTime = millis();
          bool isYouTubeRunning = false;

          while (millis() - startTime < 5000) {
            if (YTHandler->checkAppStatus(device.applicationUrl, device) == 200) {
              isYouTubeRunning = true;
              break;
            }
            delay(100);
          }

          if (isYouTubeRunning) {
            if (fetchScreenIdWithRetries(device.applicationUrl, device, YTHandler)) {
              YTHandler->YTService.sendCommand("setPlaylist", yturl.c_str(), device);
            }
          } else {
            Serial.println("Timeout reached. YouTube app is not running.");
          }
        } else if (appHandler->getType() == HandlerType::RokuController) {
          RokuController* RKHandler = static_cast<RokuController*>(appHandler);

          if (RKHandler->isRokuDevice(device.applicationUrl.c_str())) {
            bool KeySpam = ShouldRokuKeySpam;

            if (KeySpam) {

              RKHandler->ExecuteKeyCommand(device.applicationUrl.c_str());
            } else {
              RKHandler->launchApp(device.applicationUrl.c_str());
            }
          }
        } else if (appHandler->getType() == HandlerType::NetflixController) {
          Serial.println("Launching App");
          appHandler->launchApp(device.applicationUrl);
        }
        Serial.println(device.uniqueServiceName);
      }
    }
  }
}


bool DIALClient::parseSSDPResponse(const String& response, Device& device) {
  Serial.println(F("[parseSSDPResponse] Parsing SSDP response."));

  if (response.length() == 0) {
    return false;
  }


  device.location = "";
  device.uniqueServiceName = "";
  device.wakeup = "";


  int idx = 0;
  while (idx < response.length()) {
    int nextIdx = response.indexOf('\n', idx);
    if (nextIdx == -1) {
      nextIdx = response.length();
    }

    String line = response.substring(idx, nextIdx);
    idx = nextIdx + 1;

    line.trim();

    if (line.startsWith("LOCATION:")) {
      device.location = line.substring(9);
      device.location.trim();
    } else if (line.startsWith("USN:")) {
      device.uniqueServiceName = line.substring(4);
      device.uniqueServiceName.trim();
    } else if (line.startsWith("WAKEUP:")) {
      device.wakeup = line.substring(7);
      device.wakeup.trim();
    }
  }
  return !device.location.isEmpty() && !device.uniqueServiceName.isEmpty();
}


String DIALClient::extractApplicationURL(HttpClient& httpc) {
  String appUrl;
  char headerChar;
  String currentLine = "";
  int consecutiveNewlines = 0;
  int maxHeadersRead = 1000;
  int readCount = 0;

  while (httpc.connected() && readCount < maxHeadersRead) {
    headerChar = httpc.readHeader();
    if (headerChar == '\n') {
      consecutiveNewlines++;

      currentLine.trim();
      currentLine.toLowerCase();
      if (currentLine.startsWith("application-url:")) {
        appUrl = currentLine.substring(currentLine.indexOf(':') + 2);  // +2 to skip over the colon and potential space
        break;
      }
      currentLine = "";

      if (consecutiveNewlines >= 2) {

        break;
      }
    } else if (headerChar != '\r') {
      consecutiveNewlines = 0;
      currentLine += headerChar;
    }
    readCount++;
  }
  return appUrl;
}


String DIALClient::getDialApplicationUrl(const String& locationUrl) {
  int ip_start = locationUrl.indexOf("//") + 2;
  int ip_end = locationUrl.indexOf(":", ip_start);
  int port_end = locationUrl.indexOf("/", ip_end);

  String localip = locationUrl.substring(ip_start, ip_end);
  String portStr = locationUrl.substring(ip_end + 1, port_end);
  uint16_t lport = portStr.toInt();

  String path = extractPathFromURL(locationUrl);

  WiFiClient wifiClient;

  HttpClient httpc(wifiClient, localip, lport);

  httpc.beginRequest();
  int getcode = httpc.get(path);
  httpc.sendHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8");
  httpc.sendHeader("User-Agent", UserAgent.c_str());
  httpc.sendHeader("Content-Type", "text/xml; charset=\"utf-8\"");
  httpc.endRequest();

  int httpCode = httpc.responseStatusCode();

  if (httpCode == 200) {
    String appUrl = extractApplicationURL(httpc);

    if (!appUrl.isEmpty()) {
      Serial.println("[getDialApplicationUrl] Application-URL: " + appUrl);
    } else {
      Serial.println(F("[getDialApplicationUrl] Couldn't find Application-URL in the headers."));
    }
    return appUrl;
  } else {
    Serial.println("Failed to fetch device description from " + locationUrl + ". HTTP Response Code: " + String(httpCode));
    return "";
  }
}
