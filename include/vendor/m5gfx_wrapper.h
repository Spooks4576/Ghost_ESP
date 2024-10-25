// m5gfx_wrapper.h
#ifndef M5GFX_WRAPPER_H
#define M5GFX_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

void init_m5gfx_display();
void m5gfx_write_pixels(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t *color_p);

#ifdef __cplusplus
}
#endif

#endif // M5GFX_WRAPPER_H