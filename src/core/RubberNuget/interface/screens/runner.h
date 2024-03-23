#ifndef SCRIPT_RUNNER_SCREEN_H
#define SCRIPT_RUNNER_SCREEN_H

#include "../lib/NuggetInterface.h"


class ScriptRunnerScreen : public NuggetScreen {
  public:
    ScriptRunnerScreen(String payload);
    bool draw();
    int update(int);
  private:
    String payload;
    bool has_run;
};

void runPayload(String payload, SH1106Wire* display, Adafruit_NeoPixel* strip);
void processDuckyScript(String ducky, SH1106Wire* display, Adafruit_NeoPixel* strip);
void pressNamedKey(String keyPress, uint8_t modifiers);
bool keyKnown(String keyPress);

#endif
