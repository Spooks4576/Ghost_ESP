#include "wps/wps_utils.h"
#include <stddef.h>

inline void uint_to_char_array(unsigned int num, unsigned int len, char *dst)
{
    unsigned int mul = 1;
	while (len--) {
		dst[len] = (num % (mul * 10) / mul) + '0';
		mul *= 10;
	}
}

void byte_array_print(const uint8_t *buffer, const unsigned int length)
{
	for (unsigned int i = 0; i < length; i++)
		printf("%02x", buffer[i]);
}

unsigned int bit_revert(unsigned int v)
{
	size_t i;
	unsigned int n = 0;
	for (i = 0; i < sizeof(unsigned int) * 8; i++) {
		const unsigned int lsb = v & 1;
		v >>= 1;
		n <<= 1;
		n |= lsb;
	}
	return n;
}

int get_int(char *in, int *out)
{
	int i, o = 0, len = strlen(in);
	for (i = 0; i < len; i++) {
		if ('0' <= *in && *in <= '9')
			o = o * 10 + *in - '0';
		else
			return 1;
		in++;
	}
	*out = o;
	return 0;
}


unsigned int hex_string_to_byte_array(char *in, uint8_t *out, const unsigned int n_len)
{
	unsigned int len = strlen(in);
	unsigned int b_len = n_len * 2 + n_len - 1;

	if (len != n_len * 2 && len != b_len)
		return 1;
	for (unsigned int i = 0; i < n_len; i++) {
		unsigned char o = 0;
		for (unsigned char j = 0; j < 2; j++) {
			o <<= 4;
			if (*in >= 'A' && *in <= 'F')
				*in += 'a'-'A';
			if (*in >= '0' && *in <= '9')
				o += *in - '0';
			else
				if (*in >= 'a' && *in <= 'f')
					o += *in - 'a' + 10;
				else
					return 1;
			in++;
		}
		*out++ = o;
		if (len == b_len) {
			if (*in == ':' || *in == '-' || *in == ' ' || *in == 0)
				in++;
			else
				return 1;
		}
	}
	return 0;
}


unsigned int hex_string_to_byte_array_max(
		char *in, uint8_t *out, const unsigned int max_len, unsigned int *m_len)
{
	uint_fast8_t o, separator = 0;
	unsigned int count = 0;
	unsigned int len = strlen(in);

	if (len > 2)
		if (in[2] == ':' || in[2] == '-' || in[2] == ' ')
			separator = 1;
	if (separator) {
		if ((len + 1) / 3 > max_len)
			return 1;
	}
	else {
		if (len / 2 > max_len)
			return 1;
	}

	for (unsigned int i = 0; i < max_len; i++) {
		o = 0;
		for (uint_fast8_t j = 0; j < 2; j++) {
			o <<= 4;
			if (*in >= 'A' && *in <= 'F')
				*in += 'a'-'A';
			if (*in >= '0' && *in <= '9')
				o += *in - '0';
			else
				if (*in >= 'a' && *in <= 'f')
					o += *in - 'a' + 10;
				else
					return 1;
			in++;
		}
		*out++ = o;
		count++;

		if (*in == 0)
			goto end;

		if (separator) {
			if (*in == ':' || *in == '-' || *in == ' ')
				in++;
			else
				return 1;
		}
	}

end:
	*m_len = count;
	return 0;
}