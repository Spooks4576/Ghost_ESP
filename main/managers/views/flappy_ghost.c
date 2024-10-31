// flappy_bird_game.c

#include "managers/views/flappy_ghost_screen.h"
#include "core/serial_manager.h"
#include "managers/views/main_menu_screen.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "esp_wifi.h"   // For internet connectivity check
#include "esp_http_client.h"  // For HTTP requests
#include "managers/settings_manager.h"
#include "esp_crt_bundle.h"

lv_obj_t *name_text_area = NULL;
lv_obj_t *keyboard = NULL;
bool internet_connected = false;

#ifndef FLAPPY_GHOST_WEB_HOOK
    #define FLAPPY_GHOST_WEB_HOOK ""
#endif

// Define a structure for a single pipe
typedef struct {
    lv_obj_t *pipe;
    int gap_center_y;
} pipe_set_t;

// Constants
#define MAX_PIPE_SETS 2
#define PIPE_SPEED 3
#define PIPE_WIDTH 30
#define PIPE_GAP_RATIO LV_VER_RES <= 128 ? 0.05f : 0.3f


#define DAY_NIGHT_CYCLE_DURATION 10000 // Duration of day-night cycle in milliseconds
#define NIGHT_COLOR_BG lv_color_hex(0x0D0D40)
#define NIGHT_COLOR_GRAD lv_color_hex(0x0A0A30)
#define DAY_COLOR_BG lv_color_hex(0x87CEEB) // Light blue for sky
#define DAY_COLOR_GRAD lv_color_hex(0xFFA500) // Light orange for gradient

// Global Variables
lv_obj_t *flappy_bird_canvas = NULL;
lv_obj_t *bird = NULL;
pipe_set_t pipes[MAX_PIPE_SETS];
lv_obj_t *score_label = NULL;

lv_timer_t *game_loop_timer = NULL;
int bird_y_position = 0;
float bird_velocity = 0;
float gravity = 2;
float flap_strength = -10;
int pipe_gap; // Gap size between pipe and top of the screen
int pipe_min_gap_y;
int pipe_max_gap_y;
int score = 0;
int day_night_cycle_time = 0;
bool is_game_over = false;

// Function Prototypes
void draw_halloween_night_sky(lv_obj_t *parent, lv_color_t bg_color, lv_color_t grad_color);
void flappy_bird_view_create(void);
void flappy_bird_view_destroy(void);
void flappy_bird_view_hardwareinput_callback(InputEvent *event);
void flappy_bird_view_get_hardwareinput_callback(void **callback);
void flappy_bird_game_loop(lv_timer_t *timer);
int flappy_bird_check_collision(lv_obj_t *bird, lv_obj_t *pipe);
void flappy_bird_game_over();
void flappy_bird_restart();

// Drawing the background
void draw_halloween_night_sky(lv_obj_t *parent, lv_color_t bg_color, lv_color_t grad_color) {
    // Set up the gradient for the night sky background
    lv_obj_set_style_bg_color(parent, bg_color, 0);
    lv_obj_set_style_bg_grad_color(parent, grad_color, 0);
    lv_obj_set_style_bg_grad_dir(parent, LV_GRAD_DIR_VER, 0);

    // Create the moon
    lv_obj_t *moon = lv_obj_create(parent);
    lv_obj_set_size(moon, 40, 40);
    lv_obj_set_style_bg_color(moon, lv_color_hex(0xFFFFDD), 0); // Pale yellow
    lv_obj_set_pos(moon, LV_HOR_RES - 60, 30);
    lv_obj_set_scrollbar_mode(moon, LV_SCROLLBAR_MODE_OFF);

    // Create the ground
    lv_obj_t *ground = lv_obj_create(parent);
    lv_obj_set_size(ground, LV_HOR_RES, 40);
    lv_obj_set_style_bg_color(ground, lv_color_hex(0x101010), 0); // Dark gray
    lv_obj_set_pos(ground, 0, LV_VER_RES - 40);
}

bool check_internet_connectivity() {
    wifi_ap_record_t info;
    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        return true;
    }
    return false;
}

void submit_score_to_api(const char *name, int score) {
    esp_log_level_set("esp_http_client", ESP_LOG_NONE);
    char post_data[128];
    snprintf(post_data, sizeof(post_data), "{\"name\": \"%s\", \"score\": %d, \"width\": %d, \"height\": %d}", 
             name, score, LV_HOR_RES, LV_VER_RES);

    const char* WebHookURL = FLAPPY_GHOST_WEB_HOOK;
    
    esp_http_client_config_t config = {
        .url = WebHookURL,
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("HTTP", "Score submitted successfully");
    } else {
        ESP_LOGE("HTTP", "Failed to submit score");
    }

    esp_http_client_cleanup(client);
    esp_log_level_set("esp_http_client", ESP_LOG_INFO);
}

// View Creation
void flappy_bird_view_create(void) {
    if (flappy_bird_view.root != NULL) {
        return;
    }

    // Initialize variables based on the screen dimensions
    pipe_gap = (int)(LV_VER_RES * PIPE_GAP_RATIO); // 30% of screen height
    pipe_min_gap_y = (int)(LV_VER_RES * 0.05);    // Minimum gap position near the top
    pipe_max_gap_y = LV_VER_RES - pipe_gap - 40;  // 40 accounts for ground height
    bird_y_position = LV_VER_RES / LV_VER_RES <= 128 ? 3 : 2;

    // Create root object
    flappy_bird_view.root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(flappy_bird_view.root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(flappy_bird_view.root, lv_color_black(), 0);

    // Draw background
    draw_halloween_night_sky(flappy_bird_view.root, DAY_COLOR_BG, DAY_COLOR_GRAD);

    // Create canvas for pipes
    flappy_bird_canvas = lv_canvas_create(flappy_bird_view.root);
    lv_obj_set_size(flappy_bird_canvas, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_scrollbar_mode(flappy_bird_view.root, LV_SCROLLBAR_MODE_OFF);

    // Create bird
    bird = lv_img_create(flappy_bird_view.root);

    bool use_ghost_image = rand() % 2 == 0;

    if (use_ghost_image) {
        lv_img_set_src(bird, &ghost);
    } else {
        lv_img_set_src(bird, &yappy);
    }

    lv_obj_set_size(bird, 32, 32);
    lv_obj_set_pos(bird, LV_HOR_RES / 4, bird_y_position);

    // Create pipe sets
    for (int i = 0; i < MAX_PIPE_SETS; i++) {
        // Initialize gap center position
        pipes[i].gap_center_y = rand() % (pipe_max_gap_y - pipe_min_gap_y + 1) + pipe_min_gap_y;

        // Create pipe
        pipes[i].pipe = lv_obj_create(flappy_bird_canvas);
        lv_obj_set_size(pipes[i].pipe, PIPE_WIDTH, LV_VER_RES - pipes[i].gap_center_y - 40); // 40 for ground
        lv_obj_set_style_bg_color(pipes[i].pipe, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_radius(pipes[i].pipe, 0, 0); // Ensure sharp edges
        lv_obj_set_pos(pipes[i].pipe, LV_HOR_RES + i * (LV_HOR_RES / MAX_PIPE_SETS), pipes[i].gap_center_y + pipe_gap);
        lv_obj_set_scrollbar_mode(pipes[i].pipe, LV_SCROLLBAR_MODE_OFF);
    }

    // Create score label
    score_label = lv_label_create(flappy_bird_view.root);
    lv_label_set_text_fmt(score_label, "Score: %d", score);
    lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(score_label, lv_color_white(), 0);

    // Add status bar
    display_manager_add_status_bar(LV_VER_RES > 135 ? "Flappy Ghost" : "Flap");

    // Create game loop timer
    game_loop_timer = lv_timer_create(flappy_bird_game_loop, 20, NULL);
}

// View Destruction
void flappy_bird_view_destroy(void) {
    if (flappy_bird_view.root != NULL) {
        is_game_over = false;
        bird_y_position = LV_VER_RES / 2;
        bird_velocity = 0;
        score = 0;

        if (game_loop_timer != NULL) {
            lv_timer_del(game_loop_timer);
            game_loop_timer = NULL;
        }

        lv_obj_del(flappy_bird_view.root);
        flappy_bird_view.root = NULL;
        flappy_bird_canvas = NULL;
        bird = NULL;
        for (int i = 0; i < MAX_PIPE_SETS; i++) {
            pipes[i].pipe = NULL;
        }
        score_label = NULL;
    }
}

// Input Callback
void flappy_bird_view_hardwareinput_callback(InputEvent *event) {
    if (is_game_over) {
        lv_obj_t *game_over_container = lv_obj_get_child(flappy_bird_view.root, -1);
        if (!game_over_container) return;

        lv_obj_t *game_over_label = lv_obj_get_child(game_over_container, -1);

        if (event->type == INPUT_TYPE_TOUCH) {
            int touch_x = event->data.touch_data.point.x;
            int touch_y = event->data.touch_data.point.y;

            lv_area_t area;
            lv_obj_get_coords(game_over_container, &area);

            int padding = 10;

            if (touch_x >= area.x1 - padding && touch_x <= area.x2 + padding &&
                touch_y >= area.y1 - padding && touch_y <= area.y2 + padding) {
                display_manager_switch_view(&main_menu_view);
            } else {
                flappy_bird_restart();
            }
        } else if (event->type == INPUT_TYPE_JOYSTICK) {
            if (event->data.joystick_index == 1) {
                flappy_bird_restart();
            } else if (event->data.joystick_index == 0) {
                display_manager_switch_view(&main_menu_view);
            }
        }
        return;
    }

    if (event->type == INPUT_TYPE_JOYSTICK) {
        int button = event->data.joystick_index;
        if (button == 1) {
            bird_velocity = flap_strength;
        }
    } else if (event->type == INPUT_TYPE_TOUCH) {
        bird_velocity = flap_strength;
    }
}

// Get Input Callback
void flappy_bird_view_get_hardwareinput_callback(void **callback) {
    if (callback != NULL) {
        *callback = (void *)flappy_bird_view_hardwareinput_callback;
    }
}

// Game Loop
void flappy_bird_game_loop(lv_timer_t *timer) {
    if (is_game_over) {
        return;
    }

    // Update bird's physics
    bird_velocity += gravity;
    bird_y_position += (int)bird_velocity;
    lv_obj_set_pos(bird, LV_HOR_RES / 4, bird_y_position);

    // Update bird's angle based on velocity
    int angle = (int)(bird_velocity * 5); 
    if (angle > 45) angle = 45;
    if (angle < -45) angle = -45;
    lv_img_set_angle(bird, angle * 10);

    // Bypass checks for smaller screens
   
    if ((bird_y_position + lv_obj_get_height(bird)) >= (LV_VER_RES - 40) || bird_y_position <= 0) {
        flappy_bird_game_over();
    }

    // Update each pipe set
    for (int i = 0; i < MAX_PIPE_SETS; i++) {
        // Move pipe left
        int pipe_x = lv_obj_get_x(pipes[i].pipe);
        pipe_x -= PIPE_SPEED;

        if (pipe_x < -PIPE_WIDTH) {
            pipe_x = LV_HOR_RES;

            // Update gap center position
            pipes[i].gap_center_y = rand() % (pipe_max_gap_y - pipe_min_gap_y + 1) + pipe_min_gap_y;

            // Resize and reposition pipe
            lv_obj_set_size(pipes[i].pipe, PIPE_WIDTH, LV_VER_RES - pipes[i].gap_center_y - 40); // 40 for ground
            lv_obj_set_pos(pipes[i].pipe, pipe_x, pipes[i].gap_center_y + pipe_gap);

            // Update score
            score++;
            lv_label_set_text_fmt(score_label, "Score: %d", score);
        } else {
            // Update pipe position
            lv_obj_set_x(pipes[i].pipe, pipe_x);
        }

        // Check collision with pipe
        if (flappy_bird_check_collision(bird, pipes[i].pipe)) {
            flappy_bird_game_over();
        }
    }
}

// Collision Detection
int flappy_bird_check_collision(lv_obj_t *bird, lv_obj_t *pipe) {
    int padding = LV_VER_RES <= 128 ? 0 : 2;

    // Get bird's absolute coordinates
    lv_area_t bird_area;
    lv_obj_get_coords(bird, &bird_area);

    // Get pipe's absolute coordinates
    lv_area_t pipe_area;
    lv_obj_get_coords(pipe, &pipe_area);

    // Check for overlap with padding
    bool overlap_x = (bird_area.x2 + padding) >= pipe_area.x1 && (bird_area.x1 - padding) <= pipe_area.x2;
    bool overlap_y = (bird_area.y2 + padding) >= pipe_area.y1 && (bird_area.y1 - padding) <= pipe_area.y2;

    bool collision = overlap_x && overlap_y;

    return collision ? 1 : 0;
}

// Game Over Handling
void flappy_bird_game_over() {
    if (is_game_over) return; // Prevent multiple triggers
    is_game_over = true;

    internet_connected = check_internet_connectivity();

    if (internet_connected)
    {
        submit_score_to_api(settings_get_flappy_ghost_name(&G_Settings), score);
    }

    // Create a semi-transparent overlay
    lv_obj_t *game_over_container = lv_obj_create(flappy_bird_view.root);
    lv_obj_set_size(game_over_container, LV_HOR_RES - 40, 100);
    lv_obj_align(game_over_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(game_over_container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(game_over_container, LV_OPA_70, 0);
    lv_obj_set_style_border_width(game_over_container, 2, 0);
    lv_obj_set_style_border_color(game_over_container, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_scrollbar_mode(game_over_container, LV_SCROLLBAR_MODE_OFF);

    // Create "Game Over" label
    lv_obj_t *game_over_label = lv_label_create(game_over_container);
    lv_label_set_text(game_over_label, "Game Over!");
    lv_obj_set_style_text_color(game_over_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_font(game_over_label, &lv_font_montserrat_24, 0);
    lv_obj_align(game_over_label, LV_ALIGN_CENTER, 0, 0);
}

// Restart Game
void flappy_bird_restart() {
    is_game_over = false;
    bird_y_position = LV_VER_RES / LV_VER_RES <= 128 ? 3 : 2;
    bird_velocity = 0;
    score = 0;
    lv_label_set_text_fmt(score_label, "Score: %d", score);

    // Reset bird position
    lv_obj_set_pos(bird, LV_HOR_RES / 4, bird_y_position);

    // Reset pipe positions and sizes
    for (int i = 0; i < MAX_PIPE_SETS; i++) {
        // Reposition pipe off-screen
        lv_obj_set_pos(pipes[i].pipe, LV_HOR_RES + i * (LV_HOR_RES / MAX_PIPE_SETS), pipes[i].gap_center_y + pipe_gap);

        // Reset pipe size based on new gap center
        pipes[i].gap_center_y = rand() % (pipe_max_gap_y - pipe_min_gap_y + 1) - 40;
        lv_obj_set_size(pipes[i].pipe, PIPE_WIDTH, LV_VER_RES - pipes[i].gap_center_y - 40); // 40 for ground
    }

    // Remove the game over overlay if it exists
    lv_obj_t *game_over_container = lv_obj_get_child(flappy_bird_view.root, -1);
    if (game_over_container) {
        lv_obj_del(game_over_container);
    }
}

// View Structure Initialization
View flappy_bird_view = {
    .root = NULL,
    .create = flappy_bird_view_create,
    .destroy = flappy_bird_view_destroy,
    .input_callback = flappy_bird_view_hardwareinput_callback,
    .name = "FlappyBirdView",
    .get_hardwareinput_callback = flappy_bird_view_get_hardwareinput_callback
};