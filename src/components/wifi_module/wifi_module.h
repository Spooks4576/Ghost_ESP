#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <LinkedList.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include "esp_wifi_types.h"
#include "esp_wifi.h"
#include <WiFiClientSecure.h>

const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

const char* ouivuln[] = {
    "00055D - DLink D-Link Systems, Inc.",
    "00095B - Netgear Netgear",
    "000D88 - DLink D-Link Corporation",
    "000F3D - DLink D-Link Corporation",
    "000FB5 - Netgear Netgear",
    "001150 - Belkin Belkin Corporation",
    "001195 - DLink D-Link Corporation",
    "001346 - DLink D-Link Corporation",
    "00146C - Netgear Netgear",
    "0015E9 - DLink D-Link Corporation",
    "00173F - BelkinIntern Belkin International Inc.",
    "00179A - DLink D-Link Corporation",
    "00184D - Netgear Netgear",
    "00195B - DLink D-Link Corporation",
    "001B11 - DLink D-Link Corporation",
    "001B2F - Netgear Netgear",
    "001CDF - BelkinIntern Belkin International Inc.",
    "001CF0 - DLink D-Link Corporation",
    "001E2A - Netgear Netgear",
    "001E58 - DLink D-Link Corporation",
    "001F33 - Netgear Netgear",
    "002191 - DLink D-Link Corporation",
    "00223F - Netgear Netgear",
    "002275 - BelkinIntern Belkin International Inc.",
    "0022B0 - DLink D-Link Corporation",
    "002401 - DLink D-Link Corporation",
    "0024B2 - Netgear Netgear",
    "00265A - DLink D-Link Corporation",
    "0026F2 - Netgear Netgear",
    "0030BD - BelkinCompon Belkin Components",
    "0050BA - DLink D-Link Corporation",
    "0080C8 - DLink D-Link Systems, Inc.",
    "008EF2 - Netgear Netgear",
    "00AD24 - DLinkInterna D-Link International",
    "04A151 - Netgear Netgear",
    "04BAD6 - DLink D-Link Corporation",
    "08028E - Netgear Netgear",
    "0836C9 - Netgear Netgear",
    "085A11 - DLinkInterna D-Link International",
    "08863B - BelkinIntern Belkin International Inc.",
    "08BD43 - Netgear Netgear",
    "0C0E76 - DLinkInterna D-Link International",
    "0C73EBD0/28 - DLink D-Link （Shanghai）Limited Corp.",
    "0CB6D2 - DLinkInterna D-Link International",
    "100C6B - Netgear Netgear",
    "100D7F - Netgear Netgear",
    "1062EB - DLinkInterna D-Link International",
    "10BEF5 - DLinkInterna D-Link International",
    "10DA43 - Netgear Netgear",
    "1459C0 - Netgear Netgear",
    "149182 - BelkinIntern Belkin International Inc.",
    "14D64D - DLinkInterna D-Link International",
    "180F76 - DLinkInterna D-Link International",
    "1C5F2B - DLinkInterna D-Link International",
    "1C7EE5 - DLinkInterna D-Link International",
    "1CAFF7 - DLinkInterna D-Link International",
    "1CBDB9 - DLinkInterna D-Link International",
    "200CC8 - Netgear Netgear",
    "204E7F - Netgear Netgear",
    "20E52A - Netgear Netgear",
    "24F5A2 - BelkinIntern Belkin International Inc.",
    "28107B - DLinkInterna D-Link International",
    "283B82 - DLinkInterna D-Link International",
    "288088 - Netgear Netgear",
    "289401 - Netgear Netgear",
    "28C68E - Netgear Netgear",
    "2C3033 - Netgear Netgear",
    "2CB05D - Netgear Netgear",
    "302303 - BelkinIntern Belkin International Inc.",
    "30469A - Netgear Netgear",
    "340804 - DLink D-Link Corporation",
    "340A33 - DLinkInterna D-Link International",
    "3498B5 - Netgear Netgear",
    "3894ED - Netgear Netgear",
    "3C1E04 - DLinkInterna D-Link International",
    "3C3332 - DLink D-Link Corporation",
    "3C3786 - Netgear Netgear",
    "405D82 - Netgear Netgear",
    "4086CB - DLink D-Link Corporation",
    "409BCD - DLinkInterna D-Link International",
    "4494FC - Netgear Netgear",
    "44A56E - Netgear Netgear",
    "48EE0C - DLinkInterna D-Link International",
    "4C60DE - Netgear Netgear",
    "504A6E - Netgear Netgear",
    "506A03 - Netgear Netgear",
    "54077D - Netgear Netgear",
    "54B80A - DLinkInterna D-Link International",
    "58D56E - DLinkInterna D-Link International",
    "58EF68 - BelkinIntern Belkin International Inc.",
    "5CD998 - DLink D-Link Corporation",
    "6038E0 - BelkinIntern Belkin International Inc.",
    "60634C - DLinkInterna D-Link International",
    "642943 - DLink D-Link Corporation",
    "6C198F - DLinkInterna D-Link International",
    "6C7220 - DLinkInterna D-Link International",
    "6CB0CE - Netgear Netgear",
    "6CCDD6 - Netgear Netgear",
    "7062B8 - DLinkInterna D-Link International",
    "744401 - Netgear Netgear",
    "74DADA - DLinkInterna D-Link International",
    "78321B - DLinkInterna D-Link International",
    "78542E - DLinkInterna D-Link International",
    "7898E8 - DLinkInterna D-Link International",
    "78D294 - Netgear Netgear",
    "802689 - DLinkInterna D-Link International",
    "803773 - Netgear Netgear",
    "80691A - BelkinIntern Belkin International Inc.",
    "80CC9C - Netgear Netgear",
    "841B5E - Netgear Netgear",
    "84C9B2 - DLinkInterna D-Link International",
    "8876B9 - DLink D-Link Corporation",
    "8C3BAD - Netgear Netgear",
    "908D78 - DLinkInterna D-Link International",
    "9094E4 - DLinkInterna D-Link International",
    "94103E - BelkinIntern Belkin International Inc.",
    "941865 - Netgear Netgear",
    "944452 - BelkinIntern Belkin International Inc.",
    "94A67E - Netgear Netgear",
    "9C3DCF - Netgear Netgear",
    "9CC9EB - Netgear Netgear",
    "9CD36D - Netgear Netgear",
    "9CD643 - DLinkInterna D-Link International",
    "A00460 - Netgear Netgear",
    "A021B7 - Netgear Netgear",
    "A040A0 - Netgear Netgear",
    "A06391 - Netgear Netgear",
    "A09F7A - DLinkMiddleE D-Link Middle East FZCO",
    "A0A3F0 - DLinkInterna D-Link International",
    "A0AB1B - DLinkInterna D-Link International",
    "A42A95 - DLinkInterna D-Link International",
    "A42B8C - Netgear Netgear",
    "A8637D - DLinkInterna D-Link International",
    "ACF1DF - DLinkInterna D-Link International",
    "B03956 - Netgear Netgear",
    "B07FB9 - Netgear Netgear",
    "B0B98A - Netgear Netgear",
    "B0C554 - DLinkInterna D-Link International",
    "B437D8 - DLink D-Link (Shanghai) Limited Corp.",
    "B4750E - BelkinIntern Belkin International Inc.",
    "B8A386 - DLinkInterna D-Link International",
    "BC0F9A - DLinkInterna D-Link International",
    "BC2228 - DLinkInterna D-Link International",
    "BCA511 - Netgear Netgear",
    "BCF685 - DLinkInterna D-Link International",
    "C03F0E - Netgear Netgear",
    "C05627 - BelkinIntern Belkin International Inc.",
    "C0A0BB - DLinkInterna D-Link International",
    "C0FFD4 - Netgear Netgear",
    "C40415 - Netgear Netgear",
    "C412F5 - DLinkInterna D-Link International",
    "C43DC7 - Netgear Netgear",
    "C4411E - BelkinIntern Belkin International Inc.",
    "C4A81D - DLinkInterna D-Link International",
    "C4E90A - DLinkInterna D-Link International",
    "C8787D - DLink D-Link Corporation",
    "C89E43 - Netgear Netgear",
    "C8BE19 - DLinkInterna D-Link International",
    "C8D3A3 - DLinkInterna D-Link International",
    "CC40D0 - Netgear Netgear",
    "CCB255 - DLinkInterna D-Link International",
    "D8EC5E - BelkinIntern Belkin International Inc.",
    "D8FEE3 - DLinkInterna D-Link International",
    "DCEAE7 - DLink D-Link Corporation",
    "DCEF09 - Netgear Netgear",
    "E01CFC - DLinkInterna D-Link International",
    "E0469A - Netgear Netgear",
    "E046EE - Netgear Netgear",
    "E091F5 - Netgear Netgear",
    "E46F13 - DLinkInterna D-Link International",
    "E4F4C6 - Netgear Netgear",
    "E89F80 - BelkinIntern Belkin International Inc.",
    "E8CC18 - DLinkInterna D-Link International",
    "E8FCAF - Netgear Netgear",
    "EC1A59 - BelkinIntern Belkin International Inc.",
    "EC2280 - DLinkInterna D-Link International",
    "ECADE0 - DLinkInterna D-Link International",
    "F07D68 - DLink D-Link Corporation",
    "F0B4D2 - DLinkInterna D-Link International",
    "F48CEB - DLinkInterna D-Link International",
    "F87394 - Netgear Netgear",
    "F8E903 - DLinkInterna D-Link International",
    "FC7516 - DLinkInterna D-Link International",
};

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
    int16_t fctl;              // Frame control field
    int16_t duration;          // Duration field
    uint8_t da[6];             // Destination address (DA) - MAC address
    uint8_t sa[6];             // Source address (SA) - MAC address
    uint8_t bssid[6];          // BSSID - MAC address
    int16_t seqctl;            // Sequence control field
    unsigned char payload[];   // Frame body (variable length)
} WifiMgmtHdr;

typedef struct {
    uint16_t frame_ctrl;       // Frame control field
    uint16_t duration_id;      // Duration/ID field
    uint8_t addr1[6];          // Address 1 (Receiver Address)
    uint8_t addr2[6];          // Address 2 (Sender Address)
    uint8_t addr3[6];          // Address 3 (Filtering Address)
    uint16_t sequence_ctrl;    // Sequence control field
    uint8_t addr4[6];          // Address 4 (optional, used in certain frame types)
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
  String Manufacturer;
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


inline LinkedList<AccessPoint>* access_points;
inline LinkedList<AccessPoint*> WPSAccessPoints;
inline LinkedList<ssid>* ssids;
inline LinkedList<Station>* stations;
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
    AT_Karma,
    AT_WPS
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
    ST_raw,
    ST_Deauth
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
    void SendWebRequest(const char* SSID, const char* Password, String requestType, String url, String data = "");
    void broadcastRandomSSID();
    void insertWPA2Info(uint8_t *packet, int ssidLength);
    void insertTimestamp(uint8_t *packet);
    void RunSetup();
    int findMostActiveWiFiChannel();
    void LaunchEvilPortal();
    bool isVulnerableBSSID(const uint8_t *bssid, AccessPoint* ap);
    bool isAccessPointAlreadyAdded(LinkedList<AccessPoint*>& accessPoints, const uint8_t* bssid);
    void Calibrate();
    void getMACatoffset(char *addr, uint8_t* data, uint16_t offset);
    void getMACatoffset(uint8_t *addr, uint8_t* data, uint16_t offset);
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
    bool EnableBLEWardriving;
    LinkedList<BeaconPacket> BeaconsToBroadcast;
    WiFiClientSecure wifiClientSecure;
};