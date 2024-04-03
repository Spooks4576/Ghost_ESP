#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "board_config.h"
#include "core/globals.h"
#include <Arduino.h>
#include <SD.h>

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
  return 0;
}

uint32_t lastTick = 0;

void loop() {

#ifdef DISPLAY_SUPPORT
    uint16_t x, y, z;
    z = displaymodule->tft.getTouchRawZ();
    z = (z < 12) ? 0 : z; // Helps Remove Noise Values

    if (z > 0)
    {
        displaymodule->tft.getTouch(&x, &y);
        TS_Point point{x, y, z};
        displaymodule->checkTouch(point);
    }
    lv_tick_inc(millis() - lastTick);
    lastTick = millis();
    lv_timer_handler();
    delay(5);
#endif
#ifndef DISPLAY_SUPPORT
    if (!HasRanCommand)
    {
        double currentTime = millis();

        cli->main(currentTime);
    }
#endif
}

void SerialCheckTask(void *pvParameters) {
    while (1) {
        if (HasRanCommand)
        {   
            if (Serial.available() > 0) {
                String message = Serial.readStringUntil('\n');
                Serial.println(message);

                if (message.startsWith("stop"))
                {
                    if (HasRanCommand)
                    {
                        esp_restart();
                    }
                    else 
                    {
                        wifimodule->shutdownWiFi();
                        #ifdef SD_CARD_CS_PIN
                        sdCardmodule->stopPcapLogging();
                        #endif
                        #ifdef HAS_BT
                        BleModule->shutdownBLE();
                        #endif
                    }
                }
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void setup() 
{
    Serial.begin(115200);

    #ifdef DISPLAY_SUPPORT
        Serial.println("About to Init Display");
        displaymodule = new DisplayModule();
        displaymodule->Init();
        Serial.println("Init Display");
    #endif
    
    #ifdef OLD_LED
        rgbmodule = new RGBLedModule(LED_R, LED_G, LED_B);
        rgbmodule->init();
    #elif NEOPIXEL_PIN
        neopixelmodule = new NeopixelModule(Pixels, NEOPIXEL_PIN);
        neopixelmodule->init();
    #endif

#ifdef SD_CARD_CS_PIN
#ifdef DISPLAY_SUPPORT
displaymodule->UpdateSplashStatus("Attempting to Mount SD Card", 25);
#endif
        sdCardmodule = new SDCardModule(); 
        pinMode(SD_CARD_CS_PIN, OUTPUT);
        delay(10);
        digitalWrite(SD_CARD_CS_PIN, HIGH);

        bool status = sdCardmodule->init();

        if (status)
        {   
            LOG_MESSAGE_TO_SD("Mounted SD Card Successfully");
        }
#ifdef DISPLAY_SUPPORT
        if (status)
        {
            displaymodule->UpdateSplashStatus("Mounted SD Card Successfully", 50);
            delay(100);
        }
        else 
        {
            displaymodule->UpdateSplashStatus("Failed to Mount SD Card Continuing", 50);
            delay(100);
        }
#endif
    #endif

    #ifdef HAS_GPS
    
    #endif

    #ifdef HAS_BT
    BleModule = new BLEModule();
    BleModule->init();

    LOG_MESSAGE_TO_SD("Initilized BLE...");
    #ifdef DISPLAY_SUPPORT
        displaymodule->UpdateSplashStatus("Initilized BLE...", 80);
        delay(1000);
        displaymodule->UpdateSplashStatus(esp_get_idf_version(), 80);
    #endif
    #endif

    Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));
    LOG_MESSAGE_TO_SD("ESP IDF Version = ");
    LOG_MESSAGE_TO_SD(esp_get_idf_version());

    wifimodule = new WiFiModule();

    wifimodule->RunSetup();

    cli->RunSetup();

    LOG_MESSAGE_TO_SD("Wifi Initilized");
#ifdef DISPLAY_SUPPORT
    displaymodule->UpdateSplashStatus("Wifi Initilized", 95);
    delay(500);
#endif
#ifndef DISPLAY_SUPPORT
    xTaskCreate(SerialCheckTask, "SerialCheckTask", 2048, NULL, 1, NULL);
#endif
    LOG_MESSAGE_TO_SD("Registered Multithread Callbacks");
#ifdef DISPLAY_SUPPORT
    displaymodule->UpdateSplashStatus("Ghost ESP...", 100);
    delay(500);
#endif
}