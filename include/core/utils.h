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

void scale_grb_by_brightness(uint8_t *g, uint8_t *r, uint8_t *b, float brightness) {
    *g = (uint8_t)(*g * brightness);
    *r = (uint8_t)(*r * brightness); 
    *b = (uint8_t)(*b * brightness);
}

bool is_in_task_context(void);

void url_decode(char *decoded, const char *encoded);

int get_query_param_value(const char *query, const char *key, char *value,
                          size_t value_size);

int get_next_pcap_file_index(const char *base_name);

#define WRAP_MESSAGE(msg) wrap_message(msg, __FILE__, __LINE__)

#endif // SERIAL_MANAGER_H