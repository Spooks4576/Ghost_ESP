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

#ifndef H_NIMBLE_TRANSPORT_IMPL_
#define H_NIMBLE_TRANSPORT_IMPL_

#ifdef __cplusplus
extern "C" {
#endif

#include "nimble/porting/nimble/include/os/os_mempool.h"
#include "nimble/porting/nimble/include/syscfg/syscfg.h"

/* Init functions to be implemented for transport acting as HS/LL side */
extern void ble_transport_ll_init(void);
extern void ble_transport_hs_init(void);

/* APIs to be implemented by HS/LL side of transports */
extern int ble_transport_to_ll_cmd_impl(void *buf);
extern int ble_transport_to_ll_acl_impl(struct os_mbuf *om);
extern int ble_transport_to_hs_evt_impl(void *buf);
extern int ble_transport_to_hs_acl_impl(struct os_mbuf *om);

#if MYNEWT_VAL(BLE_TRANSPORT_INT_FLOW_CTL)
/* To be implemented if transport supports internal flow control between cores */
extern int ble_transport_int_flow_ctl_get(void);
extern void ble_transport_int_flow_ctl_put(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_NIMBLE_TRANSPORT_IMPL_ */
