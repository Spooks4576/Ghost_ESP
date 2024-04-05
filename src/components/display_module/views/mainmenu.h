#pragma once

#include "interface/view_i.h"

static lv_coord_t grid_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static lv_coord_t grid_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

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

lv_obj_t* grid_container;

lv_obj_t * add_battery_module(lv_obj_t * status_bar);
lv_obj_t * create_status_bar(lv_obj_t * parent);
lv_obj_t * add_version_module(lv_obj_t * status_bar);
lv_obj_t * create_grid_container(lv_obj_t * parent);
bool is_point_inside_button(TS_Point p, lv_obj_t* btn);
virtual void HandleTouch(TS_Point P) override;
virtual void Render() override;
void CreateGridButtons();
void UpdateWifiChannelStatus(int Channel);
virtual void HandleAnimations(unsigned long Millis, unsigned long LastTick) override;
};