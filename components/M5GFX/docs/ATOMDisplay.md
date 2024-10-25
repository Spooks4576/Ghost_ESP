# ATOM Display library


![M5Stack ATOM Display](https://static-cdn.m5stack.com/resource/docs/products/atom/atom_display/atom_display_01.webp "ATOM Display")

### Notes.
- Audio output function is not included.  
- If the image does not appear, reinsert the USB cable and wait a few seconds.  
- When inserting or removing the USB cable, please hold the ATOM itself. Repeatedly inserting and removing the USB cable while holding the docking base side may cause stress on the pin headers and result in poor contact.  

### 注意事項  
- 音声出力機能は含まれません。  
- 画像が表示されない場合はUSBケーブルを挿し直し、数秒待機してください。  
- USBケーブルを挿抜する際はATOM本体を保持してください。ドッキングベース側を保持した状態でUSB挿抜を行うと、ピンヘッダに応力が掛かり接触不良を引き起こす可能性があります。  


Sample Code:
```
#include <M5AtomDisplay.h>

// Create an instance of M5AtomDisplay
// M5AtomDisplay のインスタンスを用意
M5AtomDisplay display;


// Set the resolution in the argument (width, height, refresh rate).  default : 1280, 720, 60 .
// The value of width x height x refresh rate must be within 55,296,000.
// If the display does not match the supported resolution or frequency of the display, it will not be displayed correctly.

// 引数で解像度(幅,高さ,リフレッシュレート)を設定できます。 省略時は 1280, 720, 60
// 幅 × 高さ × リフレッシュレート の値が 55,296,000 以下である必要があります。
// また、ディスプレイが指定の解像度や周波数に対応していない場合は正しく表示されません。

// M5AtomDisplay display(1920, 480);
// M5AtomDisplay display(1280, 720);
// M5AtomDisplay display(1024, 768);
// M5AtomDisplay display( 960, 540);
// M5AtomDisplay display( 800, 600);
// M5AtomDisplay display( 640, 480);
// M5AtomDisplay display( 640, 400);
// M5AtomDisplay display( 640, 360);
// M5AtomDisplay display( 512, 212);
// M5AtomDisplay display( 256, 192);
// M5AtomDisplay display( 320, 240);
// M5AtomDisplay display( 240, 320);
// M5AtomDisplay display( 200, 200);
// M5AtomDisplay display( 240, 135);
// M5AtomDisplay display( 135, 240);
// M5AtomDisplay display( 160, 160);
// M5AtomDisplay display( 160,  80);
// M5AtomDisplay display(  80, 160);
// M5AtomDisplay display(  80,  80);
// M5AtomDisplay display( 480, 1920);

// M5AtomDisplay display(1920, 1080, 24);
// M5AtomDisplay display(1920, 960, 30);


// Depending on the supported resolution of the display, it may not be displayed correctly.
// Arguments 4 and 5 allow you to set the resolution (width and height) to be output to the display.
// Arguments 6 and 7 allow you to set the scaling factor for width and height.
// Argument 8 allows you to set the pixel clock frequency.
// If the enlarged resolution is less than the output resolution, there will be a gap around the perimeter of the screen.
// ディスプレイの対応解像度によっては正しく表示できない場合があります。
// 引数4と5でディスプレイに出力する解像度(幅,高さ)を設定できます。
// 引数6と7で幅と高さの拡大倍率を設定できます。
// 引数8でピクセルクロックの周波数を設定できます。
// なお拡大後の解像度が出力解像度に満たない場合、画面外周に隙間が生じます。

// M5AtomDisplay display ( 512, 384, 60, 1280, 800, 2, 2, 74250000 );

// ※ The width scaling factor must be a number that is divisible by the width of the output resolution.
// ※ 幅の拡大倍率は、出力解像度の幅を割り切れる数である必要があります。
// ex: M5AtomDisplay display ( 400, 250, 60, 1280, 800, 3, 3, 74250000 );
//  In this example, 1280 is not divisible by 3, so the horizontal scaling factor will be changed to 2.
//  この例は 1280 を 3で割り切れないため、横方向の拡大倍率は2に変更されます。


void setup(void)
{
// initialize
// 初期化
  display.init();

// Set the display orientation
// ディスプレイの向きを設定
// 0 = 270 deg / 1 = normal / 2 = 90 deg / 3 = 180 deg / 4~7 = upside down
  display.setRotation(1);

// Set the color depth ( bpp )
// 色深度を設定
  display.setColorDepth(24);  // 24bit per pixel color setting
//display.setColorDepth(16);  // 16bit per pixel color setting ( default )
//display.setColorDepth( 8);  //  8bit per pixel color setting

  display.startWrite();
  for (int y = 0; y < display.height(); ++y)
  {
    for (int x = 0; x < display.width(); ++x)
    {
      display.writePixel(x, y, display.color888(x, x+y, y));
    }
  }
  display.endWrite();

  for (int i = 0; i < 16; ++i)
  {
    int x = rand() % display.width();
    int y = rand() % display.height();
    display.drawCircle(x, y, 16, rand());
  }
}

void loop(void)
{
  display.startWrite();

  static constexpr const char hello_str[] = "Hello ATOM Display !";
  display.setFont(&fonts::Orbitron_Light_32);
  for (int i = -display.textWidth(hello_str); i < display.width(); ++i)
  {
    display.drawString(hello_str, i, (display.height() - display.fontHeight()) >> 1);
  }

  display.endWrite();
}
```


#### Is PSRAM required ?  can I use it with ATOM Lite or ATOM Matrix?  
- PSRAM is NOT required. it will work with ATOM Lite and ATOM Matrix. The label on the product that says "FOR ATOM PSRAM ONLY" is incorrect.  
- In fact, in the early stages of development, we assumed that we would build a 1280x720 frame buffer in the ESP32's PSRAM and output the images. As the development progressed, the frame buffer was placed on the FPGA side, and in the final product, there was no need to have a frame buffer on the ESP32 side.  

#### What is the difference between ATOM Lite and ATOM PSRAM?  
- ATOM Lite has ESP32 PICO-D4,  ATOM PSRAM has ESP32 PICO-V3-02.  
- Some of the pins on the back are different. The ATOM PSRAM has a G5 pin where the ATOM Lite had a G23 pin, but the G23 pin is gone.  



#### PSRAMは必須ですか？ATOM LiteやATOM Matrixで使えますか？  
- PSRAMは必須ではありません。ATOM LiteやATOM Matrixでも動作します。製品のラベルに記載されている "FOR ATOM PSRAM ONLY" という表記は誤りです。  
- 実は開発初期段階では、ESP32のPSRAMに1280x720のフレームバッファを構築して画像を出力する想定でした。開発が進むにつれ、フレームバッファはFPGA側に持つようになり、最終製品ではESP32側にフレームバッファを持つ必要がなくなりました。  

#### ATOM LiteとATOM PSRAMの違いは？  
- ATOM Liteには ESP32 PICO D4が搭載されています。ATOM PSRAMには ESP32 PICO V3-02 が搭載されています。 
- 背面のピンが一部違います。ATOM Liteの G23 が配置されていた箇所には、 ATOM PSRAM では G5 が配置されています。G23は無くなっているため、注意が必要です。  

