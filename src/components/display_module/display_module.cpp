#include "display_module.h"
#ifdef DISPLAY_SUPPORT

#include <JPEGDecoder.h>
#include <core/logo.h>

void DisplayModule::RenderJpg(int xpos, int ypos)
{
  uint16_t  *pImg;
  int16_t mcu_w = JpegDec.MCUWidth;
  int16_t mcu_h = JpegDec.MCUHeight;
  int32_t max_x = JpegDec.width;
  int32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  int32_t min_w = minimum(mcu_w, max_x % mcu_w);
  int32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  int32_t win_w = mcu_w;
  int32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while ( JpegDec.readSwappedBytes()) { // Swapped byte order read

    // save a pointer to the image block
    pImg = JpegDec.pImage;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      for (int h = 1; h < win_h-1; h++)
      {
        memcpy(pImg + h * win_w, pImg + (h + 1) * mcu_w, win_w << 1);
      }
    }

    // draw image MCU block only if it will fit on the screen
    if ( mcu_x < tft.width() && mcu_y < tft.height())
    {
      // Now push the image block to the screen
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    }

    else if ( ( mcu_y + win_h) >= tft.height()) JpegDec.abort();

  }

    
  drawTime = millis() - drawTime; // Calculate the time it took
}

void DisplayModule::UpdateSplashStatus(const char* Text, int Percent)
{
    Splash.Text = Text;
    Splash.Progress = Percent;

    if (logo_jpg != nullptr) {
        boolean decoded = JpegDec.decodeArray(logo_jpg, logo_jpg_size);
        if (decoded) {
            int xpos = (240 - JpegDec.width) / 2; 
            int ypos = (320 - 100 - JpegDec.height) / 2; 
            RenderJpg(xpos, ypos);
        }
    }

   
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
    int textYpos = (320 - JpegDec.height - 30) / 2 + JpegDec.height + 10;
    tft.drawString(Splash.Text, 120, textYpos, 2);
    
   if (Splash.Progress > 99)
   {
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    IsOnSplash = false;
    drawMainMenu();
    animateMenu();
   }
}

void DisplayModule::drawMainMenu()
{
    spr.fillSprite(backgroundColor);
    for (Button &btn : MainMenuButtons) {
        btn.draw(spr);
    }
    spr.pushSprite(0, 0);
}

void DisplayModule::animateMenu()
{
    
    for (int i = -TFT_WIDTH; i <= 0; i += 20) {
        spr.fillSprite(backgroundColor);
        for (Button &btn : MainMenuButtons) {
            btn.x = 60 + i;
            btn.draw(spr);
        }
        spr.pushSprite(0, 0);
        delay(5);
    }
}

void DisplayModule::setButtonCallback(int buttonIndex, void (*callback)()) {
    if (buttonIndex >= 0 && buttonIndex < sizeof(MainMenuButtons) / sizeof(MainMenuButtons[0])) {
        MainMenuButtons[buttonIndex].callback = callback;
    }
}

void DisplayModule::checkTouch(int tx, int ty) {
    for (Button &btn : MainMenuButtons) {
        if (btn.contains(tx, ty)) {
            btn.isPressed = true;
            btn.draw(spr);
            spr.pushSprite(0, 0);
            delay(100);

            if (btn.callback) { 
                btn.callback();
            }

            btn.isPressed = false;
            btn.draw(spr);
            spr.pushSprite(0, 0);
            break;
        }
    }
}

void DisplayModule::Init()
{
    IsOnSplash = true;
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    ts.begin();
    ts.setRotation(0);
}

void DisplayModule::printTouchToSerial(TS_Point p) {
  Serial.print("Pressure = ");
  Serial.print(p.z);
  Serial.print(", x = ");
  Serial.print(p.x);
  Serial.print(", y = ");
  Serial.print(p.y);
  Serial.println();
}

#endif