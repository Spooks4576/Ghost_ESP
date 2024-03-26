#include "display_module.h"
#ifdef DISPLAY_SUPPORT

#include <JPEGDecoder.h>
#include <core/logo.h>

void ReadTouchPadCallback( lv_indev_t * indev, lv_indev_data_t * data )
{
  if(touchscreen->touched())
  {
    TS_Point p = touchscreen->getPoint();
    //Some very basic auto calibration so it doesn't go out of range
    if(p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
    if(p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
    if(p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
    if(p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;
    //Map this to the pixel position
    data->point.x = map(p.x,touchScreenMinimumX,touchScreenMaximumX,1,TFT_HOR_RES); /* Touchscreen X calibration */
    data->point.y = map(p.y,touchScreenMinimumY,touchScreenMaximumY,1,TFT_VER_RES); /* Touchscreen Y calibration */
    data->state = LV_INDEV_STATE_PRESSED;
    Serial.print("Touch x ");
    Serial.print(data->point.x);
    Serial.print(" y ");
    Serial.println(data->point.y);
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void DisplayModule::drawProgressBar(int x, int y, int w, int h, int progress, uint16_t color)
{
    uint8_t radius = h / 2;
    int progressBarWidth = (w - 2) * progress / 100;

  
    tft.drawRoundRect(x, y, w, h, radius, TFT_WHITE);

    
    if (progressBarWidth > radius) {
        tft.fillRoundRect(x + 1, y + 1, progressBarWidth, h - 2, radius, color);
    } else {
       
        tft.fillRect(x + 1, y + 1, progressBarWidth, h - 2, color);
    }
}

void DisplayModule::drawCurrentMenuItem() {
    if (mainmenu.prevItemIndex != mainmenu.currentItemIndex) {
        int direction = (mainmenu.prevItemIndex < mainmenu.currentItemIndex) ? 1 : -1;
        if (mainmenu.prevItemIndex == -1) {
            direction = 0;
        }

        for (int offset = 0; offset <= tft.width(); offset += 20) {  
            tft.fillScreen(TFT_BLACK);
            int x = (tft.width() - 200) / 2 - direction * offset; 
            int y = (tft.height() - 50) / 2;

           
            if (mainmenu.prevItemIndex != -1 && offset < tft.width()) {
                MenuItem &prevItem = mainmenu.menuItems[mainmenu.prevItemIndex];
                drawButton(x + direction * tft.width(), y, 200, 50, prevItem.label, false);
            }

            // Draw current item
            MenuItem &item = mainmenu.menuItems[mainmenu.currentItemIndex];
            drawButton(x, y, 200, 50, item.label, false);

            delay(25);
        }
    }
    mainmenu.prevItemIndex = mainmenu.currentItemIndex; 
}

void DisplayModule::detectSwipeAndSwitchItems() {
    if (touchscreen->touched()) {
        TS_Point p = touchscreen->getPoint();
        p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
        p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

        if (!mainmenu.swipeStart) {
            mainmenu.swipeStart = true;
            mainmenu.startX = p.x;
        }
    } else {
        if (mainmenu.swipeStart) { 
            mainmenu.swipeStart = false;
            mainmenu.endX = mainmenu.lastTouchPoint.x;

            int dx = mainmenu.endX - mainmenu.startX; 
            if (abs(dx) > SWIPE_THRESHOLD) {
                mainmenu.prevItemIndex = mainmenu.currentItemIndex;
                if (dx > 0) {
                    mainmenu.currentItemIndex--;
                } else {
                    mainmenu.currentItemIndex++;
                }
                mainmenu.currentItemIndex = (mainmenu.currentItemIndex + mainmenu.menuItems.size()) % mainmenu.menuItems.size();

                if (mainmenu.currentItemIndex != mainmenu.prevItemIndex) { 
                    mainmenu.needRedraw = true;
                }
            }
        }
    }

    if (mainmenu.needRedraw) {
        drawCurrentMenuItem();
        mainmenu.needRedraw = false;  // Reset redraw flag after updating display
    }
}

bool DisplayModule::isButtonTouched(Button &btn, TS_Point p) {
    return (p.x >= btn.x && p.x <= (btn.x + btn.w) && p.y >= btn.y && p.y <= (btn.y + btn.h));
}

void DisplayModule::drawButton(int x, int y, int w, int h, String text, bool isPressed) {
    uint16_t backgroundColor = isPressed ? TFT_DARKGREY : TFT_LIGHTGREY;
    uint16_t borderColor = TFT_WHITE;
    uint16_t textColor = TFT_BLACK;
    uint8_t radius = 6; 

    
    tft.fillRoundRect(x, y, w, h, radius, backgroundColor);

    
    tft.drawRoundRect(x, y, w, h, radius, borderColor);

    
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(text, x + w / 2, y + h / 2 - 8);
}

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
   }
}

void DisplayModule::RenderSplashScreen()
{
    IsOnSplash = true;
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    touchscreen = new XPT2046_Touchscreen(XPT2046_CS, XPT2046_IRQ);
    touchscreenSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); 
    touchscreen->begin(); 
    touchscreen->setRotation(0);
    mainmenu.menuItems.add({"Ble Attacks", TFT_BLUE, TFT_WHITE});
    mainmenu.menuItems.add({"WiFi Utils", TFT_GREEN, TFT_WHITE});
    mainmenu.menuItems.add({"Led Utils", TFT_CYAN, TFT_WHITE});
}

#endif