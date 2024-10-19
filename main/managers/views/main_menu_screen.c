#include "managers/views/main_menu_screen.h"
#include "managers/views/music_visualizer.h"
#include <stdio.h>


lv_obj_t *menu_container;
static int selected_item_index = 0;

typedef struct {
    const char *name;
    const lv_img_dsc_t *icon;
    lv_color_t border_color;
} menu_item_t;

static menu_item_t menu_items[] = {
    {"SET", &Settings},
    {"GPS", &Map},
    {"BLE", &bluetooth},
    {"WiFi", &wifi},
    {"Rave", &rave}
};

static int num_items = sizeof(menu_items) / sizeof(menu_items[0]);

/**
 * @brief Callback function for menu item click events.
 */
#ifdef USE_TOUCHSCREEN
static void menu_item_event_handler(lv_event_t *e) {
    lv_obj_t *menu_item = lv_event_get_target(e);
    int item_index = (int)lv_obj_get_user_data(menu_item);

    printf("Called\n");

    // Handle each item index differently
    switch (item_index) {
        case 0:
            printf("Wi-Fi selected\n");
            break;
        case 1:
            printf("BLE selected\n");
            break;
        case 2:
            printf("GPS selected\n");
            break;
        case 3:
            printf("Settings selected\n");
            break;
        default:
            printf("Unknown menu item selected\n");
            break;
    }
}
#else
/**
 * @brief Updates the style of the selected and unselected menu items.
 */
static void update_menu_item_styles(void) {
    for (int i = 0; i < num_items; i++) {
        lv_obj_t *menu_item = (lv_obj_t *)lv_obj_get_child(menu_container, i);
        if (i == selected_item_index) {
            lv_obj_set_style_border_color(menu_item, lv_color_make(255, 255, 0), 0);
            lv_obj_set_style_border_width(menu_item, 4, 0);
        } else {
            lv_obj_set_style_border_color(menu_item, menu_items[i].border_color, 0);
            lv_obj_set_style_border_width(menu_item, 2, 0);
        }
    }
}

/**
 * @brief Handles selection of menu items with hardware buttons.
 */
static void select_menu_item(int index) {
    if (index < 0) index = num_items - 1;
    if (index >= num_items) index = 0;

    selected_item_index = index;
    update_menu_item_styles();
}
#endif


void handle_hardware_button_press(int ButtonPressed) {
#ifndef USE_TOUCHSCREEN
    if (ButtonPressed == 0) {
        select_menu_item(selected_item_index - 1);
    } else if (ButtonPressed == 3) {
        select_menu_item(selected_item_index + 1);
    } else if (ButtonPressed == 1) {
        switch (selected_item_index) {
            case 0:
                printf("Settings selected\n");
                SelectedMenuType = OT_Settings;
                display_manager_switch_view(&options_menu_view);
                break;
            case 1:
                printf("GPS selected\n");
                SelectedMenuType = OT_GPS;
                display_manager_switch_view(&options_menu_view);
                break;
            case 2:
                printf("BLE selected\n");
                SelectedMenuType = OT_Bluetooth;
                display_manager_switch_view(&options_menu_view);
                break;
            case 3:
                printf("Wi-Fi selected\n");
                SelectedMenuType = OT_Wifi;
                display_manager_switch_view(&options_menu_view);
                break;
            case 4: 
                printf("Rave Selected\n");
                display_manager_switch_view(&music_visualizer_view);
                break;
            default:
                printf("Unknown menu item selected\n");
                break;
        }
    }
#endif
}

/**
 * @brief Creates the main menu screen view.
 */
void main_menu_create(void) {

    
    menu_items[0].border_color = lv_color_make(255, 255, 255);
    menu_items[1].border_color = lv_color_make(255, 0, 0);  
    menu_items[2].border_color = lv_color_make(0, 0, 255);  
    menu_items[3].border_color = lv_color_make(0, 255, 0);
    menu_items[4].border_color = lv_color_make(147, 112, 219);

    display_manager_fill_screen(lv_color_black());

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    
    menu_container = lv_obj_create(lv_scr_act());
    main_menu_view.root = menu_container;
    lv_obj_set_grid_dsc_array(menu_container, col_dsc, row_dsc);
    lv_obj_set_size(menu_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(menu_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_column(menu_container, 10, 0);
    lv_obj_set_style_bg_opa(menu_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_container, 0, 0);
    lv_obj_set_style_pad_all(menu_container, 0, 0);
    lv_obj_set_style_radius(menu_container, 0, 0);

    lv_obj_align(menu_container, LV_ALIGN_BOTTOM_MID, 0, 0);

    
    lv_disp_t *disp = lv_disp_get_default();
    int hor_res = lv_disp_get_hor_res(disp);
    int ver_res = lv_disp_get_ver_res(disp);

    int button_width = hor_res / 4;
    int button_height = ver_res / 3;

    int icon_width = 50;
    int icon_height = 50;

    const lv_font_t *font;
    if (ver_res <= 128) {
        font = &lv_font_montserrat_10;
    } else if (ver_res <= 240) {
        font = &lv_font_montserrat_16;
    } else {
        font = &lv_font_montserrat_18;
    }


    for (int i = 0; i < num_items; i++) {
        lv_obj_t *menu_item = lv_btn_create(menu_container);
         lv_obj_set_size(menu_item, button_width, button_height);
        lv_obj_set_style_bg_color(menu_item, lv_color_black(), 0);
        lv_obj_set_style_border_color(menu_item, menu_items[i].border_color, 0);
        lv_obj_set_style_border_width(menu_item, 2, 0);
        lv_obj_set_style_radius(menu_item, 10, 0);
        lv_obj_set_scrollbar_mode(menu_item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_pad_all(menu_item, 5, 0);

        if (ver_res >= 240) // Dont create icons for smaller screens
        {
            if (menu_items[i].icon)
            {
                lv_obj_t *icon = lv_img_create(menu_item);
                lv_img_set_src(icon, menu_items[i].icon);
                lv_obj_set_size(icon, icon_width, icon_height);
                lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);
            }
        }

        
        lv_obj_t *label = lv_label_create(menu_item);
        lv_label_set_text(label, menu_items[i].name);
        lv_obj_set_style_text_font(label, font, 0);

        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);

        int row_idx = 2 - (i / 3);  // This reverses the row placement
        lv_obj_set_grid_cell(menu_item, LV_GRID_ALIGN_CENTER, i % 3, 1, LV_GRID_ALIGN_END, row_idx, 1);

        lv_obj_set_user_data(menu_item, (void *)(uintptr_t)i);
#ifdef USE_TOUCHSCREEN
    lv_obj_add_event_cb(menu_item, menu_item_event_handler, LV_EVENT_CLICKED, NULL);
#endif
    }

#ifndef USE_TOUCHSCREEN
    select_menu_item(0);
#endif

    display_manager_add_status_bar("Main Menu");
}

/**
 * @brief Destroys the main menu screen view.
 */
void main_menu_destroy(void) {
    if (menu_container) {
        lv_obj_clean(menu_container);
        lv_obj_del(menu_container);
        menu_container = NULL;
    }
}

void get_main_menu_callback(void **callback) {
    *callback = main_menu_view.hardwareinput_callback;
}

/**
 * @brief Main menu screen view object.
 */
View main_menu_view = {
    .root = NULL,
    .create = main_menu_create,
    .destroy = main_menu_destroy,
    .hardwareinput_callback = handle_hardware_button_press,
    .name = "Main Menu",
    .get_hardwareinput_callback = get_main_menu_callback
};