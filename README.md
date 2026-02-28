# QuaternionRC - ESP32 Remote Controller

This repository is the remote side of my quadcopter project.
The idea is simple: get a reliable wireless link first, then layer flight commands on top of it.

## What this project does right now

### Wi-Fi link setup (`components/wifi/link_layer.c`)
- Initializes NVS, network stack, and event loop with ESP-IDF.
- Connects as a station (`WIFI_MODE_STA`) to the drone access point.
- Tracks connection state through `wifiStatus`.
- Retries connection automatically when the link drops.

### Transport layer (`components/wifi/transport_layer.c`)
- Opens a TCP client connection to the flight controller.
- Uses `recvn()` and `sendn()` helpers to avoid partial read/write issues.
- Runs a basic startup handshake:
  - send `"Hello Drone !"`
  - wait for and verify `"Hello Remote !"`
- Keeps a UDP socket skeleton ready for real-time control packets.

### App flow (`main/main.c`)
- Boots the remote firmware.
- Waits for Wi-Fi connectivity.
- Starts a FreeRTOS transport task.
- Synchronizes startup with semaphores before moving to control logic.

## Project structure

- `main/`: application entry point and startup sequence.
- `components/wifi/link_layer.*`: Wi-Fi connection and status handling.
- `components/wifi/transport_layer.*`: TCP/UDP sockets and handshake logic.

## Build and flash (ESP-IDF)

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

## Planned next steps

- Define a clean packet format for throttle, roll, pitch, and yaw.
- Stream control commands over UDP at a fixed rate.
- Add a robust critical-message path (arming/disarming/failsafe).
- Improve timeout and reconnection behavior.
- Validate the full remote-to-drone protocol with repeatable tests.

## Author

Tarik
