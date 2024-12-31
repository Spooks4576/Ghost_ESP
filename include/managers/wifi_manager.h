// wifi_manager.h

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_wifi_types.h"

#define RANDOM_SSID_LEN 8
#define BEACON_INTERVAL 0x0064  // 100 Time Units (TU)
#define CAPABILITY_INFO 0x0411  // Capability information (ESS)
#define MAX_STATIONS 50

typedef struct {
    uint8_t station_mac[6];  // MAC address of the station (client)
    uint8_t ap_bssid[6];     // BSSID (MAC address) of the access point
} station_ap_pair_t;

static station_ap_pair_t station_ap_list[MAX_STATIONS];  // Array to store station-AP pairs
static int station_count = 0;

extern wifi_ap_record_t* scanned_aps;
extern wifi_ap_record_t selected_ap;

static void* beacon_task_handle;
static void* deauth_task_handle;
static int beacon_task_running = 0;

typedef struct {
    uint8_t frame_control[2];  // Frame Control
    uint16_t duration;         // Duration
    uint8_t dest_addr[6];      // Destination Address
    uint8_t src_addr[6];       // Source Address (AP MAC)
    uint8_t bssid[6];          // BSSID (AP MAC)
    uint16_t seq_ctrl;         // Sequence Control
    uint64_t timestamp;        // Timestamp (microseconds since the AP started)
    uint16_t beacon_interval;  // Beacon Interval (in Time Units)
    uint16_t cap_info;         // Capability Information
} __attribute__((packed)) wifi_beacon_frame_t;


typedef struct {
    unsigned protocol_version : 2;
    unsigned type : 2;
    unsigned subtype : 4;
    unsigned to_ds : 1;
    unsigned from_ds : 1;
    unsigned more_frag : 1;
    unsigned retry : 1;
    unsigned pwr_mgmt : 1;
    unsigned more_data : 1;
    unsigned protected_frame : 1;
    unsigned order : 1;
} wifi_ieee80211_frame_ctrl_t;


typedef struct {
    wifi_ieee80211_frame_ctrl_t frame_ctrl;  // Frame control field
    uint16_t duration_id;                    // Duration/ID field
    uint8_t addr1[6];                        // Address 1 (Destination MAC or BSSID)
    uint8_t addr2[6];                        // Address 2 (Source MAC)
    uint8_t addr3[6];                        // Address 3 (BSSID or Destination MAC)
    uint16_t seq_ctrl;                       // Sequence control field
    uint8_t addr4[6];                        // Optional address 4 (used in certain cases)
} wifi_ieee80211_hdr_t;


typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seqno;
} __attribute__((__packed__)) icmp_packet_t;


typedef struct {
    uint16_t frame_ctrl;   // Frame control field
    uint16_t duration_id;  // Duration field
    uint8_t addr1[6];      // Receiver address (RA)
    uint8_t addr2[6];      // Transmitter address (TA)
    uint8_t addr3[6];      // BSSID or destination address
    uint16_t seq_ctrl;     // Sequence control field
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_hdr_t hdr;  // The 802.11 header
    uint8_t payload[];         // Variable-length payload (data)
} wifi_ieee80211_packet_t;

typedef struct {
    char ip[16];
    uint16_t open_ports[64];
    uint8_t num_open_ports;
} host_result_t;

typedef struct {
    const char* ssid;
    const char* password;
} wifi_credentials_t;

typedef struct {
    char subnet_prefix[16];
    host_result_t* results;
    size_t max_results;
    size_t num_active_hosts;
} scanner_ctx_t;

typedef void (* wifi_promiscuous_cb_t_t)(void *buf, wifi_promiscuous_pkt_type_t type);

// Initialize WiFiManager
void wifi_manager_init();

// Start scanning for available networks
void wifi_manager_start_scan();

// Stop scanning for networks
void wifi_manager_stop_scan();

// Print the scan results with BSSID to company mapping
void wifi_manager_print_scan_results_with_oui();

// broadcast ap beacon with optional ssid
esp_err_t wifi_manager_broadcast_ap(const char *ssid);

void wifi_manager_start_beacon(const char *ssid);

void wifi_manager_auto_deauth();

void wifi_manager_stop_beacon();

void wifi_manager_start_ip_lookup();

void wifi_manager_connect_wifi(const char* ssid, const char* password);

void wifi_manager_stop_monitor_mode();

void wifi_manager_start_monitor_mode(wifi_promiscuous_cb_t_t callback);

void wifi_manager_list_stations();

void wifi_manager_start_deauth();

void wifi_manager_select_ap(int index);

void wifi_manager_stop_deauth();

esp_err_t wifi_manager_broadcast_deauth(uint8_t bssid[6], int channel, uint8_t mac[6]);

void wifi_stations_sniffer_callback(void *buf, wifi_promiscuous_pkt_type_t type);

void wifi_manager_stop_evil_portal();

esp_err_t wifi_manager_start_evil_portal(const char* URL, const char* SSID, const char* Password, const char* ap_ssid, const char* domain);

void screen_music_visualizer_task(void *pvParameters);

void rgb_visualizer_server_task(void *pvParameters);

void animate_led_based_on_amplitude(void *pvParameters);

void wifi_manager_scan_for_open_ports();

bool get_subnet_prefix(scanner_ctx_t* ctx);

bool is_host_active(const char* ip_addr);

scanner_ctx_t* scanner_init(void);

bool wifi_manager_scan_subnet();

void scan_ports_on_host(const char* target_ip, host_result_t* result);

bool scan_ip_port_range(const char* target_ip, uint16_t start_port, uint16_t end_port);

static const uint16_t COMMON_PORTS[] = {
    20,     // FTP Data
    21,     // FTP Control
    22,     // SSH
    23,     // Telnet
    25,     // SMTP
    53,     // DNS
    69,     // TFTP
    80,     // HTTP
    88,     // Kerberos
    110,    // POP3
    111,    // RPCBind
    123,    // NTP
    135,    // MSRPC
    137,    // NetBIOS Name Service
    138,    // NetBIOS Datagram Service
    139,    // NetBIOS Session Service
    143,    // IMAP
    161,    // SNMP
    389,    // LDAP
    443,    // HTTPS
    445,    // SMB
    465,    // SMTPS
    500,    // IKE (VPN)
    514,    // Syslog
    515,    // LPD/LPR Printer
    587,    // SMTP (submission)
    631,    // IPP (Printing)
    636,    // LDAPS
    993,    // IMAPS
    995,    // POP3S
    1080,   // SOCKS Proxy
    1433,   // MSSQL
    1434,   // MSSQL Browser
    1521,   // Oracle DB
    1701,   // L2TP
    1723,   // PPTP
    1883,   // MQTT
    2049,   // NFS
    2082,   // cPanel
    2083,   // cPanel SSL
    2086,   // WHM
    2087,   // WHM SSL
    2222,   // Alternative SSH
    3306,   // MySQL
    3389,   // RDP
    5060,   // SIP
    5222,   // XMPP
    5432,   // PostgreSQL
    5900,   // VNC
    5901,   // VNC-1
    5902,   // VNC-2
    6379,   // Redis
    8080,   // HTTP Proxy
    8443,   // HTTPS Alt
    8883,   // MQTT SSL
    9100,   // Printer
    27017,  // MongoDB
    32400,  // Plex Media Server
    51820,  // Wireguard
    55443   // Alt HTTP
};
#define NUM_PORTS (sizeof(COMMON_PORTS) / sizeof(COMMON_PORTS[0]))

#endif // WIFI_MANAGER_H