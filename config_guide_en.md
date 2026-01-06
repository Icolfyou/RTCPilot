# RTCPilot Configuration Guide

This document describes the configuration options found in `RTCPilot/RTCPilot/config.yaml`. Restart the service after making changes.

## Logging (`log`)
- `log_level`: Log level. One of `debug`, `info`, `warn`, `error`. Use `debug` for development troubleshooting.
- `log_path`: File path for log output, e.g. `server0.log`.
- (Note: `log_console` was previously available to enable console logging; it may be omitted depending on your configuration.)

## WebSocket server (`websocket_server`)
- `listen_ip`: IP address to bind to (e.g. `0.0.0.0` to listen on all interfaces).
- `port`: Port for WebSocket (for example `7443`).

## WebRTC candidates / network interfaces (`candidates`)
`candidates` is a list of network endpoints used for RTP/UDP traffic. Each entry contains:
- `nettype`: Network type (e.g. `udp`).
- `candidate_ip`: The IP presented to peers (often a public IP or NAT mapping).
- `listen_ip`: Local binding IP (the server's actual bind address, often `0.0.0.0` or a private address).
- `port`: Port number for the candidate (RTP/UDP port).

Use case: set `candidate_ip` to your public IP when the server is behind NAT, and `listen_ip` to the local bind address.

## Certificates (`cert_path` / `key_path`)
- `cert_path`: Path to TLS/SSL certificate file (used for secure WebSocket and DTLS).
- `key_path`: Path to the private key file. Keep the key secure and with proper filesystem permissions.

## Packet loss injection (`downlink_discard_percent` / `uplink_discard_percent`)
- `downlink_discard_percent`: Percentage of downlink packets to drop (for testing), default `0`.
- `uplink_discard_percent`: Percentage of uplink packets to drop (for testing), default `0`.

## Cluster center (`pilot_center`)
- `enable`: Enable communication with the `pilot_center` service (`true`/`false`).
- `host`: `pilot_center` hostname or IP.
- `port`: `pilot_center` port.
- `subpath`: Path prefix used when registering/reporting (e.g. `/pilot/center`).

When enabled, the SFU registers to `pilot_center` for discovery and information forwarding. Ensure `pilot_center` is reachable and started according to `pilot_center/requirements.txt`.

## RTC relay (`rtc_relay`)
- `relay_server_ip`: Relay server IP for RTP relay scenarios.
- `relay_udp_start` / `relay_udp_end`: UDP port range used by the relay for forwarding.
- `send_discard_percent` / `recv_discard_percent`: Packet drop percentages for relay send/receive (for testing).

## Recommendations
- Restart the SFU after changing configuration files.
- Use `info` or `warn` for `log_level` in production, and keep console logging disabled if logs are handled by a file or external aggregator.
- Protect the private key (`key_path`) with proper file permissions.
- When deploying behind NAT, set `candidate_ip` to the public/mapped IP and `listen_ip` to the local bind address.
- Use packet discard fields only in controlled test environments to avoid affecting production users.

If you need example deployment configurations (production, NAT, or `pilot_center` startup steps), tell me which scenario and I will provide a sample.
