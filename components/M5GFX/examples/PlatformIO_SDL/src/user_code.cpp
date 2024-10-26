
#include <M5GFX.h>

static M5GFX gfx;

void setup(void)
{
  gfx.init();
}

void loop(void)
{
  gfx.fillCircle(rand()%gfx.width(), rand()%gfx.height(), 16, rand());
}



#if defined ( ESP_PLATFORM ) && !defined ( ARDUINO )
extern "C" {
int app_main(int, char**)
{
    setup();
    for (;;) {
      loop();
    }
    return 0;
}
}
#endif
