
// serial_manager.h

#ifndef UTILS_H
#define UTILS_H

#include <esp_types.h>
#include <stdio.h>

const char *wrap_message(const char *message, const char *file, int line) {
  int size =
      snprintf(NULL, 0, "File: %s, Line: %d, Message: %s", file, line, message);

  char *buffer = (char *)malloc(size + 1);

  if (buffer != NULL) {
    snprintf(buffer, size + 1, "File: %s, Line: %d, Message: %s", file, line,
             message);
  }
  return buffer;
}

void scale_grb_by_brightness(uint8_t *g, uint8_t *r, uint8_t *b,
                             float brightness) {
  float scale_factor = brightness < 0.0f ? -brightness : brightness;

  if (scale_factor > 1.0f) {
    scale_factor = 1.0f;
  }

  int original_g = *g;
  int original_r = *r;
  int original_b = *b;

  *g = (int)((float)(original_g)*scale_factor);
  *r = (int)((float)(original_r)*scale_factor);
  *b = (int)((float)(original_b)*scale_factor);

  if (*g > 255)
    *g = 255;
  if (*r > 255)
    *r = 255;
  if (*b > 255)
    *b = 255;

  if (*g < 0)
    *g = 0;
  if (*r < 0)
    *r = 0;
  if (*b < 0)
    *b = 0;
}

bool is_in_task_context(void);

void url_decode(char *decoded, const char *encoded);

int get_query_param_value(const char *query, const char *key, char *value,
                          size_t value_size);

int get_next_pcap_file_index(const char *base_name);

#define WRAP_MESSAGE(msg) wrap_message(msg, __FILE__, __LINE__)

#endif // SERIAL_MANAGER_H