#include "wps/pixie.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "esp_wps_i.h"
#include "wps/wps_i.h"
#include "esp_wifi.h"
#include "managers/ap_manager.h"
#include "core/system_manager.h"
#include <esp_event_base.h>
#include "wps/wps_defs.h"

#define TAG "WPS"


void print_hex(const char *label, const u8 *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
        if (i < len - 1) printf(":");
    }
    printf("\n");
}

static void wps_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_SUCCESS) {
        wifi_event_sta_wps_er_success_t* wps_success = (wifi_event_sta_wps_er_success_t*)event_data;
        
        ESP_LOGI(TAG, "WPS connection successful.");
        ESP_LOGI(TAG, "Connected to SSID: %s", wps_success->ap_cred->ssid);
        
        // Start Wi-Fi connection
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_FAILED) {
        ESP_LOGE(TAG, "WPS connection failed.");
        struct wps_sm *wps_sm_inst = wps_sm_get();
        if (wps_sm_inst == NULL || wps_sm_inst->wps == NULL) {
            ESP_LOGE(TAG, "Failed to get the current WPS state machine instance or WPS data");
            return;
        }

        struct wps_data *wps_data_inst = wps_sm_inst->wps;

        
        print_hex("UUID-E", wps_data_inst->uuid_e, WPS_UUID_LEN);
        print_hex("UUID-R", wps_data_inst->uuid_r, WPS_UUID_LEN);
        print_hex("MAC Addr-E", wps_data_inst->mac_addr_e, ETH_ALEN);
        print_hex("Nonce-E", wps_data_inst->nonce_e, WPS_NONCE_LEN);
        print_hex("Nonce-R", wps_data_inst->nonce_r, WPS_NONCE_LEN);
        print_hex("PSK1", wps_data_inst->psk1, WPS_PSK_LEN);
        print_hex("PSK2", wps_data_inst->psk2, WPS_PSK_LEN);
        print_hex("SNonce", wps_data_inst->snonce, 2 * WPS_SECRET_NONCE_LEN);
        print_hex("Peer Hash1", wps_data_inst->peer_hash1, WPS_HASH_LEN);
        print_hex("Peer Hash2", wps_data_inst->peer_hash2, WPS_HASH_LEN);
        print_hex("AuthKey", wps_data_inst->authkey, WPS_AUTHKEY_LEN);
        print_hex("KeyWrapKey", wps_data_inst->keywrapkey, WPS_KEYWRAPKEY_LEN);
        print_hex("EMSK", wps_data_inst->emsk, WPS_EMSK_LEN);

        if (wps_data_inst->dh_privkey != NULL) {
        print_hex("DH Private Key", wpabuf_head_u8(wps_data_inst->dh_privkey), wpabuf_len(wps_data_inst->dh_privkey));
        } else {
            printf("DH Private Key: NULL\n");
        }

        if (wps_data_inst->dh_pubkey_e != NULL) {
            print_hex("DH Public Key E", wpabuf_head_u8(wps_data_inst->dh_pubkey_e), wpabuf_len(wps_data_inst->dh_pubkey_e));
        } else {
            printf("DH Public Key E: NULL\n");
        }

        if (wps_data_inst->dh_pubkey_r != NULL) {
            print_hex("DH Public Key R", wpabuf_head_u8(wps_data_inst->dh_pubkey_r), wpabuf_len(wps_data_inst->dh_pubkey_r));
        } else {
            printf("DH Public Key R: NULL\n");
        }

        if (wps_data_inst->dev_password != NULL) {
            print_hex("Device Password", wps_data_inst->dev_password, wps_data_inst->dev_password_len);
        } else {
            printf("Device Password: NULL\n");
        }

        if (wps_data_inst->alt_dev_password != NULL) {
            print_hex("Alternate Device Password", wps_data_inst->alt_dev_password, wps_data_inst->alt_dev_password_len);
        } else {
            printf("Alternate Device Password: NULL\n");
        }

        printf("Request Type: %02x\n", wps_data_inst->request_type);
        printf("Encryption Type: %04x\n", wps_data_inst->encr_type);
        printf("Authentication Type: %04x\n", wps_data_inst->auth_type);

        if (wps_data_inst->new_psk != NULL) {
            print_hex("New PSK", wps_data_inst->new_psk, wps_data_inst->new_psk_len);
        } else {
            printf("New PSK: NULL\n");
        }

        printf("WPS PIN Revealed: %d\n", wps_data_inst->wps_pin_revealed);
        printf("Config Error: %04x\n", wps_data_inst->config_error);
        printf("Error Indication: %04x\n", wps_data_inst->error_indication);
        printf("PBC in M1: %d\n", wps_data_inst->pbc_in_m1);

        print_hex("Peer Public Key Hash", wps_data_inst->peer_pubkey_hash, WPS_OOB_PUBKEY_HASH_LEN);
        printf("Peer Public Key Hash Set: %d\n", wps_data_inst->peer_pubkey_hash_set);

    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_TIMEOUT) {
        ESP_LOGE(TAG, "WPS connection timed out.");
        esp_wifi_wps_disable();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_PIN) {
        wifi_event_sta_wps_er_pin_t* wps_pin_event = (wifi_event_sta_wps_er_pin_t*)event_data;
        ESP_LOGI(TAG, "WPS PIN Event: PIN = %s", wps_pin_event->pin_code);
    }
}


void wps_start_connection(uint8_t *bssid) {
	
    esp_wifi_set_mode(WIFI_MODE_STA);

    
    esp_wps_config_t wps_config = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PIN);

    
    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.bssid, bssid, 6);
    wifi_config.sta.bssid_set = true;
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wps_event_handler, NULL);

    
    esp_wifi_wps_enable(&wps_config);
    esp_wifi_wps_start(0);

	ESP_LOGI(TAG, "WPS PIN connection initiated for BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
				bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
}