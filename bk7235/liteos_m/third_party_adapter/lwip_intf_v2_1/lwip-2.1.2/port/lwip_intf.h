/*
 * Copyright (C) 2022 Beken Corporation
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __LWIP_INTF_H__
#define __LWIP_INTF_H__

#define LWIP_INTF_DEBUG
#ifdef LWIP_INTF_DEBUG
#define LWIP_INTF_PRT      warning_prf
#define LWIP_INTF_WARN     warning_prf
#define LWIP_INTF_FATAL    fatal_prf
#else
#define LWIP_INTF_PRT      null_prf
#define LWIP_INTF_WARN     null_prf
#define LWIP_INTF_FATAL    null_prf
#endif

#endif
// eof

