#pragma once
#include "wifi_module.h"
#include <ArduinoJson.h>
#include <core/system_manager.h>

namespace CallBackUtils
{
    uint16_t ntohs(uint16_t netshort) {
      return (netshort >> 8) | (netshort << 8);
  }
}

void apSnifferCallbackFull(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String essid = "";
  String bssid = "";

  if (Serial.available() > 0)
  {
    SystemManager::getInstance().wifiModule.shutdownWiFi();
  }

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = CallBackUtils::ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
    int buf = 0;


    if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
    {
      char addr[] = "00:00:00:00:00:00";
      SystemManager::getInstance().wifiModule.getMACatoffset(addr, snifferPacket->payload, 10);

      bool in_list = false;
      bool mac_match = true;

      for (int i = 0; i < access_points->size(); i++) {
        mac_match = true;
        
        for (int x = 0; x < 6; x++) {
          if (snifferPacket->payload[x + 10] != access_points->get(i).bssid[x]) {
            mac_match = false;
            break;
          }
        }
        if (mac_match) {
          in_list = true;
          break;
        }
      }

      if (!in_list) {
#ifdef OLD_LED
        SystemManager::getInstance().rgbModule->breatheLED(SystemManager::getInstance().rgbModule->greenPin, 100);
#endif
#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(0, 255, 0), 300, false);
#endif
      
        delay(random(0, 10));
        LOG_RESULTS("ScanResult.txt", "scan", "RSSI: ");
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        LOG_RESULTS("ScanResult.txt", "scan", String(snifferPacket->rx_ctrl.rssi).c_str());
        Serial.print(" Ch: ");
        LOG_RESULTS("ScanResult.txt", "scan", " Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        LOG_RESULTS("ScanResult.txt", "scan", String(snifferPacket->rx_ctrl.channel).c_str());
        Serial.print(" BSSID: ");
        LOG_RESULTS("ScanResult.txt", "scan", " BSSID: ");
        Serial.print(addr);
        LOG_RESULTS("ScanResult.txt", "scan", addr);
        display_string.concat(addr);
        Serial.print(" ESSID: ");
        LOG_RESULTS("ScanResult.txt", "scan", " ESSID: ");
        display_string.concat(" -> ");
        for (int i = 0; i < snifferPacket->payload[37]; i++)
        {
          Serial.print((char)snifferPacket->payload[i + 38]);
          display_string.concat((char)snifferPacket->payload[i + 38]);
          essid.concat((char)snifferPacket->payload[i + 38]);
        }

        bssid.concat(addr);
        LOG_RESULTS("ScanResult.txt", "scan", essid.c_str());

        int temp_len = display_string.length();
        for (int i = 0; i < 40 - temp_len; i++)
        {
          display_string.concat(" ");
        }
  
        Serial.print(" ");
        
        if (essid == "") {
          essid = bssid;
          Serial.print(essid + " ");
        }

        AccessPoint ap;
        ap.essid = essid;
        ap.channel = snifferPacket->rx_ctrl.channel;
        ap.bssid[0] = snifferPacket->payload[10];
        ap.bssid[1] = snifferPacket->payload[11];
        ap.bssid[2] = snifferPacket->payload[12];
        ap.bssid[3] = snifferPacket->payload[13];
        ap.bssid[4] = snifferPacket->payload[14];
        ap.bssid[5] = snifferPacket->payload[15];
        ap.selected = false;
        ap.stations = new LinkedList<int>();
        
        ap.beacon = new LinkedList<char>();

        ap.beacon->add(snifferPacket->payload[34]);
        ap.beacon->add(snifferPacket->payload[35]);

        Serial.print("\nBeacon: ");

        for (int i = 0; i < ap.beacon->size(); i++) {
          char hexCar[4];
          sprintf(hexCar, "%02X", ap.beacon->get(i));
          Serial.print(hexCar);
          if ((i + 1) % 16 == 0)
            Serial.print("\n");
          else
            Serial.print(" ");
        }

        ap.rssi = snifferPacket->rx_ctrl.rssi;

        access_points->add(ap);

        Serial.print(access_points->size());
        Serial.print(" ");
        Serial.print(access_points->size());
        Serial.print(" ");
        Serial.print(esp_get_free_heap_size());
    }
    }
  }
}

void stationSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String mac = "";
  if (Serial.available() > 0)
  {
    SystemManager::getInstance().wifiModule.shutdownWiFi();
  }

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = CallBackUtils::ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  }

  char ap_addr[] = "00:00:00:00:00:00";
  char dst_addr[] = "00:00:00:00:00:00";

  int ap_index = 0;


  uint8_t frame_offset = 0;
  int offsets[2] = {10, 4};
  bool matched_ap = false;
  bool ap_is_src = false;

  bool mac_match = true;

  for (int y = 0; y < 2; y++) {
    for (int i = 0; i < access_points->size(); i++) {
      mac_match = true;
      
      for (int x = 0; x < 6; x++) {
        if (snifferPacket->payload[x + offsets[y]] != access_points->get(i).bssid[x]) {
          mac_match = false;
          break;
        }
      }
      if (mac_match) {
        matched_ap = true;
        if (offsets[y] == 10)
          ap_is_src = true;
        ap_index = i;
        SystemManager::getInstance().wifiModule.getMACatoffset(ap_addr, snifferPacket->payload, offsets[y]);
        break;
      }
    }
    if (matched_ap)
      break;
  }


  if (!matched_ap)
  {
    return;
  }
  else {
    if (ap_is_src)
      frame_offset = 4;
    else
      frame_offset = 10;
  }
  
  bool in_list = false;
  for (int i = 0; i < stations->size(); i++) {
    mac_match = true;
    
    for (int x = 0; x < 6; x++) {
      if (snifferPacket->payload[x + frame_offset] != stations->get(i).mac[x]) {
        mac_match = false;
        break;
      }
    }
    if (mac_match) {
      in_list = true;
      break;
    }
  }

  SystemManager::getInstance().wifiModule.getMACatoffset(dst_addr, snifferPacket->payload, 4);

  // Check if dest is broadcast
  if ((in_list) || (strcmp(dst_addr, "ff:ff:ff:ff:ff:ff") == 0))
  {
    return;
  }
  
  // Add to list of stations
  Station sta = {
                {snifferPacket->payload[frame_offset],
                 snifferPacket->payload[frame_offset + 1],
                 snifferPacket->payload[frame_offset + 2],
                 snifferPacket->payload[frame_offset + 3],
                 snifferPacket->payload[frame_offset + 4],
                 snifferPacket->payload[frame_offset + 5]},
                false};

  stations->add(sta);
#ifdef OLD_LED
SystemManager::getInstance().rgbModule->breatheLED(SystemManager::getInstance().rgbModule->greenPin, 100);
#endif
#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(0, 255, 0), 300, false);
#endif


  Serial.print((String)stations->size() + ": ");
  
  char sta_addr[] = "00:00:00:00:00:00";
  
  if (ap_is_src) {
    Serial.print("ap: ");
    Serial.print(ap_addr);
    Serial.print(" -> sta: ");
    SystemManager::getInstance().wifiModule.getMACatoffset(sta_addr, snifferPacket->payload, 4);
    Serial.println(sta_addr);
  }
  else {
    Serial.print("sta: ");
    SystemManager::getInstance().wifiModule.getMACatoffset(sta_addr, snifferPacket->payload, 10);
    Serial.print(sta_addr);
    Serial.print(" -> ap: ");
    Serial.println(ap_addr);
  }
  display_string.concat(sta_addr);
  display_string.concat(" -> ");
  display_string.concat(access_points->get(ap_index).essid);

  int temp_len = display_string.length();

  AccessPoint ap = access_points->get(ap_index);
  ap.stations->add(stations->size() - 1);

  access_points->set(ap_index, ap);
}

void deauthSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = CallBackUtils::ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    
    int buf = 0;

    if ((snifferPacket->payload[0] == 0xA0 || snifferPacket->payload[0] == 0xC0 ) && (buf == 0))
    {
      delay(random(0, 10));
      Serial.print("RSSI: ");
      LOG_RESULTS("DeauthSniff.txt", "sniff", "RSSI: ");
      Serial.print(snifferPacket->rx_ctrl.rssi);
      LOG_RESULTS("DeauthSniff.txt", "sniff", String(snifferPacket->rx_ctrl.rssi).c_str());
      Serial.print(" Ch: ");
      LOG_RESULTS("DeauthSniff.txt", "sniff", " Ch: ");
      Serial.print(snifferPacket->rx_ctrl.channel);
      LOG_RESULTS("DeauthSniff.txt", "sniff", String(snifferPacket->rx_ctrl.channel).c_str());
      Serial.print(" BSSID: ");
      LOG_RESULTS("DeauthSniff.txt", "sniff", " BSSID: ");
      char addr[] = "00:00:00:00:00:00";
      char dst_addr[] = "00:00:00:00:00:00";
      SystemManager::getInstance().wifiModule.getMACatoffset(addr, snifferPacket->payload, 10);
      SystemManager::getInstance().wifiModule.getMACatoffset(dst_addr, snifferPacket->payload, 4);
      Serial.print(addr);
      LOG_RESULTS("DeauthSniff.txt", "sniff", addr);
      Serial.print(" -> ");
      LOG_RESULTS("DeauthSniff.txt", "sniff", " -> ");
      Serial.print(dst_addr);
      LOG_RESULTS("DeauthSniff.txt", "sniff", dst_addr);
    }
  }
}

void pwnSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{ 
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String src = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    
    int buf = 0;
    
    if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
    {
      char addr[] = "00:00:00:00:00:00";
      SystemManager::getInstance().wifiModule.getMACatoffset(addr, snifferPacket->payload, 10);
      src.concat(addr);
      if (src == "de:ad:be:ef:de:ad") {
        
        
        delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" BSSID: ");
        Serial.print(addr);
        display_string.concat("CH: " + (String)snifferPacket->rx_ctrl.channel);
        Serial.print(" ESSID: ");
        display_string.concat(" -> ");


        for (int i = 0; i < len - 37; i++)
        {
          Serial.print((char)snifferPacket->payload[i + 38]);
          if (isAscii(snifferPacket->payload[i + 38]))
            essid.concat((char)snifferPacket->payload[i + 38]);
          else
            Serial.println("Got non-ascii character: " + (String)(char)snifferPacket->payload[i + 38]);
        }

        
        DynamicJsonDocument json(1024);
        if (deserializeJson(json, essid)) {
          Serial.println("\nCould not parse Pwnagotchi json");
          display_string.concat(essid);
        }
        else {
          Serial.println("\nSuccessfully parsed json");
          String json_output;
          serializeJson(json, json_output);
          Serial.println(json_output);
          display_string.concat(json["name"].as<String>() + " pwnd: " + json["pwnd_tot"].as<String>());
        }
  
        int temp_len = display_string.length();
        for (int i = 0; i < 40 - temp_len; i++)
        {
          display_string.concat(" ");
        }
  
        Serial.print(" ");

        Serial.println();

#ifdef SD_CARD_CS_PIN
SystemManager::getInstance().sdCardModule.logPacket(snifferPacket->payload, len);
#endif
      }
    }
  }
}

void beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    int buff = 0;
    if ((snifferPacket->payload[0] == 0x80) && (buff == 0))
    {
        bool found = false;
        uint8_t targ_index = 0;
        AccessPoint targ_ap;

       
        for (int i = 0; i < access_points->size(); i++) {
          if (access_points->get(i).selected) {
            uint8_t addr[] = {snifferPacket->payload[10],
                              snifferPacket->payload[11],
                              snifferPacket->payload[12],
                              snifferPacket->payload[13],
                              snifferPacket->payload[14],
                              snifferPacket->payload[15]};
            for (int x = 0; x < 6; x++) {
              if (addr[x] != access_points->get(i).bssid[x]) {
                found = false;
                break;
              }
              else
                found = true;
            }
            if (found) {
              targ_ap = access_points->get(i);
              targ_index = i;
              break;
            }
          }
        }
        if (!found)
          return;

        if ((targ_ap.rssi + 5 < snifferPacket->rx_ctrl.rssi) || (snifferPacket->rx_ctrl.rssi + 5 < targ_ap.rssi)) {
          targ_ap.rssi = snifferPacket->rx_ctrl.rssi;
          access_points->set(targ_index, targ_ap);
          Serial.println((String)access_points->get(targ_index).essid + " RSSI: " + (String)access_points->get(targ_index).rssi);
          return;
        }
#ifdef SD_CARD_CS_PIN
SystemManager::getInstance().sdCardModule.logPacket(snifferPacket->payload, len);
#endif
    }
  }
}

void probeSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {

  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;


   
    int buf = 0;

    if ((snifferPacket->payload[0] == 0x40) && (buf == 0))
    {
        delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" Client: ");
        char addr[] = "00:00:00:00:00:00";
        SystemManager::getInstance().wifiModule.getMACatoffset(addr, snifferPacket->payload, 10);
        Serial.print(addr);
        display_string.concat(addr);
        Serial.print(" Requesting: ");
        display_string.concat(" -> ");
        for (int i = 0; i < snifferPacket->payload[25]; i++)
        {
          Serial.print((char)snifferPacket->payload[26 + i]);
          display_string.concat((char)snifferPacket->payload[26 + i]);
        }
        
        Serial.println();    

#ifdef SD_CARD_CS_PIN
SystemManager::getInstance().sdCardModule.logPacket(snifferPacket->payload, len);
#endif
    }
  }
}

void rawSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  }
    Serial.print("RSSI: ");
    Serial.print(snifferPacket->rx_ctrl.rssi);
    Serial.print(" Ch: ");
    Serial.print(snifferPacket->rx_ctrl.channel);
    Serial.print(" BSSID: ");
    char addr[] = "00:00:00:00:00:00";
    SystemManager::getInstance().wifiModule.getMACatoffset(addr, snifferPacket->payload, 10);
    Serial.print(addr);

    display_string.concat(" ");
    display_string.concat(addr);

    int temp_len = display_string.length();

    Serial.println();

#ifdef SD_CARD_CS_PIN
  SystemManager::getInstance().sdCardModule.logPacket(snifferPacket->payload, len);
#endif
}

void eapolSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  bool send_deauth = true;
  
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = CallBackUtils::ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  }

  
  int buff = 0;


  if (send_deauth) {
    if (snifferPacket->payload[0] == 0x80) {
      
      deauth_frame_default[10] = snifferPacket->payload[10];
      deauth_frame_default[11] = snifferPacket->payload[11];
      deauth_frame_default[12] = snifferPacket->payload[12];
      deauth_frame_default[13] = snifferPacket->payload[13];
      deauth_frame_default[14] = snifferPacket->payload[14];
      deauth_frame_default[15] = snifferPacket->payload[15];
    
      deauth_frame_default[16] = snifferPacket->payload[10];
      deauth_frame_default[17] = snifferPacket->payload[11];
      deauth_frame_default[18] = snifferPacket->payload[12];
      deauth_frame_default[19] = snifferPacket->payload[13];
      deauth_frame_default[20] = snifferPacket->payload[14];
      deauth_frame_default[21] = snifferPacket->payload[15];      
    
      // Send packet
      esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
      delay(1);
    }
  }

  if (((snifferPacket->payload[30] == 0x88 && snifferPacket->payload[31] == 0x8e)|| ( snifferPacket->payload[32] == 0x88 && snifferPacket->payload[33] == 0x8e) )){
    Serial.println("Received EAPOL:");

    char addr[] = "00:00:00:00:00:00";
    SystemManager::getInstance().wifiModule.getMACatoffset(addr, snifferPacket->payload, 10);
    display_string.concat(addr);

    int temp_len = display_string.length();

    Serial.println(addr);    
#ifdef SD_CARD_CS_PIN
  SystemManager::getInstance().sdCardModule.logPacket(snifferPacket->payload, len);
#endif
  }
}