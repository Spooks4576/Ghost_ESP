#include <Adafruit_NeoPixel.h>
#include "core/RubberNuget/RubberNugget.h"
#include "Arduino.h"
#include <base64.h>
#include "base64.hpp"

#include <WiFiClient.h>
#include <WebServer.h>

#include "core/RubberNuget/utils.h"
#include "core/RubberNuget/interface/screens/splash.h"
#include "core/RubberNuget/interface/screens/dir.h"
#include "core/RubberNuget/interface/screens/runner.h"
#include "core/RubberNuget/interface/lib/NuggetInterface.h"


TaskHandle_t nuggweb;

NuggetInterface* nuggetInterface;


inline void NuggetEntryPoint()
{
  RubberNugget::init();
  nuggetInterface = new NuggetInterface;
  NuggetScreen* dirScreen = new DirScreen("/");
  NuggetScreen* splashScreen = new SplashScreen(1500);
  nuggetInterface->pushScreen(dirScreen);
  nuggetInterface->pushScreen(splashScreen);
  nuggetInterface->start();
}
