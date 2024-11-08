#ifndef SCRIPT_INTERPRETER_H
#define SCRIPT_INTERPRETER_H

#include <stdint.h>
#include <stdbool.h>
#include "managers/display_manager.h"
#include "lvgl/lvgl.h"

#define MAX_SCRIPT_SIZE 4096
#define MAX_INSTRUCTIONS 100
#define MAX_LABELS 10

extern FILE *script_file;
extern const char* SelectedScriptPath;
extern View script_view;

// Enum for supported commands
typedef enum {
    CMD_SET_COLOR,
    CMD_DRAW_RECT,
    CMD_WAIT,
    CMD_GOTO,
    CMD_CALL,
    CMD_RETURN,
    CMD_END,
    CMD_INVALID
} CommandType;

// Struct for each command
typedef struct {
    CommandType type;
    int args[4];
} Command;

// Label for jumps and calls
typedef struct {
    char label[20];
    int instruction_index;
} Label;

// Interpreter context struct
typedef struct {
    Command instructions[MAX_INSTRUCTIONS];
    Label labels[MAX_LABELS];
    int label_count;
    int instruction_count;
    int call_stack[MAX_LABELS];
    int call_stack_index;
    lv_color_t current_color;
    int program_counter;
} ScriptContext;

// Function prototypes
void interpreter_init(ScriptContext *context);
bool interpreter_load_script(ScriptContext *context, const char *script);
void interpreter_run(ScriptContext *context);
bool load_script_from_sd(ScriptContext *context, const char *file_path);
void interpreter_destroy(ScriptContext *context);

#endif // SCRIPT_INTERPRETER_H