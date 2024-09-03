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

#include <stdint.h>
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "../include/controller/ble_ll.h"
#include "../include/controller/ble_ll_hci.h"
#include "../include/controller/ble_ll_sync.h"
#include "../include/controller/ble_ll_adv.h"
#include "../include/controller/ble_ll_scan.h"
#include "../include/controller/ble_hw.h"
#include "ble_ll_conn_priv.h"
#include "ble_ll_priv.h"

#if MYNEWT_VAL(BLE_LL_HCI_VS)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) \
        (sizeof(array) / sizeof((array)[0]))
#endif

SLIST_HEAD(ble_ll_hci_vs_list, ble_ll_hci_vs_cmd);
static struct ble_ll_hci_vs_list g_ble_ll_hci_vs_list;

static int
ble_ll_hci_vs_rd_static_addr(uint16_t ocf,
                             const uint8_t *cmdbuf, uint8_t cmdlen,
                             uint8_t *rspbuf, uint8_t *rsplen)
{
    struct ble_hci_vs_rd_static_addr_rp *rsp = (void *) rspbuf;
    ble_addr_t addr;

    if (cmdlen != 0) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (ble_hw_get_static_addr(&addr) < 0) {
        return BLE_ERR_UNSPECIFIED;
    }

    memcpy(rsp->addr, addr.val, sizeof(rsp->addr));

    *rsplen = sizeof(*rsp);

    return BLE_ERR_SUCCESS;
}

/* disallow changing TX power if there is any radio activity
 * note: we could allow to change it if there is no TX activity (eg only
 * passive scan or sync) but lets just keep this simple for now
 */
static int
ble_ll_hci_vs_is_controller_busy(void)
{
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL) || MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    struct ble_ll_conn_sm *cur;
    int i = 0;
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV) && MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)
    if (ble_ll_sync_enabled()) {
        return 1;
    }
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_BROADCASTER)
    if (ble_ll_adv_enabled()) {
        return 1;
    }
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)
    if (ble_ll_scan_enabled()) {
        return 1;
    }
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    if (g_ble_ll_conn_create_sm.connsm) {
        return 1;
    }
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL) || MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    STAILQ_FOREACH(cur, &g_ble_ll_conn_free_list, free_stqe) {
        i++;
    }

    /* check if all connection objects are free */
    if (i < MYNEWT_VAL(BLE_MAX_CONNECTIONS)) {
        return 1;
    }
#endif

    return 0;
}

static int
ble_ll_hci_vs_set_tx_power(uint16_t ocf, const uint8_t *cmdbuf, uint8_t cmdlen,
                           uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_vs_set_tx_pwr_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_vs_set_tx_pwr_rp *rsp = (void *) rspbuf;

    if (cmdlen != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (ble_ll_hci_vs_is_controller_busy()) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    if (cmd->tx_power == 127) {
        /* restore reset default */
        g_ble_ll_tx_power = MYNEWT_VAL(BLE_LL_TX_PWR_DBM);
    } else {
        g_ble_ll_tx_power = ble_phy_txpower_round(cmd->tx_power);
    }

    rsp->tx_power = g_ble_ll_tx_power;
    *rsplen = sizeof(*rsp);

    return BLE_ERR_SUCCESS;
}


#if MYNEWT_VAL(BLE_LL_HCI_VS_CONN_STRICT_SCHED)
#if !MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
static int
ble_ll_hci_vs_css_configure(const uint8_t *cmdbuf, uint8_t cmdlen,
                            uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_vs_css_configure_cp *cmd = (const void *)cmdbuf;
    uint32_t slot_us;
    uint32_t period_slots;

    if (cmdlen != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (!SLIST_EMPTY(&g_ble_ll_conn_active_list)) {
        return BLE_ERR_CTLR_BUSY;
    }

    slot_us = le32toh(cmd->slot_us);
    period_slots = le32toh(cmd->period_slots);

    if (slot_us % BLE_LL_CONN_ITVL_USECS) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if ((slot_us == 0) || (period_slots == 0)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    ble_ll_sched_css_set_params(slot_us, period_slots);

    return BLE_ERR_SUCCESS;
}
#endif

static int
ble_ll_hci_vs_css_set_next_slot(const uint8_t *cmdbuf, uint8_t cmdlen,
                                uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_vs_css_set_next_slot_cp *cmd = (const void *)cmdbuf;
    uint16_t slot_idx;

    if (cmdlen != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    slot_idx = le16toh(cmd->slot_idx);
    if ((slot_idx >= ble_ll_sched_css_get_period_slots()) &&
        (slot_idx != BLE_LL_CONN_CSS_NO_SLOT)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (ble_ll_conn_css_is_slot_busy(slot_idx)) {
        return BLE_ERR_CTLR_BUSY;
    }

    ble_ll_conn_css_set_next_slot(slot_idx);

    return BLE_ERR_SUCCESS;
}

static int
ble_ll_hci_vs_css_set_conn_slot(const uint8_t *cmdbuf, uint8_t cmdlen,
                                uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_vs_css_set_conn_slot_cp *cmd = (const void *)cmdbuf;
    struct ble_ll_conn_sm *connsm;
    uint16_t conn_handle;
    uint16_t slot_idx;

    if (cmdlen != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    slot_idx = le16toh(cmd->slot_idx);
    if ((slot_idx >= ble_ll_sched_css_get_period_slots()) &&
        (slot_idx != BLE_LL_CONN_CSS_NO_SLOT)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (ble_ll_conn_css_is_slot_busy(slot_idx)) {
        return BLE_ERR_CTLR_BUSY;
    }

    conn_handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_active_conn(conn_handle);
    if (!connsm) {
        return BLE_ERR_UNK_CONN_ID;
    }

    if (connsm->css_slot_idx_pending != BLE_LL_CONN_CSS_NO_SLOT) {
        return BLE_ERR_DIFF_TRANS_COLL;
    }

    if (connsm->css_slot_idx == slot_idx) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    if (ble_ll_conn_css_move(connsm, slot_idx) < 0) {
        return BLE_ERR_CTLR_BUSY;
    }

    return BLE_ERR_SUCCESS;
}

static int
ble_ll_hci_vs_css(uint16_t ocf, const uint8_t *cmdbuf, uint8_t cmdlen,
                  uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_vs_css_cp *cmd = (const void *)cmdbuf;

    if (cmdlen < sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    *rsplen = 0;

    switch (cmd->opcode) {
#if !MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
    case BLE_HCI_VS_CSS_OP_CONFIGURE:
        return ble_ll_hci_vs_css_configure(cmdbuf, cmdlen, rspbuf, rsplen);
#endif
    case BLE_HCI_VS_CSS_OP_SET_NEXT_SLOT:
        return ble_ll_hci_vs_css_set_next_slot(cmdbuf, cmdlen, rspbuf, rsplen);
    case BLE_HCI_VS_CSS_OP_SET_CONN_SLOT:
        return ble_ll_hci_vs_css_set_conn_slot(cmdbuf, cmdlen, rspbuf, rsplen);
    }

    return BLE_ERR_INV_HCI_CMD_PARMS;
}
#endif

static struct ble_ll_hci_vs_cmd g_ble_ll_hci_vs_cmds[] = {
    BLE_LL_HCI_VS_CMD(BLE_HCI_OCF_VS_RD_STATIC_ADDR,
                      ble_ll_hci_vs_rd_static_addr),
    BLE_LL_HCI_VS_CMD(BLE_HCI_OCF_VS_SET_TX_PWR,
            ble_ll_hci_vs_set_tx_power),
#if MYNEWT_VAL(BLE_LL_HCI_VS_CONN_STRICT_SCHED)
    BLE_LL_HCI_VS_CMD(BLE_HCI_OCF_VS_CSS,
                      ble_ll_hci_vs_css),
#endif
};

static struct ble_ll_hci_vs_cmd *
ble_ll_hci_vs_find_by_ocf(uint16_t ocf)
{
    struct ble_ll_hci_vs_cmd *entry;

    entry = SLIST_FIRST(&g_ble_ll_hci_vs_list);
    while (entry) {
        if (entry->ocf == ocf) {
            return entry;
        }

        entry = SLIST_NEXT(entry, link);
    }

    return NULL;
}

int
ble_ll_hci_vs_cmd_proc(const uint8_t *cmdbuf, uint8_t cmdlen, uint16_t ocf,
                       uint8_t *rspbuf, uint8_t *rsplen)
{
    struct ble_ll_hci_vs_cmd *cmd;
    int rc;

    cmd = ble_ll_hci_vs_find_by_ocf(ocf);
    if (!cmd) {
        rc = BLE_ERR_UNKNOWN_HCI_CMD;
    } else {
        rc = cmd->cb(ocf, cmdbuf, cmdlen, rspbuf, rsplen);
    }

    return rc;
}

void
ble_ll_hci_vs_register(struct ble_ll_hci_vs_cmd *cmds, uint32_t num_cmds)
{
    uint32_t i;

    /* Assume all cmds are registered early on init, so just assert in case of
     * invalid request since it means something is wrong with the code itself.
     */

    for (i = 0; i < num_cmds; i++, cmds++) {
        BLE_LL_ASSERT(cmds->cb != NULL);
        BLE_LL_ASSERT(ble_ll_hci_vs_find_by_ocf(cmds->ocf) == NULL);

        SLIST_INSERT_HEAD(&g_ble_ll_hci_vs_list, cmds, link);
    }
}

void
ble_ll_hci_vs_init(void)
{
    SLIST_INIT(&g_ble_ll_hci_vs_list);

    ble_ll_hci_vs_register(g_ble_ll_hci_vs_cmds,
                           ARRAY_SIZE(g_ble_ll_hci_vs_cmds));
}

#endif

#endif /* !ESP_PLATFORM */