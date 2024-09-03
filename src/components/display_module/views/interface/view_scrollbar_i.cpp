#include "view_scrollbar_i.h"

void ScrollableMenu::Render() {
    status_bar = create_status_bar(lv_scr_act());
    List = lv_roller_create(lv_scr_act());
    lv_obj_set_style_opa(List, LV_OPA_0, 0);
    lv_obj_set_style_bg_color(List, lv_color_hex(0x525252), 0);
    lv_obj_set_style_border_color(List, lv_color_hex(0x158FCA), 0);
    lv_obj_set_size(List, 200, 200);
    lv_obj_set_x(List, 280);
    lv_obj_set_y(List, 23);
    lv_obj_set_scrollbar_mode(List, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_transform_pivot_x(List, lv_obj_get_width(List) / 2, 0);
    lv_obj_set_style_transform_pivot_y(List, lv_obj_get_height(List) / 2, 0);  
    lv_obj_set_style_transform_angle(List, 900, 0);
    lv_obj_clear_flag(List, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_fade_in(status_bar, 300, 0);
    lv_obj_fade_in(List, 300, 0);

    if (ViewID == "WifiMenu")
    {
        addItem("Scan Access Points\n Scan Stations\nList Access Points\nSelect Access Point\nSelect Station\nAdd Random SSID\nAdd Specific SSID\nBeacon Attack List\nBeacon Attack Random\nBeacon Attack RickRoll\nBeacon Attack Karma\nBeacon Attack Rickroll\nAttack Deauth\nCast V2 Connect\nDial Connect\nDeauth Detector\nCalibrate\nSniff Raw\n Sniff Beacon\n Sniff Probe\nSniff Pwn\nSniff PMKID");
    }
    else if (ViewID == "BluetoothMenu")
    {
        addItem("Ble Spam\nFind The Flippers\nDetect BLE Spam\nAir Tag Scan");
    }
    else if (ViewID == "LEDUtils")
    {
        addItem("Rainbow LED");
    }
    else if (ViewID == "GPSMenu")
    {
        addItem("Street Detector\nWardrive (WiFi)\nWardrive (Bluetooth)");
    }

    BackBtn = RenderImageToButton(lv_obj_get_parent(status_bar), &Arrow, 90, 40, 100, 35, 35);
    // ConfirmButton = RenderImageToButton(lv_scr_act(), &Arrow, 0, 12, 285, 70, 33);
    // MoveSelectUpBtn = RenderImageToButton(lv_scr_act(), &Arrow, 180, 12, 295, 70, 33);
    // MoveSelectDownBtn = RenderImageToButton(lv_scr_act(), &Arrow, -180, 12, 245, 70, 33);
}


void ScrollableMenu::HandleTouch(TS_Point P)  {

}


void ScrollableMenu::HandleAnimations(unsigned long Millis, unsigned long LastTick)  {
    
}


void ScrollableMenu::addItem(const char *text) {
    lv_roller_set_options(List, text, LV_ROLLER_MODE_INFINITE);
}