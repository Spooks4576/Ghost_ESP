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
    tft.setRotation(1);
    drawMainMenu();
   }
}

void DisplayModule::drawMainMenu()
{
    tft.fillScreen(TFT_BLACK);
    for (int i = 0; i < numCards; i++) {
        drawCard(cards[i]);
    }
}

void DisplayModule::animateMenu()
{

}

void DisplayModule::setButtonCallback(int buttonIndex, void (*callback)()) {
   
}

void DisplayModule::animateCardPop(const Card &card) {
    int expandAmount = 4;
    for (int i = 0; i <= expandAmount; i += 2) {
        tft.fillRect(card.x - i, card.y - i, card.w + 2*i, card.h + 2*i, card.isSelected ? TFT_RED : card.bgColor);
        tft.drawRect(card.x - i, card.y - i, card.w + 2*i, card.h + 2*i, TFT_WHITE);  // Card outline
        delay(15);
    }
    for (int i = expandAmount; i >= 0; i -= 2) {
        tft.fillRect(card.x - i, card.y - i, card.w + 2*i, card.h + 2*i, TFT_BLACK);  // Erase previous size
        drawCard(card);
        delay(15);
    }
}

void DisplayModule::checkTouch(int tx, int ty) {
     for (int i = 0; i < numCards; i++) {
        if (tx > cards[i].x && tx < cards[i].x + cards[i].w && ty > cards[i].y && ty < cards[i].y + cards[i].h) {
            cards[i].isSelected = true;
            drawCard(cards[i]);
            drawSelectedLabel(cards[i].title);
        } 
        else 
        {
            cards[i].isSelected = false;
            drawCard(cards[i]);
        }
    }
}

void DisplayModule::drawSelectedLabel(const String &label) {
    tft.fillRect(0, 0, 240, 30, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString(label, 5, 10, 2);
}

void DisplayModule::drawCard(const Card &card) {
    
    tft.fillRect(card.x, card.y, card.w, card.h, card.bgColor);
    
    if (card.imageBuffer != nullptr) {
        tft.pushImage(card.x, card.y, card.w, card.h, card.imageBuffer);
    }
}

void DisplayModule::Init()
{
    IsOnSplash = true;
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin();
    ts.setRotation(1);
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
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