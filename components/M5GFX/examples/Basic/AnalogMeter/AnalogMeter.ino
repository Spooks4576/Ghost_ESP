
#include <Arduino.h>

#include <M5GFX.h>
M5GFX display;

//#include <M5UnitOLED.h>
//M5UnitOLED display; // default setting
//M5UnitOLED display ( 21, 22, 400000 ); // SDA, SCL, FREQ

//#include <M5UnitLCD.h>
//M5UnitLCD display;  // default setting
//M5UnitLCD display  ( 21, 22, 400000 ); // SDA, SCL, FREQ

//#include <M5UnitGLASS2.h>
//M5UnitGLASS2 display;  // default setting
//M5UnitGLASS2 display ( 21, 22, 400000 ); // SDA, SCL, FREQ

//#include <M5AtomDisplay.h>
//M5AtomDisplay display;  // default setting
//M5AtomDisplay display ( 320, 180 ); // width, height


static constexpr float deg_to_rad = 0.017453292519943295769236907684886;
static constexpr int TFT_GREY = 0x5AEB;
static constexpr int LOOP_PERIOD = 35; // Display updates every 35 ms

int value[6] = {0, 0, 0, 0, 0, 0};
int old_value[6] = { -1, -1, -1, -1, -1, -1};
int liner_count = 6;
int d = 0;

int liner_height;
int meter_height;
int needle_x;
int needle_y;
int needle_r;
int osx, osy; // Saved x & y coords
uint32_t updateTime = 0;       // time for next update

void plotNeedle(int value)
{
  float sdeg = map(value, 0, 100, -135, -45); // Map value to angle
  // Calcualte tip of needle coords
  float sx = cosf(sdeg * deg_to_rad);
  float sy = sinf(sdeg * deg_to_rad);

  display.setClipRect(0, 0, display.width(), meter_height - 5);
  // Erase old needle image
  display.drawLine(needle_x - 1, needle_y, osx - 1, osy, TFT_WHITE);
  display.drawLine(needle_x + 1, needle_y, osx + 1, osy, TFT_WHITE);
  display.drawLine(needle_x    , needle_y, osx    , osy, TFT_WHITE);

  // Re-plot text under needle
  display.setTextColor(TFT_BLACK);
  if (display.width() > 100)
  {
    display.setFont(&fonts::Font4);
    display.setTextDatum(textdatum_t::middle_center);
    display.drawString("%RH", needle_x, needle_y>>1);
  }

  display.setFont(&fonts::Font2);
  display.setTextDatum(textdatum_t::bottom_left);
  display.drawString("%RH", display.width() - 36, meter_height - 8);

  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setTextDatum(textdatum_t::bottom_right);
  display.drawNumber(value, 36, meter_height - 8);


  // Store new needle end coords for next erase
  osx = roundf(sx * needle_r) + needle_x;
  osy = roundf(sy * needle_r) + needle_y;

  // Draw the needle in the new postion, magenta makes needle a bit bolder
  // draws 3 lines to thicken needle
  display.drawLine(needle_x - 1, needle_y, osx - 1, osy, TFT_RED);
  display.drawLine(needle_x + 1, needle_y, osx + 1, osy, TFT_RED);
  display.drawLine(needle_x    , needle_y, osx    , osy, TFT_MAGENTA);

  display.clearClipRect();
}

void analogMeter()
{
  display.fillRect(0, 0, display.width()   , meter_height  , TFT_WHITE);
  display.drawRect(1, 1, display.width()-2 , meter_height-2, TFT_BLACK);

  int r3 = needle_y * 13 / 15;
  int r2 = needle_y * 12 / 15;
  int r1 = needle_y * 11 / 15;
  needle_r = r1 - 3;
  display.fillArc(needle_x, needle_y, r1, r3, 270, 292, TFT_GREEN);
  display.fillArc(needle_x, needle_y, r1, r3, 292, 315, TFT_ORANGE);
  display.fillArc(needle_x, needle_y, r1, r1, 225, 315, TFT_BLACK);

  display.setTextColor(TFT_BLACK);
  display.setFont(&fonts::Font2);
  display.setTextDatum(textdatum_t::bottom_center);
  for (int i = 0; i <= 20; i++)
  {
    display.fillArc(needle_x, needle_y, r1, (i % 5) ? r2 : r3, 225 + i * 4.5f, 225 + i * 4.5f, TFT_BLACK);
    if (0 == (i % 5) && display.width() > 100)
    {
      display.drawNumber(i * 5, needle_x - cosf((45+i*4.5) * deg_to_rad) * r3
                              , needle_y - sinf((45+i*4.5) * deg_to_rad) * r3 - 2);
    }
  }
}

void plotLinear(const char *label, int x, int y, int w, int h)
{
  display.drawRect(x, y, w, h, TFT_GREY);
  display.fillRect(x + 2, y + 18, w - 3, h - 36, TFT_WHITE);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.setTextDatum(textdatum_t::middle_center);
  display.setFont(&fonts::Font2);
  display.setTextPadding(display.textWidth("100"));
  display.drawString(label, x + w / 2, y + 10);

  int plot_height = h - (19*2);

  for (int i = 0; i <= 10; i ++)
  {
    display.drawFastHLine(x + w/2, y + 19 + (i+1) * plot_height / 12, w*(3+(0==(i%5)))/16, TFT_BLACK);
  }
}

void plotPointer(void)
{
  display.setTextColor(TFT_GREEN, TFT_BLACK);
  display.setTextDatum(textdatum_t::middle_right);

  int plot_height = liner_height - (19*2);
  int pw = (display.width() / liner_count) / 3;

  for (int i = 0; i < liner_count; i++)
  {
    display.drawNumber(value[i], display.width() * (i + 0.8) / liner_count, display.height() - 10);
    int dx = display.width() * i / liner_count + 3;
    int dy = meter_height + 19 + (old_value[i]+10) * plot_height / 120;
    display.fillTriangle(dx, dy - 5, dx, dy + 5, dx + pw, dy, TFT_WHITE);
    old_value[i] = value[i];
    dy = meter_height + 19 + (old_value[i]+10) * plot_height / 120;
    display.fillTriangle(dx, dy - 5, dx, dy + 5, dx + pw, dy, TFT_RED);
  }
}

void setup(void)
{
  display.begin();
  display.setEpdMode(epd_mode_t::epd_fastest);
  if (display.width() > display.height())
  {
    display.setRotation(display.getRotation() ^ 1);
  }

  display.startWrite();
  display.fillScreen(TFT_BLACK);

  liner_height = display.height() * 3 / 5;
  meter_height = display.height() * 2 / 5;
  needle_x = display.width() / 2;
  needle_y = (meter_height*2 + display.width()) / 3;
  osx = needle_x;
  osy = needle_y;
  liner_count = std::min(6, display.width() / 40);

  analogMeter(); // Draw analogue meter
  int w = display.width() / liner_count;
  char name[] = "A0";
  // Draw linear meters
  for (int i = 0; i < liner_count; i++)
  {
    name[1] = '0' + i;
    plotLinear(name, display.width() * i / liner_count, meter_height, w, liner_height);
  }
  display.endWrite();

  updateTime = millis(); // Next update time
}

void loop(void)
{
  auto msec = millis();
  if (updateTime <= msec)
  {
    d = (msec >> 3) % 360;

    // Create a Sine wave for testing
    value[0] = 50 + roundf(50 * sinf((d +   0) * deg_to_rad));
    value[1] = 50 + roundf(50 * sinf((d +  60) * deg_to_rad));
    value[2] = 50 + roundf(50 * sinf((d + 120) * deg_to_rad));
    value[3] = 50 + roundf(50 * sinf((d + 180) * deg_to_rad));
    value[4] = 50 + roundf(50 * sinf((d + 240) * deg_to_rad));
    value[5] = 50 + roundf(50 * sinf((d + 300) * deg_to_rad));

    //value[0] = map(analogRead(36), 0, 4095, 0, 100); // Test with value form GPIO36

    if (!display.displayBusy())
    {
      updateTime = msec + LOOP_PERIOD;
    //unsigned long t = millis();
      display.startWrite();
      plotPointer();
      plotNeedle(roundf(value[0]));
      display.endWrite();
    //Serial.println(millis()-t); // Print time taken for meter update
    }
  }
}
