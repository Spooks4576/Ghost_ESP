#pragma once

#include "interface/view_i.h"

class MainMenu : public ViewInterface
{
public:
    MainMenu(const char* ID = "mainmenu") // Should Always = mainmenu
    {
        ViewID = ID;
    }

    MainMenu()
    {

    }

lv_obj_t * status_bar;
lv_obj_t * versionlabel;
lv_obj_t * batteryversion;
lv_obj_t * flashIcon;
lv_obj_t * WifiChannelLabel;
LinkedList<lv_obj_t*> SBIcons;

lv_obj_t * add_battery_module(lv_obj_t * status_bar);
lv_obj_t * create_status_bar(lv_obj_t * parent);
lv_obj_t * add_version_module(lv_obj_t * status_bar);
virtual void HandleTouch(TS_Point P) override;
virtual void Render() override;
void UpdateWifiChannelStatus(int Channel);
virtual void HandleAnimations(unsigned long Millis, unsigned long LastTick) override;
};