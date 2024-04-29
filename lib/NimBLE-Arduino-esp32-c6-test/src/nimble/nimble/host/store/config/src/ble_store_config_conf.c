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

/*
 * Modified work Copyright (c) 2021 Ryan Powell.
 */

#ifndef ESP_PLATFORM

#include "nimble/porting/nimble/include/syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)

#include <inttypes.h>
//#include <string.h>

#include "nimble/porting/nimble/include/sysinit/sysinit.h"
#include "../../../include/host/ble_hs.h"
//#include "config/config.h"
#include "base64/base64.h"
#include "../include/store/config/ble_store_config.h"
#include "ble_store_config_priv.h"
#include "ble_bond_nvs.h"

/*
static int
ble_store_config_conf_set(int argc, char **argv, char *val);
static int
ble_store_config_conf_export(void (*func)(char *name, char *val),
                             enum conf_export_tgt tgt);

static struct conf_handler ble_store_config_conf_handler = {
    .ch_name = "ble_hs",
    .ch_get = NULL,
    .ch_set = ble_store_config_conf_set,
    .ch_commit = NULL,
    .ch_export = ble_store_config_conf_export
};
*/
#define BLE_STORE_CONFIG_SEC_ENCODE_SZ      \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_sec))

#define BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ  \
    (MYNEWT_VAL(BLE_STORE_MAX_BONDS) * BLE_STORE_CONFIG_SEC_ENCODE_SZ + 1)

#define BLE_STORE_CONFIG_CCCD_ENCODE_SZ     \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_cccd))

#define BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ \
    (MYNEWT_VAL(BLE_STORE_MAX_CCCDS) * BLE_STORE_CONFIG_CCCD_ENCODE_SZ + 1)

#if MYNEWT_VAL(ENC_ADV_DATA)
#define BLE_STORE_CONFIG_EAD_ENCODE_SZ     \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_ead))

#define BLE_STORE_CONFIG_EAD_SET_ENCODE_SZ \
    (MYNEWT_VAL(BLE_STORE_MAX_EADS) * BLE_STORE_CONFIG_EAD_ENCODE_SZ + 1)
#endif

#define BLE_STORE_CONFIG_RPA_REC_ENCODE_SZ \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_rpa_rec))

#define BLE_STORE_CONFIG_RPA_REC_SET_ENCODE_SZ \
    (MYNEWT_VAL(BLE_STORE_MAX_BONDS) * BLE_STORE_CONFIG_RPA_REC_ENCODE_SZ + 1)

static void
ble_store_config_serialize_arr(const void *arr, int obj_sz, int num_objs,
                               char *out_buf, int buf_sz)
{
    int arr_size;

    arr_size = obj_sz * num_objs;
    assert(arr_size <= buf_sz);

    base64_encode(arr, arr_size, out_buf, 1);
}

static int
ble_store_config_deserialize_arr(const char *enc,
                                 void *out_arr,
                                 int obj_sz,
                                 int *out_num_objs)
{
    int len;

    len = base64_decode(enc, out_arr);
    if (len < 0) {
        return OS_EINVAL;
    }

    *out_num_objs = len / obj_sz;
    return 0;
}
/*
static int
ble_store_config_conf_set(int argc, char **argv, char *val)
{
    int rc;

    if (argc == 1) {
        if (strcmp(argv[0], "our_sec") == 0) {
            rc = ble_store_config_deserialize_arr(
                    val,
                    ble_store_config_our_secs,
                    sizeof *ble_store_config_our_secs,
                    &ble_store_config_num_our_secs);
            printf("\nloading: %s %s rc=%d\n", *argv, val, rc);
            return rc;
        } else if (strcmp(argv[0], "peer_sec") == 0) {
            rc = ble_store_config_deserialize_arr(
                    val,
                    ble_store_config_peer_secs,
                    sizeof *ble_store_config_peer_secs,
                    &ble_store_config_num_peer_secs);
            printf("\nloading: %s %s rc=%d\n", *argv, val, rc);
            return rc;
        } else if (strcmp(argv[0], "cccd") == 0) {
            rc = ble_store_config_deserialize_arr(
                    val,
                    ble_store_config_cccds,
                    sizeof *ble_store_config_cccds,
                    &ble_store_config_num_cccds);
            printf("\nloading: %s %s rc=%d\n", *argv, val, rc);
            return rc;
        }
#if MYNEWT_VAL(ENC_ADV_DATA)
        else if (strcmp(argv[0], "ead") == 0) {
            rc = ble_store_config_deserialize_arr(
                    val,
                    ble_store_config_eads,
                    sizeof *ble_store_config_eads,
                    &ble_store_config_num_eads);
            return rc;
        }
#endif
        else if (strcmp(argv[0],"rpa_rec") == 0){
            rc = ble_store_config_deserialize_arr(
                    val,
                    ble_store_config_rpa_recs,
                    sizeof *ble_store_config_rpa_recs,
                    &ble_store_config_num_rpa_recs);
            return rc;
        }
    }
    return OS_ENOENT;
}

static int
ble_store_config_conf_export(void (*func)(char *name, char *val),
                             enum conf_export_tgt tgt)
{
    union {
        char sec[BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ];
        char cccd[BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ];
        char rpa_rec[BLE_STORE_CONFIG_RPA_REC_SET_ENCODE_SZ];
    } buf;

    ble_store_config_serialize_arr(ble_store_config_our_secs,
                                   sizeof *ble_store_config_our_secs,
                                   ble_store_config_num_our_secs,
                                   buf.sec,
                                   sizeof buf.sec);
    func("ble_hs/our_sec", buf.sec);

    ble_store_config_serialize_arr(ble_store_config_peer_secs,
                                   sizeof *ble_store_config_peer_secs,
                                   ble_store_config_num_peer_secs,
                                   buf.sec,
                                   sizeof buf.sec);
    func("ble_hs/peer_sec", buf.sec);

    ble_store_config_serialize_arr(ble_store_config_cccds,
                                   sizeof *ble_store_config_cccds,
                                   ble_store_config_num_cccds,
                                   buf.cccd,
                                   sizeof buf.cccd);
    func("ble_hs/cccd", buf.cccd);

#if MYNEWT_VAL(ENC_ADV_DATA)
    ble_store_config_serialize_arr(ble_store_config_eads,
                                   sizeof *ble_store_config_eads,
                                   ble_store_config_num_eads,
                                   buf.ead,
                                   sizeof buf.ead);
    func("ble_hs/ead", buf.ead);
#endif
    ble_store_config_serialize_arr(ble_store_config_rpa_recs,
                                   sizeof *ble_store_config_rpa_recs,
                                   ble_store_config_num_rpa_recs,
                                   buf.rpa_rec,
                                   sizeof buf.rpa_rec);
    return 0;
}
*/
static int
ble_store_config_persist_sec_set(uint32_t setting_name,
                                 const struct ble_store_value_sec *secs,
                                 int num_secs)
{
    char buf[BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ];
    int rc;

    ble_store_config_serialize_arr(secs, sizeof *secs, num_secs,
                                   buf, sizeof buf);

    rc = ble_bond_nvs_save_entry(setting_name, buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }

    return 0;
}

int
ble_store_config_persist_our_secs(void)
{
    int rc;

    rc = ble_store_config_persist_sec_set(our_sec,
                                          ble_store_config_our_secs,
                                          ble_store_config_num_our_secs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
ble_store_config_persist_peer_secs(void)
{
    int rc;

    rc = ble_store_config_persist_sec_set(peer_sec,
                                          ble_store_config_peer_secs,
                                          ble_store_config_num_peer_secs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
ble_store_config_persist_cccds(void)
{
    char buf[BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ];
    int rc;

    ble_store_config_serialize_arr(ble_store_config_cccds,
                                   sizeof *ble_store_config_cccds,
                                   ble_store_config_num_cccds,
                                   buf,
                                   sizeof buf);

    rc = ble_bond_nvs_save_entry(cccd, buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }

    return 0;
}

#if MYNEWT_VAL(ENC_ADV_DATA)
int
ble_store_config_persist_eads(void)
{
    char buf[BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ];
    int rc;
    ble_store_config_serialize_arr(ble_store_config_eads,
                                   sizeof *ble_store_config_eads,
                                   ble_store_config_num_eads,
                                   buf,
                                   sizeof buf);
    rc = conf_save_one("ble_hs/ead", buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }
    return 0;
}
#endif

int
ble_store_config_persist_rpa_recs(void)
{
#ifdef ESP_PLATFORM
    char buf[BLE_STORE_CONFIG_RPA_REC_SET_ENCODE_SZ];
    int rc;
    ble_store_config_serialize_arr(ble_store_config_rpa_recs,
                                   sizeof *ble_store_config_rpa_recs,
                                   ble_store_config_num_rpa_recs,
                                   buf,
                                   sizeof buf);
    rc = conf_save_one("ble_hs/rpa_rec", buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }
    return 0;
#endif
    return BLE_HS_ESTORE_FAIL;
}

#ifndef ESP_PLATFORM
int ble_store_config_persist_local_irk(void){
    return BLE_HS_ESTORE_FAIL;
}
#endif

void
ble_store_config_conf_init(void)
{
    uint32_t val_addr = 0;
    int rc = 0;

    rc = ble_bond_nvs_get_entry(our_sec, &val_addr);
    if (rc == 0) {
        rc = ble_store_config_deserialize_arr(
                (char*)val_addr,
                ble_store_config_our_secs,
                sizeof *ble_store_config_our_secs,
                &ble_store_config_num_our_secs);
        if (rc != 0) {
             BLE_HS_LOG(ERROR, "our_sec restore error rc=%d\n", rc);
            return;
        }

        rc = ble_bond_nvs_get_entry(peer_sec, &val_addr);
        if (rc == 0) {
            rc = ble_store_config_deserialize_arr(
                    (char*)val_addr,
                    ble_store_config_peer_secs,
                    sizeof *ble_store_config_peer_secs,
                    &ble_store_config_num_peer_secs);
            if (rc != 0) {
                 BLE_HS_LOG(ERROR, "peer_sec restore error rc=%d\n", rc);
                return;
            }

        } else {
            /* If we have a security entry for our security but not a peer
             * we should assume something wrong with the store so delete it.
             */
            BLE_HS_LOG(ERROR, "peer info not found\n");
            ble_store_clear();
            return;
        }

        rc = ble_bond_nvs_get_entry(cccd, &val_addr);
        if (rc == 0) {
            rc = ble_store_config_deserialize_arr(
                    (char*)val_addr,
                    ble_store_config_cccds,
                    sizeof *ble_store_config_cccds,
                    &ble_store_config_num_cccds);
            if (rc != 0) {
                 BLE_HS_LOG(ERROR, "cccd restore error rc=%d\n", rc);
                return;
            }
        }
    }

 /*   int rc;

    rc = conf_register(&ble_store_config_conf_handler);
    SYSINIT_PANIC_ASSERT_MSG(rc == 0,
                             "Failed to register ble_store_config conf");
 */
}

#endif /* MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST) */
#endif /* ESP_PLATFORM */