#include "commandline.h"
#include "../components/wifi_module/Controllers/AppController.h"
#include "../components/wifi_module/Controllers/YoutubeController.h"
#include "../components/wifi_module/Controllers/NetflixController.h"
#include "../components/wifi_module/Controllers/RokuController.h"
#include "../components/ble_module/ble_module.h"
#include <components/wifi_module/Features/Dial.h>
#include "../components/wifi_module/Features/DeauthDetector.h"
#include <components/wifi_module/Features/ESPmDNSHelper.h>

CommandLine::CommandLine() {
}

void CommandLine::RunSetup() {
  Serial.println(F("         Ghost ESP     \n"));
  Serial.println(F("       By: Spooky with features from justcallmekoko \n"));
  LOG_MESSAGE_TO_SD("         Ghost ESP     \n");
  LOG_MESSAGE_TO_SD(("       By: Spooky with features from justcallmekoko \n"));
}

String CommandLine::getSerialInput() {
  String input = "";

  if (Serial.available() > 0)
  {
    input = Serial.readStringUntil('\n');

    input.trim();
    Serial.println(input);
  }
   
  return input;
}

void CommandLine::main(uint32_t currentTime) {
  String input = this->getSerialInput();

  this->runCommand(input);

  if (input != "")
    Serial.print("> ");
}

LinkedList<String> CommandLine::parseCommand(String input, char* delim) {
  LinkedList<String> cmd_args;

  bool inQuote = false;
  bool inApostrophe = false;
  String buffer = "";

  for (int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);

    if (c == '"') {
      // Check if the quote is within an apostrophe
      if (inApostrophe) {
        buffer += c;
      } else {
        inQuote = !inQuote;
      }
    } else if (c == '\'') {
      // Check if the apostrophe is within a quote
      if (inQuote) {
        buffer += c;
      } else {
        inApostrophe = !inApostrophe;
      }
    } else if (!inQuote && !inApostrophe && strchr(delim, c) != NULL) {
      cmd_args.add(buffer);
      buffer = "";
    } else {
      buffer += c;
    }
  }

  // Add the last argument
  if (!buffer.isEmpty()) {
    cmd_args.add(buffer);
  }

  return cmd_args;
}

int CommandLine::argSearch(LinkedList<String>* cmd_args_list, String key) {
  for (int i = 0; i < cmd_args_list->size(); i++) {
    if (cmd_args_list->get(i) == key)
      return i;
  }

  return -1;
}

bool CommandLine::checkValueExists(LinkedList<String>* cmd_args_list, int index) {
  if (index < cmd_args_list->size() - 1)
    return true;
    
  return false;
}

bool CommandLine::inRange(int max, int index) {
  if ((index >= 0) && (index < max))
    return true;

  return false;
}

bool CommandLine::apSelected() {
  for (int i = 0; i < access_points->size(); i++) {
    if (access_points->get(i).selected)
      return true;
  }

  return false;
}

bool CommandLine::hasSSIDs() {
  if (ssids->size() == 0)
    return false;

  return true;
}

String CommandLine::toLowerCase(String str) {
  String result = str;
  for (int i = 0; i < str.length(); i++) {
    int charValue = str.charAt(i);
    if (charValue >= 65 && charValue <= 90) { // ASCII codes for uppercase letters
      charValue += 32;
      result.setCharAt(i, char(charValue));
    }
  }
  return result;
}

void CommandLine::filterAccessPoints(String filter) {
  int count_selected = 0;
  int count_unselected = 0;

  // Split the filter string into individual filters
  LinkedList<String> filters;
  int start = 0;
  int end = filter.indexOf(" or ");
  while (end != -1) {
    filters.add(filter.substring(start, end));
    start = end + 4;
    end = filter.indexOf(" or ", start);
  }
  filters.add(filter.substring(start));

  // Loop over each access point and check if it matches any of the filters
  for (int i = 0; i < access_points->size(); i++) {
    bool matchesFilter = false;
    for (int j = 0; j < filters.size(); j++) {
      String f = toLowerCase(filters.get(j));
      if (f.substring(0, 7) == "equals ") {
        String ssidEquals = f.substring(7);
        if ((ssidEquals.charAt(0) == '\"' && ssidEquals.charAt(ssidEquals.length() - 1) == '\"' && ssidEquals.length() > 1) ||
            (ssidEquals.charAt(0) == '\'' && ssidEquals.charAt(ssidEquals.length() - 1) == '\'' && ssidEquals.length() > 1)) {
          ssidEquals = ssidEquals.substring(1, ssidEquals.length() - 1);
        }
        if (access_points->get(i).essid.equalsIgnoreCase(ssidEquals)) {
          matchesFilter = true;
          break;
        }
      } else if (f.substring(0, 9) == "contains ") {
        String ssidContains = f.substring(9);
        if ((ssidContains.charAt(0) == '\"' && ssidContains.charAt(ssidContains.length() - 1) == '\"' && ssidContains.length() > 1) ||
            (ssidContains.charAt(0) == '\'' && ssidContains.charAt(ssidContains.length() - 1) == '\'' && ssidContains.length() > 1)) {
          ssidContains = ssidContains.substring(1, ssidContains.length() - 1);
        }
        String essid = toLowerCase(access_points->get(i).essid);
        if (essid.indexOf(ssidContains) != -1) {
          matchesFilter = true;
          break;
        }
      }
    }
    // Toggles the selected state of the AP
    AccessPoint new_ap = access_points->get(i);
    new_ap.selected = matchesFilter;
    access_points->set(i, new_ap);

    if (matchesFilter) {
      count_selected++;
    } else {
      count_unselected++;
    }
  }
}

void CommandLine::runCommand(String input)
{
    if (input == "") 
    {
        return;
    }

    LinkedList<String> cmd_args = this->parseCommand(input, " ");

    if (cmd_args.get(0) == "gpsdata")
    {
        #ifdef HAS_GPS
            // if (gpsmodule->getGpsModuleStatus()) {
            //     Serial.println("Getting GPS Data. Stop with stopscan");

            //   HasRanCommand = true;
            // }
        #endif
    }
    if (cmd_args.get(0) == "blespam")
    {
      #ifdef HAS_BT
      int bt_type_sw = this->argSearch(&cmd_args, "-t");
      if (bt_type_sw != -1) {
        String bt_type = cmd_args.get(bt_type_sw + 1);
        if (bt_type == "apple") {
          Serial.println("Starting Sour Apple attack. Stop with " + (String)"stopscan");
          LOG_MESSAGE_TO_SD("Starting Sour Apple attack.");
          HasRanCommand = true;
          BleModule->executeSpam(Apple, true);
          return;
        }

        if (bt_type == "windows") {
          Serial.println("Starting Swiftpair Spam attack. Stop with " + (String)"stopscan");
          LOG_MESSAGE_TO_SD("Starting Swiftpair Spam attack.");
          HasRanCommand = true;
          BleModule->executeSpam(Microsoft, true);
          return;
        }

        if (bt_type == "samsung") {
          Serial.println("Starting Samsung Spam attack. Stop with " + (String)"stopscan");
          LOG_MESSAGE_TO_SD("Starting Samsung Spam attack.");
          HasRanCommand = true;
          BleModule->executeSpam(Samsung, true);
          return;
        }

        if (bt_type == "google") {
          Serial.println("Starting Google Spam attack. Stop with " + (String)"stopscan");
          LOG_MESSAGE_TO_SD("Starting Google Spam attack.");
          HasRanCommand = true;
          BleModule->executeSpam(Google, true);
          return;
        }

        if (bt_type == "all")
        {
          Serial.println("Starting Spam all attack. Stop with " + (String)"stopscan");
          LOG_MESSAGE_TO_SD("Starting random wifi beacon attack.");
          HasRanCommand = true;
          BleModule->executeSpamAll();
        }
      }
      #endif
    }

    if (cmd_args.get(0) == "attack")
    {
      int wifi_type_sw = this->argSearch(&cmd_args, "-t");

      if (wifi_type_sw != -1) {
        String attack_type = cmd_args.get(wifi_type_sw + 1);

        if (attack_type == "beacon")
        {
          int israndom = this->argSearch(&cmd_args, "-r");
          int islist = this->argSearch(&cmd_args, "-l");

          if (israndom != -1)
          {
            HasRanCommand = true;
            Serial.println("Starting random wifi beacon attack. Stop with " + (String)"stopscan");
            LOG_MESSAGE_TO_SD("Starting random wifi beacon attack.");
            wifimodule->Attack(AT_RandomSSID);
            return;
          }

          if (islist != -1)
          {
            if (ssids->size() > 0)
            {
              HasRanCommand = true;
              Serial.println("Starting random wifi list attack. Stop with " + (String)"stopscan");
              LOG_MESSAGE_TO_SD("Starting random wifi list attack.");
              wifimodule->Attack(AT_ListSSID);
            }
            else 
            {
              Serial.println("Add Some SSIDs...");
              LOG_MESSAGE_TO_SD("Add Some SSIDs...");
            }
            return;
          }
        }

        if(attack_type == "deauth"){
          bool IsSelected = access_points->size() > 0;
          if (IsSelected)
          {
            HasRanCommand = true;
            Serial.println("Starting Deauth attack. Stop with " + (String)"stopscan");
            LOG_MESSAGE_TO_SD("Starting Deauth attack.");
            wifimodule->Attack(AT_DeauthAP);
          }
          else 
          {
            Serial.println("Scan for Access Points First...");
            LOG_MESSAGE_TO_SD("Scan for Access Points First...");
          }
          return;
        }

        if (attack_type == "rickroll")
        {
          Serial.println("Starting Rickroll wifi beacon attack. Stop with " + (String)"stopscan");
          LOG_MESSAGE_TO_SD("Starting Rickroll wifi beacon attack.");
          wifimodule->Attack(AT_Rickroll);
          return;
        }

      }
    }

    if (cmd_args.get(0) == "scanap")
    {
      Serial.println("Starting to scan access points");
      LOG_MESSAGE_TO_SD("Starting to scan access points");
      wifimodule->Scan(SCAN_AP);
      return;
    }

    if (cmd_args.get(0) == "led")
    {
      if (!RainbowLEDActive)
      {
        RainbowLEDActive = true;
        RainbowTask();
        RainbowTask();
        RainbowTask();
        RainbowTask();
#ifdef OLD_LED
        rgbmodule->breatheLED(0, 1000, true);
#endif
#ifdef NEOPIXEL_PIN
        neopixelmodule->breatheLED(0, 1000, true);
#endif
        RainbowLEDActive = false;
      }
      return;
    }

    if (cmd_args.get(0) == "castv2connect")
    {
      HasRanCommand = true;
      int ssid = this->argSearch(&cmd_args, "-s");
      int password = this->argSearch(&cmd_args, "-p");
      int value = this->argSearch(&cmd_args, "-v");

      if (ssid != -1 && password != -1)
      {
        String SSID = cmd_args.get(ssid + 1);
        String Password = cmd_args.get(password + 1);
        if (value != -1)
        {
          String Value = cmd_args.get(value + 1);
          ESPmDNSHelper* helper = new ESPmDNSHelper(SSID.c_str(), Password.c_str(), "", Value.c_str(), "233637DE");
        }
      }
    }

    if (cmd_args.get(0) == "deauthdetector")
    {
      HasRanCommand = true;
      int ssid = this->argSearch(&cmd_args, "-s");
      int password = this->argSearch(&cmd_args, "-p");
      int webhook = this->argSearch(&cmd_args, "-w");
      int channel_index = this->argSearch(&cmd_args, "-c");

      String SSID;
      String Password;
      String WebHook;
      String channel;
      if (ssid != -1 && password != -1)
      {
        SSID = cmd_args.get(ssid + 1);
        Password = cmd_args.get(password + 1);

        if (webhook != -1)
        {
          WebHook = cmd_args.get(webhook + 1);
        }
      }

      if (channel_index != -1)
      {
        channel = cmd_args.get(channel_index + 1);
      }
      InitDeauthDetector(channel, SSID, Password, WebHook);
    }

    if (cmd_args.get(0) == "dialconnect")
    {
      HasRanCommand = true;
      int ssid = this->argSearch(&cmd_args, "-s");
      int password = this->argSearch(&cmd_args, "-p");
      int type = this->argSearch(&cmd_args, "-t");
      int value = this->argSearch(&cmd_args, "-v");

      if (ssid != -1 && password != -1)
      {
        String SSID = cmd_args.get(ssid + 1);
        String Password = cmd_args.get(password + 1);
        if (type != -1 && value != -1)
        {
          String Type = cmd_args.get(type + 1);
          String Value = cmd_args.get(value + 1);
          AppController* controller = nullptr;
          if (Type.startsWith("youtube"))
          {
            controller = new YoutubeController();
          }
          else if (Type.startsWith("roku"))
          {
            controller = new RokuController();
          }
          else if (Type.startsWith("netflix"))
          {
            controller = new NetflixController();
          }
          else 
          {
            Serial.println("Type is Invalid....");
            LOG_MESSAGE_TO_SD("Type is Invalid....");
            return;
          }

          DIALClient* dial = new DIALClient(Value.c_str(), SSID.c_str(), Password.c_str(), controller);
#ifdef OLD_LED
rgbmodule->setColor(LOW, HIGH, LOW);
#endif
#ifdef NEOPIXEL_PIN
neopixelmodule->setColor(neopixelmodule->strip.Color(255, 0, 0));
#endif
          dial->Execute();
          delete dial;
          delete controller;
          return;
        }
        else 
        {
          Serial.println("Please Select a Type Of Dial Attack to Perform...");
          LOG_MESSAGE_TO_SD("Please Select a Type Of Dial Attack to Perform...");
        }
      }
      else 
      {
        Serial.println("SSID and Password are Empty...");
        LOG_MESSAGE_TO_SD("SSID and Password are Empty...");
      }
    }

    if (cmd_args.get(0) == "scansta")
    {
      if (access_points->size() > 0)
      {
        Serial.println("Starting to scan stations");
        LOG_MESSAGE_TO_SD("Starting to scan stations");
        wifimodule->Scan(SCAN_STA);
      }
      else 
      {
        Serial.println("Please Scan For a Access Point First");
        LOG_MESSAGE_TO_SD("Please Scan For a Access Point First");
      }
      
      return;
    }

    if (cmd_args.get(0) == "ssid")
    {
      int ap_sw = this->argSearch(&cmd_args, "-a");
      int ss_sw = this->argSearch(&cmd_args, "-g");
      int nn_sw = this->argSearch(&cmd_args, "-n");

      if (ap_sw != -1 && ss_sw != -1)
      {
        String NumSSids = cmd_args.get(ss_sw + 1);
        NumSSids.trim();
        wifimodule->generateSSIDs(NumSSids.toInt());
        Serial.printf("%i Random Ssids Generated\n", NumSSids.toInt());
        LOG_MESSAGE_TO_SD("Random Ssids Generated\n");
        LOG_MESSAGE_TO_SD(String(NumSSids.toInt()).c_str());
      }

      if (ap_sw != -1 && nn_sw != -1)
      {
        String SSIDName = cmd_args.get(nn_sw + 1);
        SSIDName.trim();
        wifimodule->addSSID(SSIDName);
        Serial.println("Added SSID " + SSIDName);
        LOG_MESSAGE_TO_SD("Added SSID ");
        LOG_MESSAGE_TO_SD(SSIDName.c_str());
      }
    }

    if (cmd_args.get(0) == "select")
    {
      int ap_sw = this->argSearch(&cmd_args, "-a");
      int SSIDIndex = cmd_args.get(ap_sw + 1).toInt();
      int ssid_sw = this->argSearch(&cmd_args, "-s");
      String SSIDArg = cmd_args.get(ssid_sw + 1);
      bool Selected = false;

      for (int i = 0; i < access_points->size(); i++) {
        AccessPoint AP = access_points->get(i);

        if (SSIDIndex != -1 && i == SSIDIndex)
        {
          AP.selected = true;
          Selected = true;
          break;
        }

        if (ssid_sw != -1 && AP.essid == SSIDArg)
        {
          AP.selected = true;
          Selected = true;
          break;
        }
      }
      if (!Selected)
      {
        Serial.println("Did not Select Anything Possible Index out of range");
        LOG_MESSAGE_TO_SD("Did not Select Anything Possible Index out of range");
      }
      else 
      {
        Serial.println("Successfully Selected The Access Point");
        LOG_MESSAGE_TO_SD("Successfully Selected The Access Point");
      }
    }

    if (cmd_args.get(0) == "clearlist")
    {
      int ap_sw = this->argSearch(&cmd_args, "-a");
      int ss_sw = this->argSearch(&cmd_args, "-s");
      int cl_sw = this->argSearch(&cmd_args, "-c");

      if (ap_sw != -1)
      {
        wifimodule->ClearList(ClearType::CT_AP);
        Serial.println("Cleared Access Point List");
        LOG_MESSAGE_TO_SD("Cleared Access Point List");
      }
      else if (ss_sw != -1)
      {
        wifimodule->ClearList(ClearType::CT_SSID);
        Serial.println("Cleared SSID List");
        LOG_MESSAGE_TO_SD("Cleared SSID List");
      }
      else if (cl_sw != -1)
      {
        wifimodule->ClearList(ClearType::CT_STA);
        Serial.println("Cleared Station List");
        LOG_MESSAGE_TO_SD("Cleared Station List");
      }
    }



    if (cmd_args.get(0) == "list") {
    int ap_sw = this->argSearch(&cmd_args, "-a");
    int ss_sw = this->argSearch(&cmd_args, "-s");
    int cl_sw = this->argSearch(&cmd_args, "-c");

    // List APs
    if (ap_sw != -1 && access_points != nullptr) {
      for (int i = 0; i < access_points->size(); i++) {
        if (access_points->get(i).selected) {
          Serial.println("[" + (String)i + "][CH:" + (String)access_points->get(i).channel + "] " + access_points->get(i).essid + " " + (String)access_points->get(i).rssi + " (selected)");
          count_selected += 1;
        } else
          Serial.println("[" + (String)i + "][CH:" + (String)access_points->get(i).channel + "] " + access_points->get(i).essid + " " + (String)access_points->get(i).rssi);
      }
    }
    // List SSIDs
    else if (ss_sw != -1 && ssids != nullptr) {
      for (int i = 0; i < ssids->size(); i++) {
        if (ssids->get(i).selected) {
          Serial.println("[" + (String)i + "] " + ssids->get(i).essid + " (selected)");
          count_selected += 1;
        } else
          Serial.println("[" + (String)i + "] " + ssids->get(i).essid);
      }
    }
    // List Stations
    else if (cl_sw != -1 && access_points != nullptr) { 
      char sta_mac[] = "00:00:00:00:00:00";
      for (int x = 0; x < access_points->size(); x++) {
        if (access_points->get(x).stations != nullptr) {
          Serial.println("[" + (String)x + "] " + access_points->get(x).essid + " " + (String)access_points->get(x).rssi + ":");
          for (int i = 0; i < access_points->get(x).stations->size(); i++) {
            wifimodule->getMACatoffset(sta_mac, stations->get(access_points->get(x).stations->get(i)).mac, 0);
            if (stations->get(access_points->get(x).stations->get(i)).selected) {
              Serial.print("  [" + (String)access_points->get(x).stations->get(i) + "] ");
              Serial.print(sta_mac);
              Serial.println(" (selected)");
              count_selected += 1;
            } else {
              Serial.print("  [" + (String)access_points->get(x).stations->get(i) + "] ");
              Serial.println(sta_mac);
            }
          }
        }
      }
    } else {
      Serial.println("You did not specify which list to show");
      LOG_MESSAGE_TO_SD("You did not specify which list to show");
      return;
    }

    if (cmd_args.get(0) == "stop")
    {
      #ifdef OLD_LED
      rgbmodule->setColor(1, 1, 1);
      #endif
      #ifdef NEOPIXEL_PIN
      neopixelmodule->strip.setBrightness(0);
      #endif
      wifimodule->shutdownWiFi();
      #ifdef HAS_BT
      BleModule->shutdownBLE();  
      #endif
    }
  }
}