#include "managers/views/main_menu_screen.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "managers/settings_manager.h"
#include "managers/views/app_gallery_screen.h"
#include "managers/views/flappy_ghost_screen.h"
#include "managers/views/music_visualizer.h"
#include <time.h>
#ifdef CONFIG_HAS_RTC_CLOCK
#include "esp_sntp.h"
#include "vendor/drivers/pcf8563.h"
#endif
#include <stdio.h>

static const char *TAG = "MainMenu";

lv_obj_t *menu_container;
static int selected_item_index = 0;

typedef struct {
  const char *name;
  const lv_img_dsc_t *icon;
  lv_color_t border_color;
} menu_item_t;

static menu_item_t menu_items[] = {{"BLE", &bluetooth},
                                   {"WiFi", &wifi},
                                   {"GPS", &Map},
                                   {"Apps", &GESPAppGallery}};

static int num_items = sizeof(menu_items) / sizeof(menu_items[0]);

lv_obj_t *time_label;
lv_timer_t *time_update_timer;
#ifdef CONFIG_HAS_RTC_CLOCK
RTC_Date current_time;
#endif

#ifdef CONFIG_HAS_RTC_CLOCK

bool wifi_is_connected() {
  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    return true;
  }
  return false;
}

void update_time_label(lv_timer_t *timer) {
  extern DisplayManager dm;

  if (xSemaphoreTake(dm.mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE) {
    ESP_LOGI(TAG, "Updating time with timezone: %s",
             G_Settings.selected_timezone);
    setenv("TZ", G_Settings.selected_timezone, 1);
    tzset();
    xSemaphoreGive(dm.mutex);

    static bool time_synced = false;

    if (wifi_is_connected() && !time_synced) {
      ESP_LOGI(TAG, "Internet connection detected, syncing time...");

      time_t now;
      struct tm timeinfo;
      time(&now);
      localtime_r(&now, &timeinfo);

      if (timeinfo.tm_year >= (2020 - 1900)) {
        printf("SNTP time synchronized: %s", asctime(&timeinfo));

        // Sync RTC with SNTP time
        RTC_Date rtc_time = {
            .year = timeinfo.tm_year + 1900,
            .month = timeinfo.tm_mon + 1,
            .day = timeinfo.tm_mday,
            .hour = timeinfo.tm_hour,
            .minute = timeinfo.tm_min,
            .second = timeinfo.tm_sec,
        };

        if (pcf8563_set_datetime(&rtc_time) == ESP_OK) {
          printf("RTC successfully synchronized with SNTP\n");
          time_synced = true; // Mark synchronization as completed
        } else {
          printf("Failed to sync RTC with SNTP\n");
        }
      } else {
        printf("Failed to synchronize time using SNTP\n");
      }
    }

    if (pcf8563_get_datetime(&current_time) == ESP_OK) {
      char time_str[16];
      int hour = current_time.hour;
      const char *period = "AM";

      if (hour >= 12) {
        period = "PM";
        if (hour > 12) {
          hour -= 12;
        }
      } else if (hour == 0) {
        hour = 12;
      }

      snprintf(time_str, sizeof(time_str), "%02d:%02d %s", hour,
               current_time.minute, period);

      lv_label_set_text(time_label, time_str);
      lv_obj_set_pos(time_label, 70, 33);
    }
  } else {
    ESP_LOGW(TAG, "Failed to take mutex for time update");
  }
}
#endif

/**
 * @brief Combined handler for menu item events using either touchscreen or
 * joystick input.
 * @param event The input event data (either touch data or button press index).
 */
static void menu_item_event_handler(InputEvent *event) {
  if (event->type == INPUT_TYPE_TOUCH) {
    lv_indev_data_t *data = &event->data.touch_data;
    int touched_item_index = -1;

    printf("Touch detected at X: %d, Y: %d\n", data->point.x, data->point.y);

    for (int i = 0; i < num_items; i++) {
      lv_obj_t *menu_item = (lv_obj_t *)lv_obj_get_child(menu_container, i);
      lv_area_t item_area;
      lv_obj_get_coords(menu_item, &item_area);

      printf("Menu item %d area: x1=%d, y1=%d, x2=%d, y2=%d\n", i, item_area.x1,
             item_area.y1, item_area.x2, item_area.y2);

      if (data->point.x >= item_area.x1 && data->point.x <= item_area.x2 &&
          data->point.y >= item_area.y1 && data->point.y <= item_area.y2) {
        touched_item_index = i;
        break;
      }
    }

    if (touched_item_index >= 0) {
      printf("Touch input detected on menu item: %d\n", touched_item_index);
      handle_menu_item_selection(touched_item_index);
    } else {
      printf("Touch input detected but no menu item found at touch "
             "coordinates. X: %d, Y: %d\n",
             data->point.x, data->point.y);
    }

  } else if (event->type == INPUT_TYPE_JOYSTICK) {
    int button = event->data.joystick_index;
    handle_hardware_button_press(button);
  }
}

/**
 * @brief Handles the selection of menu items.
 * @param item_index The index of the selected menu item.
 */
 static void handle_menu_item_selection(int item_index) {
  switch (item_index) {
  case 1:
      printf("Wi-Fi selected\n");
      SelectedMenuType = OT_Wifi;
      display_manager_switch_view(&options_menu_view);
      break;
  case 0:
      printf("BLE selected\n");
      SelectedMenuType = OT_Bluetooth;
      display_manager_switch_view(&options_menu_view);
      break;
  case 2:
      printf("GPS selected\n");
      SelectedMenuType = OT_GPS;
      display_manager_switch_view(&options_menu_view);
      break;
  case 3:
      printf("Apps View Selected\n");
      display_manager_switch_view(&apps_menu_view);
      break;
  default:
      printf("Unknown menu item selected\n");
      break;
  }
}

/**
 * @brief Handles hardware button presses for menu navigation.
 * @param ButtonPressed The index of the pressed button.
 */
void handle_hardware_button_press(int ButtonPressed) {
  if (ButtonPressed == 0) {
    select_menu_item(selected_item_index - 1);
  } else if (ButtonPressed == 3) {
    select_menu_item(selected_item_index + 1);
  } else if (ButtonPressed == 1) {
    handle_menu_item_selection(selected_item_index);
  }
}

/**
 * @brief Updates the style of the selected and unselected menu items.
 */
 static void update_menu_item_styles(void) {
  for (int i = 0; i < num_items; i++) {
      lv_obj_t *menu_item = (lv_obj_t *)lv_obj_get_child(menu_container, i);
      if (i == selected_item_index) {
          lv_obj_set_style_bg_color(menu_item, lv_color_hex(0x333333), 0); // Slightly lighter for selection
      } else {
          lv_obj_set_style_bg_color(menu_item, lv_color_hex(0x1E1E1E), 0); // Default dark gray
      }
  }
}

/**
 * @brief Handles the selection of menu items with hardware buttons.
 * @param index The index of the menu item to select.
 */
static void select_menu_item(int index) {
  if (index < 0)
    index = num_items - 1;
  if (index >= num_items)
    index = 0;

  selected_item_index = index;
  update_menu_item_styles();
}

/**
 * @brief Creates the main menu screen view.
 */
 void main_menu_create(void) {

  display_manager_fill_screen(lv_color_hex(0x121212)); 

  menu_container = lv_obj_create(lv_scr_act());
  main_menu_view.root = menu_container;
  lv_obj_set_size(menu_container, LV_HOR_RES, LV_VER_RES - 20); 
  lv_obj_set_style_bg_opa(menu_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(menu_container, 0, 0);
  lv_obj_set_scrollbar_mode(menu_container, LV_SCROLLBAR_MODE_OFF);
  lv_obj_align(menu_container, LV_ALIGN_TOP_MID, 0, 0);

  printf("Screen dimensions: width=%d, height=%d\n", LV_HOR_RES, LV_VER_RES);

  int button_width = LV_HOR_RES / 5; 
  int button_height = 100; 
  int icon_width = 50;
  int icon_height = 50;

  int num_rows = 3;
  int total_button_height = button_height * num_rows; 
  int available_height = LV_VER_RES - 20; 
  int row_padding = (available_height - total_button_height) / (num_rows + 1); 
  if (row_padding < 10) { 
      row_padding = 10;
      button_height = (available_height - (row_padding * (num_rows + 1))) / num_rows;
  }

  int num_columns = 3;
  int total_button_width = button_width * num_columns; 
  int column_padding = (LV_HOR_RES - total_button_width) / (num_columns + 1); 

  printf("Button size: width=%d, height=%d\n", button_width, button_height);
  printf("Padding: column=%d, row=%d\n", column_padding, row_padding);
  printf("Total grid size: width=%d, height=%d\n", total_button_width, total_button_height);

  int grid_width = total_button_width + column_padding * (num_columns - 1); 
  int grid_height = total_button_height + row_padding * (num_rows - 1); 
  int x_offset = (LV_HOR_RES - grid_width) / 2 - 30; 
  int y_offset = (LV_VER_RES - 20 - grid_height) / 2 - 40; 

  if (x_offset < 0) x_offset = 0;
  if (y_offset < 0) y_offset = 0;

  int x_positions[3];
  int y_positions[3];
  for (int i = 0; i < num_columns; i++) {
      x_positions[i] = x_offset + i * (button_width + column_padding);
  }
  for (int i = 0; i < num_rows; i++) {
      y_positions[i] = y_offset + i * (button_height + row_padding);
  }

  printf("X positions: [%d, %d, %d]\n", x_positions[0], x_positions[1], x_positions[2]);
  printf("Y positions: [%d, %d, %d]\n", y_positions[0], y_positions[1], y_positions[2]);

  lv_color_t icon_colors[4] = {
      lv_color_hex(0x1976D2), 
      lv_color_hex(0xD32F2F), 
      lv_color_hex(0x388E3C), 
      lv_color_hex(0x7B1FA2)  
  };

  for (int i = 0; i < num_items; i++) {
      lv_obj_t *menu_item = lv_btn_create(menu_container);
      lv_obj_set_size(menu_item, button_width, button_height);
      lv_obj_set_style_bg_color(menu_item, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
      lv_obj_set_style_shadow_width(menu_item, 3, LV_PART_MAIN);
      lv_obj_set_style_shadow_color(menu_item, lv_color_hex(0x000000), LV_PART_MAIN);
      lv_obj_set_style_border_width(menu_item, 0, LV_PART_MAIN);
      lv_obj_set_style_radius(menu_item, 10, LV_PART_MAIN);
      lv_obj_set_scrollbar_mode(menu_item, LV_SCROLLBAR_MODE_OFF);

      lv_obj_set_layout(menu_item, LV_LAYOUT_FLEX);
      lv_obj_set_flex_flow(menu_item, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(menu_item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_set_style_pad_all(menu_item, 3, 0);
      lv_obj_set_style_pad_row(menu_item, 3, 0);

      if (menu_items[i].icon) {
          lv_obj_t *icon = lv_img_create(menu_item);
          lv_img_set_src(icon, menu_items[i].icon);
          lv_obj_set_size(icon, icon_width, icon_height);
          lv_obj_set_style_img_recolor(icon, icon_colors[i], 0);
          lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
      }

      lv_obj_t *label = lv_label_create(menu_item);
      lv_label_set_text(label, menu_items[i].name);
      lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
      lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

      int row_idx = 2 - (i / 3); 
      int col_idx = i % 3;
      lv_obj_set_pos(menu_item, x_positions[col_idx], y_positions[row_idx]);

      printf("Button %d (%s): x=%d, y=%d\n", i, menu_items[i].name, x_positions[col_idx], y_positions[row_idx]);
  }

  display_manager_add_status_bar(LV_VER_RES > 320 ? "Main Menu" : "Menu");
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
  }

  if (time_update_timer) {
      lv_timer_del(time_update_timer);
      time_update_timer = NULL;
  }

  if (time_label) {
      lv_obj_del(time_label);
      time_label = NULL;
  }
}

void get_main_menu_callback(void **callback) {
  *callback = main_menu_view.input_callback;
}

/**
 * @brief Main menu screen view object.
 */
View main_menu_view = {
    .root = NULL,
    .create = main_menu_create,
    .destroy = main_menu_destroy,
    .input_callback = menu_item_event_handler,
    .name = "Main Menu",
    .get_hardwareinput_callback = get_main_menu_callback,
};