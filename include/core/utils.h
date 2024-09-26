
// serial_manager.h

#ifndef UTILS_H
#define UTILS_H

#include <esp_types.h>
#include <stdio.h>

const char* wrap_message(const char *message, const char *file, int line) {
    int size = snprintf(NULL, 0, "File: %s, Line: %d, Message: %s", file, line, message);

    char *buffer = (char *)malloc(size + 1);
    
    if (buffer != NULL) {
        snprintf(buffer, size + 1, "File: %s, Line: %d, Message: %s", file, line, message);
    }
    return buffer;
}


#define WRAP_MESSAGE(msg) wrap_message(msg, __FILE__, __LINE__)

#endif // SERIAL_MANAGER_H