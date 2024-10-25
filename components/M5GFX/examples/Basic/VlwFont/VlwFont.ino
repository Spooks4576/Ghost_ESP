#include <SD.h>
#include <SPIFFS.h>

#include <M5GFX.h>

M5GFX display;

static constexpr const uint8_t PIN_SD_CS = GPIO_NUM_4;

void setup(void)
{
  display.init();
  display.print("SD wait.\n");

  SPIFFS.begin();

  if (!display.loadFont(SPIFFS, "/font.vlw"))
  {
#if defined (_SD_H_)
    while (!SD.begin(PIN_SD_CS, SPI, 25000000))
    {
      ESP_LOGD("debug","SD begin failed");
      delay(500);
    }
    if (!display.loadFont(SD, "/font.vlw"))
#endif
    {
      display.print("Font data not found...");
    }
  }
}

void loop(void)
{
  display.print("Hello World !");
  delay(500);
}

#if !defined ( ARDUINO )
extern "C" {
  void loopTask(void*)
  {
    setup();
    for (;;) {
      loop();
    }
    vTaskDelete(NULL);
  }

  void app_main()
  {
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
#endif
