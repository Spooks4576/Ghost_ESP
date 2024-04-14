#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <LinkedList.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include "esp_wifi_types.h"
#include "esp_wifi.h"

const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

uint8_t packet[128] PROGMEM = { 0x80, 0x00, 0x00, 0x00, //Frame Control, Duration
                    /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //Destination address 
                    /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //Source address - overwritten later
                    /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //BSSID - overwritten to the same as the source address
                    /*22*/  0xc0, 0x6c, //Seq-ctl
                    /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, //timestamp - the number of microseconds the AP has been active
                    /*32*/  0x64, 0x00, //Beacon interval
                    /*34*/  0x01, 0x04, //Capability info
                    /* SSID */
                    /*36*/  0x00
                    };

    uint8_t prob_req_packet[128] PROGMEM = {0x40, 0x00, 0x00, 0x00, 
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination
                                  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Dest
                                  0x01, 0x00, // Sequence
                                  0x00, // SSID Parameter
                                  0x00, // SSID Length
                                  /* SSID */
                                  };

    uint8_t deauth_frame_default[26] PROGMEM = {
                              0xc0, 0x00, 0x3a, 0x01,
                              0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0xf0, 0xff, 0x02, 0x00
                          };

typedef struct
{
    int16_t fctl;
    int16_t duration;
    uint8_t da;
    uint8_t sa;
    uint8_t bssid;
    int16_t seqctl;
    unsigned char payload[];
} WifiMgmtHdr;

typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    uint8_t payload[0];
    wifi_ieee80211_mac_hdr_t hdr;
} wifi_ieee80211_packet_t_2;

typedef struct {
    uint8_t payload[0];
    WifiMgmtHdr hdr;
} wifi_ieee80211_packet_t;

struct mac_addr {
    unsigned char bytes[6];

    bool operator==(const mac_addr& other) const {
        for (int i = 0; i < 6; ++i) {
            if (this->bytes[i] != other.bytes[i]) {
                return false;  // Return false as soon as any bytes differ
            }
        }
        return true;  // Return true if all bytes are the same
    }
};

const char* rick_roll[8] = {
      "01 Never gonna give you up",
      "02 Never gonna let you down",
      "03 Never gonna run around",
      "04 and desert you",
      "05 Never gonna make you cry",
      "06 Never gonna say goodbye",
      "07 Never gonna tell a lie",
      "08 and hurt you"
};

const char* KarmaSSIDs[] = {
        "ShawOpen",
        "D-LINK",
        "attwifi",
        "NETGEAR",
        "NETGEAR24",
        "netgear11",
        "Apple",
        "NETGEAR23",
        "Wi-Fi Arnet",
        "Linksys2",
        "holidayinn",
        "Starbucks WiFi"
};

char alfa[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

struct ssid {
  String essid;
  uint8_t channel;
  int bssid[6];
  bool selected;
};

struct AccessPoint {
  String essid;
  int channel;
  uint8_t bssid[6];
  bool selected;
  LinkedList<char>* beacon;
  int rssi;
  LinkedList<int>* stations;
};

struct Station {
  uint8_t mac[6];
  bool selected;
};

struct BeaconPacket{
    uint8_t* packet;
    int channel;
};


LinkedList<AccessPoint>* access_points;
LinkedList<ssid>* ssids;
LinkedList<Station>* stations;
String PROGMEM version_number;
String PROGMEM board_target;
bool HasRanCommand;

enum ClearType
{
    CT_AP,
    CT_STA,
    CT_SSID
};

enum AttackType
{
    AT_RandomSSID,
    AT_Rickroll,
    AT_DeauthAP,
    AT_ListSSID,
    AT_Karma
};

enum ScanType
{
    SCAN_AP,
    SCAN_STA,
    SCAN_WARDRIVE
};

enum SniffType
{
    ST_beacon,
    ST_pmkid,
    ST_probe,
    ST_pwn,
    ST_raw
};

class WiFiModule
{
public:
    String freeRAM();
    int ClearList(ClearType type);
    void Attack(AttackType type);
    bool addSSID(String essid);
    void Scan(ScanType type);
    void Sniff(SniffType Type, int TargetChannel);
    int generateSSIDs(int count);
    String getApMAC();
    bool shutdownWiFi();
    void broadcastRandomSSID();
    void insertWPA2Info(uint8_t *packet, int ssidLength);
    void insertTimestamp(uint8_t *packet);
    void RunSetup();
    int findMostActiveWiFiChannel();
    void LaunchEvilPortal();
    void Calibrate();
    void getMACatoffset(char *addr, uint8_t* data, uint16_t offset);
    void broadcastSetSSID(const char* ESSID, uint8_t channel);
    void sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]) ;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t ap_config;
    String dst_mac = "ff:ff:ff:ff:ff:ff";
    byte src_mac[6] = {};
    uint8_t packets_sent;
    bool wifi_initialized;
    bool BeaconSpamming;
    uint8_t initTime;
    int MostActiveChannel;
    LinkedList<BeaconPacket> BeaconsToBroadcast;
};