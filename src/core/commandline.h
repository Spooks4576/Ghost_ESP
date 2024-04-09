#pragma once

#include "globals.h"
#include <LinkedList.h>
#include <ArduinoJson.h>

class CommandLine {
  public:
    String getSerialInput();
    void executeJsonScript(const char* json);
    LinkedList<String> parseCommand(String input, char* delim);
    String toLowerCase(String str);
    void filterAccessPoints(String filter);
    void runCommand(String input);
    bool checkValueExists(LinkedList<String>* cmd_args_list, int index);
    bool inRange(int max, int index);
    bool apSelected();
    bool hasSSIDs();
    int argSearch(LinkedList<String>* cmd_args, String key);
public:
    CommandLine();

    void RunSetup();
    void main(uint32_t currentTime);
    uint8_t count_selected;
};