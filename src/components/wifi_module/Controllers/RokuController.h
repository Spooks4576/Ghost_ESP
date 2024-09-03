#pragma once
#include "AppController.h"

enum RokuKeyPress {
  RokuKeyPress_UP,
  RokuKeyPress_DOWN,
  RokuKeyPress_LEFT,
  RokuKeyPress_RIGHT,
  RokuKeyPress_HOME,
  RokuKeyPress_PLAY
};

inline RokuKeyPress getRandomRokuKeyPress() {
  RokuKeyPress keys[] = {
    RokuKeyPress_UP,
    RokuKeyPress_DOWN,
    RokuKeyPress_LEFT,
    RokuKeyPress_RIGHT,
    RokuKeyPress_HOME,
    RokuKeyPress_PLAY
  };

  int randomIndex = random(0, sizeof(keys) / sizeof(keys[0]));
  return keys[randomIndex];
}


class RokuController : public AppController {
public:

  // Make sure to set this before calling LaunchApp
  const char* AppIDToLaunch;

  RokuController();

  HandlerType getType() const override {
    return HandlerType::RokuController;
  }

  virtual bool launchApp(const String& appUrl) override;

  virtual int checkAppStatus(const String& appUrl, Device& device) override;

  bool isRokuDevice(const char* appURL);

  void ExecuteKeyCommand(const char* AppUrl);
};