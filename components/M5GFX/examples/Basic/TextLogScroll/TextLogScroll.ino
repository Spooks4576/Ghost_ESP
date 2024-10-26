
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

static constexpr char text0[] = "hello world";
static constexpr char text1[] = "this";
static constexpr char text2[] = "is";
static constexpr char text3[] = "text";
static constexpr char text4[] = "log";
static constexpr char text5[] = "vertical";
static constexpr char text6[] = "scroll";
static constexpr char text7[] = "sample";
static constexpr const char* text[] = { text0, text1, text2, text3, text4, text5, text6, text7 };

//*
/// Example of using canvas
M5Canvas canvas(&display);
void setup(void)
{
  display.begin();

  if (display.isEPD())
  {
    display.setEpdMode(epd_mode_t::epd_fastest);
    display.invertDisplay(true);
    display.clear(TFT_BLACK);
  }
  if (display.width() < display.height())
  {
    display.setRotation(display.getRotation() ^ 1);
  }

  canvas.setColorDepth(1); // mono color
  canvas.createSprite(display.width(), display.height());
  canvas.setTextSize((float)canvas.width() / 160);
  canvas.setTextScroll(true);
}

void loop(void)
{
  static int count = 0;

  canvas.printf("%04d:%s\r\n", count, text[count & 7]);
  canvas.pushSprite(0, 0);
  ++count;
}

/*/

/// Example without canvas
void setup(void)
{
  display.begin();

  if (display.isEPD())
  {
    display.setEpdMode(epd_mode_t::epd_fastest);
    display.invertDisplay(true);
    display.clear(TFT_BLACK);
  }
  if (display.width() < display.height())
  {
    display.setRotation(display.getRotation() ^ 1);
  }

  display.setTextSize((float)display.width() / 160);
  display.setTextScroll(true);
}

void loop(void)
{
  static int count = 0;

  display.printf("%04d:%s\r\n", count, text[count & 7]);
  ++count;
}
//*/