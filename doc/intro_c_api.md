# IBM Watson™ IoT Platform C Client Developer's Guide
## For _NXP i_._MX_ Platform with IC A71CH Secure Element

This document describes the C interface for communicating with the IBM Watson Internet of Things Platform
from NXP IC A71CH Secure Element enabled devices and gateways.

Supported Features
------------------

| Feature   |      Supported?      |	Description	       |
|----------|:--------------------:|:--------------------------:|
| [Device connectivity](./device_c_api.md) |  &#10004; | Connect your device(s) to Watson IoT Platform with ease using this library. For more information, refer to github documentation below.|
| [Gateway connectivity](./gateway_c_api.md) |    &#10004;   | Connect your gateway(s) to Watson IoT Platform with ease using this library. Refer to github documentation below for more details.
| SSL/TLS | &#10004; | Connects your the devices, and gateways securely to Watson IoT Platform. Port 8883 support secure connections using TLS with the MQTT.
| Client side Certificate based authentication | &#10004; | Supports Client Side Certificates stored in NXP IC A71CH Secure Element.
| Device Management for Devices | &#10008; | Connects your device(s) as managed device(s) to Watson IoT Platform. To be added in a future release.
| Device Management for Gateway | &#10008; | Connects your gateway(s) as managed gateway(s) to Watson IoT Platform. To be added in a future release.
| Device Management Extension(DME) | &#10008; | Provides support for custom device management actions. To be added in a future release.
| Auto reconnect | &#10004; | Enables device/gateway/application to automatically reconnect to Watson IoT Platform while they are in a disconnected state.
| Event/Command publish using MQTT| &#10004; | Enables device/gateway/application to publish messages using MQTT. Refer to github documentation below for more details.


