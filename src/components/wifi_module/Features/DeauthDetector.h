#pragma once
#include <Arduino.h>
#include "../wifi_module.h"
#include "esp_wifi_types.h"
#include <HTTPClient.h>
const int MAX_CHANNELS = 12;


struct Wifianalyticsdata {
    unsigned long deauthCount[MAX_CHANNELS];
    unsigned long authCount[MAX_CHANNELS];
    unsigned long beaconCount[MAX_CHANNELS];
    unsigned long probeReqCount[MAX_CHANNELS];
    unsigned long probeRespCount[MAX_CHANNELS];
    unsigned long otherPktCount[MAX_CHANNELS];
    unsigned long startTime[MAX_CHANNELS];
    unsigned long endTime[MAX_CHANNELS];         
    String deviceMac;
};

struct DeauthDetectorConfig
{
unsigned long deauthCount = 0;
unsigned long lastCheckTime = 0;
unsigned long lastChannelCheckTime = 0;
unsigned long startTime = 0;
unsigned long checkInterval = 1000;
unsigned long ChannelHopInterval = 5000;
unsigned long scanDuration = 2 * 60000;
unsigned long deauthThreshold = 20;
int CurrentChannel;
bool ConfigHasChannel;
Wifianalyticsdata* analyticsdata;
};

DeauthDetectorConfig DeauthConfig;

void AnalyticsCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t_2 *ipkt = (wifi_ieee80211_packet_t_2 *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;


    switch(hdr->frame_ctrl & 0xFC) { // Mask to get type and subtype only
        case 0xB0: // Authentication frame
            DeauthConfig.analyticsdata->authCount[DeauthConfig.CurrentChannel]++;
            break;
        case 0xC0: // Deauthentication frame
            DeauthConfig.analyticsdata->deauthCount[DeauthConfig.CurrentChannel]++;
            DeauthConfig.deauthCount++;
            break;
        case 0x00: // Association request frame
            DeauthConfig.analyticsdata->probeReqCount[DeauthConfig.CurrentChannel]++;
            break;
        case 0x30: // Reassociation response frame
            DeauthConfig.analyticsdata->probeRespCount[DeauthConfig.CurrentChannel]++;
            break;
        case 0x40: // Probe request frame
            DeauthConfig.analyticsdata->probeReqCount[DeauthConfig.CurrentChannel]++;
            break;
        case 0x50: // Probe response frame
            DeauthConfig.analyticsdata->probeRespCount[DeauthConfig.CurrentChannel]++;
            break;
        case 0x80: // Beacon frame
            DeauthConfig.analyticsdata->beaconCount[DeauthConfig.CurrentChannel]++;
            break;
        default: 
            DeauthConfig.analyticsdata->otherPktCount[DeauthConfig.CurrentChannel]++;
            break;
    }
}

String prepareAnalyticsSummary() {
    String summary = "WiFi Analytics Summary:\n------------------------------\n";

    for (int channelIndex = 1; channelIndex < MAX_CHANNELS; channelIndex++) {
        summary += "Monitored Channel: " + String(channelIndex) + "\n";
        summary += "Monitoring Duration: " +
            String((DeauthConfig.analyticsdata->endTime[channelIndex] - DeauthConfig.analyticsdata->startTime[channelIndex]) / 60000) +
            " minutes (" +
            String((DeauthConfig.analyticsdata->endTime[channelIndex] - DeauthConfig.analyticsdata->startTime[channelIndex]) / 1000) +
            " seconds)\n";
        summary += "Deauthentication Packets: " + String(DeauthConfig.analyticsdata->deauthCount[channelIndex]) + "\n";
        summary += "Authentication Packets: " + String(DeauthConfig.analyticsdata->authCount[channelIndex]) + "\n";
        summary += "Beacon Packets: " + String(DeauthConfig.analyticsdata->beaconCount[channelIndex]) + "\n";
        summary += "Probe Request Packets: " + String(DeauthConfig.analyticsdata->probeReqCount[channelIndex]) + "\n";
        summary += "Probe Response Packets: " + String(DeauthConfig.analyticsdata->probeRespCount[channelIndex]) + "\n";
        summary += "Other Packet Types: " + String(DeauthConfig.analyticsdata->otherPktCount[channelIndex]) + "\n";
        summary += "------------------------------\n";
    }

    return summary;
}

void sendWifiAnalitics(String Channel, String SSID, String Password, String WebHookUrl) {
#ifdef OLD_LED
SystemManager::getInstance().rgbModule->breatheLED(SystemManager::getInstance().rgbModule->greenPin, 1000);
#endif

#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(0, 255, 0), 1000, false);
#endif

if (!WebHookUrl.isEmpty())
{
    WiFi.begin(SSID, Password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    if (WebHookUrl.startsWith("http://") || WebHookUrl.startsWith("https://")) {
        
        int protocolEndIndex = WebHookUrl.indexOf("://") + 3;

        
        int firstSlashAfterProtocol = WebHookUrl.indexOf('/', protocolEndIndex);
        

        String host = WebHookUrl.substring(protocolEndIndex, firstSlashAfterProtocol);
        String path = WebHookUrl.substring(firstSlashAfterProtocol); // Until the end of the string

        
        Serial.println("Host: " + host);
        Serial.println("Path: " + path);


        int port = 443;
        
        String message;
        
        unsigned long duration = DeauthConfig.analyticsdata->endTime - DeauthConfig.analyticsdata->startTime; 

        
        for (int channelIndex = 1; channelIndex < MAX_CHANNELS; channelIndex++) {
            
            WiFiClientSecure wifiClient;
            wifiClient.setInsecure();
            HttpClient http(wifiClient, host, port);

            String message = "{\"content\": \"\", \"embeds\": [{";
            message += "\"title\": \"WiFi Analytics Summary - Channel " + String(channelIndex) + "\",";
            message += "\"color\": 3447003,";
            message += "\"fields\": [";
            message += "{\"name\":\"Monitored Channel\",\"value\":\"" + String(channelIndex) + "\",\"inline\":true},";
            message += "{\"name\":\"Monitoring Duration\",\"value\":\"" + String((DeauthConfig.analyticsdata->endTime[channelIndex] - DeauthConfig.analyticsdata->startTime[channelIndex]) / 60000) + " minutes (" + String((DeauthConfig.analyticsdata->endTime[channelIndex] - DeauthConfig.analyticsdata->startTime[channelIndex]) / 1000) + " seconds)\",\"inline\":true},";
            message += "{\"name\":\"Deauthentication Packets\",\"value\":\"" + String(DeauthConfig.analyticsdata->deauthCount[channelIndex]) + "\",\"inline\":false},";
            message += "{\"name\":\"Authentication Packets\",\"value\":\"" + String(DeauthConfig.analyticsdata->authCount[channelIndex]) + "\",\"inline\":false},";
            message += "{\"name\":\"Beacon Packets\",\"value\":\"" + String(DeauthConfig.analyticsdata->beaconCount[channelIndex]) + "\",\"inline\":false},";
            message += "{\"name\":\"Probe Request Packets\",\"value\":\"" + String(DeauthConfig.analyticsdata->probeReqCount[channelIndex]) + "\",\"inline\":false},";
            message += "{\"name\":\"Probe Response Packets\",\"value\":\"" + String(DeauthConfig.analyticsdata->probeRespCount[channelIndex]) + "\",\"inline\":false},";
            message += "{\"name\":\"Other Packet Types\",\"value\":\"" + String(DeauthConfig.analyticsdata->otherPktCount[channelIndex]) + "\",\"inline\":false}";
            message += "]}]}";

            
            http.beginRequest();
            http.post(path, "application/json", message);
            int httpResponseCode = http.responseStatusCode();
            if (httpResponseCode > 0) {
                Serial.print("Data for channel ");
                Serial.print(channelIndex);
                Serial.println(" sent successfully");
            } else {
                Serial.print("Error sending data for channel ");
                Serial.println(channelIndex);
            }
            http.endRequest();

            wifiClient.stop();
            delay(1000);
        }
    } else {
        Serial.println("Invalid URL: does not start with 'http://' or 'https://'");
    }
    WiFi.disconnect();
}


Serial.println("WiFi Analytics Summary:");
Serial.println("------------------------------");

for (int channelIndex = 1; channelIndex < MAX_CHANNELS; channelIndex++) {
    Serial.print("Monitored Channel: ");
    Serial.println(channelIndex);
    Serial.print("Monitoring Duration: ");
    
    Serial.print((DeauthConfig.analyticsdata->endTime[channelIndex] - DeauthConfig.analyticsdata->startTime[channelIndex]) / 60000);
    Serial.print(" minutes (");
    Serial.print((DeauthConfig.analyticsdata->endTime[channelIndex] - DeauthConfig.analyticsdata->startTime[channelIndex]) / 1000);
    Serial.println(" seconds)");
    Serial.print("Deauthentication Packets: ");
    Serial.println(DeauthConfig.analyticsdata->deauthCount[channelIndex]);
    Serial.print("Authentication Packets: ");
    Serial.println(DeauthConfig.analyticsdata->authCount[channelIndex]);
    Serial.print("Beacon Packets: ");
    Serial.println(DeauthConfig.analyticsdata->beaconCount[channelIndex]);
    Serial.print("Probe Request Packets: ");
    Serial.println(DeauthConfig.analyticsdata->probeReqCount[channelIndex]);
    Serial.print("Probe Response Packets: ");
    Serial.println(DeauthConfig.analyticsdata->probeRespCount[channelIndex]);
    Serial.print("Other Packet Types: ");
    Serial.println(DeauthConfig.analyticsdata->otherPktCount[channelIndex]);
    Serial.println("------------------------------");
}

LOG_RESULTS("PacketReport.txt", "/WifiAnalytics", prepareAnalyticsSummary().c_str());

#ifdef OLD_LED
SystemManager::getInstance().rgbModule->breatheLED(SystemManager::getInstance().rgbModule->greenPin, 1000);
#endif


#ifdef NEOPIXEL_PIN
SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(0, 255, 0), 1000, false);
#endif

delete DeauthConfig.analyticsdata;

Serial.println("Command Finished");
}


void InitDeauthDetector(String Channel, String SSID, String Password, String WebHookUrl) 
{
    DeauthConfig.startTime = millis();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Channel.trim();

    DeauthConfig.analyticsdata = new Wifianalyticsdata();

    DeauthConfig.ConfigHasChannel = !Channel.isEmpty();

    if (DeauthConfig.ConfigHasChannel)
    {
        DeauthConfig.CurrentChannel = Channel.toInt();
    }

    Serial.println("Channel");
    Serial.println(DeauthConfig.CurrentChannel);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(AnalyticsCallback);
    esp_wifi_set_channel(DeauthConfig.CurrentChannel, WIFI_SECOND_CHAN_NONE);

    while (millis() - DeauthConfig.startTime < DeauthConfig.scanDuration) {
        if (millis() - DeauthConfig.lastCheckTime > DeauthConfig.checkInterval) {
            if (DeauthConfig.deauthCount > DeauthConfig.deauthThreshold) {
                #ifdef OLD_LED
                if (!SystemManager::getInstance().RainbowLEDActive)
                {
                    SystemManager::getInstance().rgbModule->breatheLED(SystemManager::getInstance().rgbModule->redPin, 500);
                }
                #endif
                #ifdef NEOPIXEL_PIN
                SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(255, 0, 0), 1000, false);
                #endif
                Serial.println("Deauth Detected...");
            } else {
                #ifdef OLD_LED
                if (!SystemManager::getInstance().RainbowLEDActive)
                {
                    SystemManager::getInstance().rgbModule->breatheLED(SystemManager::getInstance().rgbModule->greenPin, 500);
                }
               
                #endif
                #ifdef NEOPIXEL_PIN
                SystemManager::getInstance().neopixelModule->breatheLED(SystemManager::getInstance().neopixelModule->strip.Color(0, 255, 0), 1000, false);
                #endif
                Serial.println("Normal Network Behavior...");
            }
            DeauthConfig.deauthCount = 0;
            DeauthConfig.lastCheckTime = millis();
        }

        if (millis() - DeauthConfig.lastChannelCheckTime > DeauthConfig.ChannelHopInterval && !DeauthConfig.ConfigHasChannel)
        {
            DeauthConfig.CurrentChannel = (DeauthConfig.CurrentChannel % MAX_CHANNELS) + 1;
            esp_wifi_set_channel(DeauthConfig.CurrentChannel, WIFI_SECOND_CHAN_NONE);
            DeauthConfig.lastChannelCheckTime = millis();
        }
    }

    for (int channelIndex = 1; channelIndex < MAX_CHANNELS; channelIndex++) 
    {
        DeauthConfig.analyticsdata->endTime[channelIndex] = millis();
        DeauthConfig.analyticsdata->startTime[channelIndex] =  DeauthConfig.startTime;
    }


    
    esp_wifi_set_promiscuous(false);
    sendWifiAnalitics(Channel, SSID, Password, WebHookUrl);
}