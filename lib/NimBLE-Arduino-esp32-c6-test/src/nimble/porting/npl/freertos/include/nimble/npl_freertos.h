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

#ifndef _NPL_FREERTOS_H_
#define _NPL_FREERTOS_H_

#include "nimble/porting/nimble/include/syscfg/syscfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_NIMBLE_STACK_USE_MEM_POOLS
typedef void ble_npl_event_fn(struct ble_npl_event *ev);

struct ble_npl_event_freertos {
    bool queued;
    ble_npl_event_fn *fn;
    void *arg;
};

struct ble_npl_eventq_freertos {
    QueueHandle_t q;
};

struct ble_npl_callout_freertos {
#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
   esp_timer_handle_t handle;
#else
    TimerHandle_t handle;
#endif
    struct ble_npl_eventq *evq;
    struct ble_npl_event ev;
};

struct ble_npl_mutex_freertos {
    SemaphoreHandle_t handle;
};

struct ble_npl_sem_freertos {
    SemaphoreHandle_t handle;
};

typedef void ble_npl_event_fn_freertos(struct ble_npl_event_freertos *ev);
#endif

struct ble_npl_eventq *npl_freertos_eventq_dflt_get(void);

struct ble_npl_event *npl_freertos_eventq_get_(struct ble_npl_eventq *evq,
                                              ble_npl_time_t tmo);

void npl_freertos_eventq_put_(struct ble_npl_eventq *evq,
                             struct ble_npl_event *ev);

void npl_freertos_eventq_remove_(struct ble_npl_eventq *evq,
                                struct ble_npl_event *ev);

ble_npl_error_t npl_freertos_mutex_init_(struct ble_npl_mutex *mu);
ble_npl_error_t npl_freertos_mutex_deinit_(struct ble_npl_mutex *mu);

ble_npl_error_t npl_freertos_mutex_pend_(struct ble_npl_mutex *mu,
                                        ble_npl_time_t timeout);

ble_npl_error_t npl_freertos_mutex_release_(struct ble_npl_mutex *mu);

ble_npl_error_t npl_freertos_sem_init_(struct ble_npl_sem *sem, uint16_t tokens);
ble_npl_error_t npl_freertos_sem_deinit_(struct ble_npl_sem *sem);

ble_npl_error_t npl_freertos_sem_pend_(struct ble_npl_sem *sem,
                                      ble_npl_time_t timeout);

ble_npl_error_t npl_freertos_sem_release_(struct ble_npl_sem *sem);

int npl_freertos_callout_init_(struct ble_npl_callout *co,
                               struct ble_npl_eventq *evq,
                               ble_npl_event_fn *ev_cb, void *ev_arg);

void npl_freertos_callout_deinit_(struct ble_npl_callout *co);

void npl_freertos_callout_stop_(struct ble_npl_callout *co);

bool npl_freertos_callout_is_active_(struct ble_npl_callout *co);

ble_npl_time_t npl_freertos_callout_get_ticks_(struct ble_npl_callout *co);

ble_npl_error_t npl_freertos_callout_reset_(struct ble_npl_callout *co,
                                           ble_npl_time_t ticks);

ble_npl_time_t npl_freertos_callout_remaining_ticks_(struct ble_npl_callout *co,
						    ble_npl_time_t now);

ble_npl_error_t npl_freertos_time_ms_to_ticks_(uint32_t ms,
                                              ble_npl_time_t *out_ticks);

ble_npl_error_t npl_freertos_time_ticks_to_ms_(ble_npl_time_t ticks,
                                              uint32_t *out_ms);

void npl_freertos_hw_set_isr(int irqn, void (*addr)(void));

uint32_t npl_freertos_hw_enter_critical(void);

void npl_freertos_hw_exit_critical(uint32_t ctx);

#ifdef __cplusplus
}
#endif

#endif  /* _NPL_FREERTOS_H_ */
