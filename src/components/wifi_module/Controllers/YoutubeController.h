#pragma once
#include "AppController.h"
#include "../Services/YoutubeService.h"

class YoutubeController : public AppController {
public:

  virtual bool launchApp(const String& appUrl) override;
  virtual int checkAppStatus(const String& appUrl, Device& device) override;
  HandlerType getType() const override { return HandlerType::YoutubeController; }

  String extractScreenId(const String& xmlData) {
    String startTag = "<screenId>";
    String endTag = "</screenId>";

    int startIndex = xmlData.indexOf(startTag);
    if (startIndex == -1) {
      Serial.println(F("Start tag not found."));
      return "";
    }
    startIndex += startTag.length();

    int endIndex = xmlData.indexOf(endTag, startIndex);
    if (endIndex == -1) {
      Serial.println(F("End tag not found."));
      return "";
    }

    String extractedId = xmlData.substring(startIndex, endIndex);
    if (extractedId.length() == 0) {
      Serial.println(F("Extracted screenId is empty."));
      return "";
    }

    Serial.println("Extracted screenId: " + extractedId);
    return extractedId;
  }

  YouTubeService YTService;
};