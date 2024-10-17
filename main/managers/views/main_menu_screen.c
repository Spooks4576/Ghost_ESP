#include "managers/views/main_menu_screen.h"
#include <stdio.h>


static lv_obj_t *menu_container;

typedef struct {
    const char *name;
    const lv_img_dsc_t *icon;
    lv_color_t border_color;
} menu_item_t;

static menu_item_t menu_items[] = {
    {"Settings", &Settings},
    {"GPS", &Map},
    {"BLE", &bluetooth},
    {"Wi-Fi", &wifi},
};

static int num_items = sizeof(menu_items) / sizeof(menu_items[0]);

/**
 * @brief Callback function for menu item click events.
 */
static void menu_item_event_handler(lv_event_t *e) {
    lv_obj_t *menu_item = lv_event_get_target(e);
    int item_index = (int)lv_event_get_user_data(e);

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

/**
 * @brief Creates the main menu screen view.
 */
void main_menu_create(void) {

    
    menu_items[0].border_color = lv_color_make(255, 255, 255);
    menu_items[1].border_color = lv_color_make(255, 0, 0);  
    menu_items[2].border_color = lv_color_make(0, 0, 255);  
    menu_items[3].border_color = lv_color_make(0, 255, 0);

    display_manager_fill_screen(lv_color_black());

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    
    menu_container = lv_obj_create(lv_scr_act());
    lv_obj_set_grid_dsc_array(menu_container, col_dsc, row_dsc);
    lv_obj_set_size(menu_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scroll_dir(menu_container, LV_DIR_NONE); 
    lv_obj_set_scrollbar_mode(menu_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_column(menu_container, 10, 0);
    lv_obj_set_style_bg_opa(menu_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_container, 0, 0);
    lv_obj_set_style_pad_all(menu_container, 0, 0);
    lv_obj_set_style_radius(menu_container, 0, 0);
    lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(menu_container, LV_ALIGN_BOTTOM_MID, 0, 0);


    for (int i = 0; i < num_items; i++) {
        lv_obj_t *menu_item = lv_obj_create(menu_container);
        lv_obj_set_size(menu_item, 70, 100);
        lv_obj_set_style_bg_color(menu_item, lv_color_black(), 0);
        lv_obj_set_style_border_color(menu_item, menu_items[i].border_color, 0);
        lv_obj_set_style_border_width(menu_item, 2, 0);
        lv_obj_set_style_radius(menu_item, 10, 0);
        lv_obj_set_scrollbar_mode(menu_item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_pad_all(menu_item, 5, 0);

        
        lv_obj_t *icon = lv_img_create(menu_item);
        lv_img_set_src(icon, menu_items[i].icon);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 10);

        
        lv_obj_t *label = lv_label_create(menu_item);
        lv_label_set_text(label, menu_items[i].name);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);

        int row_idx = 2 - (i / 3);  // This reverses the row placement
        lv_obj_set_grid_cell(menu_item, LV_GRID_ALIGN_CENTER, i % 3, 1, LV_GRID_ALIGN_END, row_idx, 1);


        lv_obj_add_event_cb(menu_item, menu_item_event_handler, LV_EVENT_CLICKED, (void *)i);
    }

    display_manager_add_status_bar();
}

/**
 * @brief Destroys the main menu screen view.
 */
void main_menu_destroy(void) {
    if (menu_container) {
        lv_obj_del(menu_container);
        menu_container = NULL;
    }
}

/**
 * @brief Main menu screen view object.
 */
View main_menu_view = {
    .root = NULL,
    .create = main_menu_create,
    .destroy = main_menu_destroy
};