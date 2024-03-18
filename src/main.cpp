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

    
    #ifdef OLD_LED
        rgbmodule = new RGBLedModule(LED_R, LED_G, LED_B);
        rgbmodule->init();

    #elif NEOPIXEL_PIN
        neopixelmodule = new NeopixelModule(Pixels, NEOPIXEL_PIN);
        neopixelmodule->init();
    #endif

    #if DISPLAY_SUPPORT
        
    #endif

    #ifdef SD_CARD_CS_PIN
        sdCardmodule = new SDCardModule(); 
        pinMode(SD_CARD_CS_PIN, OUTPUT);
        delay(10);
        digitalWrite(SD_CARD_CS_PIN, HIGH);

        sdCardmodule->init();
    #endif

    #ifdef HAS_GPS
    gpsmodule = new GpsInterface();
    gpsmodule->begin();
    if (gpsmodule->getGpsModuleStatus())
    {
        Serial.println("GPS Module connected");
    }
    else 
    {
        Serial.println("GPS Module NOT connected");
    }
    #endif

    #ifdef HAS_BT

    BleModule = new BLEModule();
    BleModule->init();
    #endif
    Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));

    wifimodule = new WiFiModule();

    wifimodule->RunSetup();

    cli->RunSetup();

    registerCallback(
        [](String &msg) { return msg.indexOf("reset") != -1; },
        [](String &msg) { 
            Serial.println("Reset command received. Rebooting...");
            esp_restart();
        }
    );

    registerCallback(
        [](String &msg) { return msg.indexOf("stop") != -1; },
        [](String &msg) { 
            Serial.println("Stop command received. Handling stop...");
        }
    );

    registerCallback(
          [](String &msg) { return msg.indexOf("stopscan") != -1; },
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

    xTaskCreate(SerialCheckTask, "SerialCheckTask", 2048, NULL, 1, NULL);
}