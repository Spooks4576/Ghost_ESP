#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   Graphical settings
 *====================*/
#define LV_HOR_RES_MAX    240  // Width of the ILI9341 display
#define LV_VER_RES_MAX    320  // Height of the ILI9341 display

/* Color depth (16-bit for ILI9341 - RGB565) */
#define LV_COLOR_DEPTH    16

/*=======================
   Memory configuration
 *=======================*/
#define LV_MEM_SIZE       (32U * 1024U)  // 32 KB for LVGL memory pool

/*=======================
   Features and modules
 *=======================*/
#define LV_USE_ANIMATION  1
#define LV_USE_LABEL      1
#define LV_USE_BTN        1
#define LV_USE_IMG        1

/* Enable logging (Optional: useful for debugging) */
#define LV_USE_LOG        1
#define LV_LOG_LEVEL      LV_LOG_LEVEL_WARN

#endif /* LV_CONF_H */