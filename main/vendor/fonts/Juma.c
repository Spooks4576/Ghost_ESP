/*******************************************************************************
 * Size: 17 px
 * Bpp: 1
 * Opts: --bpp 1 --size 17 --no-compress --font Juma.ttf --symbols ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890 --format lvgl -o Juma.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef JUMA
#define JUMA 1
#endif

#if JUMA

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0030 "0" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0031 "1" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0032 "2" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0033 "3" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0034 "4" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0035 "5" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0036 "6" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0037 "7" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0038 "8" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0039 "9" */
    0x7f, 0xf3, 0xff, 0xef, 0xff, 0xb6, 0xd7, 0xfa,
    0xfe, 0xdd, 0xff, 0xff, 0xbf, 0xfe, 0x3f, 0xf0,
    0x0, 0x4, 0x2, 0x0, 0x40, 0x44, 0x0,

    /* U+0041 "A" */
    0x2, 0x0, 0x60, 0x7, 0x0, 0xf0, 0xf, 0x81,
    0xf8, 0x19, 0xc3, 0x9c, 0x3f, 0xe7, 0xfe, 0x60,
    0x6e, 0x7,

    /* U+0042 "B" */
    0xfc, 0x7f, 0x30, 0xd8, 0x6c, 0x37, 0xfb, 0xfd,
    0x83, 0xc1, 0xe0, 0xff, 0xdf, 0xc0,

    /* U+0043 "C" */
    0x1f, 0x7, 0xf9, 0xc7, 0xb0, 0x3c, 0x1, 0x80,
    0x30, 0x6, 0x0, 0x60, 0x6e, 0x3c, 0xff, 0xf,
    0x80,

    /* U+0044 "D" */
    0xfe, 0x3f, 0xcc, 0x3b, 0x6, 0xc0, 0xf0, 0x3c,
    0xf, 0x3, 0xc1, 0xb0, 0xef, 0xf3, 0xf8,

    /* U+0045 "E" */
    0xff, 0xff, 0xc0, 0xc0, 0xc0, 0xfe, 0xfe, 0xc0,
    0xc0, 0xc0, 0xff, 0xff,

    /* U+0046 "F" */
    0xff, 0xff, 0xc0, 0xc0, 0xc0, 0xfe, 0xfe, 0xc0,
    0xc0, 0xc0, 0xc0, 0xc0,

    /* U+0047 "G" */
    0xf, 0x83, 0xfc, 0x70, 0xe6, 0x7, 0xc0, 0xc,
    0x3f, 0xc3, 0xfc, 0x3, 0x60, 0x77, 0xf, 0x3f,
    0xf0, 0xfb,

    /* U+0048 "H" */
    0xc1, 0xe0, 0xf0, 0x78, 0x3c, 0x1f, 0xff, 0xff,
    0x83, 0xc1, 0xe0, 0xf0, 0x78, 0x30,

    /* U+0049 "I" */
    0xff, 0xd8, 0xc6, 0x31, 0x8c, 0x63, 0x3f, 0xf0,

    /* U+004A "J" */
    0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
    0xc3, 0xe7, 0x7e, 0x3c,

    /* U+004B "K" */
    0xc1, 0xf0, 0xec, 0x73, 0x38, 0xdc, 0x3f, 0xf,
    0xc3, 0x70, 0xce, 0x31, 0xcc, 0x3b, 0x7,

    /* U+004C "L" */
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
    0xc0, 0xc0, 0xff, 0xff,

    /* U+004D "M" */
    0x4, 0x20, 0x6, 0x60, 0x6, 0x60, 0xf, 0xf0,
    0xf, 0xf0, 0x1f, 0xf8, 0x1b, 0xd8, 0x39, 0x9c,
    0x31, 0x8c, 0x71, 0x8e, 0x61, 0x86, 0xe1, 0x87,

    /* U+004E "N" */
    0x80, 0xf0, 0x3e, 0xf, 0xc3, 0xf8, 0xf7, 0x3c,
    0xef, 0x1f, 0xc3, 0xf0, 0x7c, 0xf, 0x1,

    /* U+004F "O" */
    0xf, 0x3, 0xfc, 0x70, 0xe6, 0x6, 0xc0, 0x3c,
    0x3, 0xc0, 0x3c, 0x3, 0x60, 0x67, 0xe, 0x3f,
    0xc0, 0xf0,

    /* U+0050 "P" */
    0xfe, 0x7f, 0xb0, 0x78, 0x3c, 0x1f, 0xfb, 0xf9,
    0x80, 0xc0, 0x60, 0x30, 0x18, 0x0,

    /* U+0051 "Q" */
    0xf, 0x3, 0xfc, 0x70, 0xe6, 0x6, 0xc0, 0x3c,
    0x3, 0xc0, 0x3c, 0x3, 0xe0, 0x77, 0xe, 0x3f,
    0xc1, 0xf8, 0x6, 0x0, 0x60,

    /* U+0052 "R" */
    0xfe, 0x7f, 0xb0, 0x78, 0x3c, 0x1e, 0xff, 0x7d,
    0x9c, 0xc6, 0x63, 0xb0, 0xd8, 0x30,

    /* U+0053 "S" */
    0x3e, 0x3f, 0x9c, 0xfc, 0x37, 0x83, 0xf8, 0x7e,
    0x7, 0x61, 0xb0, 0xdf, 0xc7, 0xc0,

    /* U+0054 "T" */
    0xff, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18,

    /* U+0055 "U" */
    0xc1, 0xe0, 0xf0, 0x78, 0x3c, 0x1e, 0xf, 0x7,
    0x83, 0xc1, 0xf1, 0xdf, 0xc7, 0xc0,

    /* U+0056 "V" */
    0xe0, 0x36, 0x7, 0x70, 0x63, 0xe, 0x38, 0xc1,
    0x9c, 0x1d, 0x80, 0xf8, 0xf, 0x0, 0x70, 0x6,
    0x0, 0x20,

    /* U+0057 "W" */
    0xe1, 0x87, 0x61, 0x86, 0x71, 0x8e, 0x31, 0x8c,
    0x39, 0x9c, 0x1b, 0xd8, 0x1f, 0xf8, 0xf, 0xf0,
    0xf, 0x70, 0x6, 0x60, 0x6, 0x60, 0x4, 0x20,

    /* U+0058 "X" */
    0xe0, 0xee, 0x38, 0xc7, 0x1d, 0xc1, 0xb0, 0x3e,
    0x7, 0xc0, 0xd8, 0x3b, 0x86, 0x39, 0xc7, 0x70,
    0x70,

    /* U+0059 "Y" */
    0xe1, 0xd8, 0x67, 0x38, 0xcc, 0x3f, 0x7, 0x81,
    0xc0, 0x30, 0xc, 0x3, 0x0, 0xc0, 0x30,

    /* U+005A "Z" */
    0xff, 0xff, 0xc0, 0xe0, 0xe0, 0xe0, 0xe0, 0x70,
    0x70, 0x70, 0x70, 0x3f, 0xff, 0xf0,

    /* U+0061 "a" */
    0x3d, 0xbf, 0xf8, 0xf8, 0x3c, 0x1e, 0xf, 0x8e,
    0xff, 0x3d, 0x80,

    /* U+0062 "b" */
    0xc0, 0x60, 0x30, 0x1b, 0xcf, 0xf7, 0x1f, 0x7,
    0x83, 0xc1, 0xf1, 0xff, 0xdb, 0xc0,

    /* U+0063 "c" */
    0x3c, 0x3f, 0xb9, 0xd8, 0xc, 0x6, 0x3, 0x9c,
    0xfe, 0x3c, 0x0,

    /* U+0064 "d" */
    0x1, 0x80, 0xc0, 0x67, 0xb7, 0xff, 0x1f, 0x7,
    0x83, 0xc1, 0xf1, 0xdf, 0xe7, 0xb0,

    /* U+0065 "e" */
    0x1e, 0x3f, 0xbc, 0xfc, 0x3f, 0xc7, 0xe3, 0x8e,
    0xfe, 0x3e, 0x0,

    /* U+0066 "f" */
    0xf, 0x1f, 0x38, 0x30, 0x7e, 0x7e, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0xe0, 0xc0,

    /* U+0067 "g" */
    0x3d, 0xbf, 0xf8, 0xf8, 0x3c, 0x1e, 0xf, 0x8e,
    0xff, 0x3d, 0x80, 0xc0, 0x67, 0xf3, 0xe0,

    /* U+0068 "h" */
    0xc0, 0xc0, 0xc0, 0xdc, 0xfe, 0xe7, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3,

    /* U+0069 "i" */
    0x70, 0xff, 0xff, 0xc0,

    /* U+006A "j" */
    0x8, 0xc0, 0x1, 0x8c, 0x63, 0x18, 0xc6, 0x31,
    0x8c, 0x7e, 0xe0,

    /* U+006B "k" */
    0xc0, 0x60, 0x30, 0x18, 0xec, 0xe6, 0xe3, 0xe1,
    0xf0, 0xdc, 0x67, 0x31, 0x98, 0xe0,

    /* U+006C "l" */
    0xff, 0xff, 0xff,

    /* U+006D "m" */
    0xfd, 0xef, 0xff, 0xc6, 0x3c, 0x63, 0xc6, 0x3c,
    0x63, 0xc6, 0x3c, 0x63, 0xc6, 0x30,

    /* U+006E "n" */
    0xdc, 0xfe, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3,

    /* U+006F "o" */
    0x3e, 0x3f, 0xb8, 0xf8, 0x3c, 0x1e, 0xf, 0x8e,
    0xfe, 0x3e, 0x0,

    /* U+0070 "p" */
    0xde, 0x7f, 0xb8, 0xf8, 0x3c, 0x1e, 0xf, 0x8f,
    0xfe, 0xde, 0x60, 0x30, 0x18, 0xc, 0x0,

    /* U+0071 "q" */
    0x3d, 0xbf, 0xf8, 0xf8, 0x3c, 0x1e, 0xf, 0x8e,
    0xff, 0x3d, 0x80, 0xc0, 0x60, 0x30, 0x18,

    /* U+0072 "r" */
    0xdf, 0xfe, 0x30, 0xc3, 0xc, 0x30, 0xc0,

    /* U+0073 "s" */
    0x3d, 0xff, 0x9f, 0x87, 0xc3, 0xf1, 0xff, 0x7c,

    /* U+0074 "t" */
    0x30, 0xcf, 0xff, 0x30, 0xc3, 0xc, 0x30, 0xc3,
    0x0,

    /* U+0075 "u" */
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7f,
    0x3b,

    /* U+0076 "v" */
    0xe1, 0xd8, 0x67, 0x38, 0xcc, 0x3f, 0x7, 0x81,
    0xe0, 0x30, 0xc, 0x0,

    /* U+0077 "w" */
    0xe3, 0x1d, 0x8c, 0x67, 0x33, 0x8d, 0xec, 0x3f,
    0xf0, 0x7f, 0x81, 0xce, 0x3, 0x30, 0x8, 0x40,

    /* U+0078 "x" */
    0xe3, 0xb9, 0xcd, 0xc7, 0xc1, 0xe1, 0xf0, 0xdc,
    0xe6, 0xe3, 0x80,

    /* U+0079 "y" */
    0xe1, 0x98, 0xe7, 0x30, 0xdc, 0x37, 0xf, 0x81,
    0xe0, 0x70, 0x1c, 0x6, 0x3, 0x80, 0xc0, 0x70,
    0x0,

    /* U+007A "z" */
    0xff, 0xfc, 0x70, 0xe3, 0x8e, 0x1c, 0x7f, 0xfe
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 46, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 138, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 161, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 245, .box_w = 14, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 191, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 166, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 196, .box_w = 11, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 182, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 294, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 306, .adv_w = 149, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 318, .adv_w = 203, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 336, .adv_w = 180, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 92, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 138, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 179, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 149, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 252, .box_w = 16, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 188, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 200, .box_w = 12, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 454, .adv_w = 163, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 468, .adv_w = 200, .box_w = 12, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 489, .adv_w = 169, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 155, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 517, .adv_w = 146, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 529, .adv_w = 169, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 543, .adv_w = 194, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 561, .adv_w = 251, .box_w = 16, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 585, .adv_w = 177, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 602, .adv_w = 165, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 617, .adv_w = 155, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 631, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 642, .adv_w = 161, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 656, .adv_w = 151, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 667, .adv_w = 161, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 681, .adv_w = 150, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 692, .adv_w = 104, .box_w = 8, .box_h = 16, .ofs_x = -1, .ofs_y = -4},
    {.bitmap_index = 708, .adv_w = 162, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 723, .adv_w = 150, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 735, .adv_w = 61, .box_w = 2, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 61, .box_w = 5, .box_h = 17, .ofs_x = -2, .ofs_y = -4},
    {.bitmap_index = 750, .adv_w = 153, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 61, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 767, .adv_w = 224, .box_w = 12, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 781, .adv_w = 150, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 790, .adv_w = 153, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 801, .adv_w = 161, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 816, .adv_w = 161, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 831, .adv_w = 106, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 838, .adv_w = 129, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 846, .adv_w = 106, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 855, .adv_w = 150, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 864, .adv_w = 156, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 876, .adv_w = 229, .box_w = 14, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 892, .adv_w = 152, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 903, .adv_w = 153, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = -4},
    {.bitmap_index = 920, .adv_w = 132, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 48, .range_length = 10, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 65, .range_length = 26, .glyph_id_start = 11,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 97, .range_length = 26, .glyph_id_start = 37,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 1, 0, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 0, 17, 18, 19, 20, 10,
    0, 13, 21, 13, 22, 23, 24, 18,
    25, 25, 26, 27, 28, 0, 29, 30,
    0, 31, 0, 0, 32, 0, 0, 33,
    0, 32, 32, 34, 34, 0, 35, 36,
    37, 0, 38, 38, 39, 38, 35
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 0, 12, 0, 0,
    0, 12, 0, 13, 14, 0, 0, 11,
    0, 12, 0, 12, 0, 15, 16, 17,
    18, 18, 19, 20, 21, 22, 0, 22,
    23, 22, 24, 22, 0, 0, 0, 0,
    0, 0, 0, 22, 0, 22, 0, 25,
    26, 27, 28, 28, 29, 30, 31
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, -12, -11, -11, -7, -7, -5, -12,
    -7, -6, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -6,
    -11, 0, -11, -10, -8, -10, -11, -10,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -6, -10,
    -11, 0, -4, -6, -4, -11, -4, -11,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -8, -12, -11,
    -5, 0, -5, -4, -12, -4, -11, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -6, -12, -11, -6,
    -3, 0, -4, -12, -4, -11, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -6, -12, -11, -6, -3,
    -6, 0, -13, -4, -11, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -13, -1, -10, -13, -13, -12,
    -13, 0, -13, -10, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -7, -12, -11, -6, -4, -6, -4,
    -12, 0, -11, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, -11, -6, -12, -12, -10, -12, -11,
    -11, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -11, 0, 0, -4, -24, -10,
    -27, 0, -28, 0, -4, -7, -10, 0,
    -14, -3, -22, 0, -20, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -4, 0, 0, 0, 0, -2, 0, -9,
    -6, -11, 0, 0, 0, 0, 0, 0,
    0, -1, -2, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -9,
    0, -3, 0, 0, -2, 0, -9, -10,
    -11, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -12, 0,
    -5, 0, 0, -5, 0, -12, -13, -14,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, -5, -8, 0, -3, -2, -4, 0,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -26, -5, 0, -18,
    0, 0, 0, 0, 0, 0, 0, -13,
    -14, -8, -15, 0, -10, 0, 0, 0,
    -7, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -7, 0, -14, 0, -17, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, -5, -7,
    0, -3, -2, -5, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -9, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -16, 0, -7, -6, 0, 0, 0,
    0, 0, 0, -12, -13, -11, -7, -12,
    -10, -16, 0, -16, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -6, 0, 0, 0, -29, -4, -32, 0,
    -33, 0, 0, 0, -9, 0, -13, 0,
    -27, 0, -26, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -23, 0,
    0, -19, 0, 0, 0, 0, -5, -2,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -7, 0, 0, 0, -5, 0, -7, 0,
    -3, -6, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, -7, -5, -9, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -25, -5, 0, -27, 0,
    0, 0, 0, 0, 0, 0, -21, -20,
    -11, -21, 0, -8, 0, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -27, -11, 0, -29, -4, 0,
    0, 0, 0, 0, 0, -22, -22, -14,
    -21, -5, -15, -4, -6, -4, -11, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -13, 0, -5, -4, 0, 0,
    0, 0, 0, 0, -9, -11, -12, -5,
    -9, -8, -11, 0, -11, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -27, -13, 0, -30, -6, 0, 0, 0,
    0, 0, 0, -23, -23, -17, -23, -8,
    -17, -6, -9, -6, -14, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -4, -5, -7, 0, 0, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, -8,
    -9, -7, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -7, -7,
    -6, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -7, -7, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -8, 0, -7, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -10, -12, -2,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, -9, -7, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, -3, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, -8, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -9,
    -9, 0, -4, 0, 0, 0, 0, 0,
    0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 39,
    .right_class_cnt     = 31,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 3,
    .bpp = 1,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t Juma = {
#else
lv_font_t Juma = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if JUMA*/

