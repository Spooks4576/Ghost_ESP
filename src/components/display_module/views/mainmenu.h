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

lv_obj_t* grid_container;


lv_obj_t * create_grid_container(lv_obj_t * parent, int x, int y);
virtual void HandleTouch(TS_Point P) override;
virtual void Render() override;
void CreateGridButtons();
void UpdateWifiChannelStatus(int Channel);
virtual void HandleAnimations(unsigned long Millis, unsigned long LastTick) override;
};