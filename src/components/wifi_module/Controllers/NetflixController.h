#pragma once
#include "AppController.h"

class NetflixController : public AppController {
public:

  virtual void launchApp(const String& appUrl) override;

  virtual int checkAppStatus(const String& appUrl, Device& device) override
  {
    Serial.println("Not Implemented checkAppStatus");
  }

  HandlerType getType() const override { return HandlerType::NetflixController; }
};