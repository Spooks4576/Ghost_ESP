
#include <M5GFX.h>

M5GFX display;

void setup(void)
{
  display.init();
  display.setFont(&fonts::Font4);

  if (!display.touch())
  {
    display.setTextDatum(textdatum_t::middle_center);
    display.drawString("Touch not found.", display.width() / 2, display.height() / 2);
  }

  display.setEpdMode(epd_mode_t::epd_fastest);
  display.startWrite();
}

void loop(void)
{
  static bool drawed = false;
  lgfx::touch_point_t tp[3];

  int nums = display.getTouchRaw(tp, 3);
  if (nums)
  {
    for (int i = 0; i < nums; ++i)
    {
      display.setCursor(16, 16 + i * 24);
      display.printf("Raw X:%03d  Y:%03d", tp[i].x, tp[i].y);
    }

    display.convertRawXY(tp, nums);

    for (int i = 0; i < nums; ++i)
    {
      display.setCursor(16, 128 + i * 24);
      display.printf("Convert X:%03d  Y:%03d", tp[i].x, tp[i].y);
    }
    display.display();

    display.setColor(display.isEPD() ? TFT_BLACK : TFT_WHITE);
    for (int i = 0; i < nums; ++i)
    {
      int s = tp[i].size + 3;
      switch (tp[i].id)
      {
      case 0:
        display.fillCircle(tp[i].x, tp[i].y, s);
        break;
      case 1:
        display.drawLine(tp[i].x-s, tp[i].y-s, tp[i].x+s, tp[i].y+s);
        display.drawLine(tp[i].x-s, tp[i].y+s, tp[i].x+s, tp[i].y-s);
        break;
      default:
        display.fillTriangle(tp[i].x-s, tp[i].y +s, tp[i].x+s, tp[i].y+s, tp[i].x, tp[i].y-s);
        break;
      }
      display.display();
    }
    drawed = true;
  }
  else if (drawed)
  {
    drawed = false;
    display.waitDisplay();
    display.clear();
    display.display();
  }
  vTaskDelay(1);
}
