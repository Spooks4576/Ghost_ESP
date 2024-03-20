#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <LinkedList.h>
#include <WiFi.h>
#include "esp_wifi_types.h"
#include "esp_wifi.h"

const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

inline uint8_t packet[128] = { 0x80, 0x00, 0x00, 0x00, //Frame Control, Duration
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

    inline uint8_t prob_req_packet[128] = {0x40, 0x00, 0x00, 0x00, 
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination
                                  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Dest
                                  0x01, 0x00, // Sequence
                                  0x00, // SSID Parameter
                                  0x00, // SSID Length
                                  /* SSID */
                                  };

    inline uint8_t deauth_frame_default[26] = {
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

inline const char* rick_roll[8] = {
      "01 Never gonna give you up",
      "02 Never gonna let you down",
      "03 Never gonna run around",
      "04 and desert you",
      "05 Never gonna make you cry",
      "06 Never gonna say goodbye",
      "07 Never gonna tell a lie",
      "08 and hurt you"
};

inline char alfa[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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

class CommandLine;

inline CommandLine* cli;
inline LinkedList<AccessPoint>* access_points;
inline LinkedList<ssid>* ssids;
inline LinkedList<Station>* stations;
inline String PROGMEM version_number;
inline String PROGMEM board_target;
inline bool HasRanCommand;

class WiFiModule
{
public:
    String freeRAM();
    int clearAPs();
    int clearSSIDs();
    int clearStations();
    bool addSSID(String essid);
    int generateSSIDs(int count);
    String getApMAC();
    bool shutdownWiFi();
    void broadcastRickroll();
    void broadcastRandomSSID();
    void RunAPScan();
    void RunStaScan();
    void InitRandomSSIDAttack();
    void InitListSSIDAttack();
    void insertWPA2Info(uint8_t *packet, int ssidLength);
    void insertTimestamp(uint8_t *packet);
    void RunSetup();
    void getMACatoffset(char *addr, uint8_t* data, uint16_t offset);
    void broadcastSetSSID(const char* ESSID);
    void broadcastDeauthAP();
    void sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]) ;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t ap_config;
    String dst_mac = "ff:ff:ff:ff:ff:ff";
    byte src_mac[6] = {};
    uint8_t packets_sent;
    bool wifi_initialized;
    bool BeaconSpamming;
    uint8_t initTime;
    LinkedList<BeaconPacket> BeaconsToBroadcast;
};