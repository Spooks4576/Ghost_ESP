#include "managers/views/scripting/script_interpreter.h"
#include "managers/views/main_menu_screen.h"
#include "lvgl/lvgl.h"
#include <stdio.h>

const char* SelectedScriptPath = NULL;
lv_obj_t *root;


lv_timer_t *script_timer = NULL;


#define GAME_LOOP_INTERVAL_MS LV_VER_RES > 320 ? 10 : 25

bool interpreter_run_step(ScriptContext *context) {
    if (context->program_counter >= context->instruction_count) {
        printf("End of script reached.\n");
        return false;
    }

    Command *cmd = &context->instructions[context->program_counter++];
    printf("Executing command %d: Type %d\n", context->program_counter - 1, cmd->type);

    switch (cmd->type) {
        case CMD_SET_COLOR:
            context->current_color = lv_color_make(cmd->args[0], cmd->args[1], cmd->args[2]);
            printf("Set color to RGB(%d, %d, %d)\n", cmd->args[0], cmd->args[1], cmd->args[2]);
            break;

        case CMD_DRAW_RECT: {
            lv_obj_t *rect = lv_obj_create(lv_scr_act());
            lv_obj_set_style_bg_color(rect, context->current_color, 0);
            lv_obj_set_size(rect, cmd->args[2], cmd->args[3]);
            lv_obj_align(rect, LV_ALIGN_TOP_LEFT, cmd->args[0], cmd->args[1]);
            printf("Draw rect at (%d, %d) with size (%d, %d)\n", cmd->args[0], cmd->args[1], cmd->args[2], cmd->args[3]);
            break;
        }

        case CMD_WAIT:
            printf("Wait for %d milliseconds\n", cmd->args[0]);
            lv_timer_reset(script_timer);
            lv_timer_set_period(script_timer, cmd->args[0]);
            return true;

        case CMD_GOTO: {
            bool found = false;
            for (int i = 0; i < context->label_count; i++) {
                if (strcmp(context->labels[i].label, (char*)cmd->args) == 0) {
                    context->program_counter = context->labels[i].instruction_index;
                    found = true;
                    printf("Goto label '%s' at instruction index %d\n", context->labels[i].label, context->program_counter);
                    break;
                }
            }
            if (!found) {
                printf("Error: Label not found for GOTO.\n");
                return false;
            }
            return true;
        }

        case CMD_CALL:
            if (context->call_stack_index >= MAX_LABELS - 1) {
                printf("Error: Call stack overflow.\n");
                return false;
            }
            context->call_stack[++context->call_stack_index] = context->program_counter;
            for (int i = 0; i < context->label_count; i++) {
                if (strcmp(context->labels[i].label, (char*)cmd->args) == 0) {
                    context->program_counter = context->labels[i].instruction_index;
                    printf("Call label '%s' at instruction index %d\n", context->labels[i].label, context->program_counter);
                    break;
                }
            }
            return true;

        case CMD_RETURN:
            if (context->call_stack_index < 0) {
                printf("Error: Call stack underflow on RETURN.\n");
                return false;
            }
            context->program_counter = context->call_stack[context->call_stack_index--];
            printf("Return to instruction index %d\n", context->program_counter);
            return true;

        case CMD_END:
            printf("End of script command encountered.\n");
            return false;

        default:
            printf("Error: Unknown command type.\n");
            return false;
    }
    lv_timer_set_period(script_timer, GAME_LOOP_INTERVAL_MS);
    return true;
}


// Script execution timer callback function
void script_timer_callback(lv_timer_t *timer) {
    ScriptContext *context = (ScriptContext *)timer->user_data;

    
    bool continue_execution = interpreter_run_step(context);

    if (!continue_execution) {
        display_manager_switch_view(&main_menu_view);
        lv_timer_del(script_timer);
        script_timer = NULL;
    }
}

// Create function for the Script View
void script_view_create(void) {
    ScriptContext script_context;
    root = lv_obj_create(lv_scr_act());
    script_view.root = root;
    script_context.current_color = lv_color_black();
    display_manager_fill_screen(lv_color_black());

    // Initialize the interpreter
    interpreter_init(&script_context);

    // Load the script from the SD card
    if (!load_script_from_sd(&script_context, SelectedScriptPath)) {
        printf("Failed to load script\n");
        display_manager_switch_view(&main_menu_view);
        return;
    }


    script_timer = lv_timer_create(script_timer_callback, GAME_LOOP_INTERVAL_MS, &script_context);
    if (script_timer == NULL) {
        printf("Failed to create script timer\n");
        display_manager_switch_view(&main_menu_view);
    }

    //display_manager_add_status_bar("Script");
}

// Destroy function for the Script View
void script_view_destroy(void) {
    if (script_timer != NULL) {
        lv_timer_del(script_timer);
        script_timer = NULL;
    }

    lv_obj_clean(script_view.root);
    script_view.root = NULL;
}

// Hardware input callback placeholder
void script_view_get_hardwareinput_callback(void **callback) {
    *callback = NULL;
}

// Optional input callback placeholder
void script_view_input_callback(InputEvent* event) {

}


View script_view = {
    .root = NULL,
    .create = script_view_create,
    .destroy = script_view_destroy,
    .name = "Script View",
    .get_hardwareinput_callback = script_view_get_hardwareinput_callback,
    .input_callback = script_view_input_callback
};