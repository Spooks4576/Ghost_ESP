#include "wps/wps_defs.h"
#include "mbedtls/aes.h"
#include <string.h>
#include <aes_alt.h>
#include <mbedtls/md.h>

int aes_128_cbc_decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *input, size_t length, uint8_t *output) {
    mbedtls_aes_context aes;
    
    
    mbedtls_aes_init(&aes);

    // Set decryption key (128-bit key length)
    if (mbedtls_aes_setkey_dec(&aes, key, 128) != 0) {
        mbedtls_aes_free(&aes);
        return -1;
    }

    
    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, length, iv, input, output) != 0) {
        mbedtls_aes_free(&aes);
        return -1;
    }

    
    mbedtls_aes_free(&aes);
    return 0;
}


void hmac_sha256(const void *key, size_t key_len, const void *data, size_t data_len, uint8_t *hmac_output) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *md_info;

    mbedtls_md_init(&ctx);

    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    mbedtls_md_setup(&ctx, md_info, 1);  // 1 indicates HMAC mode
    mbedtls_md_hmac_starts(&ctx, key, key_len);
    mbedtls_md_hmac_update(&ctx, data, data_len);
    mbedtls_md_hmac_finish(&ctx, hmac_output);

    mbedtls_md_free(&ctx);
}

struct ie_vtag *find_vtag(void *vtagp, int vtagl, void *vidp, int vlen)
{
	uint8_t *vid = vidp;
	struct ie_vtag *vtag = vtagp;
	while (0 < vtagl) {
		const int len = ntohs(vtag->len);
		if (vid && memcmp(vid, &vtag->id, 2) != 0)
			goto next_vtag;
		if (!vlen || len == vlen)
			return vtag;

next_vtag:
		vtagl -= len + VTAG_SIZE;
		vtag = (struct ie_vtag *)((uint8_t *)vtag + len + VTAG_SIZE);
	}
	return NULL;
}

void kdf(const void *key, uint8_t *res)
{
	const uint32_t kdk_len = (WPS_AUTHKEY_LEN + WPS_KEYWRAPKEY_LEN + WPS_EMSK_LEN) * 8;
	uint_fast8_t j = 0;

	uint8_t *buffer = malloc(sizeof(kdf_salt) + sizeof(uint32_t) * 2);

	for (uint32_t i = 1; i < 4; i++) {
		uint32_t be = htonl(i);
		memcpy(buffer, &be, sizeof(uint32_t));
		memcpy(buffer + sizeof(uint32_t), kdf_salt, sizeof(kdf_salt));
		be = htonl(kdk_len);
		memcpy(buffer + sizeof(uint32_t) + sizeof(kdf_salt), &be, sizeof(uint32_t));
		hmac_sha256(key, WPS_HASH_LEN, buffer, sizeof(kdf_salt) + sizeof(uint32_t) * 2, res + j);
		j += WPS_HASH_LEN;
	}
	free(buffer);
}


uint8_t *decrypt_encr_settings(uint8_t *keywrapkey, const uint8_t *encr, size_t encr_len) {
    uint8_t *decrypted;
    const size_t block_size = 16;
    size_t i;
    uint8_t pad;
    const uint8_t *pos;
    size_t n_encr_len;

    
    if (encr == NULL || encr_len < 2 * block_size || encr_len % block_size) {
        return NULL;
    }

    
    decrypted = (uint8_t*)malloc(encr_len - block_size);
    if (decrypted == NULL) {
        return NULL;
    }

    
    const uint8_t *iv = encr;

    
    const uint8_t *ciphertext = encr + block_size;
    n_encr_len = encr_len - block_size; 

    
    if (aes_128_cbc_decrypt(keywrapkey, iv, ciphertext, n_encr_len, decrypted)) {
        free(decrypted);
        return NULL;
    }

    
    pos = decrypted + n_encr_len - 1;
    pad = *pos;
    if (pad > n_encr_len) {
        free(decrypted);
        return NULL;
    }
    for (i = 0; i < pad; i++) {
        if (*pos-- != pad) {
            free(decrypted);
            return NULL;
        }
    }
    return decrypted;
}


static inline uint_fast8_t wps_pin_checksum(uint_fast32_t pin)
{
	unsigned int acc = 0;
	while (pin) {
		acc += 3 * (pin % 10);
		pin /= 10;
		acc += pin % 10;
		pin /= 10;
	}
	return (10 - acc % 10) % 10;
}


static inline uint_fast8_t wps_pin_valid(uint_fast32_t pin)
{
	return wps_pin_checksum(pin / 10) == (pin % 10);
}

/* Checks if PKe == 2 */
static inline uint_fast8_t check_small_dh_keys(const uint8_t *data)
{
	uint_fast8_t i = WPS_PKEY_LEN - 2;
	while (--i) {
		if (data[i] != 0)
			break;
	}
	i = (i == 0 && data[WPS_PKEY_LEN - 1] == 0x02) ? 1 : 0;
	return i;
}