/**
 * Simple MSC device, use fat partition
 * author: chegewara
 */
#include "flashdisk.h"

#if CFG_TUD_MSC

FlashUSB dev;
FlashUSB dev1;
char *l1 = "ffat";
char *l2 = "ffat1";
void setup()
{
  Serial.begin(115200);

  if (dev.init("/fat1", "ffat"))
  {
    if (dev.begin())
    {
      Serial.println("MSC lun 1 begin");
    }
    else
      log_e("LUN 1 failed");
  }
  if (dev1.init("/fat2", "ffat1"))
  {
    if (dev1.begin())
    {
      Serial.println("MSC lun 2 begin");
    }
    else
      log_e("LUN 2 failed");
  }
}

void loop()
{
  delay(1000);
}

#endif
