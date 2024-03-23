#include "runner.h"

#include "hidkeyboard.h"

#include "../../RubberNugget.h"
#include "Nugget/RubberNugget/keyboardlayout.h"
#include "../graphics.h"

extern HIDkeyboard keyboard; //TODO: remove this

ScriptRunnerScreen::ScriptRunnerScreen(String payload) {
  this->has_run = false;
  this->payload = payload;
  Serial.println("[ScriptRunnerInit]");
  Serial.println(payload);
}

int ScriptRunnerScreen::update(int btn) {
  if (!has_run) {
    runPayload(this->payload, this->display, this->strip);
    has_run = true;
    return SCREEN_REDRAW;
  }
  if (btn==BTN_LEFT){
    return SCREEN_BACK;
  }
  return SCREEN_REDRAW;
}

bool ScriptRunnerScreen::draw() {
  if (!has_run) {
    display->clear();
    return true;
  }
  display->drawXbm(0, 0, 128, 64, cat_with_exclamation_points_image_bits);
  display->drawString(3,9,"Press LEFT");
  display->drawString(3,19,"to go back");
  display->drawLine(0, 54, 127, 54);
  display->drawLine(0, 53, 127, 53);
  display->drawString(0, 54, "FINISHED PAYLOAD");
  return true;
}

void runPayload(String payload, SH1106Wire* display, Adafruit_NeoPixel* strip) {
    strip->setPixelColor(0, strip->Color(255,0, 0));
    strip->show(); strip->show(); strip->show();

    String command;

    for (int i=0; i < payload.length(); i++) {
        if (payload.charAt(i) == '\n') {
          Serial.println(command);
          processDuckyScript(command, display, strip);
          command = "";
        }
        command+=payload[i];
    }
    processDuckyScript(command, display, strip);
    display->clear();

    //manually update display
    display->drawXbm(0, 0, 128, 64, cat_with_exclamation_points_image_bits);
    display->display();
    strip->setPixelColor(0, strip->Color(0,0, 0));
    strip->show(); strip->show();
}

bool keyKnown(String keyPress) {
  Serial.print("looking for: ");
  Serial.println(keyPress);
  for (int i=0; i< (sizeof(keyMapRN)/sizeof(keyMapRN[0])); i++) {
    if (keyPress.equals(keyMapRN[i].title)) {
      Serial.print(keyMapRN[i].title);
      Serial.println(" found!");
      return true;
    }
  }
  return false;
}

void pressNamedKey(String keyPress, uint8_t modifiers) {
  for (int i=0; i< (sizeof(keyMapRN)/sizeof(keyMapRN[0])); i++) {
    if (keyPress.equals(keyMapRN[i].title)) {
      keyboard.sendPress(keyMapRN[i].key, modifiers);
    }
  }
}

void processDuckyScript(String ducky, SH1106Wire* display, Adafruit_NeoPixel* strip) {
  uint16_t defaultDelay = 10;
  String tCommand = ducky.substring(0, ducky.indexOf(' ')); // get command
  tCommand.toUpperCase(); tCommand.trim();

  display->clear();
  display->drawLine(0, 54, 127, 54);
  display->drawLine(0, 53, 127, 53);
  display->drawString(0, 54, "RUNNING PAYLOAD");
  display->display();
  
  if (tCommand.equals("REM")) {
    Serial.println("Comment");
  }
  else if (tCommand.equals("LOCALE")) {
    String locale = ducky.substring(ducky.indexOf(' ')+1, ducky.length());
    Serial.printf("Locale:[%s]\n", locale);
  }
  else if (tCommand.equals("DELAY")) {
    display->drawString(3,12,"DELAY: ");
    display->drawString(3,22,(String) ducky.substring(ducky.indexOf(' ')+1, ducky.length()));
    display->drawXbm(0, 0, 128, 64, cat_with_reload_spinner_image_bits);
    display->display();
    delay(ducky.substring(ducky.indexOf(' ')+1, ducky.length()).toInt()); // delay in MS
    Serial.println("Delayed!");       
  }
  else if (tCommand.equals("DEFAULT_DELAY") or tCommand.equals("DEFAULTDELAY")) {
    display->drawString(3,12,"DEFAULT");
    display->drawString(3,22,"DELAY:");
    display->drawString(3,32,(String) ducky.substring(ducky.indexOf(' ')+1, ducky.length()));
    display->drawXbm(0, 0, 128, 64, cat_with_reload_spinner_image_bits);
    display->display();
    defaultDelay = ducky.substring(ducky.indexOf(' ')+1, ducky.length()).toInt();
  }
  else if (tCommand.equals("LED")) {
    display->drawString(3,12,"COLOR:");
    display->drawString(3,22,(String) ducky.substring(ducky.indexOf(' ')+1, ducky.length())); // accept single color parameter
    display->drawXbm(0, 0, 128, 64, cat_with_reload_spinner_image_bits);
    display->display();
    String color = (String) ducky.substring(ducky.indexOf(' ')+1, ducky.length());
    color.toUpperCase();
    
    if (color.equals("R")) { strip->setPixelColor(0, strip->Color(255,0, 0)); }
    else if (color.equals("G")) {
      strip->setPixelColor(0, strip->Color(0,255, 0));
    }
     else if (color.equals("B")) {
      strip->setPixelColor(0, strip->Color(0,0, 255));
    }
     else if (color.equals("Y")) {
      strip->setPixelColor(0, strip->Color(255,255, 0));
    }
     else if (color.equals("C")) {
      strip->setPixelColor(0, strip->Color(0,255, 255));
    }
     else if (color.equals("M")) {
      strip->setPixelColor(0, strip->Color(255,0, 255));
    }
     else if (color.equals("W")) {
      strip->setPixelColor(0, strip->Color(120,120, 120));
    }
    strip->show(); strip->show();
  }
  else if (tCommand.equals("STRING")) {
    display->drawString(3,12,"STRING: ");
    if (String(ducky.substring(ducky.indexOf(' ')+1, ducky.length())).length() > 11) {
      display->drawString(3,22,String(ducky.substring(ducky.indexOf(' ')+1, ducky.length())).substring(0,8)+"...");
    }
    else {
      display->drawString(3,22,String(ducky.substring(ducky.indexOf(' ')+1, ducky.length())));
    }
    display->drawXbm(0, 0, 128, 64, cat_with_one_exclamation_point_image_bits);
    display->display();
    Serial.println("String");
    String tmpString = String(ducky.substring(ducky.indexOf(' ')+1, ducky.length()));
    keyboard.sendString(tmpString);    
  }  
  
  else if (keyKnown(tCommand)) {
    display->drawString(3,12,"KEY PRESS");
    ducky.trim(); // remove leading, trailing whitespace
    int currentTokenLeftIndex = 0;
    int currentTokenRightIndex = 0;
    String currentToken;
    uint8_t modifiers = 0;

    while (currentTokenLeftIndex < ducky.length()) {
      int nextSpace = ducky.indexOf(' ', currentTokenLeftIndex);
      if (nextSpace==-1){
        currentTokenRightIndex = ducky.length();
      } else {
        currentTokenRightIndex = nextSpace;
      }
      currentToken = ducky.substring(currentTokenLeftIndex, currentTokenRightIndex);
      
      if (currentToken.length() != 1) {
        // Search for named key, e.g. DELETE or TAB
        if (keyKnown(currentToken)){
          pressNamedKey(currentToken, modifiers);
          delay(2);
        } else {
          // unknown named key
          display->drawString(3,22,String("ERROR"));
          display->display();
          delay(1000);
        }
      } else {
        // Single letter like the 's' in: CTRL s
        unsigned char keycode = keymap[currentToken[0]].usage;
        keyboard.sendPress(keycode, modifiers);
        delay(2);
      }
      currentTokenLeftIndex = currentTokenRightIndex + 1;
    }
    keyboard.sendRelease();
  }
  else {
    Serial.println("Command not found");
  }

  display->display();
}