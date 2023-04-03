# Quality of Service (QoS) using Differentiated services

Differentiated services or DiffServ is a mechanism for classifying network traffic and providing quality of service on IP networks.
DiffServ uses dedicated fields in the IP header for packet classification purposes.
By marking outgoing packets using a Differentiated Services Code Point (DSCP) the network can prioritize accordingly.

The DSCP value on outgoing packets is controlled by the following channel argument:

* **GRPC_ARG_DSCP**
  * This channel argument accepts integer values 0 to 63. See [dscp-registry](https://www.iana.org/assignments/dscp-registry/dscp-registry.xhtml) for details.
  * Default value is to use system default, i.e. not set.

## Platform specifics

### Windows

To set DSCP values the Windows QOS subsystem is used, which is provided by its qWave library.
The behavior of this library is a bit different compared to a POSIX socket using socket options.
The subsystem provides two alternative ways to set the DSCP value, which both are used **best-effort** by gRPC.

* DSCP categorized into [traffic types](https://learn.microsoft.com/en-us/windows/win32/api/qos2/ne-qos2-qos_traffic_type).
  The configured DSCP value will be categorized into a traffic type with optimal DCSP marking.
  The qWave library provides traffic types that results in DSCP 0, 8, 40 and 56.
  E.g. a configured value of 40 to 55 will result in 40, and a value of 56 to 63 results in 56.

* Exact DSCP value.
  The configured value is set on outgoing packets.
  This requires the application to be run as a member of the `Administrators` or the `Network Configuration Operators` group.

Another note is that only packets leaving the network interface card will get a DSCP marking, i.e. localhost communication will not be marked.
See [link](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/qos/introduction-to-qos2--qwave-) for more information.

#### Developer information

According to the API information on [QOSAddSocketToFlow()](https://learn.microsoft.com/en-us/windows/win32/api/qos2/nf-qos2-qosaddsockettoflow#return-value)
the given destination address would not support mixed IPv4/IPv6, resulting in return code `ERROR_INVALID_PARAMETER`.
Since gRPC uses IPv6 sockets and IPv4-mapped IPv6 adresses that would be a problem.
This can be remedied by adding the socket to the flow **after** the connection is accepted instead of just after `bind()` which is normally required.
