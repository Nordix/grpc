// Copyright 2023 The gRPC Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GRPC_SRC_CORE_LIB_IOMGR_QOS_WINDOWS_H
#define GRPC_SRC_CORE_LIB_IOMGR_QOS_WINDOWS_H

#ifdef GRPC_WINSOCK_SOCKET

#include <grpc/support/port_platform.h>

#include <grpc/event_engine/endpoint_config.h>

#include "src/core/lib/iomgr/resolved_address.h"
#include "src/core/lib/iomgr/socket_windows.h"

// Get Differentiated Services Code Point (DSCP) from config
int grpc_qos_get_dscp(
    const grpc_event_engine::experimental::EndpointConfig& config);

// Set the DSCP via Windows QOS subsystem
void grpc_qos_set_dscp(const grpc_winsocket* socket,
                       const grpc_resolved_address* address, int dscp);

#endif

#endif  // GRPC_SRC_CORE_LIB_IOMGR_QOS_WINDOWS_H
