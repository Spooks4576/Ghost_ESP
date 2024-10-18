#ifndef TERMINAL_VIEW_H
#define TERMINAL_VIEW_H

#include "lvgl.h"
#include "managers/display_manager.h"


extern View terminal_view;


void terminal_view_add_text(const char *text);

void terminal_view_create(void);

void terminal_view_destroy(void);

#endif // TERMINAL_VIEW_H