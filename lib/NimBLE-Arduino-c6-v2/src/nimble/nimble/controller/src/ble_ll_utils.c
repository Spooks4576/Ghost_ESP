/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef ESP_PLATFORM

#include <assert.h>
#include <stdlib.h>
#include "nimble/nimble/include/nimble/ble.h"
#include "../include/controller/ble_ll.h"
#include "../include/controller/ble_ll_tmr.h"
#include "../include/controller/ble_ll_utils.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/* 37 bits require 5 bytes */
#define BLE_LL_CHMAP_LEN (5)

/* Sleep clock accuracy table (in ppm) */
static const uint16_t g_ble_sca_ppm_tbl[8] = {
    500, 250, 150, 100, 75, 50, 30, 20
};

uint32_t
ble_ll_utils_calc_access_addr(void)
{
    uint32_t aa;
    uint16_t aa_low;
    uint16_t aa_high;
    uint32_t temp;
    uint32_t mask;
    uint32_t prev_bit;
    uint8_t bits_diff;
    uint8_t consecutive;
    uint8_t transitions;
    uint8_t ones;
    int tmp;

    /* Calculate a random access address */
    aa = 0;
    while (1) {
        /* Get two, 16-bit random numbers */
        aa_low = ble_ll_rand() & 0xFFFF;
        aa_high = ble_ll_rand() & 0xFFFF;

        /* All four bytes cannot be equal */
        if (aa_low == aa_high) {
            continue;
        }

        /* Upper 6 bits must have 2 transitions */
        tmp = (int16_t)aa_high >> 10;
        if (__builtin_popcount(tmp ^ (tmp >> 1)) < 2) {
            continue;
        }

        /* Cannot be access address or be 1 bit different */
        aa = aa_high;
        aa = (aa << 16) | aa_low;
        bits_diff = 0;
        temp = aa ^ BLE_ACCESS_ADDR_ADV;
        for (mask = 0x00000001; mask != 0; mask <<= 1) {
            if (mask & temp) {
                ++bits_diff;
                if (bits_diff > 1) {
                    break;
                }
            }
        }
        if (bits_diff <= 1) {
            continue;
        }

        /* Cannot have more than 24 transitions */
        transitions = 0;
        consecutive = 1;
        ones = 0;
        mask = 0x00000001;
        while (mask < 0x80000000) {
            prev_bit = aa & mask;
            mask <<= 1;
            if (mask & aa) {
                if (prev_bit == 0) {
                    ++transitions;
                    consecutive = 1;
                } else {
                    ++consecutive;
                }
            } else {
                if (prev_bit == 0) {
                    ++consecutive;
                } else {
                    ++transitions;
                    consecutive = 1;
                }
            }

            if (prev_bit) {
                ones++;
            }

            /* 8 lsb should have at least three 1 */
            if (mask == 0x00000100 && ones < 3) {
                break;
            }

            /* 16 lsb should have no more than 11 transitions */
            if (mask == 0x00010000 && transitions > 11) {
                break;
            }

            /* This is invalid! */
            if (consecutive > 6) {
                /* Make sure we always detect invalid sequence below */
                mask = 0;
                break;
            }
        }

        /* Invalid sequence found */
        if (mask != 0x80000000) {
            continue;
        }

        /* Cannot be more than 24 transitions */
        if (transitions > 24) {
            continue;
        }

        /* We have a valid access address */
        break;
    }
    return aa;
}

uint8_t
ble_ll_utils_remapped_channel(uint8_t remap_index, const uint8_t *chanmap)
{
    uint8_t cntr;
    uint8_t mask;
    uint8_t usable_chans;
    uint8_t chan;
    int i, j;

    /* NOTE: possible to build a map but this would use memory. For now,
     * we just calculate
     * Iterate through channel map to find this channel
     */
    chan = 0;
    cntr = 0;
    for (i = 0; i < BLE_LL_CHMAP_LEN; i++) {
        usable_chans = chanmap[i];
        if (usable_chans != 0) {
            mask = 0x01;
            for (j = 0; j < 8; j++) {
                if (usable_chans & mask) {
                    if (cntr == remap_index) {
                        return (chan + j);
                    }
                    ++cntr;
                }
                mask <<= 1;
            }
        }
        chan += 8;
    }

    /* we should never reach here */
    BLE_LL_ASSERT(0);
    return 0;
}

uint8_t
ble_ll_utils_calc_num_used_chans(const uint8_t *chan_map)
{
    uint32_t u32 = 0;
    uint32_t num_used_chans = 0;
    unsigned idx;

    for (idx = 0; idx < 37; idx++) {
        if ((idx % 8) == 0) {
            u32 = chan_map[idx / 8];
        }
        if (u32 & 1) {
            num_used_chans++;
        }
        u32 >>= 1;
    }

    return num_used_chans;
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CSA2)
#if __thumb2__
static inline uint32_t
ble_ll_utils_csa2_perm(uint32_t val)
{
    __asm__ volatile (".syntax unified      \n"
                      "rbit %[val], %[val]  \n"
                      "rev %[val], %[val]   \n"
                      : [val] "+r" (val));

    return val;
}
#else
static uint32_t
ble_ll_utils_csa2_perm(uint32_t in)
{
    uint32_t out = 0;
    int i;

    for (i = 0; i < 8; i++) {
        out |= ((in >> i) & 0x00000001) << (7 - i);
    }

    for (i = 8; i < 16; i++) {
        out |= ((in >> i) & 0x00000001) << (15 + 8 - i);
    }

    return out;
}
#endif

static inline uint32_t
ble_ll_utils_csa2_mam(uint32_t a, uint32_t b)
{
    return (17 * a + b) % 65536;
}

static uint16_t
ble_ll_utils_csa2_prn_s(uint16_t counter, uint16_t ch_id)
{
    uint32_t prn_s;

    prn_s = counter ^ ch_id;

    prn_s = ble_ll_utils_csa2_perm(prn_s);
    prn_s = ble_ll_utils_csa2_mam(prn_s, ch_id);

    prn_s = ble_ll_utils_csa2_perm(prn_s);
    prn_s = ble_ll_utils_csa2_mam(prn_s, ch_id);

    prn_s = ble_ll_utils_csa2_perm(prn_s);
    prn_s = ble_ll_utils_csa2_mam(prn_s, ch_id);

    return prn_s;
}

static uint16_t
ble_ll_utils_csa2_prng(uint16_t counter, uint16_t ch_id)
{
    uint16_t prn_s;
    uint16_t prn_e;

    prn_s = ble_ll_utils_csa2_prn_s(counter, ch_id);
    prn_e = prn_s ^ ch_id;

    return prn_e;
}

/* Find remap_idx for given chan_idx */
static uint16_t
ble_ll_utils_csa2_chan2remap(uint16_t chan_idx, const uint8_t *chan_map)
{
    uint16_t remap_idx = 0;
    uint32_t u32 = 0;
    unsigned idx;

    for (idx = 0; idx < 37; idx++) {
        if ((idx % 8) == 0) {
            u32 = chan_map[idx / 8];
        }
        if (u32 & 1) {
            if (idx == chan_idx) {
                return remap_idx;
            }
            remap_idx++;
        }
        u32 >>= 1;
    }

    BLE_LL_ASSERT(0);

    return 0;
}

/* Find chan_idx at given remap_idx */
static uint16_t
ble_ll_utils_csa2_remap2chan(uint16_t remap_idx, const uint8_t *chan_map)
{
    uint32_t u32 = 0;
    unsigned idx;

    for (idx = 0; idx < 37; idx++) {
        if ((idx % 8) == 0) {
            u32 = chan_map[idx / 8];
        }
        if (u32 & 1) {
            if (!remap_idx) {
                return idx;
            }
            remap_idx--;
        }
        u32 >>= 1;
    }

    BLE_LL_ASSERT(0);

    return 0;
}

static uint16_t
ble_ll_utils_csa2_calc_chan_idx(uint16_t prn_e, uint8_t num_used_chans,
                                const uint8_t *chanm_map, uint16_t *remap_idx)
{
    uint16_t chan_idx;

    chan_idx = prn_e % 37;
    if (chanm_map[chan_idx / 8] & (1 << (chan_idx % 8))) {
        *remap_idx = ble_ll_utils_csa2_chan2remap(chan_idx, chanm_map);
        return chan_idx;
    }

    *remap_idx = (num_used_chans * prn_e) / 65536;
    chan_idx = ble_ll_utils_csa2_remap2chan(*remap_idx, chanm_map);

    return chan_idx;
}

uint8_t
ble_ll_utils_dci_csa2(uint16_t counter, uint16_t chan_id,
                      uint8_t num_used_chans, const uint8_t *chan_map)
{
    uint16_t prn_e;
    uint16_t chan_idx;
    uint16_t remap_idx;

    prn_e = ble_ll_utils_csa2_prng(counter, chan_id);

    chan_idx = ble_ll_utils_csa2_calc_chan_idx(prn_e, num_used_chans, chan_map,
                                               &remap_idx);

    return chan_idx;
}

uint16_t
ble_ll_utils_dci_iso_event(uint16_t counter, uint16_t chan_id,
                           uint16_t *prn_sub_lu, uint8_t num_used_chans,
                           const uint8_t *chan_map, uint16_t *remap_idx)
{
    uint16_t prn_s;
    uint16_t prn_e;
    uint16_t chan_idx;

    prn_s = ble_ll_utils_csa2_prn_s(counter, chan_id);
    prn_e = prn_s ^ chan_id;

    *prn_sub_lu = prn_s;

    chan_idx = ble_ll_utils_csa2_calc_chan_idx(prn_e, num_used_chans, chan_map,
                                               remap_idx);

    return chan_idx;
}

uint16_t
ble_ll_utils_dci_iso_subevent(uint16_t chan_id, uint16_t *prn_sub_lu,
                              uint8_t num_used_chans, const uint8_t *chan_map,
                              uint16_t *remap_idx)
{
    uint16_t prn_sub_se;
    uint16_t chan_idx;
    uint16_t d;

    *prn_sub_lu = ble_ll_utils_csa2_perm(*prn_sub_lu);
    *prn_sub_lu = ble_ll_utils_csa2_mam(*prn_sub_lu, chan_id);
    prn_sub_se = *prn_sub_lu ^ chan_id;

    /* Core 5.3, Vol 6, Part B, 4.5.8.3.6 (enjoy!) */
    /* TODO optimize this somehow */
    d = max(1, max(min(3, num_used_chans - 5),
                   min(11, (num_used_chans - 10) / 2)));
    *remap_idx = (*remap_idx + d + prn_sub_se *
                  (num_used_chans - 2 * d + 1) / 65536) % num_used_chans;

    chan_idx = ble_ll_utils_csa2_remap2chan(*remap_idx, chan_map);

    return chan_idx;
}
#endif

uint32_t
ble_ll_utils_calc_window_widening(uint32_t anchor_point,
                                  uint32_t last_anchor_point,
                                  uint8_t central_sca)
{
    uint32_t total_sca_ppm;
    uint32_t window_widening;
    int32_t time_since_last_anchor;
    uint32_t delta_msec;

    window_widening = 0;

    time_since_last_anchor = (int32_t)(anchor_point - last_anchor_point);
    if (time_since_last_anchor > 0) {
        delta_msec = ble_ll_tmr_t2u(time_since_last_anchor) / 1000;
        total_sca_ppm = g_ble_sca_ppm_tbl[central_sca] + MYNEWT_VAL(BLE_LL_SCA);
        window_widening = (total_sca_ppm * delta_msec) / 1000;
    }

    return window_widening;
}
#endif
