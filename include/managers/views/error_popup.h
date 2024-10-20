#ifndef ERROR_POPUP_H
#define ERROR_POPUP_H

#include "lvgl.h"

// Function to create an error popup with a given message
void error_popup_create(const char *message);

// Function to destroy the error popup
void error_popup_destroy(void);

bool is_error_popup_rendered();

#endif // ERROR_POPUP_H