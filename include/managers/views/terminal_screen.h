#ifndef TERMINAL_VIEW_H
#define TERMINAL_VIEW_H

#include "lvgl.h"
#include "managers/display_manager.h"

extern View terminal_view;

void terminal_view_add_text(const char *text);

void terminal_view_create(void);

void terminal_view_destroy(void);

#ifdef CONFIG_WITH_SCREEN
#define TERMINAL_VIEW_ADD_TEXT(fmt, ...)                                       \
  do {                                                                         \
    char buffer[350];                                                          \
    snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__);                      \
    terminal_view_add_text(buffer);                                            \
  } while (0)
#else
#define TERMINAL_VIEW_ADD_TEXT(fmt, ...)                                       \
  do {                                                                         \
  } while (0)
#endif

#endif // TERMINAL_VIEW_H