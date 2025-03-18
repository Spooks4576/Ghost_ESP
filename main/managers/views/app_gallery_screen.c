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
    display_manager_fill_screen(lv_color_hex(0x121212)); // Dark charcoal gray background

    total_pages = (num_apps + apps_per_page - 1) / apps_per_page;
    printf("Screen: %dx%d, Total apps: %d, Pages: %d\n", LV_HOR_RES, LV_VER_RES, num_apps, total_pages);

    apps_container = lv_obj_create(lv_scr_act());
    if (!apps_container) {
        printf("Failed to create apps_container\n");
        return;
    }
    apps_menu_view.root = apps_container;
    lv_obj_set_size(apps_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(apps_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(apps_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(apps_container, 0, 0); // No border on container
    lv_obj_align(apps_container, LV_ALIGN_TOP_MID, 0, 0);

    // Dynamic sizing like main menu
    int button_width = LV_HOR_RES / 5; // Match main menu’s 5-column division
    int button_height = (LV_VER_RES - 20) / 3.2; // Match main menu’s height scaling
    int icon_size = LV_VER_RES > 240 ? 50 : 30;
    apps_per_page = 6; // 3x2 grid per page

    // Grid container for apps
    lv_obj_t *grid_container = lv_obj_create(apps_container);
    lv_obj_set_size(grid_container, LV_HOR_RES, LV_VER_RES - 80); // Space for nav buttons
    lv_obj_set_layout(grid_container, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid_container, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid_container, 20, 0); // Match main menu padding
    lv_obj_set_style_pad_row(grid_container, 20, 0);
    lv_obj_set_style_pad_column(grid_container, 20, 0);
    lv_obj_set_style_bg_opa(grid_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_container, 0, 0); // No border on grid
    lv_obj_set_scrollbar_mode(grid_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(grid_container, LV_ALIGN_TOP_MID, 0, 20);

    // Populate apps
    int start_index = current_page * apps_per_page;
    int end_index = start_index + apps_per_page;
    if (end_index > num_apps) end_index = num_apps;

    for (int i = start_index; i < end_index; i++) {
        int display_index = i - start_index;
        lv_obj_t *app_item = lv_btn_create(grid_container);
        lv_obj_set_size(app_item, button_width, button_height);
        lv_obj_set_style_bg_color(app_item, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
        lv_obj_set_style_shadow_width(app_item, 3, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(app_item, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_set_style_border_width(app_item, 0, LV_PART_MAIN); // No white border
        lv_obj_set_style_radius(app_item, 10, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(app_item, LV_SCROLLBAR_MODE_OFF);

        lv_obj_set_layout(app_item, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(app_item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(app_item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(app_item, 5, 0);
        lv_obj_set_style_pad_row(app_item, 5, 0);

        if (app_items[i].icon) {
            lv_obj_t *icon = lv_img_create(app_item);
            lv_img_set_src(icon, app_items[i].icon);
            lv_obj_set_size(icon, icon_size, icon_size);
            // No tinting applied, keeping original icon colors
        }

        lv_obj_t *label = lv_label_create(app_item);
        lv_label_set_text(label, app_items[i].name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

        int row_idx = display_index / 3;
        int col_idx = display_index % 3;
        lv_obj_set_grid_cell(app_item, LV_GRID_ALIGN_CENTER, col_idx, 1, LV_GRID_ALIGN_CENTER, row_idx, 1);
        lv_obj_set_user_data(app_item, (void *)(intptr_t)i);
    }

    // Back button (styled like main menu navigation)
    back_button = lv_btn_create(apps_container);
    lv_obj_set_size(back_button, 60, 60);
    lv_obj_align(back_button, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_set_style_bg_color(back_button, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_radius(back_button, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(back_button, 0, LV_PART_MAIN); // No border
    lv_obj_t *back_label = lv_label_create(back_button);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_user_data(back_button, (void *)(intptr_t)(-1));

    #ifdef CONFIG_USE_TOUCHSCREEN
    if (total_pages > 1) {
        prev_button = lv_btn_create(apps_container);
        lv_obj_set_size(prev_button, 60, 60);
        lv_obj_align(prev_button, LV_ALIGN_BOTTOM_RIGHT, -80, -10);
        lv_obj_set_style_bg_color(prev_button, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_style_radius(prev_button, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(prev_button, 0, LV_PART_MAIN); // No border
        lv_obj_t *prev_label = lv_label_create(prev_button);
        lv_label_set_text(prev_label, LV_SYMBOL_PREV);
        lv_obj_center(prev_label);
        lv_obj_set_style_text_color(prev_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_user_data(prev_button, (void *)(intptr_t)(-3));
        if (current_page <= 0) lv_obj_add_flag(prev_button, LV_OBJ_FLAG_HIDDEN);

        next_button = lv_btn_create(apps_container);
        lv_obj_set_size(next_button, 60, 60);
        lv_obj_align(next_button, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
        lv_obj_set_style_bg_color(next_button, lv_color_hex(0x333333), LV_PART_MAIN);
        lv_obj_set_style_radius(next_button, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(next_button, 0, LV_PART_MAIN); // No border
        lv_obj_t *next_label = lv_label_create(next_button);
        lv_label_set_text(next_label, LV_SYMBOL_NEXT);
        lv_obj_center(next_label);
        lv_obj_set_style_text_color(next_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_user_data(next_button, (void *)(intptr_t)(-2));
        if (current_page >= total_pages - 1) lv_obj_add_flag(next_button, LV_OBJ_FLAG_HIDDEN);
    }
    #endif

    select_app_item(current_page * apps_per_page);
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
    lv_obj_t *grid_container = lv_obj_get_child(apps_container, 0);
    uint32_t child_count = lv_obj_get_child_cnt(grid_container);

    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *app_item = lv_obj_get_child(grid_container, i);
        int index = (int)(intptr_t)lv_obj_get_user_data(app_item);

        if (index == selected_app_index) {
            lv_obj_set_style_bg_color(app_item, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(app_item, LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(app_item, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
            lv_obj_set_style_bg_grad_dir(app_item, LV_GRAD_DIR_NONE, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(app_item, LV_OPA_COVER, LV_PART_MAIN);
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
        int index = (int)(intptr_t)lv_obj_get_user_data(child);
        if (index >= 0) lv_obj_del(child); // Delete app buttons only
    }

    int start_index = current_page * apps_per_page;
    int end_index = start_index + apps_per_page;
    if (end_index > num_apps) end_index = num_apps;

    int button_width = LV_HOR_RES / 5;
    int button_height = (LV_VER_RES - 20) / 3.2;
    int icon_size = LV_VER_RES > 240 ? 50 : 30;

    lv_obj_t *grid_container = lv_obj_get_child(apps_container, 0);
    for (int i = start_index; i < end_index; i++) {
        int display_index = i - start_index;
        lv_obj_t *app_item = lv_btn_create(grid_container);
        lv_obj_set_size(app_item, button_width, button_height);
        lv_obj_set_style_bg_color(app_item, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
        lv_obj_set_style_shadow_width(app_item, 3, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(app_item, lv_color_hex(0x000000), LV_PART_MAIN);
        lv_obj_set_style_border_width(app_item, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(app_item, 10, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(app_item, LV_SCROLLBAR_MODE_OFF);

        lv_obj_set_layout(app_item, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(app_item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(app_item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(app_item, 5, 0);
        lv_obj_set_style_pad_row(app_item, 5, 0);

        if (app_items[i].icon) {
            lv_obj_t *icon = lv_img_create(app_item);
            lv_img_set_src(icon, app_items[i].icon);
            lv_obj_set_size(icon, icon_size, icon_size);
        }

        lv_obj_t *label = lv_label_create(app_item);
        lv_label_set_text(label, app_items[i].name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

        int row_idx = display_index / 3;
        int col_idx = display_index % 3;
        lv_obj_set_grid_cell(app_item, LV_GRID_ALIGN_CENTER, col_idx, 1, LV_GRID_ALIGN_CENTER, row_idx, 1);
        lv_obj_set_user_data(app_item, (void *)(intptr_t)i);
    }

    #ifdef CONFIG_USE_TOUCHSCREEN
    if (current_page <= 0) lv_obj_add_flag(prev_button, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(prev_button, LV_OBJ_FLAG_HIDDEN);
    if (current_page >= total_pages - 1) lv_obj_add_flag(next_button, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(next_button, LV_OBJ_FLAG_HIDDEN);
    #endif

    if (selected_app_index < start_index || selected_app_index >= end_index) {
        selected_app_index = start_index;
    }
    update_app_item_styles();
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
        printf("Touch detected at X: %d, Y: %d\n", data->point.x, data->point.y);

        // Get the grid container (first child of apps_container)
        lv_obj_t *grid_container = lv_obj_get_child(apps_container, 0);
        uint32_t grid_child_count = lv_obj_get_child_cnt(grid_container);
        bool action_taken = false;

        // Check app items first (inside grid_container)
        for (int32_t i = 0; i < grid_child_count; i++) {
            lv_obj_t *app_item = lv_obj_get_child(grid_container, i);
            int index = (int)(intptr_t)lv_obj_get_user_data(app_item);
            lv_area_t item_area;
            lv_obj_get_coords(app_item, &item_area);

            if (data->point.x >= item_area.x1 && data->point.x <= item_area.x2 &&
                data->point.y >= item_area.y1 && data->point.y <= item_area.y2) {
                printf("Touch input detected on app item: %s (index %d)\n", app_items[index].name, index);
                select_app_item(index);
                display_manager_switch_view(app_items[index].view);
                action_taken = true;
                break;
            }
        }

        // If no app item was touched, check navigation buttons
        if (!action_taken) {
            uint32_t container_child_count = lv_obj_get_child_cnt(apps_container);
            for (int32_t i = 0; i < container_child_count; i++) {
                lv_obj_t *child = lv_obj_get_child(apps_container, i);
                int index = (int)(intptr_t)lv_obj_get_user_data(child);
                if (index >= 0) continue; // Skip app items (handled above)

                lv_area_t item_area;
                lv_obj_get_coords(child, &item_area);

                if (data->point.x >= item_area.x1 && data->point.x <= item_area.x2 &&
                    data->point.y >= item_area.y1 && data->point.y <= item_area.y2) {
                    if (index == -1) { // Back button
                        printf("Back button touched\n");
                        display_manager_switch_view(&main_menu_view);
                        action_taken = true;
                    }
                    #ifdef CONFIG_USE_TOUCHSCREEN
                    else if (index == -2 && current_page < total_pages - 1) { // Next button
                        printf("Next button touched\n");
                        current_page++;
                        refresh_apps_menu();
                        action_taken = true;
                    }
                    else if (index == -3 && current_page > 0) { // Prev button
                        printf("Prev button touched\n");
                        current_page--;
                        refresh_apps_menu();
                        action_taken = true;
                    }
                    #endif
                    break;
                }
            }
        }

        if (!action_taken) {
            printf("No valid touch target found\n");
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