// flappy_bird_game.c

#include "managers/views/flappy_ghost_screen.h"
#include "core/serial_manager.h"
#include "managers/views/main_menu_screen.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "esp_wifi.h"            // For internet connectivity check
#include "esp_http_client.h"     // For HTTP requests
#include "managers/settings_manager.h"
#include "esp_crt_bundle.h"


#define MAX_PIPE_SETS 2


#define GAME_LOOP_INTERVAL_MS LV_VER_RES > 320 ? 10 : 25


typedef enum {
    SCREEN_SIZE_SMALL,
    SCREEN_SIZE_MEDIUM,
    SCREEN_SIZE_LARGE
} screen_size_t;


typedef struct {
    int pipe_speed;          // Speed at which pipes move left
    int pipe_width;          // Width of each pipe
    float gravity;           // Gravity affecting the bird
    float flap_strength;     // Upward velocity when the bird flaps
    float pipe_gap_ratio;    // Ratio of the screen height used as pipe gap
    int bird_size;           // Size of the bird (width and height)
    lv_font_t *score_font;   // Font size for the score label
    int ground_height;      // Height of the ground
    int buffer_top;         // Top buffer zone height
    int buffer_bottom;      // Bottom buffer zone height
} game_settings_t;

// Global Game Settings
game_settings_t settings;

// Function Prototypes
screen_size_t get_screen_size(int height);
void set_game_settings(int height);
void draw_halloween_night_sky(lv_obj_t *parent, lv_color_t bg_color, lv_color_t grad_color);
void flappy_bird_view_create(void);
void flappy_bird_view_destroy(void);
void flappy_bird_view_hardwareinput_callback(InputEvent *event);
void flappy_bird_view_get_hardwareinput_callback(void **callback);
void flappy_bird_game_loop(lv_timer_t *timer);
int flappy_bird_check_collision(lv_obj_t *bird, lv_obj_t *pipe);
void flappy_bird_game_over();
void flappy_bird_restart();
bool check_internet_connectivity();
void submit_score_to_api(const char *name, int score);

// Global Variables
lv_obj_t *name_text_area = NULL;
lv_obj_t *keyboard = NULL;
bool internet_connected = false;

// Define a structure for a single pipe
typedef struct {
    lv_obj_t *pipe;
    int gap_center_y;
} pipe_set_t;

// Global Game Objects
lv_obj_t *flappy_bird_canvas = NULL;
lv_obj_t *bird = NULL;
pipe_set_t pipes[MAX_PIPE_SETS];
lv_obj_t *score_label = NULL;

lv_timer_t *game_loop_timer = NULL;
int bird_y_position = 0;
float bird_velocity = 0;
int pipe_gap; // Gap size between pipe and top of the screen
int pipe_min_gap_y;
int pipe_max_gap_y;
int score = 0;
bool is_game_over = false;

// Define Web Hook URL if not defined
#ifndef FLAPPY_GHOST_WEB_HOOK
    #define FLAPPY_GHOST_WEB_HOOK ""
#endif

// Initialize the Flappy Bird View
View flappy_bird_view = {
    .root = NULL,
    .create = flappy_bird_view_create,
    .destroy = flappy_bird_view_destroy,
    .input_callback = flappy_bird_view_hardwareinput_callback,
    .name = "FlappyBirdView",
    .get_hardwareinput_callback = flappy_bird_view_get_hardwareinput_callback
};

// Function to determine screen size category
screen_size_t get_screen_size(int height) {
    if (height <= 135) {
        return SCREEN_SIZE_SMALL;
    } else if (height <= 320) {
        return SCREEN_SIZE_MEDIUM;
    } else {
        return SCREEN_SIZE_LARGE;
    }
}

// Function to set game settings based on screen size
void set_game_settings(int height) {
    screen_size_t size = get_screen_size(height);
    switch (size) {
        case SCREEN_SIZE_SMALL:
            settings.pipe_speed = 2;
            settings.pipe_width = 20;
            settings.gravity = 1.5f;
            settings.flap_strength = -8.0f;
            settings.pipe_gap_ratio = 0.2f; // 20% of screen height
            settings.bird_size = 24;
            settings.score_font = &lv_font_montserrat_14;
            settings.ground_height = (int)(height * 0.1f); // 10% of screen height
            break;
        case SCREEN_SIZE_MEDIUM:
            settings.pipe_speed = 3;
            settings.pipe_width = 30;
            settings.gravity = 2.0f;
            settings.flap_strength = -10.0f;
            settings.pipe_gap_ratio = 0.3f; // 30% of screen height
            settings.bird_size = 32;
            settings.score_font = &lv_font_montserrat_16;
            settings.ground_height = (int)(height * 0.1f); // 10% of screen height
            break;
        case SCREEN_SIZE_LARGE:
            settings.pipe_speed = 4;
            settings.pipe_width = 40;
            settings.gravity = 2.5f;
            settings.flap_strength = -12.0f;
            settings.pipe_gap_ratio = 0.35f; // 35% of screen height
            settings.bird_size = 40;
            settings.score_font = &lv_font_montserrat_24;
            settings.ground_height = (int)(height * 0.1f); // 10% of screen height
            break;
    }

    settings.buffer_top = (int)(height * 0.25f);    // 5% of screen height
    settings.buffer_bottom = (int)(height * 0.05f); // 5% of screen height
}

// Function to draw the Halloween night sky background
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
    lv_obj_set_style_bg_opa(moon, LV_OPA_COVER, 0);
    lv_obj_clear_flag(moon, LV_OBJ_FLAG_SCROLLABLE);

    // Create the ground with scaled height
    lv_obj_t *ground = lv_obj_create(parent);
    lv_obj_set_size(ground, LV_HOR_RES, settings.ground_height);
    lv_obj_set_style_bg_color(ground, lv_color_hex(0x101010), 0); // Dark gray
    lv_obj_set_pos(ground, 0, LV_VER_RES - settings.ground_height);
    lv_obj_clear_flag(ground, LV_OBJ_FLAG_SCROLLABLE);
}

// Function to check internet connectivity
bool check_internet_connectivity() {
    wifi_ap_record_t info;
    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        return true;
    }
    return false;
}

// Function to submit score to API
void submit_score_to_api(const char *name, int score) {
    if (strlen(FLAPPY_GHOST_WEB_HOOK) == 0) {
        ESP_LOGE("HTTP", "WebHook URL is not defined.");
        return;
    }

    esp_log_level_set("esp_http_client", ESP_LOG_NONE);

    // Prepare JSON payload
    int post_data_len = snprintf(NULL, 0, 
                                  "{"
                                  "\"embeds\": [{"
                                  "\"title\": \"New Score Submitted!\","
                                  "\"description\": \"A new score has been recorded.\","
                                  "\"color\": 16766720,"
                                  "\"fields\": ["
                                  "{ \"name\": \"Player\", \"value\": \"%s\", \"inline\": true },"
                                  "{ \"name\": \"Score\", \"value\": \"%d\", \"inline\": true },"
                                  "{ \"name\": \"Resolution\", \"value\": \"%dx%d\", \"inline\": true }"
                                  "]"
                                  "}]"
                                  "}", 
                                  name, score, LV_HOR_RES, LV_VER_RES) + 1;

    char *post_data = (char *)malloc(post_data_len);
    if (!post_data) {
        ESP_LOGE("HTTP", "Failed to allocate memory for post_data");
        return;
    }

    snprintf(post_data, post_data_len, 
             "{"
             "\"embeds\": [{"
             "\"title\": \"New Score Submitted!\","
             "\"description\": \"A new score has been recorded.\","
             "\"color\": 16766720,"
             "\"fields\": ["
             "{ \"name\": \"Player\", \"value\": \"%s\", \"inline\": true },"
             "{ \"name\": \"Score\", \"value\": \"%d\", \"inline\": true },"
             "{ \"name\": \"Resolution\", \"value\": \"%dx%d\", \"inline\": true }"
             "]"
             "}]"
             "}", 
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
        ESP_LOGI("HTTP", "Score submitted successfully to Discord webhook");
    } else {
        ESP_LOGE("HTTP", "Failed to submit score to Discord webhook");
    }

    free(post_data);

    esp_http_client_cleanup(client);
    esp_log_level_set("esp_http_client", ESP_LOG_INFO);
}

// Function to create the Flappy Bird view
void flappy_bird_view_create(void) {
    if (flappy_bird_view.root != NULL) {
        return;
    }

    // Determine screen height
    int screen_height = LV_VER_RES;
    set_game_settings(screen_height);

    // Initialize variables based on the screen dimensions
    pipe_gap = (int)(screen_height * settings.pipe_gap_ratio);
    pipe_min_gap_y = (int)(screen_height * 0.05f);    // 5% of screen height
    pipe_max_gap_y = screen_height - pipe_gap - settings.ground_height;
    bird_y_position = screen_height <= 128 ? 3 : screen_height / 2;

    // Create root object
    flappy_bird_view.root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(flappy_bird_view.root, LV_HOR_RES, screen_height);
    lv_obj_set_style_bg_color(flappy_bird_view.root, lv_color_black(), 0);
    lv_obj_clear_flag(flappy_bird_view.root, LV_OBJ_FLAG_SCROLLABLE);

    // Draw background
    draw_halloween_night_sky(flappy_bird_view.root, 
                             (screen_height <= 135) ? lv_color_hex(0x0D0D40) : 
                             lv_color_hex(0x87CEEB), 
                             (screen_height <= 135) ? lv_color_hex(0x0A0A30) : 
                             lv_color_hex(0xFFA500));

    // Create canvas for pipes
    flappy_bird_canvas = lv_canvas_create(flappy_bird_view.root);
    lv_obj_set_size(flappy_bird_canvas, LV_HOR_RES, screen_height);
    lv_obj_set_scrollbar_mode(flappy_bird_canvas, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(flappy_bird_canvas, LV_OBJ_FLAG_SCROLLABLE);

    // Create bird
    bird = lv_img_create(flappy_bird_view.root);

    bool use_ghost_image = rand() % 2 == 0;

    lv_img_set_src(bird, &ghost);

    lv_obj_set_size(bird, settings.bird_size, settings.bird_size);
    lv_obj_set_pos(bird, LV_HOR_RES / 4, bird_y_position);
    lv_obj_clear_flag(bird, LV_OBJ_FLAG_SCROLLABLE);

    // Create pipe sets
    for (int i = 0; i < MAX_PIPE_SETS; i++) {
        // Initialize gap center position
        pipes[i].gap_center_y = rand() % (pipe_max_gap_y - pipe_min_gap_y + 1) + pipe_min_gap_y;

        // Create pipe
        pipes[i].pipe = lv_obj_create(flappy_bird_canvas);
        lv_obj_set_size(pipes[i].pipe, settings.pipe_width, screen_height - pipes[i].gap_center_y - settings.ground_height);
        lv_obj_set_style_bg_color(pipes[i].pipe, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_radius(pipes[i].pipe, 0, 0); // Ensure sharp edges
        lv_obj_set_pos(pipes[i].pipe, LV_HOR_RES + i * (LV_HOR_RES / MAX_PIPE_SETS), pipes[i].gap_center_y + pipe_gap);
        lv_obj_set_scrollbar_mode(pipes[i].pipe, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(pipes[i].pipe, LV_OBJ_FLAG_SCROLLABLE);
    }

    // Create score label
    score_label = lv_label_create(flappy_bird_view.root);
    lv_label_set_text_fmt(score_label, "Score: %d", score);
    lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(score_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(score_label, settings.score_font, 0);
    lv_obj_clear_flag(score_label, LV_OBJ_FLAG_SCROLLABLE);

    // Add status bar
    display_manager_add_status_bar(settings.pipe_speed > 3 ? "Flappy Ghost" : "Flap");

    // Create game loop timer
    game_loop_timer = lv_timer_create(flappy_bird_game_loop, GAME_LOOP_INTERVAL_MS, NULL);
}

// Function to destroy the Flappy Bird view
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

// Input Callback Function
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

            if (touch_x >= (area.x1 - padding) && touch_x <= (area.x2 + padding) &&
                touch_y >= (area.y1 - padding) && touch_y <= (area.y2 + padding)) {
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
            bird_velocity = settings.flap_strength;
        }
    } else if (event->type == INPUT_TYPE_TOUCH) {
        bird_velocity = settings.flap_strength;
    }
}

// Function to retrieve the input callback
void flappy_bird_view_get_hardwareinput_callback(void **callback) {
    if (callback != NULL) {
        *callback = (void *)flappy_bird_view_hardwareinput_callback;
    }
}

// Game Loop Function
void flappy_bird_game_loop(lv_timer_t *timer) {
    if (is_game_over) {
        return;
    }

    // Update bird's physics
    bird_velocity += settings.gravity;
    bird_y_position += (int)bird_velocity;
    lv_obj_set_pos(bird, LV_HOR_RES / 4, bird_y_position);

    // Update bird's angle based on velocity
    int angle = (int)(bird_velocity * 5); 
    if (angle > 45) angle = 45;
    if (angle < -45) angle = -45;
    lv_img_set_angle(bird, angle * 10);

    // Check collision with ground or ceiling
    if ((bird_y_position + lv_obj_get_height(bird)) >= (LV_VER_RES - settings.ground_height + settings.buffer_bottom) || 
        (bird_y_position <= -settings.buffer_top)) {
        flappy_bird_game_over();
    }

    // Update each pipe set
    for (int i = 0; i < MAX_PIPE_SETS; i++) {
        // Move pipe left
        int pipe_x = lv_obj_get_x(pipes[i].pipe);
        pipe_x -= settings.pipe_speed;

        if (pipe_x < -settings.pipe_width) {
            pipe_x = LV_HOR_RES;

            // Update gap center position
            pipes[i].gap_center_y = rand() % (pipe_max_gap_y - pipe_min_gap_y + 1) + pipe_min_gap_y;

            // Resize and reposition pipe
            lv_obj_set_size(pipes[i].pipe, settings.pipe_width, LV_VER_RES - pipes[i].gap_center_y - settings.ground_height);
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

// Collision Detection Function
int flappy_bird_check_collision(lv_obj_t *bird, lv_obj_t *pipe) {
    float padding_ratio = 0.02f; // 2% of screen height
    int padding = (int)(LV_VER_RES * padding_ratio);

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

// Game Over Handling Function
void flappy_bird_game_over() {
    if (is_game_over) return; // Prevent multiple triggers
    is_game_over = true;

    // Create a semi-transparent overlay
    lv_obj_t *game_over_container = lv_obj_create(flappy_bird_view.root);
    lv_obj_set_size(game_over_container, LV_HOR_RES - 40, LV_VER_RES * 0.3f); // 30% of screen height
    lv_obj_align(game_over_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(game_over_container, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(game_over_container, LV_OPA_70, 0);
    lv_obj_set_style_border_width(game_over_container, 2, 0);
    lv_obj_set_style_border_color(game_over_container, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_scrollbar_mode(game_over_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(game_over_container, LV_OBJ_FLAG_SCROLLABLE);

    // Create "Game Over" label with scaled font
    lv_obj_t *game_over_label = lv_label_create(game_over_container);
    lv_label_set_text(game_over_label, "Game Over!");
    lv_obj_set_style_text_color(game_over_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_font(game_over_label, settings.score_font, 0); // Reuse score_font for simplicity
    lv_obj_align(game_over_label, LV_ALIGN_CENTER, 0, -10); // Adjust position as needed

    if (settings.pipe_speed > 3)
    {
        // Create "Retry" label
        lv_obj_t *retry_label = lv_label_create(game_over_container);
        lv_label_set_text(retry_label, "Tap to Retry");
        lv_obj_set_style_text_color(retry_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(retry_label, settings.score_font, 0);
        lv_obj_align(retry_label, LV_ALIGN_CENTER, 0, 20);
    }

    // Check internet connectivity and submit score if connected
    internet_connected = check_internet_connectivity();

    if (internet_connected) {
        submit_score_to_api(settings_get_flappy_ghost_name(&G_Settings), score);
    }
}

// Restart Game Function
void flappy_bird_restart() {
    is_game_over = false;
    bird_y_position = (LV_VER_RES <= 128) ? 3 : (LV_VER_RES / 2);
    bird_velocity = 0;
    score = 0;
    lv_label_set_text_fmt(score_label, "Score: %d", score);

    // Reset bird position
    lv_obj_set_pos(bird, LV_HOR_RES / 4, bird_y_position);

    // Reset pipe positions and sizes
    for (int i = 0; i < MAX_PIPE_SETS; i++) {
        // Reposition pipe off-screen
        lv_obj_set_pos(pipes[i].pipe, LV_HOR_RES + i * (LV_HOR_RES / MAX_PIPE_SETS), pipes[i].gap_center_y + pipe_gap);

        // Update gap center position
        pipes[i].gap_center_y = rand() % (pipe_max_gap_y - pipe_min_gap_y + 1) + pipe_min_gap_y;

        // Reset pipe size based on new gap center
        lv_obj_set_size(pipes[i].pipe, settings.pipe_width, LV_VER_RES - pipes[i].gap_center_y - settings.ground_height);
    }

    // Remove the game over overlay if it exists
    lv_obj_t *game_over_container = lv_obj_get_child(flappy_bird_view.root, -1);
    if (game_over_container) {
        lv_obj_del(game_over_container);
    }

    // Restart game loop timer if necessary
    if (game_loop_timer == NULL) {
        game_loop_timer = lv_timer_create(flappy_bird_game_loop, GAME_LOOP_INTERVAL_MS, NULL);
    }
}