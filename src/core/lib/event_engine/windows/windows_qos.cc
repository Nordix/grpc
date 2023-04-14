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
#include <grpc/support/port_platform.h>

#ifdef GPR_WINDOWS

#include <qos2.h>

#include <grpc/grpc.h>
#include <grpc/support/log.h>

#include "src/core/lib/event_engine/windows/windows_qos.h"
#include "src/core/lib/iomgr/error.h"

namespace grpc_event_engine {
namespace experimental {

namespace {

// Differentiated Services Code Point values
// https://datatracker.ietf.org/doc/html/rfc2474
enum DiffServCodePoint {
  DSCP_NOT_SET = -1,
  DSCP_CS0 = 0,  // Default
  DSCP_CS1 = 8,
  DSCP_CS2 = 16,
  DSCP_CS3 = 24,
  DSCP_CS4 = 32,
  DSCP_CS5 = 40,
  DSCP_CS6 = 48,
  DSCP_CS7 = 56,
};

// Translate DSCP to the TrafficType categories used by Windows QoS subsystem.
// https://learn.microsoft.com/en-us/windows/win32/api/qos2/ne-qos2-qos_traffic_type
QOS_TRAFFIC_TYPE DscpToTrafficType(int dscp) {
  if (dscp >= DSCP_CS1 && dscp < DSCP_CS5) {
    return QOSTrafficTypeBackground;
  } else if (dscp >= DSCP_CS5 && dscp < DSCP_CS7) {
    return QOSTrafficTypeAudioVideo;
  } else if (dscp >= DSCP_CS7) {
    return QOSTrafficTypeControl;
  }
  return QOSTrafficTypeBestEffort;
}

// Adjust configuration values
int AdjustValue(int default_value, int min_value, int max_value,
                absl::optional<int> actual_value) {
  if (!actual_value.has_value() || *actual_value < min_value ||
      *actual_value > max_value) {
    return default_value;
  }
  return *actual_value;
}

}  //  namespace

void SetDscpIfConfigured(SOCKET socket,
                         const EventEngine::ResolvedAddress& address,
                         const EndpointConfig& config) {
  int dscp = AdjustValue(DSCP_NOT_SET, 0, 63, config.GetInt(GRPC_ARG_DSCP));
  if (dscp == DSCP_NOT_SET) return;

  // Get a handle to Windows QoS subsystem
  HANDLE handle = nullptr;
  QOS_VERSION version;
  version.MajorVersion = 1;
  version.MinorVersion = 0;
  if (QOSCreateHandle(&version, &handle) == 0) {
    gpr_log(GPR_ERROR, "%s",
            GRPC_WSA_ERROR(WSAGetLastError(), "QOSCreateHandle")
                .ToString()
                .c_str());
    return;
  }

  // Add socket to the traffic flow that gives a DSCP close
  // to the requested value.
  QOS_FLOWID flowId = 0;
  QOS_TRAFFIC_TYPE trafficType = DscpToTrafficType(dscp);
  int result = QOSAddSocketToFlow(handle, socket,
                                  const_cast<sockaddr*>(address.address()),
                                  trafficType, QOS_NON_ADAPTIVE_FLOW, &flowId);
  if (result == 0) {
    // This will fail with `invalid parameter` if address is "on-link", i.e
    // localhost or single IP subnet. Use a lower severity in these cases.
    DWORD err = WSAGetLastError();
    if (err == ERROR_INVALID_PARAMETER) {
      gpr_log(GPR_DEBUG, "%s",
              GRPC_WSA_ERROR(err, "QOSAddSocketToFlow").ToString().c_str());
    } else {
      gpr_log(GPR_ERROR, "%s",
              GRPC_WSA_ERROR(err, "QOSAddSocketToFlow").ToString().c_str());
    }
    return;
  }

  // Attempt to set the exact value of the outgoing DSCP marking.
  // This requires being member of the Administrators or the Network
  // Configuration Operators group.
  DWORD buf = dscp;
  result = QOSSetFlow(handle, flowId, QOSSetOutgoingDSCPValue, sizeof(buf),
                      &buf, 0, nullptr);
  if (result == 0) {
    gpr_log(GPR_DEBUG, "%s",
            GRPC_WSA_ERROR(WSAGetLastError(), "QOSSetFlow").ToString().c_str());
    return;
  }
}

}  // namespace experimental
}  // namespace grpc_event_engine

#endif  // GPR_WINDOWS
