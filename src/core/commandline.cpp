#include "commandline.h"
#include "../components/ble_module/ble_module.h"

CommandLine::CommandLine() {
}

void CommandLine::RunSetup() {
  Serial.println(F("         ESP32 Marauder Rewrite      \n"));
  Serial.println(F("       By: justcallmekoko Rewritten by: Spooky\n"));
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

void CommandLine::showCounts(int selected, int unselected) {
  Serial.print((String(selected) + " selected"));
  
  if (unselected != -1) 
    Serial.print(", " + (String) unselected + " unselected");
  
  Serial.println("");
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

  this->showCounts(count_selected, count_unselected);
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
            if (gps_obj.getGpsModuleStatus()) {
                Serial.println("Getting GPS Data. Stop with " + "stopscan");

              HasRanCommand = true;
            }
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
          HasRanCommand = true;
          BleModule->executeSpam(Apple);
          return;
        }

        if (bt_type == "windows") {
          Serial.println("Starting Swiftpair Spam attack. Stop with " + (String)"stopscan");
          HasRanCommand = true;
          BleModule->executeSpam(Microsoft);
          return;
        }

        if (bt_type == "samsung") {
          Serial.println("Starting Samsung Spam attack. Stop with " + (String)"stopscan");
          HasRanCommand = true;
          BleModule->executeSpam(Samsung);
          return;
        }

        if (bt_type == "google") {
          Serial.println("Starting Google Spam attack. Stop with " + (String)"stopscan");
          HasRanCommand = true;
          BleModule->executeSpam(Google);
          return;
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

          if (israndom != -1)
          {
            HasRanCommand = true;
            Serial.println("Starting random wifi beacon attack. Stop with " + (String)"stopscan");


            wifimodule->InitRandomSSIDAttack();
            return;
          }
        }

        if (attack_type == "rickroll")
        {
          Serial.println("Starting Rickroll wifi beacon attack. Stop with " + (String)"stopscan");
          wifimodule->broadcastRickroll();
          return;
        }

      }
    }

    if (cmd_args.get(0) == "scanap")
    {
      Serial.println("Starting to scan access points");
      HasRanCommand = true;

      wifimodule->RunAPScan();

      return;
    }

    if (cmd_args.get(0) == "scansta")
    {
      Serial.println("Starting to scan stations");
      HasRanCommand = true;

      wifimodule->RunStaScan();
      return;
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
      // this->showCounts(count_selected); // Causes Crash
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
      // this->showCounts(count_selected); // Causes Crash
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
      // this->showCounts(count_selected); // Causes Crash
    } else {
      Serial.println("You did not specify which list to show");
      return;
    }
  }
}