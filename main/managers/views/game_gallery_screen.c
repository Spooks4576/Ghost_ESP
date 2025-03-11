#include "managers/views/game_gallery_screen.h"
#include "managers/views/app_gallery_screen.h" // For switching back to app gallery
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lvgl.h"
#include <inttypes.h>
#include "esp_log.h"

// Constants for ESPG file format
#define MAGIC_NUMBER "ESPG"
#define VERSION 0x02
#define HEADER_SIZE 20
#define ASSET_ENTRY_SIZE 16
#define EVENT_ENTRY_SIZE 12

// Asset Types
#define ASSET_SPRITE 0
#define ASSET_IMAGE 1

// Image Formats
#define FORMAT_RGB565 0

// Event Types
#define EVENT_TOUCH_PRESS 0x01

// Bytecode Opcodes
#define OP_NOP 0x00
#define OP_MOVE_SPRITE 0x01
#define OP_SET_ANIMATION 0x02
#define OP_ON_EVENT 0x06
#define OP_RETURN 0x07
#define OP_DRAW_PIXEL 0x09
#define OP_DRAW_LINE 0x0A
#define OP_DRAW_RECT 0x0B
#define OP_LOAD_INT 0x10
#define OP_LOAD_STRING 0x11
#define OP_LOAD_VAR 0x12
#define OP_STORE_VAR 0x13
#define OP_ADD 0x14
#define OP_SUB 0x15
#define OP_IF_EQ 0x16
#define OP_JUMP 0x08
#define OP_END 0xFF

// Structures for ESPG file
typedef struct {
    char magic[4];
    uint8_t version;
    uint16_t asset_count;
    uint32_t asset_table_offset;
    uint16_t event_count;
    uint32_t event_table_offset;
    uint32_t logic_offset;
} EspgHeader;

typedef struct {
    uint8_t type;
    uint32_t offset;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint8_t format;
    uint8_t reserved[3];
} AssetEntry;

typedef struct {
    uint8_t type;
    uint16_t x;
    uint16_t y;
    uint16_t radius;
    uint32_t handler_offset;
    uint8_t padding;
} EventEntry;

// Game and Interpreter Structures
typedef struct {
    EspgHeader header;
    AssetEntry *assets;
    EventEntry *events;
    uint8_t *bytecode;
    size_t bytecode_size;
} EspgGame;

#define MAX_STACK 32
#define MAX_VARS 16

typedef struct {
    int32_t stack[MAX_STACK];
    uint8_t stack_ptr;
    void *variables[MAX_VARS]; // Can hold ints or strings
    uint32_t pc; // Program counter
    EspgGame *game;
    lv_obj_t *canvas; // For drawing operations
} Interpreter;

#define MAX_GAMES 100
#define GAMES_DIR "/mnt/ghostesp/games"
#define GAME_EXT ".espg"
#define GAMES_PER_PAGE 6 // 3 columns * 2 rows

static char *game_names[MAX_GAMES];
static int num_games = 0;
static int selected_game_index = 0;
static int current_page = 0;
static int total_pages = 0;

static lv_obj_t *games_container = NULL;
static lv_obj_t *back_button = NULL;
static lv_obj_t *game_canvas = NULL; // Canvas for game rendering
static Interpreter interp; // Global interpreter instance
static EspgGame *current_game = NULL; // Current loaded game

#ifdef CONFIG_USE_TOUCHSCREEN
static lv_obj_t *next_button = NULL;
static lv_obj_t *prev_button = NULL;
#endif

// Function Prototypes
static EspgGame *load_espg_game(const char *path);
static void init_interpreter(Interpreter *interp, EspgGame *game, lv_obj_t *canvas);
static void interpret(Interpreter *interp);
static void handle_touch_event(Interpreter *interp, int16_t touch_x, int16_t touch_y);
static void run_espg_game(const char *path);
static void refresh_games_menu(void);
static void update_game_item_styles(void);
static void select_game_item(int index);
static void handle_game_button_press(int button);

// Load ESPG Game from SD Card
static EspgGame *load_espg_game(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        ESP_LOGE("GameGallery", "Could not open file %s", path);
        return NULL;
    }

    EspgGame *game = malloc(sizeof(EspgGame));
    if (!game) {
        fclose(file);
        return NULL;
    }

    // Read header
    fread(&game->header, sizeof(EspgHeader), 1, file);
    if (strncmp(game->header.magic, MAGIC_NUMBER, 4) != 0 || game->header.version != VERSION) {
        ESP_LOGE("GameGallery", "Invalid ESPG file");
        free(game);
        fclose(file);
        return NULL;
    }

    // Read asset entries
    game->assets = malloc(sizeof(AssetEntry) * game->header.asset_count);
    fseek(file, game->header.asset_table_offset, SEEK_SET);
    fread(game->assets, sizeof(AssetEntry), game->header.asset_count, file);

    // Read event entries
    game->events = malloc(sizeof(EventEntry) * game->header.event_count);
    fseek(file, game->header.event_table_offset, SEEK_SET);
    fread(game->events, sizeof(EventEntry), game->header.event_count, file);

    // Read bytecode
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    game->bytecode_size = file_size - game->header.logic_offset;
    game->bytecode = malloc(game->bytecode_size);
    fseek(file, game->header.logic_offset, SEEK_SET);
    fread(game->bytecode, 1, game->bytecode_size, file);

    fclose(file);
    return game;
}

// Initialize Interpreter
static void init_interpreter(Interpreter *interp, EspgGame *game, lv_obj_t *canvas) {
    interp->stack_ptr = 0;
    interp->pc = 0;
    interp->game = game;
    interp->canvas = canvas;
    memset(interp->variables, 0, sizeof(interp->variables));
}

// Execute Bytecode
static void interpret(Interpreter *interp) {
    while (interp->pc < interp->game->bytecode_size) {
        uint8_t opcode = interp->game->bytecode[interp->pc++];
        switch (opcode) {
            case OP_LOAD_INT:
                interp->stack[interp->stack_ptr++] = *(int32_t*)&interp->game->bytecode[interp->pc];
                interp->pc += 4;
                break;
            case OP_LOAD_STRING: {
                char *str = (char*)&interp->game->bytecode[interp->pc];
                interp->stack[interp->stack_ptr++] = (int32_t)str;
                interp->pc += strlen(str) + 1;
                break;
            }
            case OP_LOAD_VAR:
                interp->stack[interp->stack_ptr++] = (int32_t)interp->variables[interp->game->bytecode[interp->pc++]];
                break;
            case OP_STORE_VAR: {
                uint8_t var_index = interp->game->bytecode[interp->pc++];
                interp->variables[var_index] = (void*)interp->stack[--interp->stack_ptr];
                break;
            }
            case OP_ADD:
                interp->stack[interp->stack_ptr - 2] += interp->stack[interp->stack_ptr - 1];
                interp->stack_ptr--;
                break;
            case OP_SUB:
                interp->stack[interp->stack_ptr - 2] -= interp->stack[interp->stack_ptr - 1];
                interp->stack_ptr--;
                break;
            case OP_IF_EQ: {
                char *val2 = (char*)interp->stack[--interp->stack_ptr];
                char *val1 = (char*)interp->stack[--interp->stack_ptr];
                int32_t offset = *(int32_t*)&interp->game->bytecode[interp->pc];
                interp->pc += 4;
                if (strcmp(val1, val2) != 0) {
                    interp->pc += offset;
                }
                break;
            }
            case OP_JUMP:
                interp->pc += *(int32_t*)&interp->game->bytecode[interp->pc];
                interp->pc += 4;
                break;
            case OP_MOVE_SPRITE: {
                int32_t y = interp->stack[--interp->stack_ptr];
                int32_t x = interp->stack[--interp->stack_ptr];
                char *sprite_id = (char*)interp->stack[--interp->stack_ptr];
                ESP_LOGI("GameGallery", "Move sprite %s to (%" PRId32 ", %" PRId32 ")", sprite_id, x, y);
                break;
            }
            case OP_SET_ANIMATION: {
                char *anim = (char*)interp->stack[--interp->stack_ptr];
                char *sprite_id = (char*)interp->stack[--interp->stack_ptr];
                ESP_LOGI("GameGallery", "Set animation %s for sprite %s", anim, sprite_id);
                break;
            }
            case OP_DRAW_PIXEL: {
                uint16_t color = (uint16_t)interp->stack[--interp->stack_ptr];
                int32_t y = interp->stack[--interp->stack_ptr];
                int32_t x = interp->stack[--interp->stack_ptr];
                lv_canvas_set_px(interp->canvas, x, y, lv_color_hex(color));
                break;
            }
            case OP_DRAW_LINE: {
                uint16_t color = (uint16_t)interp->stack[--interp->stack_ptr];
                int32_t y2 = interp->stack[--interp->stack_ptr];
                int32_t x2 = interp->stack[--interp->stack_ptr];
                int32_t y1 = interp->stack[--interp->stack_ptr];
                int32_t x1 = interp->stack[--interp->stack_ptr];

                lv_point_t points[2] = {
                    {(lv_coord_t)x1, (lv_coord_t)y1},
                    {(lv_coord_t)x2, (lv_coord_t)y2}
                };

                lv_draw_line_dsc_t line_dsc;
                lv_draw_line_dsc_init(&line_dsc);
                line_dsc.color = lv_color_hex(color);

                lv_canvas_draw_line(interp->canvas, points, 2, &line_dsc);
                break;
            }
            case OP_RETURN:
            case OP_END:
                return;
        }
    }
}

// Handle Touch Events in Game
static void handle_touch_event(Interpreter *interp, int16_t touch_x, int16_t touch_y) {
    for (int i = 0; i < interp->game->header.event_count; i++) {
        EventEntry *event = &interp->game->events[i];
        if (event->type == EVENT_TOUCH_PRESS) {
            int32_t zone_x = ((int32_t)event->x * LV_HOR_RES) / 65535;
            int32_t zone_y = ((int32_t)event->y * LV_VER_RES) / 65535;
            int32_t zone_radius = ((int32_t)event->radius * LV_HOR_RES) / 65535;

            int32_t dx = touch_x - zone_x;
            int32_t dy = touch_y - zone_y;
            int32_t dist_sq = dx * dx + dy * dy;
            int32_t radius_sq = zone_radius * zone_radius;

            if (dist_sq <= radius_sq) {
                interp->pc = event->handler_offset;
                interpret(interp);
                break;
            }
        }
    }
}

// Run an ESPG Game
static void run_espg_game(const char *path) {
    if (current_game) {
        free(current_game->assets);
        free(current_game->events);
        free(current_game->bytecode);
        free(current_game);
        current_game = NULL;
    }
    if (game_canvas) {
        lv_obj_del(game_canvas);
        game_canvas = NULL;
    }

    current_game = load_espg_game(path);
    if (!current_game) {
        ESP_LOGE("GameGallery", "Failed to load game: %s", path);
        return;
    }

    // Create canvas for rendering
    game_canvas = lv_canvas_create(lv_scr_act());
    lv_obj_set_size(game_canvas, LV_HOR_RES, LV_VER_RES);
    void *buf = malloc(LV_HOR_RES * LV_VER_RES * LV_COLOR_DEPTH / 8);
    lv_canvas_set_buffer(game_canvas, buf, LV_HOR_RES, LV_VER_RES, LV_IMG_CF_TRUE_COLOR);

    // Clear the screen
    lv_canvas_fill_bg(game_canvas, lv_color_black(), LV_OPA_COVER);

    // Initialize and run interpreter
    init_interpreter(&interp, current_game, game_canvas);
    interpret(&interp);
}

// Create the Game Gallery View
static void game_gallery_view_create(void) {
    // Clear existing game names
    for (int i = 0; i < num_games; i++) {
        free(game_names[i]);
        game_names[i] = NULL;
    }
    num_games = 0;

    // Read games from SD card
    DIR *dir = opendir(GAMES_DIR);
    if (!dir) {
        ESP_LOGE("GameGallery", "Could not open directory %s", GAMES_DIR);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && num_games < MAX_GAMES) {
        if (strstr(entry->d_name, GAME_EXT)) {
            char *name = strdup(entry->d_name);
            name[strlen(name) - strlen(GAME_EXT)] = '\0'; // Remove extension
            game_names[num_games++] = name;
        }
    }
    closedir(dir);

    // Calculate total pages
    total_pages = (num_games + GAMES_PER_PAGE - 1) / GAMES_PER_PAGE;

    // Create the container
    display_manager_fill_screen(lv_color_black());
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {
        LV_GRID_FR(1),   // Row 0: Games row 1 (top)
        LV_GRID_FR(1),   // Row 1: Games row 2
        LV_GRID_FR(0.5), // Row 2: Buffer/spacer
        #ifdef CONFIG_USE_TOUCHSCREEN
        LV_GRID_FR(0.5), // Row 3: Navigation buttons (bottom)
        #endif
        LV_GRID_TEMPLATE_LAST
    };

    games_container = lv_obj_create(lv_scr_act());
    if (!games_container) {
        ESP_LOGE("GameGallery", "Failed to create games_container");
        return;
    }
    game_gallery_view.root = games_container;
    lv_obj_set_grid_dsc_array(games_container, col_dsc, row_dsc);
    lv_obj_set_size(games_container, LV_HOR_RES, LV_VER_RES); // Full screen, e.g., 1024x600
    lv_obj_set_scrollbar_mode(games_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_column(games_container, 10, 0); // Positive spacing, was -20
    lv_obj_set_style_bg_opa(games_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(games_container, 0, 0);
    // Padding to match apps view
    lv_obj_set_style_pad_left(games_container, 10, 0);
    lv_obj_set_style_pad_bottom(games_container, 10, 0);
    lv_obj_set_style_pad_right(games_container, 10, 0);
    lv_obj_set_style_pad_top(games_container, 5, 0);
    lv_obj_set_style_radius(games_container, 0, 0);
    lv_obj_align(games_container, LV_ALIGN_TOP_MID, 0, 0); // Align to top like apps view

    // Create Back Button
    int button_width = LV_HOR_RES / 4;  // e.g., 256 pixels for 1024 width
    int button_height = LV_VER_RES / 8; // e.g., 75 pixels for 600 height
    back_button = lv_btn_create(games_container);
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

    // Place Back button in bottom-left corner (match apps view)
    #ifdef CONFIG_USE_TOUCHSCREEN
    lv_obj_set_grid_cell(back_button, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_END, 3, 1);
    #else
    lv_obj_set_grid_cell(back_button, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_END, 2, 1);
    #endif
    lv_obj_set_user_data(back_button, (void *)(uintptr_t)(-1));

    #ifdef CONFIG_USE_TOUCHSCREEN
    // Create Next Button
    next_button = lv_btn_create(games_container);
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
    prev_button = lv_btn_create(games_container);
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

    // Refresh game items
    refresh_games_menu();

    #ifndef CONFIG_USE_TOUCHSCREEN
    selected_game_index = current_page * GAMES_PER_PAGE;
    update_game_item_styles();
    #endif

    display_manager_add_status_bar("Games");
}

// Destroy the Game Gallery View
static void game_gallery_view_destroy(void) {
    for (int i = 0; i < num_games; i++) {
        free(game_names[i]);
        game_names[i] = NULL;
    }
    num_games = 0;
    if (games_container) {
        lv_obj_clean(games_container);
        lv_obj_del(games_container);
        games_container = NULL;
    }
    if (current_game) {
        free(current_game->assets);
        free(current_game->events);
        free(current_game->bytecode);
        free(current_game);
        current_game = NULL;
    }
    if (game_canvas) {
        lv_obj_del(game_canvas);
        game_canvas = NULL;
    }
}

// Update Game Item Styles for Selection
static void update_game_item_styles(void) {
    uint32_t child_count = lv_obj_get_child_cnt(games_container);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *game_item = lv_obj_get_child(games_container, i);
        int index = (int)(uintptr_t)lv_obj_get_user_data(game_item);
        if (index < 0) continue; // Skip non-game items
        if (index == selected_game_index) {
            lv_obj_set_style_border_color(game_item, lv_color_make(255, 255, 0), 0);
            lv_obj_set_style_border_width(game_item, 4, 0);
        } else {
            lv_obj_set_style_border_color(game_item, lv_color_make(0, 255, 0), 0);
            lv_obj_set_style_border_width(game_item, 2, 0);
        }
    }
}

// Select a Game Item
static void select_game_item(int index) {
    if (index < -1) index = num_games - 1;
    if (index > num_games - 1) index = -1;
    selected_game_index = index;

    int page = (selected_game_index == -1) ? 0 : (selected_game_index / GAMES_PER_PAGE);
    if (page != current_page) {
        current_page = page;
        refresh_games_menu();
    } else {
        update_game_item_styles();
    }
}

// Refresh the Games Menu
static void refresh_games_menu(void) {
    uint32_t child_count = lv_obj_get_child_cnt(games_container);
    for (int32_t i = child_count - 1; i >= 0; i--) {
        lv_obj_t *child = lv_obj_get_child(games_container, i);
        int index = (int)(uintptr_t)lv_obj_get_user_data(child);
        if (index >= 0) {
            lv_obj_del(child);
        }
    }

    int start_index = current_page * GAMES_PER_PAGE;
    int end_index = start_index + GAMES_PER_PAGE;
    if (end_index > num_games) end_index = num_games;

    int button_width = LV_HOR_RES / 4;
    int button_height = LV_VER_RES / 4;
    const lv_font_t *font = (LV_VER_RES <= 128) ? &lv_font_montserrat_10 :
    (LV_VER_RES <= 240) ? &lv_font_montserrat_16 :
    &lv_font_montserrat_18;

    for (int i = start_index; i < end_index; i++) {
        int display_index = i - start_index;
        lv_obj_t *game_item = lv_btn_create(games_container);
        lv_obj_set_size(game_item, button_width, button_height);
        lv_obj_set_style_bg_color(game_item, lv_color_black(), 0);
        lv_obj_set_style_border_color(game_item, lv_color_make(0, 255, 0), 0);
        lv_obj_set_style_border_width(game_item, 2, 0);
        lv_obj_set_style_radius(game_item, 10, 0);
        lv_obj_set_scrollbar_mode(game_item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_pad_all(game_item, 5, 0);

        lv_obj_t *label = lv_label_create(game_item);
        lv_label_set_text(label, game_names[i]);
        lv_obj_set_style_text_font(label, font, 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);

        int row_idx = 1 + (display_index / 3); // Rows 1 and 2 for games
        lv_obj_set_grid_cell(game_item, LV_GRID_ALIGN_CENTER, display_index % 3, 1,
                             LV_GRID_ALIGN_START, row_idx, 1);
        lv_obj_set_user_data(game_item, (void *)(uintptr_t)i);
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
    if (selected_game_index < start_index || selected_game_index >= end_index) {
        selected_game_index = start_index;
    }
    update_game_item_styles();
    #endif
}

// Handle Joystick Button Presses
static void handle_game_button_press(int button) {
    if (button == 0) { // Left
        select_game_item(selected_game_index - 1);
    } else if (button == 3) { // Right
        select_game_item(selected_game_index + 1);
    } else if (button == 1) { // Select
        if (selected_game_index == -1) {
            ESP_LOGI("GameGallery", "Back button pressed");
            display_manager_switch_view(&apps_menu_view);
        } else {
            char path[256];
            snprintf(path, sizeof(path), "%s/%s%s", GAMES_DIR, game_names[selected_game_index], GAME_EXT);
            ESP_LOGI("GameGallery", "Launching game via joystick: %s", game_names[selected_game_index]);
            run_espg_game(path);
        }
    }
}

// Input Callback for Touch and Joystick
static void game_gallery_view_input_callback(InputEvent *event) {
    if (event->type == INPUT_TYPE_TOUCH) {
        lv_indev_data_t *data = &event->data.touch_data;

        if (game_canvas) { // Game is running
            handle_touch_event(&interp, data->point.x, data->point.y);
            return;
        }

        int touched_game_index = -1;
        ESP_LOGI("GameGallery", "Touch detected at X: %d, Y: %d", data->point.x, data->point.y);

        uint32_t child_count = lv_obj_get_child_cnt(games_container);
        for (uint32_t i = 0; i < child_count; i++) {
            lv_obj_t *child = lv_obj_get_child(games_container, i);
            int index = (int)(uintptr_t)lv_obj_get_user_data(child);

            lv_area_t item_area;
            lv_obj_get_coords(child, &item_area);

            if (data->point.x >= item_area.x1 && data->point.x <= item_area.x2 &&
                data->point.y >= item_area.y1 && data->point.y <= item_area.y2) {
                if (index == -1) { // Back button
                    ESP_LOGI("GameGallery", "Back button touched");
                    display_manager_switch_view(&apps_menu_view);
                    return;
                }
                #ifdef CONFIG_USE_TOUCHSCREEN
                else if (index == -2) { // Next button
                    if (current_page < total_pages - 1) {
                        current_page++;
                        refresh_games_menu();
                    }
                    return;
                } else if (index == -3) { // Prev button
                    if (current_page > 0) {
                        current_page--;
                        refresh_games_menu();
                    }
                    return;
                }
                #endif
                else {
                    touched_game_index = index;
                    break;
                }
                }
        }

        if (touched_game_index >= 0) {
            char path[256];
            snprintf(path, sizeof(path), "%s/%s%s", GAMES_DIR, game_names[touched_game_index], GAME_EXT);
            ESP_LOGI("GameGallery", "Launching game: %s", game_names[touched_game_index]);
            run_espg_game(path);
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        if (game_canvas) {
            return;
        }
        handle_game_button_press(event->data.joystick_index);
    }
}

// Get Hardware Input Callback
static void game_gallery_view_get_hardwareinput_callback(void **callback) {
    if (callback != NULL) {
        *callback = (void *)game_gallery_view_input_callback;
    }
}

// Define the Game Gallery View
View game_gallery_view = {
    .root = NULL,
    .create = game_gallery_view_create,
    .destroy = game_gallery_view_destroy,
    .input_callback = game_gallery_view_input_callback,
    .name = "GameGalleryView",
    .get_hardwareinput_callback = game_gallery_view_get_hardwareinput_callback
};
