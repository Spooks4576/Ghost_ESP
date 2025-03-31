#include "managers/views/main_menu_screen.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lvgl.h"
#include "managers/views/app_gallery_screen.h"
#include <stdio.h>

static const char *TAG = "MainMenu";

lv_obj_t *menu_container;
static int selected_item_index = 0;

typedef struct {
  const char *name;
  const lv_img_dsc_t *icon;
  lv_color_t border_color;
} menu_item_t;

// Define colors as compile-time constants
static menu_item_t menu_items[] = {
    {"BLE", &bluetooth},
    {"WiFi", &wifi},
    {"GPS", &Map},
    {"Apps", &GESPAppGallery}
};

static int num_items = sizeof(menu_items) / sizeof(menu_items[0]);
lv_obj_t *current_item_obj = NULL;

static void init_menu_colors(void) {
    // Initialize colors at runtime
    menu_items[0].border_color = lv_color_hex(0x1976D2);
    menu_items[1].border_color = lv_color_hex(0xD32F2F);
    menu_items[2].border_color = lv_color_hex(0x388E3C);
    menu_items[3].border_color = lv_color_hex(0x7B1FA2);
}

// Animation callback wrapper
static void anim_set_x(void *obj, int32_t v) {
    lv_obj_set_x((lv_obj_t *)obj, (lv_coord_t)v);
}

/**
 * @brief Updates the displayed menu item with animation.
 */
static void update_menu_item(bool slide_left) {
    if (current_item_obj) {
        lv_obj_del(current_item_obj);
    }

    current_item_obj = lv_btn_create(menu_container);
    lv_obj_set_style_bg_color(current_item_obj, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(current_item_obj, 3, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(current_item_obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(current_item_obj, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(current_item_obj, menu_items[selected_item_index].border_color, LV_PART_MAIN);
    lv_obj_set_style_radius(current_item_obj, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(current_item_obj, 0, LV_PART_MAIN); // Remove padding
    lv_obj_set_style_clip_corner(current_item_obj, false, 0); // Prevent clipping by button

    int btn_size = LV_MIN(LV_HOR_RES, LV_VER_RES) * 0.6;
    if (LV_HOR_RES <= 128 && LV_VER_RES <= 128) {
        btn_size = 80; // Changed from 50 to match app menu minimum size
    }
    lv_obj_set_size(current_item_obj, btn_size, btn_size);
    lv_obj_align(current_item_obj, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *icon = lv_img_create(current_item_obj);
    lv_img_set_src(icon, menu_items[selected_item_index].icon);

    const int icon_size = 50; // Fixed size to match app menu
    lv_obj_set_size(icon, icon_size, icon_size);
    lv_img_set_size_mode(icon, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_antialias(icon, false); // Prevent scaling artifacts
    lv_obj_set_style_img_recolor(icon, menu_items[selected_item_index].border_color, 0); // Keep recoloring for main menu
    lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
    lv_obj_set_style_clip_corner(icon, false, 0); // Prevent clipping

    // Calculate centered position with offsets
    int icon_x_offset = -3;  // Match app menu
    int icon_y_offset = -5;  // Match app menu
    int x_pos = (btn_size - icon_size) / 2 + icon_x_offset;
    int y_pos = (btn_size - icon_size) / 2 + icon_y_offset;
    lv_obj_set_pos(icon, x_pos, y_pos);

    // Debug output
    lv_coord_t img_width = menu_items[selected_item_index].icon->header.w;
    lv_coord_t img_height = menu_items[selected_item_index].icon->header.h;
    printf("Button size: %d x %d, Set Icon size: %d x %d, Original: %d x %d, Pos: %d, %d\n",
           btn_size, btn_size, icon_size, icon_size, img_width, img_height, x_pos, y_pos);

    if (LV_HOR_RES > 150) {
        lv_obj_t *label = lv_label_create(current_item_obj);
        lv_label_set_text(label, menu_items[selected_item_index].name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -5);
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, current_item_obj);
    lv_anim_set_time(&a, 75); // Match app menu timing
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    int start_x = slide_left ? LV_HOR_RES : -LV_HOR_RES;
    lv_anim_set_values(&a, start_x, 0);
    lv_anim_set_exec_cb(&a, anim_set_x);
    lv_anim_start(&a);
}

/**
 * @brief Combined handler for menu item events.
 */
 static void menu_item_event_handler(InputEvent *event) {
  if (event->type == INPUT_TYPE_TOUCH) {
      lv_indev_data_t *data = &event->data.touch_data;
      int half_screen = LV_HOR_RES / 2;
      int center_area_width = LV_HOR_RES / 3;

      if (data->point.x > (LV_HOR_RES - center_area_width)/2 && 
          data->point.x < (LV_HOR_RES + center_area_width)/2) {
          handle_menu_item_selection(selected_item_index);
      }
      else if (data->point.x < half_screen) {
          select_menu_item(selected_item_index - 1, true);
      }
      else {
          select_menu_item(selected_item_index + 1, false);
      }
  } 
  else if (event->type == INPUT_TYPE_JOYSTICK) {
      int button = event->data.joystick_index;
      handle_hardware_button_press(button);
  }
}

/**
 * @brief Handles hardware button presses for menu navigation.
 */
void handle_hardware_button_press(int ButtonPressed) {
    if (ButtonPressed == 0) {
        select_menu_item(selected_item_index - 1, true);
    } else if (ButtonPressed == 3) {
        select_menu_item(selected_item_index + 1, false);
    } else if (ButtonPressed == 1) {
        handle_menu_item_selection(selected_item_index);
    }
}

/**
 * @brief Selects a menu item and updates the display.
 */
static void select_menu_item(int index, bool slide_left) {
    if (index < 0) index = num_items - 1;
    if (index >= num_items) index = 0;
    selected_item_index = index;
    update_menu_item(slide_left);
}

/**
 * @brief Handles the selection of menu items.
 */
static void handle_menu_item_selection(int item_index) {
    switch (item_index) {
        case 0: printf("BLE selected\n"); SelectedMenuType = OT_Bluetooth; display_manager_switch_view(&options_menu_view); break;
        case 1: printf("Wi-Fi selected\n"); SelectedMenuType = OT_Wifi; display_manager_switch_view(&options_menu_view); break;
        case 2: printf("GPS selected\n"); SelectedMenuType = OT_GPS; display_manager_switch_view(&options_menu_view); break;
        case 3: printf("Apps View Selected\n"); display_manager_switch_view(&apps_menu_view); break;
        default: printf("Unknown menu item selected\n"); break;
    }
}

/**
 * @brief Creates the main menu screen view.
 */
void main_menu_create(void) {
    display_manager_fill_screen(lv_color_hex(0x121212));
    init_menu_colors(); // Initialize colors at runtime

    menu_container = lv_obj_create(lv_scr_act());
    main_menu_view.root = menu_container;
    lv_obj_set_size(menu_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(menu_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(menu_container, 0, 0);
    lv_obj_set_scrollbar_mode(menu_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(menu_container, LV_ALIGN_CENTER, 0, 0);

    selected_item_index = 0;
    update_menu_item(false);

    display_manager_add_status_bar(LV_HOR_RES > 128 ? "Main Menu" : "");
}

/**
 * @brief Destroys the main menu screen view.
 */
void main_menu_destroy(void) {
    if (menu_container) {
        lv_obj_clean(menu_container);
        lv_obj_del(menu_container);
        menu_container = NULL;
        main_menu_view.root = NULL;
        current_item_obj = NULL;
    }
}

void get_main_menu_callback(void **callback) {
    *callback = main_menu_view.input_callback;
}

View main_menu_view = {
    .root = NULL,
    .create = main_menu_create,
    .destroy = main_menu_destroy,
    .input_callback = menu_item_event_handler,
    .name = "Main Menu",
    .get_hardwareinput_callback = get_main_menu_callback, // Corrected typo
};