#pragma once
#include "wifi_module.h"
#include "../../core/globals.h"

void apSnifferCallbackFull(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String essid = "";
  String bssid = "";

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
      wifimodule->getMACatoffset(addr, snifferPacket->payload, 10);

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
      
        delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" BSSID: ");
        Serial.print(addr);
        display_string.concat(addr);
        Serial.print(" ESSID: ");
        display_string.concat(" -> ");
        for (int i = 0; i < snifferPacket->payload[37]; i++)
        {
          Serial.print((char)snifferPacket->payload[i + 38]);
          display_string.concat((char)snifferPacket->payload[i + 38]);
          essid.concat((char)snifferPacket->payload[i + 38]);

          
        }

        bssid.concat(addr);
  
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

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
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
        wifimodule->getMACatoffset(ap_addr, snifferPacket->payload, offsets[y]);
        break;
      }
    }
    if (matched_ap)
      break;
  }


  if (!matched_ap)
    return;
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

  wifimodule->getMACatoffset(dst_addr, snifferPacket->payload, 4);

  // Check if dest is broadcast
  if ((in_list) || (strcmp(dst_addr, "ff:ff:ff:ff:ff:ff") == 0))
    return;
  
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


  Serial.print((String)stations->size() + ": ");
  
  char sta_addr[] = "00:00:00:00:00:00";
  
  if (ap_is_src) {
    Serial.print("ap: ");
    Serial.print(ap_addr);
    Serial.print(" -> sta: ");
    wifimodule->getMACatoffset(sta_addr, snifferPacket->payload, 4);
    Serial.println(sta_addr);
  }
  else {
    Serial.print("sta: ");
    wifimodule->getMACatoffset(sta_addr, snifferPacket->payload, 10);
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