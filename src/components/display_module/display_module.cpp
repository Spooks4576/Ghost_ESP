#include "display_module.h"
#ifdef DISPLAY_SUPPORT

#include <JPEGDecoder.h>

void DisplayModule::RenderJpg(int xpos, int ypos, int w, int h) {
    uint16_t  *pImg;
    int16_t mcu_w = JpegDec.MCUWidth;
    int16_t mcu_h = JpegDec.MCUHeight;

    // Calculate the max_x and max_y for the rendered image based on input w and h
    int32_t max_x = (w <= 0) ? JpegDec.width : min(w, JpegDec.width);
    int32_t max_y = (h <= 0) ? JpegDec.height : min(h, JpegDec.height);

    // Determine the width and height of the right and bottom edge image blocks
    int32_t min_w = minimum(mcu_w, max_x % mcu_w);
    int32_t min_h = minimum(mcu_h, max_y % mcu_h);

    // Adjust min_w and min_h for cases where image is smaller than one MCU
    min_w = (min_w == 0) ? mcu_w : min_w;
    min_h = (min_h == 0) ? mcu_h : min_h;

    // Save the coordinate of the right and bottom edges to assist image cropping
    int32_t draw_max_x = xpos + max_x;
    int32_t draw_max_y = ypos + max_y;

    // Loop through all the MCUs in the JPEG
    while (JpegDec.readSwappedBytes()) { // Swapped byte order read
        // Get a pointer to the MCU image block
        pImg = JpegDec.pImage;

        // Calculate where the image block should be drawn on the screen
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // Determine the size of this MCU block
        int draw_w = ((mcu_x + mcu_w) <= draw_max_x) ? mcu_w : (draw_max_x - mcu_x);
        int draw_h = ((mcu_y + mcu_h) <= draw_max_y) ? mcu_h : (draw_max_y - mcu_y);

        // Draw image MCU block only if it will fit on the screen
        if (mcu_x < tft.width() && mcu_y < tft.height() && draw_w > 0 && draw_h > 0) {
            tft.pushImage(mcu_x, mcu_y, draw_w, draw_h, pImg);
            if (w > 0 && h > 0)
            {
                tft.fillRect(xpos + w - 3, ypos, 3, h, TFT_BLACK);
            }
        }

        // Abort the drawing if we've reached the bottom of the intended drawing area
        if (mcu_y >= draw_max_y) JpegDec.abort();
    }
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
    drawCard(card); 
    JpegDec.decodeArray(card.imageBuffer, card.imagebuffersize);

    for (int i = 0; i <= expandAmount; i += 2) {
        // Expand the card background and outline
        tft.drawRect(card.x - i, card.y - i, card.w + 2*i, card.h + 2*i, TFT_WHITE);  // Card outline
        if (card.imageBuffer != nullptr) {
            // Center the image within the expanded area
            int imgX = card.x - i + (card.w + 2*i - card.w) / 2;
            int imgY = card.y - i + (card.h + 2*i - card.h) / 2;
            RenderJpg(imgX, imgY, card.w, card.h);
        }
        delay(15);
    }
    for (int i = expandAmount; i >= 0; i -= 2) {
        tft.fillRect(card.x - i, card.y - i, card.w + 2*i, card.h + 2*i, TFT_BLACK);
        drawCard(card);
        delay(15);
    }
}

void DisplayModule::checkTouch(int tx, int ty) {
    if (LastTouchX != tx && LastTouchY != ty)
    {
        for (int i = 0; i < numCards; i++) {
           
            if (tx > cards[i].x && tx < cards[i].x + cards[i].w && ty > cards[i].y && ty < cards[i].y + cards[i].h) {
                cards[i].isSelected = true;
                drawCard(cards[i]);
                drawSelectedLabel(cards[i].title);
                animateCardPop(cards[i]);
                LastTouchX = tx;
                LastTouchY = ty;
            } 
            else 
            {
                cards[i].isSelected = false;
                drawCard(cards[i]);
                LastTouchX = tx;
                LastTouchY = ty;
            }
        }
    }
}

void DisplayModule::drawSelectedLabel(const String &label) {
    tft.fillRect(0, 0, 240, 30, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.drawString(label, 50, 10, 2);
}

void DisplayModule::drawCard(const Card &card) {
    if (card.imageBuffer != nullptr) {
        bool decoded = JpegDec.decodeArray(card.imageBuffer, card.imagebuffersize);
        
        if (decoded)
        {
            RenderJpg(card.x, card.y, card.w, card.h);
        }
    }
    else 
    {
        tft.fillRect(card.x, card.y, card.w, card.h, card.bgColor);
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