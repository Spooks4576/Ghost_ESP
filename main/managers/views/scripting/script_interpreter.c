#include "managers/views/scripting/script_interpreter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ctype.h>

FILE *script_file = NULL;

// Helper function to match string commands with enum
CommandType parse_command(const char *cmd) {
    if (strcmp(cmd, "SET_COLOR") == 0) return CMD_SET_COLOR;
    if (strcmp(cmd, "DRAW_RECT") == 0) return CMD_DRAW_RECT;
    if (strcmp(cmd, "WAIT") == 0) return CMD_WAIT;
    if (strcmp(cmd, "GOTO") == 0) return CMD_GOTO;
    if (strcmp(cmd, "CALL") == 0) return CMD_CALL;
    if (strcmp(cmd, "RETURN") == 0) return CMD_RETURN;
    if (strcmp(cmd, "END") == 0) return CMD_END;
    return CMD_INVALID;
}

// Initialize the script context
void interpreter_init(ScriptContext *context) {
    context->instruction_count = 0;
    context->label_count = 0;
    context->call_stack_index = -1;
    context->current_color = lv_color_black();
}

// Load and parse the script into commands
bool interpreter_load_script(ScriptContext *context, const char *script) {
    printf("Starting to load script.\n");

    char line[100];
    const char *ptr = script;
    int line_number = 0;

    while (*ptr != '\0') {
        line_number++;

        
        sscanf(ptr, "%99[^\n]\n", line);
        ptr += strlen(line) + 1;
        printf("Line %d: %s\n", line_number, line);

        // Trim any trailing comments or extra whitespace after the command and arguments
        char *comment_start = strchr(line, '/');
        if (comment_start) *comment_start = '\0';  // Strip out comments
        for (int i = strlen(line) - 1; i >= 0 && isspace((unsigned char)line[i]); i--) line[i] = '\0';  // Trim trailing spaces

        // Skip empty lines
        if (strlen(line) == 0) {
            printf("Skipping empty line %d.\n", line_number);
            continue;
        }

        
        if (line[0] == ':') {
            printf("Detected label on line %d: %s\n", line_number, line);

            if (context->label_count >= MAX_LABELS) {
                printf("Error: Maximum label count reached. Cannot add more labels.\n");
                return false;
            }

            
            strcpy(context->labels[context->label_count].label, line + 1);
            context->labels[context->label_count].instruction_index = context->instruction_count;
            printf("Added label '%s' at instruction index %d.\n", context->labels[context->label_count].label, context->instruction_count);
            context->label_count++;
            continue;
        }

        
        char cmd[20];
        int args[4] = {0};
        int arg_count = sscanf(line, "%19s %d %d %d %d", cmd, &args[0], &args[1], &args[2], &args[3]);
        printf("Parsed command '%s' with %d arguments on line %d.\n", cmd, arg_count - 1, line_number);

        CommandType type = parse_command(cmd);

        
        if (type == CMD_INVALID) {
            printf("Error: Invalid command '%s' on line %d.\n", cmd, line_number);
            return false;
        }

        if (context->instruction_count >= MAX_INSTRUCTIONS) {
            printf("Error: Maximum instruction count reached. Cannot add more instructions.\n");
            return false;
        }

        
        Command *instruction = &context->instructions[context->instruction_count++];
        instruction->type = type;
        for (int i = 0; i < arg_count - 1; i++) {
            instruction->args[i] = args[i];
        }

        
        printf("Added instruction: %s", cmd);
        for (int i = 0; i < arg_count - 1; i++) {
            printf(" %d", args[i]);
        }
        printf("\n");
    }

    printf("Finished loading script with %d instructions and %d labels.\n", context->instruction_count, context->label_count);
    return true;
}

// Execute the script
void interpreter_run(ScriptContext *context) {
    int pc = 0;  // Program counter
    while (pc < context->instruction_count) {
        Command *cmd = &context->instructions[pc];
        switch (cmd->type) {
            case CMD_SET_COLOR:
                context->current_color = lv_color_make(cmd->args[0], cmd->args[1], cmd->args[2]);
                break;

            case CMD_DRAW_RECT: {
                lv_obj_t *rect = lv_obj_create(lv_scr_act());
                lv_obj_set_style_bg_color(rect, context->current_color, 0);
                lv_obj_set_size(rect, cmd->args[2], cmd->args[3]);
                lv_obj_align(rect, LV_ALIGN_TOP_LEFT, cmd->args[0], cmd->args[1]);
                break;
            }

            case CMD_WAIT:
                vTaskDelay(pdMS_TO_TICKS(cmd->args[0]));
                break;

            case CMD_GOTO: {
                bool found = false;
                for (int i = 0; i < context->label_count; i++) {
                    if (strcmp(context->labels[i].label, (char*)cmd->args) == 0) {
                        pc = context->labels[i].instruction_index;
                        found = true;
                        break;
                    }
                }
                if (!found) return;
                continue;
            }

            case CMD_CALL:
                if (context->call_stack_index >= MAX_LABELS - 1) return;
                context->call_stack[++context->call_stack_index] = pc + 1;
                for (int i = 0; i < context->label_count; i++) {
                    if (strcmp(context->labels[i].label, (char*)cmd->args) == 0) {
                        pc = context->labels[i].instruction_index;
                        break;
                    }
                }
                continue;

            case CMD_RETURN:
                if (context->call_stack_index < 0) return;
                pc = context->call_stack[context->call_stack_index--];
                continue;

            case CMD_END:
                return;

            default:
                return;
        }
        pc++;
    }
}

void interpreter_destroy(ScriptContext *context) {
    lv_obj_clean(lv_scr_act());
    interpreter_init(context); 
}

bool load_script_from_sd(ScriptContext *context, const char *file_path) {
    char script_buffer[MAX_SCRIPT_SIZE];
    
    // Open the file
    script_file = fopen(file_path, "r");
    if (!script_file) {
        printf("Failed to open script file: %s", file_path);
        return false;
    }

    // Read the file contents into the buffer
    size_t bytes_read = fread(script_buffer, 1, MAX_SCRIPT_SIZE - 1, script_file);
    if (bytes_read == 0) {
        printf("Failed to read script file or file is empty.");
        fclose(script_file);
        script_file = NULL;
        return false;
    }
    
    script_buffer[bytes_read] = '\0';

    
    bool success = interpreter_load_script(context, script_buffer);

    fclose(script_file);
    script_file = NULL;

    return success;
}