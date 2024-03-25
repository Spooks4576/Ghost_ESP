#pragma once

#include "board_config.h"

#ifdef DISPLAY_SUPPORT
#include <TFT_eSPI.h>
#include <lvgl.h>

struct SplashScreen {
    TFT_eSPI tft = TFT_eSPI(DISPLAYWIDTH, DISPLAYHEIGHT);  // Create display object
    TFT_eSprite spr = TFT_eSprite(&tft);  // Create a sprite object

    int percentage = 0; // Progress percentage
    String message = "Initializing..."; // Default message

    // Constructor
    SplashScreen() {
        tft.init();
        tft.setRotation(1);  // Adjust rotation according to your display setup
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        spr.createSprite(tft.width(), 40);  // Create a sprite for the progress bar
    }

    // Function to draw a progress bar
    void drawProgressBar() {
        spr.fillSprite(TFT_BLACK);
        spr.setTextColor(TFT_WHITE, TFT_BLACK);
        spr.setTextDatum(MC_DATUM);
        spr.drawString(message, spr.width() / 2, 10);
        spr.drawRect(0, 20, spr.width(), 10, TFT_WHITE);
        spr.fillRect(2, 22, (spr.width() - 4) * percentage / 100, 6, TFT_GREEN);
        spr.pushSprite(0, (tft.height() - spr.height()) / 2);  // Center the sprite vertically
    }

    // Function to incrementally update the splash screen
    void update(int newPercentage, String newMessage) {
        percentage = newPercentage;
        message = newMessage;
        drawProgressBar();  // Draw the updated progress bar
    }

    // Function to display the splash screen and wait for the process to complete
    void show() {
        // Display some introductory text or graphics
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Welcome to My Project", tft.width() / 2, tft.height() / 4, 4); // Font size and position

        while(percentage <= 99) { // Keep updating until process is done
            update(percentage, message);
            delay(100); // Here, just simulating progress. You should replace this with actual initialization steps and progress updates.
        }

        // After the loop ends, clear the progress bar and display a final message
        spr.fillSprite(TFT_BLACK);
        spr.pushSprite(0, (tft.height() - spr.height()) / 2);
        tft.drawString("Initialization Complete!", tft.width() / 2, tft.height() / 2, 2);
        delay(2000);  // Show the complete message for 2 seconds
    }
};

static lv_color_t buf[ DISPLAYWIDTH * DISPLAYHEIGHT / 10 ];

class DisplayModule {
public:
    SplashScreen* Splash;
    void UpdateSplashStatus(const char* Text, int Percent);
    void RenderSplashScreen();
};

#endif