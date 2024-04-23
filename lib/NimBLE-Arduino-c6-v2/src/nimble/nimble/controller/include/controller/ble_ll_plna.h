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

#ifndef H_BLE_LL_PLNA_
#define H_BLE_LL_PLNA_

#ifdef __cplusplus
extern "C" {
#endif

#include "nimble/porting/nimble/include/syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_LL_PA)
void ble_ll_plna_pa_init(void);
void ble_ll_plna_pa_enable(void);
void ble_ll_plna_pa_disable(void);
#endif

#if MYNEWT_VAL(BLE_LL_LNA)
void ble_ll_plna_lna_init(void);
void ble_ll_plna_lna_enable(void);
void ble_ll_plna_lna_disable(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_PLNA_ */
