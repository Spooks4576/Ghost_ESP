#include "mainmenu.h"
#include <core/system_manager.h>

void MainMenu::HandleAnimations(unsigned long Millis, unsigned long LastTick)
{
    
}

void MainMenu::Render()
{
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    status_bar = create_status_bar(lv_scr_act());
    grid_container = create_grid_container(lv_scr_act());
    CreateGridButtons();
    Debugging = true;
}

void MainMenu::CreateGridButtons()
{
    for (int i = 0; i < 9; i++) 
    {
        lv_obj_t *btn = lv_btn_create(grid_container);
        lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
        if (i == 1)
        {
            RenderJpg(&bt_img, 245, 25, 0, 45);
            OtherObjects.add(btn);
            lv_obj_set_pos(btn, 245, 25);
        }

        if (i == 2)
        {
            RenderJpg(&WiFi_img, 245, 95, 1, 45);
            OtherObjects.add(btn);
            lv_obj_set_pos(btn, 245, 95);
        }

        if (i == 3)
        {
            RenderJpg(&led_img, 245, 165, 2, 45);
            OtherObjects.add(btn);
            lv_obj_set_pos(btn, 245, 165);
        }

        lv_obj_set_size(btn, 50, 50);

        lv_obj_set_style_transform_angle(btn, 900, 0);
        lv_obj_set_style_transform_pivot_x(btn, lv_obj_get_width(btn) / 2, 0);
        lv_obj_set_style_transform_pivot_y(btn, lv_obj_get_height(btn) / 2, 0);

        static lv_style_t style_btn;
        lv_style_init(&style_btn);
        lv_style_set_bg_color(&style_btn, lv_color_hex(0x000));
        lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
        lv_style_set_radius(&style_btn, 10);
        lv_obj_add_style(btn, &style_btn, 0);
    }
}

void MainMenu::UpdateWifiChannelStatus(int Channel)
{
    lv_label_set_text(WifiChannelLabel, String("CH: " + Channel).c_str());
}

lv_obj_t* MainMenu::create_grid_container(lv_obj_t * parent)
{
    lv_obj_t *lgrid_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(lgrid_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_scrollbar_mode(lgrid_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(lgrid_container, LV_FLEX_FLOW_COLUMN_WRAP);
    lv_obj_set_style_border_width(lgrid_container, 0, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_x(lgrid_container, lv_obj_get_width(lgrid_container) / 2, 0);
    lv_obj_set_style_transform_pivot_y(lgrid_container, lv_obj_get_height(lgrid_container) / 2, 0);   
    lv_obj_set_style_outline_width(lgrid_container, 0, LV_PART_MAIN);
    lv_obj_set_flex_align(lgrid_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_pos(lgrid_container, 50, 0);
    static lv_style_t style_grid;
    lv_style_init(&style_grid);
    lv_style_set_bg_opa(&style_grid, LV_OPA_TRANSP);
    lv_style_set_grid_column_dsc_array(&style_grid, grid_col_dsc);
    lv_style_set_grid_row_dsc_array(&style_grid, grid_row_dsc);
    lv_style_set_layout(&style_grid, LV_LAYOUT_GRID);

    lv_obj_add_style(lgrid_container, &style_grid, 0);
    return lgrid_container;
}

void MainMenu::HandleTouch(TS_Point P)
{

    if (Debugging)
    {
        lv_obj_t *circle = lv_obj_create(lv_scr_act());
        lv_obj_set_size(circle, 20, 20);
        lv_obj_set_pos(circle, P.x, P.y);

        // Apply a style to make it look like a circle
        static lv_style_t style_circle;
        lv_style_init(&style_circle);
        lv_style_set_radius(&style_circle, LV_RADIUS_CIRCLE);
        lv_style_set_bg_opa(&style_circle, LV_OPA_COVER);
        lv_style_set_bg_color(&style_circle, lv_palette_main(LV_PALETTE_RED));
        lv_style_set_border_color(&style_circle, lv_palette_main(LV_PALETTE_RED));
        lv_style_set_border_width(&style_circle, 2);

        lv_obj_add_style(circle, &style_circle, 0);
        
        lv_timer_t *timer = lv_timer_create([](lv_timer_t *timer){
            lv_obj_del(static_cast<lv_obj_t*>(timer->user_data));
            lv_timer_del(timer);
        }, 2000, circle);
    }

    for (int i = 0; i < OtherObjects.size(); i++)
    {
        if (is_point_inside_button(P, OtherObjects[i]))
        {
            if (Debugging)
            {
                lv_obj_t *circle = lv_obj_create(lv_scr_act());
                lv_obj_set_size(circle, 20, 20);
                lv_obj_set_pos(circle, P.x, P.y);

                // Apply a style to make it look like a circle
                static lv_style_t style_circle;
                lv_style_init(&style_circle);
                lv_style_set_radius(&style_circle, LV_RADIUS_CIRCLE);
                lv_style_set_bg_opa(&style_circle, LV_OPA_COVER);
                lv_style_set_bg_color(&style_circle, lv_palette_main(LV_PALETTE_GREEN));
                lv_style_set_border_color(&style_circle, lv_palette_main(LV_PALETTE_GREEN));
                lv_style_set_border_width(&style_circle, 2);

                lv_obj_add_style(circle, &style_circle, 0);
                
                lv_timer_t *timer = lv_timer_create([](lv_timer_t *timer){
                    lv_obj_del(static_cast<lv_obj_t*>(timer->user_data));
                    lv_timer_del(timer);
                }, 2000, circle);
            }
        }
    }

}

bool MainMenu::is_point_inside_button(TS_Point p, lv_obj_t* btn) {

    int btn_x = lv_obj_get_x(btn);
    int btn_y = lv_obj_get_y(btn);

    int btn_width = lv_obj_get_width(btn);
    int btn_height = lv_obj_get_height(btn);
    

    return (p.x >= btn_x && p.x <= (btn_x + btn_width) &&
            p.y >= btn_y && p.y <= (btn_y + btn_height));
}

lv_obj_t * MainMenu::add_battery_module(lv_obj_t * status_bar) {
    lv_obj_t * battery_icon = lv_label_create(status_bar);
    lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_transform_angle(battery_icon, 900, 0);
    lv_obj_set_style_transform_pivot_x(battery_icon, lv_obj_get_width(battery_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(battery_icon, lv_obj_get_height(battery_icon) / 2, 0);   
    lv_obj_set_pos(battery_icon, 8, 200);
    lv_obj_set_style_text_color(battery_icon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    flashIcon = lv_label_create(status_bar);
    lv_label_set_text(flashIcon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_transform_angle(flashIcon, 900, 0);
    lv_obj_set_style_transform_pivot_x(flashIcon, lv_obj_get_width(flashIcon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(flashIcon, lv_obj_get_height(flashIcon) / 2, 0);   
    lv_obj_set_pos(flashIcon, 8, 185);
    lv_obj_set_style_text_color(flashIcon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    return battery_icon;
}

lv_obj_t* MainMenu::add_version_module(lv_obj_t * status_bar)
{
    lv_obj_t * version = lv_label_create(status_bar);
    lv_label_set_text(version, "V-1.5A");
    lv_obj_set_style_transform_angle(version, 900, 0);
    lv_obj_set_style_transform_pivot_x(version, lv_obj_get_width(version) / 2, 0);
    lv_obj_set_style_transform_pivot_y(version, lv_obj_get_height(version) / 2, 0);   
    lv_obj_set_pos(version, 8, 0);
    lv_obj_set_style_text_font(version, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(version, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    return version;
}

lv_obj_t * MainMenu::create_status_bar(lv_obj_t * parent) {
    lv_obj_t * status_bar = lv_obj_create(parent);
    lv_obj_set_height(status_bar, LV_PCT(100));
    lv_obj_set_width(status_bar, LV_DPX(35));
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_COVER, 0);
    lv_obj_set_layout(status_bar, LV_LAYOUT_NONE);

    lv_obj_align(status_bar, LV_ALIGN_RIGHT_MID, 0, 0);

    versionlabel = add_version_module(status_bar);
    batteryversion = add_battery_module(status_bar);

     int xOffset = 185;

#ifdef HAS_BT
    lv_obj_t* bt_icon = lv_label_create(status_bar);
    SBIcons.add(bt_icon);
    lv_label_set_text(bt_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_transform_angle(bt_icon, 900, 0);
    lv_obj_set_style_transform_pivot_x(bt_icon, lv_obj_get_width(bt_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(bt_icon, lv_obj_get_height(bt_icon) / 2, 0);   
    lv_obj_set_pos(bt_icon, 8, xOffset -= 16);
    lv_obj_set_style_text_color(bt_icon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
#endif

#ifdef SD_CARD_CS_PIN
    lv_obj_t* sd_icon = lv_label_create(status_bar);
    SBIcons.add(sd_icon);
    lv_label_set_text(sd_icon, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_transform_angle(sd_icon, 900, 0);
    lv_obj_set_style_transform_pivot_x(sd_icon, lv_obj_get_width(sd_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(sd_icon, lv_obj_get_height(sd_icon) / 2, 0);   
    lv_obj_set_pos(sd_icon, 8, xOffset -= 20);
    lv_obj_set_style_text_color(sd_icon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
#endif

    lv_obj_t* wifi_icon = lv_label_create(status_bar);
    SBIcons.add(wifi_icon);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_transform_angle(wifi_icon, 900, 0);
    lv_obj_set_style_transform_pivot_x(wifi_icon, lv_obj_get_width(wifi_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(wifi_icon, lv_obj_get_height(wifi_icon) / 2, 0);   
    lv_obj_set_pos(wifi_icon, 8, xOffset -= 23);
    lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0x158FCA), LV_STATE_DEFAULT);
    return status_bar;
}