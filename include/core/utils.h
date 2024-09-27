
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

void scale_grb_by_brightness(uint8_t *g, uint8_t *r, uint8_t *b, float brightness) {
    // Ensure brightness is within the valid range [0.0, 1.0]
    if (brightness < 0.0f) {
        brightness = 0.0f;
    } else if (brightness > 1.0f) {
        brightness = 1.0f;
    }

    // Store original values for debugging
    int original_g = *g;
    int original_r = *r;
    int original_b = *b;

    // Perform the scaling using floating-point arithmetic and cast back to int
    *g = (int)((float)(original_g) * brightness);
    *r = (int)((float)(original_r) * brightness);
    *b = (int)((float)(original_b) * brightness);

    // Ensure values remain within the valid range (0 to 255)
    if (*g > 255) *g = 255;
    if (*r > 255) *r = 255;
    if (*b > 255) *b = 255;

    if (*g < 0) *g = 0;
    if (*r < 0) *r = 0;
    if (*b < 0) *b = 0;
}

#define WRAP_MESSAGE(msg) wrap_message(msg, __FILE__, __LINE__)

#endif // SERIAL_MANAGER_H