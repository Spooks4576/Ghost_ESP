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

void loop() {
    double currentTime = millis();

    cli->main(currentTime);
}

void SerialCheckTask(void *pvParameters) {
    while (1) {
        if (Serial.available() > 0) {
            String message = Serial.readString();
            Serial.println(message);

            for (int i = 0; i < callbacks.size(); ++i) {
                if (callbacks[i].condition(message)) {
                    callbacks[i].callback(message);
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
        displaymodule->init();
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
    gpsmodule = new GpsInterface();
    gpsmodule->begin();
    if (gpsmodule->getGpsModuleStatus())
    {
        #ifdef DISPLAY_SUPPORT
        displaymodule->UpdateSplashStatus("GPS Module Connected Successfully", 75);
        delay(100);
        #endif
        Serial.println("GPS Module connected");
    }
    else 
    {
        #ifdef DISPLAY_SUPPORT
        displaymodule->UpdateSplashStatus("GPS Module Failed to Connect", 75);
        delay(100);
        #endif
        Serial.println("GPS Module NOT connected");
    }
    #endif

    #ifdef HAS_BT
    BleModule = new BLEModule();
    BleModule->init();
    #ifdef DISPLAY_SUPPORT
        displaymodule->UpdateSplashStatus("Initilized BLE...", 80);
        delay(100);
        displaymodule->UpdateSplashStatus(esp_get_idf_version(), 80);
    #endif
    #endif

    Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));

    wifimodule = new WiFiModule();

    wifimodule->RunSetup();

    cli->RunSetup();
#ifdef DISPLAY_SUPPORT
    displaymodule->UpdateSplashStatus("Wifi Initilized", 95);
    delay(100);
#endif

    registerCallback(
          [](String &msg) { return msg.indexOf("stop") != -1; },
          [](String &msg) { 
            #ifdef OLD_LED
              rgbmodule->setColor(1, 1, 1);
            #endif
              wifimodule->shutdownWiFi();

            #ifdef HAS_BT
              BleModule->shutdownBLE();  
            #endif
        }
    );

    registerCallback(
        [](String &msg) { return msg.indexOf("reset") != -1; },
        [](String &msg) { 
            Serial.println("Reset command received. Rebooting...");
            esp_restart();
        }
    );

    xTaskCreate(SerialCheckTask, "SerialCheckTask", 2048, NULL, 1, NULL);
#ifdef DISPLAY_SUPPORT
    displaymodule->UpdateSplashStatus("Registered Multithread Callbacks", 100);
#endif
}