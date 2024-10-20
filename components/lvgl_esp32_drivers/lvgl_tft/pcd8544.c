/**
 * @file pcd8544.c
 *
 * Roughly based on:
 *    https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
 *    https://github.com/olikraus/u8g2
 */

#include "disp_spi.h"
#include "driver/gpio.h"

#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pcd8544.h"
#include "esp_idf_version.h"
#if ESP_IDF_VERSION <= ESP_IDF_VERSION_VAL(5,0,0)
#include "rom/gpio.h"
#endif
#define TAG "lv_pcd8544"

/**********************
 *      MACROS
 **********************/

#define BIT_SET(a,b) ((a) |= (1U<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1U<<(b)))

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void pcd8544_send_cmd(uint8_t cmd)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(PCD8544_DC, 0);	 /*Command mode*/
    disp_spi_send_data(&cmd, 1);
}

static void pcd8544_send_data(void * data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(PCD8544_DC, 1);	 /*Data mode*/
    disp_spi_send_data(data, length);
}

static void pcd8544_send_colors(void * data, uint16_t length)
{
    gpio_set_level(PCD8544_DC, 1);	 /*Data mode*/
    disp_spi_send_colors(data, length);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void pcd8544_init(void){

    // TODO: orientation

    // Initialize non-SPI GPIOs
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)
    gpio_pad_select_gpio(PCD8544_DC);
#else
    esp_rom_gpio_pad_select_gpio(PCD8544_DC);
#endif
    gpio_set_direction(PCD8544_DC, GPIO_MODE_OUTPUT);
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)
    gpio_pad_select_gpio(PCD8544_RST);
#else
    esp_rom_gpio_pad_select_gpio(PCD8544_RST);
#endif
    gpio_set_direction(PCD8544_RST, GPIO_MODE_OUTPUT);

    // Reset the display
    gpio_set_level(PCD8544_RST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PCD8544_RST, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    pcd8544_send_cmd(0x21);     /* activate chip (PD=0), horizontal increment (V=0), enter extended command set (H=1) */
    pcd8544_send_cmd(0x06);     /* temp. control: b10 = 2  */
    pcd8544_send_cmd(0x13);     /* bias system 1:48 */
    pcd8544_send_cmd(0xc0);     /* medium Vop = Contrast 0x40 = 64 */

    pcd8544_send_cmd(0x20);     /* activate chip (PD=0), horizontal increment (V=0), enter extended command set (H=0) */
    pcd8544_send_cmd(0x0c);     /* display mode normal */
}

void pcd8544_set_contrast (uint8_t contrast){
    if (contrast > 0x7f){
        contrast = 0x7f;
    }
    pcd8544_send_cmd(0x21);                /* activate chip (PD=0), horizontal increment (V=0), enter extended command set (H=1) */
    pcd8544_send_cmd(0x80 | contrast);     /* medium Vop = Contrast */
}

void pcd8544_rounder(lv_disp_drv_t * disp_drv, lv_area_t *area){
    uint8_t hor_max = disp_drv->hor_res;
    uint8_t ver_max = disp_drv->ver_res;

    area->x1 = 0;
    area->y1 = 0;
    area->x2 = hor_max - 1;
    area->y2 = ver_max - 1;
}

void pcd8544_set_px_cb(lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
                       lv_color_t color, lv_opa_t opa){

    uint8_t set = (color.full == 0) && (LV_OPA_TRANSP != opa);

    uint16_t byte_index = x + (( y>>3 ) * buf_w);
    uint8_t  bit_index  = y & 0x7;

    if (set) {
        BIT_SET(buf[byte_index], bit_index);
    } else {
        BIT_CLEAR(buf[byte_index], bit_index);
    }
}

void pcd8544_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_map){

    pcd8544_send_cmd(0x20);     /* activate chip (PD=0), horizontal increment (V=0), enter extended command set (H=0) */

    uint8_t * buf = (uint8_t *) color_map;

    // Check if the whole frame buffer can be sent in a single SPI transaction

    if ((area->x1 == 0) && (area->y1 == 0) && (area->x2 == (disp_drv->hor_res - 1)) && (area->y2 == (disp_drv->ver_res - 1))){

        // send complete frame buffer at once. 
        // NOTE: disp_spi_send_colors triggers lv_disp_flush_ready

        pcd8544_send_cmd(0x40);  /* set Y address */
        pcd8544_send_cmd(0x80);  /* set X address */
        pcd8544_send_colors(buf, disp_drv->hor_res * disp_drv->ver_res / 8);

    } else {

        // send horizontal tiles

        uint16_t bank_start =  area->y1 / 8;
        uint16_t bank_end   =  area->y2 / 8;

        uint16_t bank;
        uint16_t cols_to_update = area->x2 - area->x1 + 1;
        for (bank = bank_start ; bank <= bank_end ; bank++ ){
            pcd8544_send_cmd(0x40 | bank );      /* set Y address */
            pcd8544_send_cmd(0x80 | area->x1 );  /* set X address */
            uint16_t offset = bank * disp_drv->hor_res + area->x1;
            pcd8544_send_data(&buf[offset], cols_to_update);
        }

        lv_disp_flush_ready(disp_drv);
    }
}
