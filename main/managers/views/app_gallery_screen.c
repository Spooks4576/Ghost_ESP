#include "managers/views/app_gallery_screen.h"
#include "managers/views/flappy_ghost_screen.h"
#include "managers/views/main_menu_screen.h"
#include "managers/views/music_visualizer.h"
#include <stdio.h>
#include <string.h>

lv_obj_t *apps_container;
static int selected_app_index = 0;

typedef struct {
    const char *name;
    const lv_img_dsc_t *icon;
    lv_color_t border_color;
    View *view;
} app_item_t;

static const lv_color_t flap_color = LV_COLOR_MAKE(255, 215, 0);
static const lv_color_t rave_color = LV_COLOR_MAKE(128, 0, 128);


static app_item_t app_items[] = {
    {"Flap", &GESPFlappyghost, flap_color, &flappy_bird_view},
    {"Rave", &rave, rave_color, &music_visualizer_view},
};

static int num_apps = sizeof(app_items) / sizeof(app_items[0]);
static int apps_per_page = 6;
static int current_page = 0;
static int total_pages = 0;

static lv_obj_t *back_button = NULL;
#ifdef CONFIG_USE_TOUCHSCREEN
static lv_obj_t *next_button = NULL;
static lv_obj_t *prev_button = NULL;
#endif

void refresh_apps_menu(void);

void apps_menu_create(void) {
    display_manager_fill_screen(lv_color_black());


    total_pages = (num_apps + apps_per_page - 1) / apps_per_page;


    bool is_landscape = (LV_HOR_RES >= LV_VER_RES);
    printf("Screen: %dx%d, Orientation: %s\n", LV_HOR_RES, LV_VER_RES, is_landscape ? "Landscape" : "Portrait");


    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {
        LV_GRID_FR(1),   // Row 0: Apps row 1 (top)
        LV_GRID_FR(1),   // Row 1: Apps row 2
        LV_GRID_FR(0.5), // Row 2: Buffer or spacer
        #ifdef CONFIG_USE_TOUCHSCREEN
        LV_GRID_FR(0.5), // Row 3: Navigation buttons (bottom)
        #endif
        LV_GRID_TEMPLATE_LAST
    };


    int button_width = LV_HOR_RES / 4;
    int button_height = LV_VER_RES / 8;

    // Create container
    apps_container = lv_obj_create(lv_scr_act());
    if (!apps_container) {
        printf("Failed to create apps_container\n");
        return;
    }
    apps_menu_view.root = apps_container;
    lv_obj_set_grid_dsc_array(apps_container, col_dsc, row_dsc);
    lv_obj_set_size(apps_container, LV_HOR_RES, LV_VER_RES); // Full screen: 1024x600
    lv_obj_set_scrollbar_mode(apps_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_column(apps_container, 10, 0);
    lv_obj_set_style_bg_opa(apps_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(apps_container, 0, 0);
    lv_obj_set_style_pad_left(apps_container, 10, 0);
    lv_obj_set_style_pad_bottom(apps_container, 10, 0);
    lv_obj_set_style_pad_right(apps_container, 10, 0);
    lv_obj_set_style_pad_top(apps_container, 5, 0);
    lv_obj_set_style_radius(apps_container, 0, 0);
    lv_obj_align(apps_container, LV_ALIGN_TOP_MID, 0, 0);

    // Create Back Button
    back_button = lv_btn_create(apps_container);
    lv_obj_set_size(back_button, button_width, button_height);
    lv_obj_set_style_bg_color(back_button, lv_color_black(), 0);
    lv_obj_set_style_border_color(back_button, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_border_width(back_button, 2, 0);
    lv_obj_set_style_radius(back_button, 10, 0);
    lv_obj_set_scrollbar_mode(back_button, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(back_button, 5, 0);
    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, "Back");
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_18, 0);
    lv_obj_align(back_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);

    #ifdef CONFIG_USE_TOUCHSCREEN
    lv_obj_set_grid_cell(back_button, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_END, 3, 1);
    #else
    lv_obj_set_grid_cell(back_button, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_END, 2, 1);
    #endif
    lv_obj_set_user_data(back_button, (void *)(uintptr_t)(-1));

    #ifdef CONFIG_USE_TOUCHSCREEN
    // Create Next Button
    next_button = lv_btn_create(apps_container);
    lv_obj_set_size(next_button, button_width, button_height);
    lv_obj_set_style_bg_color(next_button, lv_color_black(), 0);
    lv_obj_set_style_border_color(next_button, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_border_width(next_button, 2, 0);
    lv_obj_set_style_radius(next_button, 10, 0);
    lv_obj_set_scrollbar_mode(next_button, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(next_button, 5, 0);
    lv_obj_t *next_label = lv_label_create(next_button);
    lv_label_set_text(next_label, "Next");
    lv_obj_set_style_text_font(next_label, &lv_font_montserrat_18, 0);
    lv_obj_align(next_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(next_label, lv_color_white(), 0);
    lv_obj_set_grid_cell(next_button, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_END, 3, 1);
    lv_obj_set_user_data(next_button, (void *)(uintptr_t)(-2));

    // Create Previous Button
    prev_button = lv_btn_create(apps_container);
    lv_obj_set_size(prev_button, button_width, button_height);
    lv_obj_set_style_bg_color(prev_button, lv_color_black(), 0);
    lv_obj_set_style_border_color(prev_button, lv_color_make(255, 255, 255), 0);
    lv_obj_set_style_border_width(prev_button, 2, 0);
    lv_obj_set_style_radius(prev_button, 10, 0);
    lv_obj_set_scrollbar_mode(prev_button, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(prev_button, 5, 0);
    lv_obj_t *prev_label = lv_label_create(prev_button);
    lv_label_set_text(prev_label, "Prev");
    lv_obj_set_style_text_font(prev_label, &lv_font_montserrat_18, 0);
    lv_obj_align(prev_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(prev_label, lv_color_white(), 0);
    lv_obj_set_grid_cell(prev_button, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_END, 3, 1);
    lv_obj_set_user_data(prev_button, (void *)(uintptr_t)(-3));
    #endif

    refresh_apps_menu();

    #ifndef CONFIG_USE_TOUCHSCREEN
    select_app_item(current_page * apps_per_page);
    #endif

    display_manager_add_status_bar("Apps");
}

void apps_menu_destroy(void) {
    if (apps_container) {
        lv_obj_clean(apps_container);
        lv_obj_del(apps_container);
        apps_container = NULL;
    }
}

void update_app_item_styles(void) {
    uint32_t child_count = lv_obj_get_child_cnt(apps_container);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *app_item = lv_obj_get_child(apps_container, i);
        int index = (int)(uintptr_t)lv_obj_get_user_data(app_item);
        if (index < 0) continue;
        if (index == selected_app_index) {
            lv_obj_set_style_border_color(app_item, lv_color_make(255, 255, 0), 0);
            lv_obj_set_style_border_width(app_item, 4, 0);
        } else {
            lv_obj_set_style_border_color(app_item, app_items[index].border_color, 0);
            lv_obj_set_style_border_width(app_item, 2, 0);
        }
    }
}

void select_app_item(int index) {
    if (index < -1) index = num_apps - 1;
    if (index > num_apps - 1) index = -1;
    selected_app_index = index;

    int page = (selected_app_index == -1) ? 0 : (selected_app_index / apps_per_page);
    if (page != current_page) {
        current_page = page;
        refresh_apps_menu();
    } else {
        update_app_item_styles();
    }
}

void refresh_apps_menu(void) {
    uint32_t child_count = lv_obj_get_child_cnt(apps_container);
    for (int32_t i = child_count - 1; i >= 0; i--) {
        lv_obj_t *child = lv_obj_get_child(apps_container, i);
        int index = (int)(uintptr_t)lv_obj_get_user_data(child);
        if (index >= 0) {
            lv_obj_del(child);
        }
    }

    int start_index = current_page * apps_per_page;
    int end_index = start_index + apps_per_page;
    if (end_index > num_apps) end_index = num_apps;

    int button_width = LV_HOR_RES / 4;
    int button_height = LV_VER_RES / 4;
    int icon_width = 50;
    int icon_height = 50;
    const lv_font_t *font = (LV_VER_RES <= 128) ? &lv_font_montserrat_10 :
                           (LV_VER_RES <= 240) ? &lv_font_montserrat_16 :
                           &lv_font_montserrat_18;

    for (int i = start_index; i < end_index; i++) {
        int display_index = i - start_index;
        if (display_index >= 3) break;

        lv_obj_t *app_item = lv_btn_create(apps_container);
        lv_obj_set_size(app_item, button_width, button_height);
        lv_obj_set_style_bg_color(app_item, lv_color_black(), 0);
        lv_obj_set_style_border_color(app_item, app_items[i].border_color, 0);
        lv_obj_set_style_border_width(app_item, 2, 0);
        lv_obj_set_style_radius(app_item, 10, 0);
        lv_obj_set_scrollbar_mode(app_item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_pad_all(app_item, 5, 0);

        if (LV_VER_RES >= 240 && app_items[i].icon) {
            lv_obj_t *icon = lv_img_create(app_item);
            lv_img_set_src(icon, app_items[i].icon);
            lv_obj_set_size(icon, icon_width, icon_height);
            lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, -5);
        }

        lv_obj_t *label = lv_label_create(app_item);
        lv_label_set_text(label, app_items[i].name);
        lv_obj_set_style_text_font(label, font, 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);

        int row_idx = display_index / 3;
        int col_idx = display_index % 3;
        lv_obj_set_grid_cell(app_item, LV_GRID_ALIGN_CENTER, col_idx, 1, LV_GRID_ALIGN_CENTER, row_idx, 1);
        lv_obj_set_user_data(app_item, (void *)(intptr_t)i);
    }

    #ifdef CONFIG_USE_TOUCHSCREEN
    if (current_page <= 0) {
        lv_obj_add_flag(prev_button, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(prev_button, LV_OBJ_FLAG_HIDDEN);
    }
    if (current_page >= total_pages - 1) {
        lv_obj_add_flag(next_button, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(next_button, LV_OBJ_FLAG_HIDDEN);
    }
    #endif

    #ifndef CONFIG_USE_TOUCHSCREEN
    if (selected_app_index < start_index || selected_app_index >= end_index) {
        selected_app_index = start_index;
    }
    update_app_item_styles();
    #endif
}

static void handle_apps_button_press(int button) {
    if (button == 0) { // Left
        select_app_item(selected_app_index - 1);
    } else if (button == 3) { // Right
        select_app_item(selected_app_index + 1);
    } else if (button == 1) { // Select
        if (selected_app_index == -1) {
            printf("Back button pressed\n");
            display_manager_switch_view(&main_menu_view);
        } else {
            printf("Launching app via joystick: %s (index %d)\n", app_items[selected_app_index].name, selected_app_index);
            display_manager_switch_view(app_items[selected_app_index].view);
        }
    }
}

void apps_menu_event_handler(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        lv_indev_data_t *data = &event->data.touch_data;
        int touched_app_index = -1;

        printf("Touch detected at X: %d, Y: %d\n", data->point.x, data->point.y);

        uint32_t child_count = lv_obj_get_child_cnt(apps_container);
        for (uint32_t i = 0; i < child_count; i++) {
            lv_obj_t *child = lv_obj_get_child(apps_container, i);
            int index = (int)(uintptr_t)lv_obj_get_user_data(child);

            lv_area_t item_area;
            lv_obj_get_coords(child, &item_area);

            if (data->point.x >= item_area.x1 && data->point.x <= item_area.x2 &&
                data->point.y >= item_area.y1 && data->point.y <= item_area.y2) {
                if (index == -1) { // Back button
                    printf("Back button touched\n");
                    display_manager_switch_view(&main_menu_view);
                    return;
                }
                #ifdef CONFIG_USE_TOUCHSCREEN
                else if (index == -2) { // Next button
                    if (current_page < total_pages - 1) {
                        current_page++;
                        refresh_apps_menu();
                    }
                    return;
                } else if (index == -3) { // Prev button
                    if (current_page > 0) {
                        current_page--;
                        refresh_apps_menu();
                    }
                    return;
                }
                #endif
                else {
                    touched_app_index = index;
                    break;
                }
            }
        }

        if (touched_app_index >= 0) {
            printf("Touch input detected on app item: %s (index %d)\n", app_items[touched_app_index].name, touched_app_index);
            display_manager_switch_view(app_items[touched_app_index].view);
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        handle_apps_button_press(event->data.joystick_index);
    }
}

void get_apps_menu_callback(void **callback) {
    *callback = apps_menu_event_handler;
}

View apps_menu_view = {
    .root = NULL,
    .create = apps_menu_create,
    .destroy = apps_menu_destroy,
    .input_callback = apps_menu_event_handler,
    .name = "Apps Menu",
    .get_hardwareinput_callback = get_apps_menu_callback
};