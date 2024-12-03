#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdarg.h>
#include "esp_log.h"
#include "views/terminal_screen.h"

// Define a function that forwards to both printf and TERMINAL_VIEW_ADD_TEXT
static inline int __attribute__((format(printf, 1, 2))) debug_printf(const char* format, ...) {
    char buffer[512];  // Adjust size as needed
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);
    
    // Print to standard output
    vprintf(format, args1);
    
    // Format string for terminal view
    vsnprintf(buffer, sizeof(buffer), format, args2);
    TERMINAL_VIEW_ADD_TEXT(buffer);
    
    va_end(args1);
    va_end(args2);
    return 0;
}

// Replace all printf calls with our debug_printf
#define printf debug_printf

#endif // LOGGING_H