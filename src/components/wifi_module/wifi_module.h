#pragma once
#include <stdint.h>
#include <Arduino.h>
#include <LinkedList.h>
#include <map>
#include <vector>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include "esp_wifi_types.h"
#include "esp_wifi.h"
#include <WiFiClientSecure.h>

const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};

enum class ECompany {
    DLink,
    Netgear,
    Belkin,
    TPLink,
    Linksys,
    ASUS,
    Actiontec,
    Unknown
};

std::map<ECompany, std::vector<const char*>> CompanyOUIMap = {
    {ECompany::DLink, {
        "00055D", "000D88", "000F3D", "001195", "001346", "0015E9", "00179A", 
        "00195B", "001B11", "001CF0", "001E58", "002191", "0022B0", "002401", 
        "00265A", "00AD24", "04BAD6", "085A11", "0C0E76", "0CB6D2", "1062EB", 
        "10BEF5", "14D64D", "180F76", "1C5F2B", "1C7EE5", "1CAFF7", "1CBDB9", 
        "283B82", "302303", "340804", "340A33", "3C1E04", "3C3332", "4086CB", 
        "409BCD", "54B80A", "5CD998", "60634C", "642943", "6C198F", "6C7220", 
        "744401", "74DADA", "78321B", "78542E", "7898E8", "802689", "84C9B2", 
        "8876B9", "908D78", "9094E4", "9CD643", "A06391", "A0AB1B", "A42A95", 
        "A8637D", "ACF1DF", "B437D8", "B8A386", "BC0F9A", "BC2228", "BCF685", 
        "C0A0BB", "C4A81D", "C4E90A", "C8787D", "C8BE19", "C8D3A3", "CCB255", 
        "D8FEE3", "DCEAE7", "E01CFC", "E46F13", "E8CC18", "EC2280", "ECADE0", 
        "F07D68", "F0B4D2", "F48CEB", "F8E903", "FC7516"
    }},
    {ECompany::Netgear, {
        "00095B", "000FB5", "00146C", "001B2F", "001E2A", "001F33", "00223F", 
        "00224B2", "0026F2", "008EF2", "08028E", "0836C9", "08BD43", "100C6B", 
        "100D7F", "10DA43", "1459C0", "204E7F", "20E52A", "288088", "289401", 
        "28C68E", "2C3033", "2CB05D", "30469A", "3498B5", "3894ED", "3C3786", 
        "405D82", "44A56E", "4C60DE", "504A6E", "506A03", "54077D", "58EF68", 
        "6038E0", "6CB0CE", "6CCDD6", "744401", "803773", "841B5E", "8C3BAD", 
        "941865", "9C3DCF", "9CC9EB", "9CD36D", "A00460", "A021B7", "A040A0", 
        "A42B8C", "B03956", "B07FB9", "B0B98A", "BCA511", "C03F0E", "C0FFD4", 
        "C40415", "C43DC7", "C89E43", "CC40D0", "DCEF09", "E0469A", "E046EE", 
        "E091F5", "E4F4C6", "E8FCAF", "F87394"
    }},
    {ECompany::Belkin, {
        "001150", "00173F", "0030BD", "08BD43", "149182", "24F5A2", "302303", 
        "80691A", "94103E", "944452", "B4750E", "C05627", "C4411E", "D8EC5E", 
        "E89F80", "EC1A59", "EC2280"
    }},
    {ECompany::TPLink, {
        "003192", "005F67", "1027F5", "14EBB6", "1C61B4", "203626", "2887BA", 
        "30DE4B", "3460F9", "3C52A1", "40ED00", "482254", "5091E3", "54AF97", 
        "5C628B", "5CA6E6", "5CE931", "60A4B7", "687FF0", "6C5AB0", "788CB5", 
        "7CC2C6", "9C5322", "9CA2F4", "A842A1", "AC15A2", "B0A7B9", "B4B024", 
        "C006C3", "CC68B6", "E848B8", "F0A731"
    }},
    {ECompany::Linksys, {
        "00045A", "000625", "000C41", "000E08", "000F66", "001217", "001310", 
        "0014BF", "0016B6", "001839", "0018F8", "001A70", "001C10", "001D7E", 
        "001EE5", "002129", "00226B", "002369", "00259C", "002354", "0024B2", 
        "003192", "005F67", "1027F5", "14EBB6", "1C61B4", "203626", "2887BA", 
        "305A3A", "2CFDA1", "302303", "30469A", "40ED00", "482254", "5091E3", 
        "54AF97", "5CA2F4", "5CA6E6", "5CE931", "60A4B7", "687FF0", "6C5AB0", 
        "788CB5", "7CC2C6", "9C5322", "9CA2F4", "A842A1", "AC15A2", "B0A7B9", 
        "B4B024", "C006C3", "CC68B6", "E848B8", "F0A731"
    }},
    {ECompany::ASUS, {
        "000C6E", "000EA6", "00112F", "0011D8", "0013D4", "0015F2", "001731", 
        "0018F3", "001A92", "001BFC", "001D60", "001E8C", "001FC6", "002215", 
        "002354", "00248C", "002618", "00E018", "04421A", "049226", "04D4C4", 
        "04D9F5", "08606E", "086266", "08BFB8", "0C9D92", "107B44", "107C61", 
        "10BF48", "10C37B", "14DAE9", "14DDA9", "1831BF", "1C872C", "1CB72C", 
        "20CF30", "244BFE", "2C4D54", "2C56DC", "2CFDA1", "305A3A", "3085A9", 
        "3497F6", "382C4A", "38D547", "3C7C3F", "40167E", "40B076", "485B39", 
        "4CEDFB", "50465D", "50EBF6", "5404A6", "54A050", "581122", "6045CB", 
        "60A44C", "60CF84", "704D7B", "708BCD", "74D02B", "7824AF", "7C10C9", 
        "88D7F6", "90E6BA", "9C5C8E", "A036BC", "A85E45", "AC220B", "AC9E17", 
        "B06EBF", "BCAEC5", "BCEE7B", "C86000", "C87F54", "CC28AA", "D017C2", 
        "D45D64", "D850E6", "E03F49", "E0CB4E", "E89C25", "F02F74", "F07959", 
        "F46D04", "F832E4", "FC3497", "FCC233"
    }},
    {ECompany::Actiontec, {
        "000FB3", "001505", "001801", "001EA7", "001F90", "0020E0", "00247B", 
        "002662", "0026B8", "007F28", "0C6127", "105F06", "10785B", "109FA9", 
        "181BEB", "207600", "408B07", "4C8B30", "5C35FC", "7058A4", "70F196", 
        "70F220", "84E892", "941C56", "9C1E95", "A0A3E2", "A83944", "E86FF2", 
        "F8E4FB", "FC2BB2"
    }},
    {ECompany::Unknown, {}}
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
inline AccessPoint SelectedAP;
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
    void setManufacturer(AccessPoint* ap);
    void LaunchEvilPortal();
    bool isVulnerableBSSID(AccessPoint* ap);
    bool isAccessPointAlreadyAdded(LinkedList<AccessPoint*>& accessPoints, const uint8_t* bssid);
    void Calibrate();
    void getMACatoffset(char *addr, uint8_t* data, uint16_t offset);
    void getMACatoffset(uint8_t *addr, uint8_t* data, uint16_t offset);
    void broadcastSetSSID(const char* ESSID, uint8_t channel);
    void sendDeauthFrame(uint8_t* bssid, int channel, uint8_t* mac);
    void listStations();
    void listSSIDs();
    void listAccessPoints();
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