# ESP32 Pelco PTZ Controller

Pan, tilt and zoom an analogue Pelco-D PTZ camera from a phone or browser,
for DVRs/XVRs without an RS-485 port. Built for the Pelco Esprit ES30C.

``` text
Phone ─Wi-Fi─► ESP32 ─UART─► RS-485 module ─twisted pair─► Pelco PTZ camera
                                                                  │
                                                      coax video ─┴─► DVR / XVR
```

Firmware **1.0**. Video is unaffected; this adds control only.

## Hardware

-   ESP32 DevKit (classic ESP32, S3 or C3)
-   3.3 V RS-485 module (MAX3485 / SP3485), not a CAN transceiver (TJA1050)
-   Twisted pair to the camera (one Cat5e pair), up to ~1200 m at 2400 baud

| From | To | Note |
|---|---|---|
| ESP32 GPIO17 | RS-485 `DI` | UART TX |
| ESP32 GPIO16 | RS-485 `RO` | UART RX |
| ESP32 GPIO4 | RS-485 `DE` + `RE` | Bridged; direction control |
| ESP32 `3V3` / `GND` | RS-485 `VCC` / `GND` | |
| RS-485 `A` | Camera `RX+` | Data+ |
| RS-485 `B` | Camera `RX−` | Data− |

-   Leave the camera's `TX+`/`TX−` unconnected; the firmware only sends.
-   Camera not responding: swap `A` and `B`.
-   Long runs: enable the camera's 120 Ω termination.
-   Avoid 5 V MAX485 modules unless `RO` is level-shifted.
-   Auto-direction modules without `DE`: set `-DPTZ_DE_PIN=-1`.

## Setup

1.  Camera DIP switches: Pelco-D, 2400 baud, address 1
    (otherwise change `PTZ_BAUD`/`PTZ_ADDRESS` in `platformio.ini`).
2.  Copy `include/secrets.example.h` to `include/secrets.h`, set Wi-Fi and
    web login. The build fails while `WEB_PASSWORD` is `change-me` or
    shorter than 8 characters.
3.  Flash `esp32dev` ([Build](#build)), open `http://ptz.local` or the IP
    printed on the serial monitor.

## Controls

| Control | Action |
|---|---|
| Arrows, zoom, focus, iris | Hold to move, release to stop |
| Speed | Pan/tilt speed 1–63 |
| Presets 1–8 | Tap to go; tick "Save current position" to store |
| STOP | Stop everything |

The camera stops if the page goes quiet for 0.8 s (closed tab, lost Wi-Fi).
Some Pelco cameras reserve certain preset numbers.

## Remote access

Use a VPN (Tailscale, WireGuard). Do not port-forward: the page is HTTP
basic auth without TLS.

## API

Basic auth; control endpoints are form-encoded `POST`.

| Endpoint | Parameters |
|---|---|
| `POST /api/move` | `pan`/`tilt`/`zoom`/`focus`/`iris` = `-1`/`0`/`1`, `speed` = `1`–`63` |
| `POST /api/stop` | |
| `POST /api/preset` | `action` = `go`/`set`/`clear`, `id` = `1`–`255` |
| `GET /api/status` | Address, baud, moving, Wi-Fi RSSI |

``` sh
curl -u admin -d "pan=1&speed=20" http://ptz.local/api/move
```

A move lasts 0.8 s unless repeated; the page resends every 250 ms.

## Build

``` sh
pip install platformio          # macOS: brew install platformio
cp include/secrets.example.h include/secrets.h
pio run -e esp32dev -t upload
pio device monitor -e esp32dev
pio test -e native              # Pelco-D unit tests
```

| Env | Pins (TX / RX / DE) |
|---|---|
| `esp32dev` (default) | GPIO17 / GPIO16 / GPIO4 |
| `esp32s3` | GPIO17 / GPIO18 / GPIO8 |
| `esp32c3` | GPIO5 / GPIO4 / GPIO6 |

## Troubleshooting

| Symptom | Fix |
|---|---|
| Camera does not move | Swap `A`/`B`; check DIP switches match address, protocol and baud |
| Serial shows `TX:` frames, nothing happens | Check RS-485 module power, `DE` wiring and termination |
| Moves jerkily or stops early | Weak Wi-Fi; check `rssi` at `/api/status` |
| `ptz.local` not found | Use the IP from the serial monitor; some Android devices lack mDNS |
| Build error about `WEB_PASSWORD` | Set a password of 8+ characters in `include/secrets.h` |

## Roadmap

-   HTTPS and OTA updates
-   Live video on the control page

## Changelog

-   **1.0**: Web control, presets, fail-safe stop, ESP32/S3/C3.

## References

-   Pelco-D protocol: 7-byte frames `FF addr cmd1 cmd2 data1 data2 checksum`
    at 2400 baud 8N1 (`src/pelco_d.cpp`).
-   Pelco Esprit ES30C installation manual: DIP switches and RS-422/485 wiring.
