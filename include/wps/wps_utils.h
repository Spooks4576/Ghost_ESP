

#ifndef WPS_UTILS_H
#define WPS_UTILS_H


#include <stdint.h>

void byte_array_print(const uint8_t *buffer, const unsigned int length);

int get_int(char *in, int *out);

unsigned int bit_revert(unsigned int v);

unsigned int hex_string_to_byte_array_max(char *in, uint8_t *out, const unsigned int max_len, unsigned int *m_len);

unsigned int hex_string_to_byte_array(char *in, uint8_t *out, const unsigned int n_len);

#endif