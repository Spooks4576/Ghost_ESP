#include <WiFi.h>
#include <driver/adc.h>
#include <esp_log.h>

#include <M5GFX.h>
#include <M5AtomDisplay.h>

#include "png_logo.h"
#include "jpg_image.h"

/// Reducing the resolution can increase the drawing speed.
// M5AtomDisplay display(1920, 1080);
   M5AtomDisplay display(1280, 720);
// M5AtomDisplay display(960, 540);
// M5AtomDisplay display(640, 360);
// M5AtomDisplay display(320, 180);

// M5GFX display;  // for use non ATOM Display.

static int wifi_mode = 0;


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

//Returns value -32K...32K
static int isin(int i) {
    i=(i&1023);
    if (i>=512) return -isin(i-512);
    if (i>=256) i=(511-i);
    return qsintab[i]-0x8000;
}

static int icos(int i) {
    return isin(i+256);
}


void trans_clear(void)
{
  static constexpr uint8_t xtbl[16] = { 0, 2, 2, 0, 1, 3, 3, 1, 1, 3, 3, 1, 0, 2, 2, 0 };
  static constexpr uint8_t ytbl[16] = { 0, 2, 0, 2, 1, 3, 1, 3, 0, 2, 0, 2, 1, 3, 1, 3 };
  int width  = display.width()  >> 2;
  int height = display.height() >> 2;
  for (int i = 0; i < 16; ++i)
  {
    int xpos = xtbl[i];
    for (int y = 0; y < height; ++y)
    {
      int ypos = (y<<2) + ytbl[i];
      for (int x = 0; x < width; ++x)
      {
        display.writeFillRectPreclipped((x<<2)+xpos,ypos,1,1,TFT_BLACK);
      }
    }
  }
}

void spin_tile(void)
{
  display.startWrite();
  display.setColorDepth(8);
  display.fillScreen(TFT_BLACK);
  int32_t lcd_width  = display.width()  >> 2;
  int32_t lcd_height = display.height() >> 2;
  uint32_t f = 0;

  uint8_t colors[8];
  for (int i = 0; i < 8; ++i)
  {
    uint32_t p = 0;
    if (i & 1) p =0x03;
    if (i & 2) p+=0x1C;
    if (i & 4) p+=0xE0;
    colors[i] = p;
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

  for (int i = 0; i < 256 && !wifi_mode; ++i)
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
    for (uint32_t y = 0; y < lcd_height; y++)
    {
      uint32_t ypos = (y<<2);
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
              display.writeFillRectPreclipped(prev_x<<2, ypos, (x - prev_x)<<2, 4, colors[prev_color]);
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
        display.writeFillRect(prev_x<<2, ypos, (x - prev_x)<<2, 4, colors[prev_color]);
      }
    }
    p_dxx = dxx;
    p_dxy = dxy;
    p_dyx = dyx;
    p_dyy = dyy;
  }
  display.setColorDepth(16);
}


void title(void)
{
  display.clearClipRect();
  display.fillScreen(TFT_WHITE);
  display.setColorDepth(24);
  display.drawPng(png_logo, ~0u, 0, 0, display.width(), display.height()*2/3, 0, 0, 1.0f, 0.0f, datum_t::middle_center);
  display.setColorDepth(16);
  display.setTextColor(TFT_BLACK, TFT_LIGHTGRAY);
	display.setTextSize(display.width() / 400);
  display.setFont(&fonts::Orbitron_Light_32);
  display.setTextDatum(textdatum_t::top_center);
  int ypos = display.height()*2/3;
  int xpos = (display.width()>>1);
  int fontheight = display.fontHeight();
  display.setClipRect(0, ypos, display.width(), fontheight);
  for (int i = fontheight; i >= 0; --i)
  {
    display.drawString("ATOM Display", xpos, ypos +i);
  }
  display.clearClipRect();
}

void setup_wifi_scan(void)
{
  display.fillScreen(TFT_WHITE);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
	display.setTextSize(1);
  display.setFont(&fonts::FreeSansBold24pt7b);
  display.setCursor(20, 20);
  display.printf("WiFi Scanning...");
}

void loop_wifi_scan(void)
{
  int n = WiFi.scanNetworks();
  display.setCursor(20, 20);
  display.printf("Total  :  %d found.", n);
  for (int i = 0; i < 16; i++)
  {
    display.setCursor(20, 80 + 48 * i);
    display.setTextColor((WiFi.RSSI(i) > -70) ? TFT_BLUE : TFT_RED, TFT_WHITE);
    display.fillRect(20, 80 + 48 * i, 1280, 48, TFT_WHITE);
    display.printf("%d. %s : (%d)", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
}

uint16_t rainbow(int value)
{
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;

  uint8_t quadrant = value >> 5;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value & 31);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value & 31);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value & 31;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value & 31);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}

void bar_graph_demo(void)
{
  display.fillScreen(TFT_BLACK);
  display.setFont(&fonts::Font0);
  display.setTextSize(1);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.setColorDepth(16);

  int max_y[64];
  int prev_y[64];
  uint16_t colors[64];

  memset(prev_y, 0x7F, 64 * sizeof(int));
  memset(max_y, 0x7F, 64 * sizeof(int));
  for (int x = 0; x < 64; ++x)
  {
    colors[x] = rainbow(x*2);
  }

  int h = display.height();
  for (int i = 0; i < 512 && !wifi_mode; i++)
  {
    for (int x = 0; x < 64; ++x)
    {
      int y = (h>>1) - (sinf((float)((x-24)*i) / 543.0f) + sinf((float)((x-40)*i) / 210.0f)) * (h>>2);

      int xpos = x * display.width() / 64;
      int w = ((x+1) * display.width() / 64) - xpos - 1;
      if (max_y[x]+1 >= y) { max_y[x] = y-1; }
      else
      {
        display.fillRect(xpos, max_y[x]-3, w, 1, TFT_BLACK);
        max_y[x]++;
      }
      display.fillRect(xpos, max_y[x]-3, w, 3, TFT_WHITE);
      if (prev_y[x] < y)
      {
        display.fillRect(xpos, prev_y[x], w, y - prev_y[x], TFT_BLACK);
      }
      else
      {
        display.fillRect(xpos, y, w, prev_y[x] - y, colors[x]);
      }
      display.setCursor(xpos, h-8);
      display.printf("%03d", h-y);
      prev_y[x] = y;
    }
  }
}

void fill_circle(void)
{
  display.fillScreen(TFT_BLACK);
  for (int i = 0; i < 4096 && !wifi_mode; i++)
  {
    display.drawCircle(rand()%display.width(), rand()%display.height(),16 + (rand() & 127), rand());
  }
}

void fill_rect(void)
{
  display.fillScreen(TFT_BLACK);
  for (int i = 0; i < 16384 && !wifi_mode; i++)
  {
    display.drawRect((rand()%display.width()) - 32, (rand()%display.height()) -32, 64, 64, rand());
  }
}

void fill_tile_inner(int i)
{
  int xp = (display.width()>>1);
  while ((xp -= i) > 0);

  int y = (display.height()>>1);
  while ((y -= i) > 0);
  do
  {
    int x = xp;
    do
    {
      display.fillRect(x, y, i, i, display.color888(x*i, x*y, y*i));
    } while ((x += i) < display.width());
  } while ((y += i) < display.height());
}

void fill_tile(void)
{
  for (int i = 1024; i && !wifi_mode; i-=(i >> 4)+1)
  {
    fill_tile_inner(i);
  }
  for (int i = 2; i < 1024 && !wifi_mode; i+=(i >> 4)+1)
  {
    fill_tile_inner(i);
  }
}

void fill_arc(void)
{
  for (int i = 0; i < 360; ++i)
  {
    display.fillEllipseArc(display.width()>>1, display.height()>>1, 640, 480, 360, 270, i, i + 1, display.color888(255-(i*256/360), i*256/360, 255));
  }
}


unsigned long total = 0;
unsigned long tn = 0;

void printnice(int32_t v)
{
	char	str[32] = { 0 };
	sprintf(str, "%d", v);
	for (char *p = (str+strlen(str))-3; p > str; p -= 3)
	{
		memmove(p+1, p, strlen(p)+1);
		*p = ',';
		
	}
	while (strlen(str) < 10)
	{
		memmove(str+1, str, strlen(str)+1);
		*str = ' ';
	}
	display.println(str);
}

static inline uint32_t micros_start() __attribute__ ((always_inline));
static inline uint32_t micros_start()
{
	uint8_t oms = lgfx::millis();
	while ((uint8_t)lgfx::millis() == oms)
		;
	return lgfx::micros();
}

uint32_t testHaD()
{
	static constexpr uint8_t HaD_240x320[] =
	{
		0xb9, 0x50, 0x0e, 0x80, 0x93, 0x0e, 0x41, 0x11, 0x80, 0x8d, 0x11, 0x42, 0x12, 0x80, 0x89, 0x12, 
		0x45, 0x12, 0x80, 0x85, 0x12, 0x48, 0x12, 0x80, 0x83, 0x12, 0x4a, 0x13, 0x7f, 0x13, 0x4c, 0x13, 
		0x7d, 0x13, 0x4e, 0x13, 0x7b, 0x13, 0x50, 0x13, 0x79, 0x13, 0x52, 0x13, 0x77, 0x13, 0x54, 0x13, 
		0x75, 0x13, 0x57, 0x11, 0x75, 0x11, 0x5a, 0x11, 0x73, 0x11, 0x5c, 0x11, 0x71, 0x11, 0x5e, 0x10, 
		0x71, 0x10, 0x60, 0x10, 0x6f, 0x10, 0x61, 0x10, 0x6f, 0x10, 0x60, 0x11, 0x6f, 0x11, 0x5e, 0x13, 
		0x6d, 0x13, 0x5c, 0x14, 0x6d, 0x14, 0x5a, 0x15, 0x6d, 0x15, 0x58, 0x17, 0x6b, 0x17, 0x37, 0x01, 
		0x1f, 0x17, 0x6b, 0x17, 0x1f, 0x01, 0x17, 0x02, 0x1d, 0x18, 0x6b, 0x18, 0x1d, 0x02, 0x17, 0x03, 
		0x1b, 0x19, 0x6b, 0x19, 0x1b, 0x03, 0x17, 0x05, 0x18, 0x1a, 0x6b, 0x1a, 0x18, 0x05, 0x17, 0x06, 
		0x16, 0x1b, 0x6b, 0x1b, 0x16, 0x06, 0x17, 0x07, 0x14, 0x1c, 0x6b, 0x1c, 0x14, 0x07, 0x17, 0x08, 
		0x12, 0x1d, 0x6b, 0x1d, 0x12, 0x08, 0x17, 0x09, 0x10, 0x1e, 0x6b, 0x1e, 0x10, 0x09, 0x17, 0x0a, 
		0x0e, 0x1f, 0x6b, 0x1f, 0x0e, 0x0a, 0x17, 0x0b, 0x0c, 0x20, 0x6b, 0x20, 0x0c, 0x0b, 0x17, 0x0c, 
		0x0b, 0x21, 0x69, 0x21, 0x0b, 0x0c, 0x18, 0x0d, 0x08, 0x23, 0x67, 0x23, 0x08, 0x0d, 0x19, 0x0e, 
		0x06, 0x26, 0x63, 0x26, 0x06, 0x0e, 0x19, 0x0f, 0x04, 0x28, 0x61, 0x28, 0x04, 0x0f, 0x19, 0x10, 
		0x02, 0x2a, 0x5f, 0x2a, 0x02, 0x10, 0x1a, 0x3c, 0x5d, 0x3c, 0x1b, 0x3d, 0x5b, 0x3d, 0x1c, 0x3d, 
		0x59, 0x3d, 0x1d, 0x3e, 0x57, 0x3e, 0x1e, 0x3e, 0x55, 0x3e, 0x1f, 0x40, 0x51, 0x40, 0x20, 0x40, 
		0x4f, 0x40, 0x22, 0x40, 0x22, 0x09, 0x22, 0x40, 0x24, 0x40, 0x1a, 0x17, 0x1a, 0x40, 0x26, 0x40, 
		0x16, 0x1d, 0x16, 0x40, 0x28, 0x40, 0x12, 0x23, 0x12, 0x40, 0x2a, 0x40, 0x0f, 0x27, 0x0f, 0x40, 
		0x2c, 0x41, 0x0b, 0x2b, 0x0b, 0x41, 0x2f, 0x3f, 0x09, 0x2f, 0x09, 0x3f, 0x32, 0x3d, 0x08, 0x33, 
		0x08, 0x3d, 0x35, 0x3a, 0x08, 0x35, 0x08, 0x3a, 0x3a, 0x36, 0x07, 0x39, 0x07, 0x36, 0x41, 0x09, 
		0x05, 0x23, 0x07, 0x3b, 0x07, 0x23, 0x05, 0x09, 0x54, 0x21, 0x07, 0x3d, 0x07, 0x21, 0x64, 0x1f, 
		0x06, 0x41, 0x06, 0x1f, 0x66, 0x1d, 0x06, 0x43, 0x06, 0x1d, 0x68, 0x1b, 0x06, 0x45, 0x06, 0x1b, 
		0x6b, 0x18, 0x06, 0x47, 0x06, 0x18, 0x6e, 0x16, 0x06, 0x49, 0x06, 0x16, 0x70, 0x14, 0x06, 0x4b, 
		0x06, 0x14, 0x72, 0x13, 0x06, 0x4b, 0x06, 0x13, 0x74, 0x11, 0x06, 0x4d, 0x06, 0x11, 0x76, 0x0f, 
		0x06, 0x4f, 0x06, 0x0f, 0x78, 0x0e, 0x05, 0x51, 0x05, 0x0e, 0x7a, 0x0c, 0x06, 0x51, 0x06, 0x0c, 
		0x7d, 0x09, 0x06, 0x53, 0x06, 0x09, 0x80, 0x80, 0x08, 0x05, 0x55, 0x05, 0x08, 0x80, 0x82, 0x06, 
		0x05, 0x57, 0x05, 0x06, 0x80, 0x84, 0x05, 0x05, 0x57, 0x05, 0x05, 0x80, 0x86, 0x03, 0x05, 0x59, 
		0x05, 0x03, 0x80, 0x88, 0x02, 0x05, 0x59, 0x05, 0x02, 0x80, 0x8f, 0x5b, 0x80, 0x95, 0x5b, 0x80, 
		0x94, 0x5d, 0x80, 0x93, 0x5d, 0x80, 0x92, 0x5e, 0x80, 0x92, 0x5f, 0x80, 0x91, 0x5f, 0x80, 0x90, 
		0x61, 0x80, 0x8f, 0x61, 0x80, 0x8f, 0x61, 0x80, 0x8e, 0x63, 0x80, 0x8d, 0x63, 0x80, 0x8d, 0x63, 
		0x80, 0x8d, 0x63, 0x80, 0x8c, 0x19, 0x07, 0x25, 0x07, 0x19, 0x80, 0x8b, 0x16, 0x0d, 0x1f, 0x0d, 
		0x16, 0x80, 0x8b, 0x14, 0x11, 0x1b, 0x11, 0x14, 0x80, 0x8b, 0x13, 0x13, 0x19, 0x13, 0x13, 0x80, 
		0x8b, 0x12, 0x15, 0x17, 0x15, 0x12, 0x80, 0x8a, 0x12, 0x17, 0x15, 0x17, 0x12, 0x80, 0x89, 0x11, 
		0x19, 0x13, 0x19, 0x11, 0x80, 0x89, 0x11, 0x19, 0x13, 0x19, 0x11, 0x80, 0x89, 0x10, 0x1b, 0x11, 
		0x1b, 0x10, 0x80, 0x89, 0x0f, 0x1c, 0x11, 0x1c, 0x0f, 0x80, 0x89, 0x0f, 0x1c, 0x11, 0x1c, 0x0f, 
		0x80, 0x89, 0x0f, 0x1c, 0x11, 0x1c, 0x0f, 0x80, 0x89, 0x0e, 0x1d, 0x11, 0x1d, 0x0e, 0x80, 0x89, 
		0x0e, 0x1c, 0x13, 0x1c, 0x0e, 0x80, 0x89, 0x0e, 0x1b, 0x15, 0x1b, 0x0e, 0x80, 0x89, 0x0e, 0x1b, 
		0x15, 0x1b, 0x0e, 0x80, 0x89, 0x0e, 0x1a, 0x17, 0x1a, 0x0e, 0x80, 0x89, 0x0e, 0x18, 0x1b, 0x18, 
		0x0e, 0x80, 0x89, 0x0e, 0x16, 0x1f, 0x16, 0x0e, 0x80, 0x89, 0x0e, 0x14, 0x23, 0x14, 0x0e, 0x80, 
		0x89, 0x0f, 0x11, 0x27, 0x11, 0x0f, 0x80, 0x89, 0x0f, 0x0e, 0x2d, 0x0e, 0x0f, 0x80, 0x89, 0x0f, 
		0x0c, 0x31, 0x0c, 0x0f, 0x80, 0x89, 0x0f, 0x0b, 0x33, 0x0b, 0x0f, 0x80, 0x8a, 0x0f, 0x09, 0x35, 
		0x09, 0x0f, 0x80, 0x8b, 0x10, 0x08, 0x35, 0x08, 0x10, 0x80, 0x8b, 0x10, 0x07, 0x37, 0x07, 0x10, 
		0x80, 0x8b, 0x11, 0x06, 0x37, 0x06, 0x11, 0x80, 0x8b, 0x12, 0x05, 0x37, 0x05, 0x12, 0x80, 0x8c, 
		0x13, 0x03, 0x1b, 0x01, 0x1b, 0x03, 0x13, 0x80, 0x8d, 0x30, 0x03, 0x30, 0x80, 0x8d, 0x30, 0x04, 
		0x2f, 0x80, 0x8d, 0x2f, 0x05, 0x2f, 0x80, 0x8e, 0x2e, 0x06, 0x2d, 0x80, 0x8f, 0x2d, 0x07, 0x2d, 
		0x80, 0x8f, 0x2d, 0x07, 0x2d, 0x80, 0x90, 0x2c, 0x08, 0x2b, 0x80, 0x91, 0x2b, 0x09, 0x2b, 0x80, 
		0x8c, 0x01, 0x05, 0x2a, 0x09, 0x2a, 0x05, 0x01, 0x80, 0x85, 0x03, 0x05, 0x2a, 0x09, 0x2a, 0x05, 
		0x03, 0x80, 0x82, 0x04, 0x05, 0x2a, 0x09, 0x2a, 0x04, 0x05, 0x80, 0x80, 0x06, 0x05, 0x29, 0x04, 
		0x02, 0x03, 0x29, 0x05, 0x06, 0x7e, 0x07, 0x05, 0x29, 0x03, 0x03, 0x03, 0x29, 0x05, 0x07, 0x7c, 
		0x09, 0x05, 0x28, 0x02, 0x05, 0x02, 0x28, 0x05, 0x09, 0x7a, 0x0a, 0x05, 0x28, 0x02, 0x05, 0x02, 
		0x28, 0x05, 0x0a, 0x78, 0x0c, 0x05, 0x27, 0x02, 0x05, 0x02, 0x27, 0x05, 0x0c, 0x76, 0x0d, 0x06, 
		0x26, 0x01, 0x07, 0x01, 0x26, 0x06, 0x0d, 0x73, 0x10, 0x05, 0x55, 0x05, 0x10, 0x70, 0x12, 0x05, 
		0x53, 0x05, 0x12, 0x6e, 0x13, 0x06, 0x51, 0x06, 0x13, 0x6c, 0x15, 0x05, 0x51, 0x05, 0x15, 0x6a, 
		0x16, 0x06, 0x4f, 0x06, 0x16, 0x68, 0x18, 0x06, 0x4d, 0x06, 0x18, 0x66, 0x1a, 0x06, 0x4b, 0x06, 
		0x1a, 0x64, 0x1c, 0x06, 0x49, 0x06, 0x1c, 0x55, 0x07, 0x05, 0x1e, 0x06, 0x49, 0x06, 0x1e, 0x05, 
		0x07, 0x42, 0x30, 0x06, 0x47, 0x06, 0x30, 0x3a, 0x34, 0x06, 0x45, 0x06, 0x34, 0x35, 0x37, 0x06, 
		0x43, 0x06, 0x37, 0x32, 0x39, 0x07, 0x3f, 0x07, 0x39, 0x2f, 0x3c, 0x07, 0x3d, 0x07, 0x3c, 0x2c, 
		0x3e, 0x07, 0x3b, 0x07, 0x3e, 0x2a, 0x40, 0x06, 0x3b, 0x06, 0x40, 0x28, 0x40, 0x06, 0x3c, 0x07, 
		0x40, 0x26, 0x3f, 0x08, 0x3d, 0x08, 0x3f, 0x24, 0x3f, 0x09, 0x3d, 0x09, 0x3f, 0x22, 0x3f, 0x0a, 
		0x14, 0x01, 0x13, 0x02, 0x13, 0x0a, 0x3f, 0x20, 0x3f, 0x0b, 0x14, 0x01, 0x13, 0x02, 0x13, 0x0b, 
		0x3f, 0x1f, 0x3e, 0x0c, 0x14, 0x01, 0x13, 0x02, 0x13, 0x0c, 0x3e, 0x1e, 0x3e, 0x0d, 0x13, 0x02, 
		0x13, 0x02, 0x13, 0x0d, 0x3e, 0x1d, 0x3d, 0x0e, 0x13, 0x02, 0x13, 0x02, 0x13, 0x0e, 0x3d, 0x1c, 
		0x3c, 0x11, 0x11, 0x04, 0x11, 0x04, 0x11, 0x11, 0x3c, 0x1b, 0x10, 0x01, 0x2a, 0x12, 0x11, 0x04, 
		0x11, 0x04, 0x11, 0x12, 0x2a, 0x01, 0x10, 0x1a, 0x0f, 0x04, 0x28, 0x14, 0x0f, 0x06, 0x0f, 0x06, 
		0x0f, 0x14, 0x28, 0x04, 0x0f, 0x19, 0x0e, 0x06, 0x26, 0x16, 0x0d, 0x08, 0x0d, 0x08, 0x0d, 0x16, 
		0x26, 0x06, 0x0e, 0x19, 0x0d, 0x07, 0x25, 0x18, 0x0b, 0x0a, 0x0b, 0x0a, 0x0b, 0x18, 0x25, 0x07, 
		0x0d, 0x19, 0x0c, 0x09, 0x23, 0x1c, 0x06, 0x0f, 0x05, 0x10, 0x05, 0x1c, 0x23, 0x09, 0x0c, 0x18, 
		0x0c, 0x0b, 0x21, 0x69, 0x21, 0x0b, 0x0c, 0x17, 0x0b, 0x0d, 0x1f, 0x6b, 0x1f, 0x0d, 0x0b, 0x17, 
		0x0a, 0x0f, 0x1e, 0x6b, 0x1e, 0x0f, 0x0a, 0x17, 0x09, 0x11, 0x1d, 0x6b, 0x1d, 0x11, 0x09, 0x17, 
		0x07, 0x14, 0x1c, 0x6b, 0x1c, 0x14, 0x07, 0x17, 0x06, 0x16, 0x1b, 0x6b, 0x1b, 0x16, 0x06, 0x17, 
		0x05, 0x18, 0x1a, 0x6b, 0x1a, 0x18, 0x05, 0x17, 0x04, 0x1a, 0x19, 0x6b, 0x19, 0x1a, 0x04, 0x17, 
		0x03, 0x1b, 0x19, 0x6b, 0x19, 0x1b, 0x03, 0x17, 0x02, 0x1d, 0x18, 0x6b, 0x18, 0x1d, 0x02, 0x37, 
		0x17, 0x6b, 0x17, 0x58, 0x16, 0x6b, 0x16, 0x5a, 0x14, 0x6d, 0x14, 0x5c, 0x13, 0x6d, 0x13, 0x5e, 
		0x12, 0x6d, 0x12, 0x60, 0x10, 0x6f, 0x10, 0x61, 0x10, 0x6f, 0x10, 0x60, 0x11, 0x6f, 0x11, 0x5e, 
		0x11, 0x71, 0x11, 0x5c, 0x12, 0x71, 0x12, 0x5a, 0x12, 0x73, 0x12, 0x58, 0x12, 0x75, 0x12, 0x56, 
		0x13, 0x75, 0x13, 0x54, 0x13, 0x77, 0x13, 0x51, 0x14, 0x79, 0x14, 0x4e, 0x14, 0x7b, 0x14, 0x4c, 
		0x14, 0x7d, 0x14, 0x4a, 0x14, 0x7f, 0x14, 0x48, 0x13, 0x80, 0x83, 0x13, 0x46, 0x13, 0x80, 0x85, 
		0x13, 0x44, 0x12, 0x80, 0x89, 0x12, 0x42, 0x11, 0x80, 0x8d, 0x11, 0x40, 0x0f, 0x80, 0x93, 0x0f, 
		0x45, 0x04, 0x80, 0x9d, 0x04, 0xb9, 0x56, 
	};
	
	display.fillScreen(TFT_BLACK);

	uint32_t start = micros_start();

	for (int i = 0; i < 0x10; i++)
	{

		uint16_t cnt = 0;
		uint16_t color = display.color565((i << 4) | i, (i << 4) | i, (i << 4) | i);
		uint16_t curcolor = 0;

		const uint8_t *cmp = &HaD_240x320[0];

		display.setAddrWindow(0, 0, 240, 320);
		while (cmp < &HaD_240x320[sizeof(HaD_240x320)])
		{
			cnt = pgm_read_byte(cmp++);
			if (cnt & 0x80) cnt = ((cnt & 0x7f) << 8) | pgm_read_byte(cmp++);
			display.pushBlock(curcolor, cnt);	// PDQ_GFX has count
			curcolor ^= color;
		}
	}

	uint32_t t = lgfx::micros() - start;

	display.setTextColor(TFT_YELLOW);
	display.setTextSize(2);
	display.setCursor(8, 285);
	display.print("http://hackaday.io/");
	display.setCursor(96, 302);
	display.print("Xark");

	return t;
}

uint32_t testFillScreen()
{
	uint32_t start = micros_start();
    // Shortened this tedious test!
		display.fillScreen(TFT_WHITE);
		display.fillScreen(TFT_RED);
		display.fillScreen(TFT_GREEN);
		display.fillScreen(TFT_BLUE);
		display.fillScreen(TFT_BLACK);

	return (lgfx::micros() - start)/5;
}

uint32_t testText()
{
	display.fillScreen(TFT_BLACK);
	uint32_t start = micros_start();
	display.setCursor(0, 0);
	display.setTextColor(TFT_WHITE,TFT_BLACK);	display.setTextSize(1);
	display.println("Hello World!");
	display.setTextSize(2);
	display.setTextColor(display.color565(0xff, 0x00, 0x00));
	display.print("RED ");
	display.setTextColor(display.color565(0x00, 0xff, 0x00));
	display.print("GREEN ");
	display.setTextColor(display.color565(0x00, 0x00, 0xff));
	display.println("BLUE");
	display.setTextColor(TFT_YELLOW); display.setTextSize(2);
	display.println(1234.56);
	display.setTextColor(TFT_RED);		display.setTextSize(3);
	display.println(0xDEADBEEF, 16);
	display.println();
	display.setTextColor(TFT_GREEN);
	display.setTextSize(5);
	display.println("Groop");
	display.setTextSize(2);
	display.println("I implore thee,");
  display.setTextColor(TFT_GREEN);
	display.setTextSize(1);
	display.println("my foonting turlingdromes.");
	display.println("And hooptiously drangle me");
	display.println("with crinkly bindlewurdles,");
	display.println("Or I will rend thee");
	display.println("in the gobberwarts");
	display.println("with my blurglecruncheon,");
	display.println("see if I don't!");
	display.println("");
	display.println("");
	display.setTextColor(TFT_MAGENTA);
	display.setTextSize(6);
	display.println("Woot!");
	uint32_t t = lgfx::micros() - start;
	return t;
}

uint32_t testPixels()
{
	int32_t	w = display.width();
	int32_t	h = display.height();

	uint32_t start = micros_start();
	for (uint16_t y = 0; y < h; y++)
	{
		for (uint16_t x = 0; x < w; x++)
		{
			display.drawPixel(x, y, display.color565(x<<3, y<<3, x*y));
		}
	}
	return lgfx::micros() - start;
}


uint32_t testLines(uint16_t color)
{
	uint32_t start, t;
	int32_t	x1, y1, x2, y2;
	int32_t	w = display.width();
	int32_t	h = display.height();

	display.fillScreen(TFT_BLACK);

	x1 = y1 = 0;
	y2 = h - 1;

	start = micros_start();

	for (x2 = 0; x2 < w; x2 += 6)
	{
		display.drawLine(x1, y1, x2, y2, color);
	}

	x2 = w - 1;

	for (y2 = 0; y2 < h; y2 += 6)
	{
		display.drawLine(x1, y1, x2, y2, color);
	}

	t = lgfx::micros() - start; // fillScreen doesn't count against timing

	display.fillScreen(TFT_BLACK);

	x1 = w - 1;
	y1 = 0;
	y2 = h - 1;

	start = micros_start();

	for (x2 = 0; x2 < w; x2 += 6)
	{
		display.drawLine(x1, y1, x2, y2, color);
	}

	x2 = 0;
	for (y2 = 0; y2 < h; y2 += 6)
	{
		display.drawLine(x1, y1, x2, y2, color);
	}

	t += lgfx::micros() - start;

	display.fillScreen(TFT_BLACK);

	x1 = 0;
	y1 = h - 1;
	y2 = 0;

	start = micros_start();

	for (x2 = 0; x2 < w; x2 += 6)
	{
		display.drawLine(x1, y1, x2, y2, color);
	}
	x2 = w - 1;
	for (y2 = 0; y2 < h; y2 += 6)
	{
		display.drawLine(x1, y1, x2, y2, color);
	}
	t += lgfx::micros() - start;

	display.fillScreen(TFT_BLACK);

	x1 = w - 1;
	y1 = h - 1;
	y2 = 0;

	start = micros_start();

	for (x2 = 0; x2 < w; x2 += 6)
	{
		display.drawLine(x1, y1, x2, y2, color);
	}

	x2 = 0;
	for (y2 = 0; y2 < h; y2 += 6)
	{
		display.drawLine(x1, y1, x2, y2, color);
	}

	t += lgfx::micros() - start;

	return t;
}

uint32_t testFastLines(uint16_t color1, uint16_t color2)
{
	uint32_t start;
	int32_t x, y;
	int32_t w = display.width();
	int32_t h = display.height();

	display.fillScreen(TFT_BLACK);

	start = micros_start();

	for (y = 0; y < h; y += 5)
		display.drawFastHLine(0, y, w, color1);
	for (x = 0; x < w; x += 5)
		display.drawFastVLine(x, 0, h, color2);

	return lgfx::micros() - start;
}

uint32_t testRects(uint16_t color)
{
	uint32_t start;
	int32_t n, i, i2;
	int32_t cx = display.width() / 2;
	int32_t cy = display.height() / 2;

	display.fillScreen(TFT_BLACK);
	n = std::min(display.width(), display.height());
	start = micros_start();
	for (i = 2; i < n; i += 6)
	{
		i2 = i / 2;
		display.drawRect(cx-i2, cy-i2, i, i, color);
	}

	return lgfx::micros() - start;
}

uint32_t testFilledRects(uint16_t color1, uint16_t color2)
{
	uint32_t start, t = 0;
	int32_t n, i, i2;
	int32_t cx = display.width() / 2 - 1;
	int32_t cy = display.height() / 2 - 1;

	display.fillScreen(TFT_BLACK);
	n = std::min(display.width(), display.height());
	for (i = n; i > 0; i -= 6)
	{
		i2 = i / 2;

		start = micros_start();

		display.fillRect(cx-i2, cy-i2, i, i, color1);

		t += lgfx::micros() - start;

		// Outlines are not included in timing results
		display.drawRect(cx-i2, cy-i2, i, i, color2);
	}

	return t;
}

uint32_t testFilledCircles(uint8_t radius, uint16_t color)
{
	uint32_t start;
	int32_t x, y, w = display.width(), h = display.height(), r2 = radius * 2;

	display.fillScreen(TFT_BLACK);

	start = micros_start();

	for (x = radius; x < w; x += r2)
	{
		for (y = radius; y < h; y += r2)
		{
			display.fillCircle(x, y, radius, color);
		}
	}

	return lgfx::micros() - start;
}

uint32_t testCircles(uint8_t radius, uint16_t color)
{
	uint32_t start;
	int32_t x, y, r2 = radius * 2;
	int32_t w = display.width() + radius;
	int32_t h = display.height() + radius;

	// Screen is not cleared for this one -- this is
	// intentional and does not affect the reported time.
	start = micros_start();

	for (x = 0; x < w; x += r2)
	{
		for (y = 0; y < h; y += r2)
		{
			display.drawCircle(x, y, radius, color);
		}
	}

	return lgfx::micros() - start;
}

uint32_t testTriangles()
{
	uint32_t start;
	int32_t n, i;
	int32_t cx = display.width()/ 2 - 1;
	int32_t cy = display.height() / 2 - 1;

	display.fillScreen(TFT_BLACK);
	n = std::min(cx, cy);

	start = micros_start();

	for (i = 0; i < n; i += 5)
	{
		display.drawTriangle(
			cx		, cy - i, // peak
			cx - i, cy + i, // bottom left
			cx + i, cy + i, // bottom right
			display.color565(0, 0, i));
	}

	return lgfx::micros() - start;
}

uint32_t testFilledTriangles()
{
	uint32_t start, t = 0;
	int32_t i;
	int32_t cx = display.width() / 2 - 1;
	int32_t cy = display.height() / 2 - 1;

	display.fillScreen(TFT_BLACK);

	start = micros_start();

	for (i = std::min(cx,cy); i > 10; i -= 5) {
		start = micros_start();
		display.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
			display.color565(0, i, i));
		t += lgfx::micros() - start;
		display.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
			display.color565(i, i, 0));
	}

	return t;
}

uint32_t testRoundRects()
 {
	uint32_t start;
	int32_t w, i, i2;
	int32_t cx = display.width() / 2 - 1;
	int32_t cy = display.height() / 2 - 1;

	display.fillScreen(TFT_BLACK);
	
	w = std::min(display.width(), display.height());
	
	start = micros_start();

	for (i = 0; i < w; i += 6)
	{
		i2 = i / 2;
		display.drawRoundRect(cx-i2, cy-i2, i, i, i/8, display.color565(i, 0, 0));
	}

	return lgfx::micros() - start;
}

uint32_t testFilledRoundRects()
{
	uint32_t start;
	int32_t i, i2;
	int32_t cx = display.width() / 2 - 1;
	int32_t cy = display.height() / 2 - 1;

	display.fillScreen(TFT_BLACK);

	start = micros_start();

	for (i = std::min(display.width(), display.height()); i > 20; i -= 6)
	{
		i2 = i / 2;
		display.fillRoundRect(cx-i2, cy-i2, i, i, i/8, display.color565(0, i, 0));
	}

	return lgfx::micros() - start;
}

static void on_button(void)
{
  wifi_mode = !wifi_mode;
}

void setup(void)
{
  display.begin();
  display.startWrite();
  adc_power_acquire();
  pinMode(39, INPUT);
  attachInterrupt(digitalPinToInterrupt(39), on_button, FALLING);
}

bool non_wifimode_delay(int ms)
{
  while (--ms && !wifi_mode)
  {
    delay(1);
  }
  return wifi_mode;
}

void loop(void)
{
  if (wifi_mode)
  {
    setup_wifi_scan();
    while (wifi_mode)
    {
      loop_wifi_scan();
    }
  }
  else
  {
    if (non_wifimode_delay(2000)) { return; }
    title();
    if (non_wifimode_delay(3000)) { return; }

    display.setColorDepth(24);
    display.drawJpg(jpg_image, ~0u, 0, 0, display.width(), display.height(), 0, 0, 0.0f, -1.0f);
    display.setColorDepth(16);

    if (non_wifimode_delay(3000)) { return; }
    trans_clear();
    bar_graph_demo();
    if (non_wifimode_delay(100)) { return; }
    spin_tile();
    if (non_wifimode_delay(100)) { return; }
    fill_tile();
    if (non_wifimode_delay(500)) { return; }
    fill_rect();
    if (non_wifimode_delay(100)) { return; }
    fill_circle();
    if (non_wifimode_delay(100)) { return; }

    display.setFont(&fonts::Font0);

    uint32_t usecHaD = testHaD();
    display.display();
      ESP_LOGI("main", "HaD pushColor            %d",usecHaD);
    if (non_wifimode_delay(500)) { return; }
  
    uint32_t usecFillScreen = testFillScreen();
    display.display();
      ESP_LOGI("main", "Screen fill              %d",usecFillScreen);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecText = testText();
    display.display();
      ESP_LOGI("main", "Text                     %d",usecText);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecPixels = testPixels();
    display.display();
      ESP_LOGI("main", "Pixels                   %d",usecPixels);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecLines = testLines(TFT_BLUE);
    display.display();
      ESP_LOGI("main", "Lines                    %d",usecLines);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecFastLines = testFastLines(TFT_RED, TFT_BLUE);
    display.display();
      ESP_LOGI("main", "Horiz/Vert Lines         %d",usecFastLines);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecRects = testRects(TFT_GREEN);
    display.display();
      ESP_LOGI("main", "Rectangles (outline)     %d",usecRects);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecFilledRects = testFilledRects(TFT_YELLOW, TFT_MAGENTA);
    display.display();
      ESP_LOGI("main", "Rectangles (filled)      %d",usecFilledRects);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecFilledCircles = testFilledCircles(10, TFT_MAGENTA);
    display.display();
      ESP_LOGI("main", "Circles (filled)         %d",usecFilledCircles);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecCircles = testCircles(10, TFT_WHITE);
    display.display();
      ESP_LOGI("main", "Circles (outline)        %d",usecCircles);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecTriangles = testTriangles();
    display.display();
      ESP_LOGI("main", "Triangles (outline)      %d",usecTriangles);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecFilledTrangles = testFilledTriangles();
    display.display();
      ESP_LOGI("main", "Triangles (filled)       %d",usecFilledTrangles);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usecRoundRects = testRoundRects();
    display.display();
      ESP_LOGI("main", "Rounded rects (outline)  %d",usecRoundRects);
    if (non_wifimode_delay(500)) { return; }

    uint32_t usedFilledRoundRects = testFilledRoundRects();
    display.display();
      ESP_LOGI("main", "Rounded rects (filled)   %d",usedFilledRoundRects);
    if (non_wifimode_delay(500)) { return; }

      ESP_LOGI("main", "Done!");

    uint16_t c = 4;
    int8_t d = 1;
    for (int32_t i = 0; i < display.height(); i++)
    {
      display.drawFastHLine(0, i, display.width(), c);
      c += d;
      if (c <= 4 || c >= 11)
        d = -d;
    }
    
    display.setCursor(0, 0);
    display.setTextColor(TFT_MAGENTA);
    display.setTextSize(2);

    display.println("   LovyanGFX test");

    display.setTextSize(1);
    display.setTextColor(TFT_WHITE);
    display.println("");
    display.setTextSize(1);
    display.println("");
    display.setTextColor(display.color565(0x80, 0x80, 0x80));

    display.println("");


    display.setTextColor(TFT_GREEN);
    display.println(" Benchmark               microseconds");
    display.println("");
    display.setTextColor(TFT_YELLOW);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("HaD pushColor      ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecHaD);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Screen fill        ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecFillScreen);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Text               ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecText);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Pixels             ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecPixels);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Lines              ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecLines);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Horiz/Vert Lines   ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecFastLines);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Rectangles         ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecRects);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Rectangles-filled  ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecFilledRects);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Circles            ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecCircles);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Circles-filled     ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecFilledCircles);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Triangles          ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecTriangles);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Triangles-filled   ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecFilledTrangles);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Rounded rects      ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usecRoundRects);

    display.setTextColor(TFT_CYAN); display.setTextSize(1);
    display.print("Rounded rects-fill ");
    display.setTextColor(TFT_YELLOW); display.setTextSize(2);
    printnice(usedFilledRoundRects);

    display.setTextSize(1);
    display.println("");
    display.setTextColor(TFT_GREEN); display.setTextSize(2);
    display.print("Benchmark Complete!");
  }
}
