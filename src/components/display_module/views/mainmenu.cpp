#include "mainmenu.h"
#include <core/system_manager.h>

void MainMenu::HandleAnimations(unsigned long Millis, unsigned long LastTick)
{
    LastMillis = LastTick;
}

void MainMenu::Render()
{
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    status_bar = create_status_bar(lv_scr_act());
    grid_container = create_grid_container(lv_scr_act(), 0, 0);
    lv_obj_set_style_opa(grid_container, LV_OPA_0, 0);
    lv_obj_clear_flag(grid_container, LV_OBJ_FLAG_SCROLLABLE);
    CreateGridButtons();
    Debugging = false;
    lv_obj_fade_in(grid_container, 200, 0);
    lv_obj_fade_in(status_bar, 200, 0);
}

void MainMenu::CreateGridButtons()
{
    for (int i = 0; i < 5; i++) 
    {
        lv_obj_t *btn = nullptr;

        lv_obj_t *btn_container = lv_obj_create(grid_container); 
        lv_obj_set_size(btn_container, 70, 70);

        switch (i) {
            case 0:  btn = RenderImageToButton(btn_container, &Bluetooth_img, 45, 55, -5, 50, 50); break;
            case 1:  btn = RenderImageToButton(btn_container, &WiFi_img, 45, 55, -5, 50, 50); break;
            case 2:  btn = RenderImageToButton(btn_container, &LED_img, 45, 55, -5, 50, 50); break;
            case 3:  btn = RenderImageToButton(btn_container, &Map_img, 45, 55, -5, 50, 50); break;
            case 4:  btn = RenderImageToButton(btn_container, &Settings_img, 45, 55, -5, 50, 50); break;
        } 

        if (btn)
        {
            OtherObjects.add(btn_container); // For Touch Controls
            ImageObjects.add(btn);
            lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_style_transform_angle(btn, 900, 0);
            lv_obj_set_style_transform_pivot_x(btn, lv_obj_get_width(btn) / 2, 0);
            lv_obj_set_style_transform_pivot_y(btn, lv_obj_get_height(btn) / 2, 0);
            lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_scrollbar_mode(btn_container, LV_SCROLLBAR_MODE_OFF);
            lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);

            static lv_style_t style_btn;
            lv_style_init(&style_btn);
            lv_style_set_bg_color(&style_btn, lv_color_hex(0x000));
            lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
            lv_style_set_radius(&style_btn, 10);
            lv_style_set_border_width(&style_btn, 0);
            lv_obj_add_style(btn, &style_btn, 0);
            lv_obj_add_style(btn_container, &style_btn, 0);
        }

        lv_obj_t *label = lv_label_create(btn_container); 
        lv_obj_align(label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

        switch (i) {
            case 0:  lv_label_set_text(label, "BLE"); break;
            case 1:  lv_label_set_text(label, "WiFi"); break;
            case 2:  lv_label_set_text(label, "LED"); break;
            case 3:  lv_label_set_text(label, "GPS"); break;
            case 4:  lv_label_set_text(label, "SET"); break;
        } 

        static lv_style_t label_text_style;
        lv_style_init(&label_text_style);
        lv_style_set_text_font(&label_text_style, &Juma);
        lv_style_set_text_color(&label_text_style, lv_color_white());
        lv_obj_add_style(label, &label_text_style, 0); 
        lv_obj_set_style_transform_angle(label, 900, 0);
        lv_obj_set_scrollbar_mode(label, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_transform_pivot_x(label, lv_obj_get_width(label) / 2, 0);
        lv_obj_set_style_transform_pivot_y(label, lv_obj_get_height(label) / 2, 0);
    }
}

void MainMenu::UpdateWifiChannelStatus(int Channel)
{
    lv_label_set_text(WifiChannelLabel, String("CH: " + Channel).c_str());
}

lv_obj_t* MainMenu::create_grid_container(lv_obj_t *parent, int x, int y) 
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
    lv_obj_set_pos(lgrid_container, x, y);
    static lv_style_t style_grid;
    lv_style_init(&style_grid);
    lv_style_set_bg_opa(&style_grid, LV_OPA_TRANSP);
    lv_style_set_grid_column_dsc_array(&style_grid, grid_col_dsc); // Assumes you have grid_col_dsc defined
    lv_style_set_grid_row_dsc_array(&style_grid, grid_row_dsc); // Assumes you have grid_row_dsc defined
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
        if (is_point_inside_button(P, OtherObjects.get(i)))
        {
            lv_obj_fade_out(grid_container, 250, 0);
            lv_obj_fade_out(status_bar, 250, 0);

            switch (i) {
                case 0:  Destroy(MenuType::MT_BluetoothMenu); break;
                case 1:  Destroy(MenuType::MT_WifiUtilsMenu); break;
                case 2:  Destroy(MenuType::MT_LEDUtils); break;
                case 3:  Destroy(MenuType::MT_GPSMenu); break;
                case 4:  Destroy(MenuType::MT_SettingsMenu); break;
            }
        }
    }

}

