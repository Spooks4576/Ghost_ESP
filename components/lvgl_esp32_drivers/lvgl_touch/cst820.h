#ifndef __GT911_H

#define __CS820_H

#include <stdint.h>
#include <stdbool.h>
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_ADDR_CST820 0x15

#define CYD28_TouchC_SDA 33
#define CYD28_TouchC_SCL 32
#define CYD28_TouchC_INT 21
#define CYD28_TouchC_RST 25

enum GESTURE
{
    None = 0x00,
    SlideDown = 0x01,
    SlideUp = 0x02, 
    SlideLeft = 0x03,
    SlideRight = 0x04,
    SingleTap = 0x05,
    DoubleTap = 0x0B,
    LongPress = 0x0C
};

/**
  * @brief  Initialize for GT911 communication via I2C
  * @param  dev_addr: Device address on communication Bus (I2C slave address of GT911).
  * @retval None
  */
void cst820_init(void);

/**
  * @brief  Get the touch screen X and Y positions values. Ignores multi touch
  * @param  drv:
  * @param  data: Store data here
  * @retval Always false
  */
bool cst820_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

#ifdef __cplusplus
}
#endif
#endif /* __GT911_H */