
#include <Arduino.h>
#include <vector>

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
//M5AtomDisplay display;


#define LINE_COUNT 6

std::vector<int> points[LINE_COUNT];
int colors[] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW };
int xoffset, yoffset, point_count;
int virtual_width;
int virtual_height;
int zoom = 1;

int getBaseColor(int x, int y)
{
  return ((x^y)&3 || ((x-xoffset)&31 && y&31) ? TFT_BLACK : ((!y || x == xoffset) ? TFT_WHITE : TFT_DARKGREEN));
}

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
  if (display.getBoard() == m5gfx::boards::board_t::board_M5StackCoreInk)
  {
    zoom = 2;
  }
  else if (display.getBoard() == m5gfx::boards::board_t::board_M5Paper)
  {
    zoom = 3;
  }
  else
  {
    zoom = (display.width() / 480) + 1;
  }
  virtual_width  = display.width() / zoom;
  virtual_height = display.height()/ zoom;

  xoffset = virtual_width  >> 1;
  yoffset = virtual_height >> 1;
  point_count = virtual_width + 1;

  for (int i = 0; i < LINE_COUNT; i++)
  {
    points[i].resize(point_count);
  }

  display.startWrite();
  for (int y = 0; y < virtual_height; y++)
  {
    for (int x = 0; x < virtual_width; x++)
    {
      auto color = getBaseColor(x, y - yoffset);
      if (color)
      {
        display.fillRect(x * zoom, y * zoom, zoom, zoom, color);
      }
    }
  }
  display.endWrite();
}

void loop(void)
{
  static int count;

  for (int i = 0; i < LINE_COUNT; i++)
  {
    points[i][count % point_count] = (sinf((float)count / (10 + 30 * i))+sinf((float)count / (13 + 37 * i))) * (virtual_height >> 2);
  }

  ++count;
  display.waitDisplay();
  display.startWrite();
  int index1 = count % point_count;
  for (int x = 0; x < point_count-1; x++)
  {
    int index0 = index1;
    index1 = (index0 +1) % point_count;
    for (int i = 0; i < LINE_COUNT; i++)
    {
      int y = points[i][index0];
      if (y != points[i][index1])
      {
        display.fillRect(x * zoom, (y + yoffset) * zoom, zoom, zoom, getBaseColor(x, y));
      }
    }

    for (int i = 0; i < LINE_COUNT; i++)
    {
      int y = points[i][index1];
      display.fillRect(x * zoom, (y + yoffset) * zoom, zoom, zoom, colors[i]);
    }
  }
  display.endWrite();
}
