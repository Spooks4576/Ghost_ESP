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

#include <stddef.h>
#include "../include/os/os.h"
#include "../include/sysinit/sysinit.h"
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "../include/nimble/nimble_port.h"
#include "../../npl/freertos/include/nimble/nimble_port_freertos.h"
#if NIMBLE_CFG_CONTROLLER
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/porting/nimble/include/hal/hal_timer.h"
#include "nimble/porting/nimble/include/os/os_cputime.h"
#endif

#ifdef ESP_PLATFORM
#include "esp_log.h"
#  if defined __has_include
#    if __has_include ("soc/soc_caps.h")
#      include "soc/soc_caps.h"
#    endif
#  endif
#include "esp_intr_alloc.h"
#if CONFIG_BT_CONTROLLER_ENABLED
#include "esp_bt.h"
#endif

#if !SOC_ESP_NIMBLE_CONTROLLER && CONFIG_BT_CONTROLLER_ENABLED
#include "nimble/esp_port/esp-hci/include/esp_nimble_hci.h"
#endif
#if !CONFIG_BT_CONTROLLER_ENABLED
#include "nimble/nimble/transport/include/nimble/transport.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define NIMBLE_PORT_LOG_TAG          "BLE_INIT"

extern void os_msys_init(void);

#if CONFIG_BT_NIMBLE_ENABLED

extern void ble_hs_deinit(void);
static struct ble_hs_stop_listener stop_listener;

#endif //CONFIG_BT_NIMBLE_ENABLED

static struct ble_npl_eventq g_eventq_dflt;
static struct ble_npl_sem ble_hs_stop_sem;
static struct ble_npl_event ble_hs_ev_stop;

extern void os_msys_init(void);
extern void os_mempool_module_init(void);

/**
 * Called when the host stop procedure has completed.
 */
static void
ble_hs_stop_cb(int status, void *arg)
{
    ble_npl_sem_release(&ble_hs_stop_sem);
}

static void
nimble_port_stop_cb(struct ble_npl_event *ev)
{
    ble_npl_sem_release(&ble_hs_stop_sem);
}

/**
 * @brief esp_nimble_init - Initialize the NimBLE host stack
 *
 * @return esp_err_t
 */
esp_err_t esp_nimble_init(void)
{
#if CONFIG_BT_CONTROLLER_DISABLED
    esp_err_t ret;
#endif
#if !SOC_ESP_NIMBLE_CONTROLLER || !CONFIG_BT_CONTROLLER_ENABLED

#if CONFIG_NIMBLE_STACK_USE_MEM_POOLS
    /* Initialize the function pointers for OS porting */
    npl_freertos_funcs_init();

    npl_freertos_mempool_init();
#endif

#if CONFIG_BT_CONTROLLER_ENABLED
    if(esp_nimble_hci_init() != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "hci inits failed\n");
        return ESP_FAIL;
    }
#else
    //ret = ble_buf_alloc();
    if (ble_buf_alloc() != ESP_OK) {
        ble_buf_free();
        return ESP_FAIL;
    }
    ble_transport_init();
#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
    ble_adv_list_init();
#endif // MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
#endif // CONFIG_BT_CONTROLLER_ENABLED
#endif // !SOC_ESP_NIMBLE_CONTROLLER || !CONFIG_BT_CONTROLLER_ENABLED

    /* Initialize default event queue */
    /* Included for Arduino since the default event queue is not initialized
    * in the Arduino core when bludroid is used.
    */
    ble_npl_eventq_init(&g_eventq_dflt);
#if !SOC_ESP_NIMBLE_CONTROLLER || !CONFIG_BT_CONTROLLER_ENABLED
    /* Initialize the global memory pool */
    os_mempool_module_init();
    os_msys_init();

#endif // !SOC_ESP_NIMBLE_CONTROLLER || !CONFIG_BT_CONTROLLER_ENABLED
    /* Initialize the host */
    ble_transport_hs_init();
#if CONFIG_BT_CONTROLLER_DISABLED && CONFIG_BT_NIMBLE_TRANSPORT_UART
    ble_transport_ll_init();
#endif

    return ESP_OK;
}

/**
 * @brief esp_nimble_deinit - Deinitialize the NimBLE host stack
 *
 * @return esp_err_t
 */
esp_err_t esp_nimble_deinit(void)
{
#if !SOC_ESP_NIMBLE_CONTROLLER
#if CONFIG_BT_CONTROLLER_ENABLED
    if(esp_nimble_hci_deinit() != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "hci deinit failed\n");
        return ESP_FAIL;
    }
#else
#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
    ble_adv_list_deinit();
#endif
    ble_transport_deinit();
    ble_buf_free();
#endif

    ble_npl_eventq_deinit(&g_eventq_dflt);
#endif
    ble_hs_deinit();
#if !SOC_ESP_NIMBLE_CONTROLLER || !CONFIG_BT_CONTROLLER_ENABLED
#if CONFIG_NIMBLE_STACK_USE_MEM_POOLS
    npl_freertos_funcs_deinit();
#endif
#endif
    return ESP_OK;
}

/**
 * @brief nimble_port_init - Initialize controller and NimBLE host stack
 *
 * @return esp_err_t
 */
esp_err_t
nimble_port_init(void)
{
    esp_err_t ret;

#if false // Arduino disable
#if CONFIG_IDF_TARGET_ESP32 && CONFIG_BT_CONTROLLER_ENABLED
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
#endif
#if CONFIG_BT_CONTROLLER_ENABLED
    esp_bt_controller_config_t config_opts = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ret = esp_bt_controller_init(&config_opts);
    if (ret != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller init failed\n");
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller enable failed\n");
        return ret;
    }
#endif
#endif // Arduino disable

    ret = esp_nimble_init();
    if (ret != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "nimble host init failed\n");
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief nimble_port_deinit - Deinitialize controller and NimBLE host stack
 *
 * @return esp_err_t
 */

esp_err_t
nimble_port_deinit(void)
{
    esp_err_t ret;

    ret = esp_nimble_deinit();
    if(ret != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "nimble host deinit failed\n");
        return ret;
    }

#if CONFIG_BT_CONTROLLER_ENABLED
    ret = esp_bt_controller_disable();
    if(ret != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller disable failed\n");
        return ret;
    }

    ret = esp_bt_controller_deinit();
    if(ret != ESP_OK) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "controller deinit failed\n");
        return ret;
    }
#endif

    return ESP_OK;
}


int
nimble_port_stop(void)
{
    esp_err_t err = ESP_OK;
    ble_npl_error_t rc;

    rc = ble_npl_sem_init(&ble_hs_stop_sem, 0);

    if( rc != 0) {
        ESP_LOGE(NIMBLE_PORT_LOG_TAG, "sem init failed with reason: %d", rc);
	    return rc;
    }

    /* Initiate a host stop procedure. */
    err = ble_hs_stop(&stop_listener, ble_hs_stop_cb,
                     NULL);
    if (err != 0) {
        ble_npl_sem_deinit(&ble_hs_stop_sem);
        return err;
    }

    /* Wait till the host stop procedure is complete */
    ble_npl_sem_pend(&ble_hs_stop_sem, BLE_NPL_TIME_FOREVER);

    ble_npl_event_init(&ble_hs_ev_stop, nimble_port_stop_cb,
                       NULL);
    ble_npl_eventq_put(&g_eventq_dflt, &ble_hs_ev_stop);

    /* Wait till the event is serviced */
    ble_npl_sem_pend(&ble_hs_stop_sem, BLE_NPL_TIME_FOREVER);

    ble_npl_sem_deinit(&ble_hs_stop_sem);

    ble_npl_event_deinit(&ble_hs_ev_stop);

    return ESP_OK;
}

void
IRAM_ATTR nimble_port_run(void)
{
    struct ble_npl_event *ev;

    while (1) {
        ev = ble_npl_eventq_get(&g_eventq_dflt, BLE_NPL_TIME_FOREVER);
        if (ev) {
            ble_npl_event_run(ev);
            if (ev == &ble_hs_ev_stop) {
                break;
            }
        }
    }
}

struct ble_npl_eventq *
IRAM_ATTR nimble_port_get_dflt_eventq(void)
{
    return &g_eventq_dflt;
}

#else // ESP_PLATFORM

static struct ble_npl_eventq g_eventq_dflt;

extern void os_msys_init(void);
extern void os_mempool_module_init(void);

void
nimble_port_init(void)
{
#if CONFIG_NIMBLE_STACK_USE_MEM_POOLS
    npl_freertos_funcs_init();

    npl_freertos_mempool_init();
#endif

    /* Initialize default event queue */
    ble_npl_eventq_init(&g_eventq_dflt);
    /* Initialize the global memory pool */
    os_mempool_module_init();
    os_msys_init();
    /* Initialize transport */
    ble_transport_init();
    /* Initialize the host */
    ble_transport_hs_init();

#if NIMBLE_CFG_CONTROLLER
    //ble_hci_ram_init();
    hal_timer_init(5, NULL);
    os_cputime_init(32768);
    ble_transport_ll_init();
#endif

#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
    ble_adv_list_init();
#endif
}

void
nimble_port_deinit(void)
{
#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
    ble_adv_list_deinit();
#endif
    ble_npl_eventq_deinit(&g_eventq_dflt);
    ble_hs_deinit();
#if CONFIG_NIMBLE_STACK_USE_MEM_POOLS
    npl_freertos_funcs_deinit();
#endif

}

void
nimble_port_run(void)
{
    struct ble_npl_event *ev;

    while (1) {
        ev = ble_npl_eventq_get(&g_eventq_dflt, BLE_NPL_TIME_FOREVER);
        ble_npl_event_run(ev);
    }
}

struct ble_npl_eventq *
nimble_port_get_dflt_eventq(void)
{
    return &g_eventq_dflt;
}

#if NIMBLE_CFG_CONTROLLER
void
nimble_port_ll_task_func(void *arg)
{
    extern void ble_ll_task(void *);

    ble_ll_task(arg);
}
#endif

#endif // ESP_PLATFORM
