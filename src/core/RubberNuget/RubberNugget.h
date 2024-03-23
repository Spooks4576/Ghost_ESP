#pragma once

#include "Arduino.h"
#include "cdcusb.h"
#include "mscusb.h"
#include "flashdisk.h"

class RubberNugget {
  public:
    RubberNugget(){};
    static void init();
    static String* allPayloadPaths(const char* path="/");
};

struct NuggetConfig {
  String locale;
  String network;
  String password;
  long pid;
  long vid;
};

FILINFO* newFileList(const char* path, int& numFiles);
NuggetConfig getConfig();
