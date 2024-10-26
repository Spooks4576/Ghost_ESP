
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
//M5AtomDisplay display;  // default setting
//M5AtomDisplay display ( 320, 180 ); // width, height

static constexpr const int qsintab[256]={
    0x8000,0x80c9,0x8192,0x825b,0x8324,0x83ee,0x84b7,0x8580,
    0x8649,0x8712,0x87db,0x88a4,0x896c,0x8a35,0x8afe,0x8bc6,
    0x8c8e,0x8d57,0x8e1f,0x8ee7,0x8fae,0x9076,0x913e,0x9205,
    0x92cc,0x9393,0x945a,0x9521,0x95e7,0x96ad,0x9773,0x9839,
    0x98fe,0x99c4,0x9a89,0x9b4d,0x9c12,0x9cd6,0x9d9a,0x9e5e,
    0x9f21,0x9fe4,0xa0a7,0xa169,0xa22b,0xa2ed,0xa3af,0xa470,
    0xa530,0xa5f1,0xa6b1,0xa770,0xa830,0xa8ef,0xa9ad,0xaa6b,
    0xab29,0xabe6,0xaca3,0xad5f,0xae1b,0xaed7,0xaf92,0xb04d,
    0xb107,0xb1c0,0xb27a,0xb332,0xb3ea,0xb4a2,0xb559,0xb610,
    0xb6c6,0xb77c,0xb831,0xb8e5,0xb999,0xba4d,0xbb00,0xbbb2,
    0xbc64,0xbd15,0xbdc6,0xbe76,0xbf25,0xbfd4,0xc082,0xc12f,
    0xc1dc,0xc288,0xc334,0xc3df,0xc489,0xc533,0xc5dc,0xc684,
    0xc72c,0xc7d3,0xc879,0xc91f,0xc9c3,0xca67,0xcb0b,0xcbae,
    0xcc4f,0xccf1,0xcd91,0xce31,0xced0,0xcf6e,0xd00b,0xd0a8,
    0xd144,0xd1df,0xd279,0xd313,0xd3ac,0xd443,0xd4db,0xd571,
    0xd606,0xd69b,0xd72f,0xd7c2,0xd854,0xd8e5,0xd975,0xda05,
    0xda93,0xdb21,0xdbae,0xdc3a,0xdcc5,0xdd4f,0xddd9,0xde61,
    0xdee9,0xdf6f,0xdff5,0xe07a,0xe0fd,0xe180,0xe202,0xe283,
    0xe303,0xe382,0xe400,0xe47d,0xe4fa,0xe575,0xe5ef,0xe668,
    0xe6e0,0xe758,0xe7ce,0xe843,0xe8b7,0xe92b,0xe99d,0xea0e,
    0xea7e,0xeaed,0xeb5b,0xebc8,0xec34,0xec9f,0xed09,0xed72,
    0xedda,0xee41,0xeea7,0xef0b,0xef6f,0xefd1,0xf033,0xf093,
    0xf0f2,0xf150,0xf1ad,0xf209,0xf264,0xf2be,0xf316,0xf36e,
    0xf3c4,0xf41a,0xf46e,0xf4c1,0xf513,0xf564,0xf5b3,0xf602,
    0xf64f,0xf69b,0xf6e6,0xf730,0xf779,0xf7c1,0xf807,0xf84d,
    0xf891,0xf8d4,0xf916,0xf956,0xf996,0xf9d4,0xfa11,0xfa4d,
    0xfa88,0xfac1,0xfafa,0xfb31,0xfb67,0xfb9c,0xfbd0,0xfc02,
    0xfc33,0xfc63,0xfc92,0xfcc0,0xfcec,0xfd17,0xfd42,0xfd6a,
    0xfd92,0xfdb8,0xfdde,0xfe01,0xfe24,0xfe46,0xfe66,0xfe85,
    0xfea3,0xfec0,0xfedb,0xfef5,0xff0e,0xff26,0xff3c,0xff52,
    0xff66,0xff79,0xff8a,0xff9b,0xffaa,0xffb8,0xffc4,0xffd0,
    0xffda,0xffe3,0xffeb,0xfff1,0xfff6,0xfffa,0xfffd,0xffff,
};

static int isin(int i) {
    i=(i&1023);
    if (i>=512) return -isin(i-512);
    if (i>=256) i=(511-i);
    return qsintab[i]-0x8000;
}

static int icos(int i) {
    return isin(i+256);
}


int32_t lcd_width;
int32_t lcd_height;
uint32_t f;
uint8_t colors[8];
size_t scale = 1;

void setup(void)
{
  display.init();
  display.startWrite();
  display.fillScreen(TFT_BLACK);

  if (display.isEPD())
  {
    display.setEpdMode(epd_mode_t::epd_fastest);
  }
  if (display.width() < display.height())
  {
    display.setRotation(display.getRotation() ^ 1);
  }
  if (display.width() > 800)
  {
    scale = 4;
  }

  lcd_width  = display.width()  / scale;
  lcd_height = display.height() / scale;

  display.startWrite();
  display.setColorDepth(8);
  display.fillScreen(TFT_BLACK);
  f = 0;

  for (int i = 0; i < 8; ++i)
  {
    uint32_t p = 0;
    if (i & 1) p =0x03;
    if (i & 2) p+=0x1C;
    if (i & 4) p+=0xE0;
    colors[i] = p;
  }
}

  uint32_t p_dxx = 0;
  uint32_t p_dxy = 0;
  uint32_t p_dyx = 0;
  uint32_t p_dyy = 0;
  int rcos = 0;
  int rsin = 0;
  int zoom = 0;
  int px = 0;
  int py = 0;

void loop(void)
{
  uint32_t p_cxx = (zoom * ((px) * rcos - (py) * rsin) ) >> (20 - 8);
  uint32_t p_cxy = (zoom * ((py) * rcos + (px) * rsin) ) >> (20 - 8);
  ++f;
  rcos=icos(f*3);
  rsin=isin(f*3);
  zoom=((isin(f*5)+0x10000)>>8)+128;
  px=(isin(f)>>7)-110;
  py=(icos(f)>>7)-110;

  uint32_t cxx = (zoom * ((px) * rcos - (py) * rsin) ) >> (20 - 8);
  uint32_t cxy = (zoom * ((py) * rcos + (px) * rsin) ) >> (20 - 8);
  uint32_t dxx = (zoom * (( 1) * rcos - ( 0) * rsin) ) >> (20 - 8);
  uint32_t dxy = (zoom * (( 0) * rcos + ( 1) * rsin) ) >> (20 - 8);
  uint32_t dyx = (zoom * (( 0) * rcos - ( 1) * rsin) ) >> (20 - 8);
  uint32_t dyy = (zoom * (( 1) * rcos + ( 0) * rsin) ) >> (20 - 8);

  display.waitDisplay();
  for (uint32_t y = 0; y < lcd_height; y++)
  {
    uint32_t ypos = y * scale;
    p_cxx+=p_dxx;
    p_cxy+=p_dxy;
    cxx+=dxx;
    cxy+=dxy;
    uint32_t cx=cxx;
    uint32_t cy=cxy;
    uint32_t p_cx = p_cxx;
    uint32_t p_cy = p_cxy;
    uint32_t x = 0;
    uint32_t prev_x = 0;
    uint32_t prev_color = 0xAA;
    bool flg = false;
    do
    {
      p_cx+=p_dyx;
      p_cy+=p_dyy;
      cx+=dyx;
      cy+=dyy;
      auto color = ((cx^cy) >> 16) & 7;
      bool diff = color != (((p_cx ^ p_cy) >> 16) & 7);
      if (flg)
      {
        if (!diff || prev_color != color)
        {
          if (x)
          {
            display.writeFillRectPreclipped(prev_x * scale, ypos, (x - prev_x) * scale, scale, colors[prev_color]);
          }
          flg = diff;
          prev_x = x;
          prev_color = color;
        }
      }
      else
      {
        if (diff)
        {
          flg = true;
          prev_x = x;
          prev_color = color;
        }
      }
    } while (++x < lcd_width);
    if (flg)
    {
      display.writeFillRect(prev_x * scale, ypos, (x - prev_x) * scale, scale, colors[prev_color]);
    }
  }
  display.display();
  p_dxx = dxx;
  p_dxy = dxy;
  p_dyx = dyx;
  p_dyy = dyy;
}
