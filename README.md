# esp32-pelco-ptz

Control an analogue Pelco-D PTZ camera (e.g. **Pelco Esprit ES30C**) from a phone or
browser. An ESP32 serves a control page over Wi-Fi and sends Pelco-D commands over RS-485.
Video still goes over coax to the XVR; this project only adds pan/tilt/zoom control.

## Parts

- ESP32 dev board (classic ESP32, S3 or C3)
- **3.3 V RS-485 transceiver module** (MAX3485 / SP3485). Not a CAN transceiver such as
  TJA1050. Avoid 5 V MAX485 modules unless RO is level-shifted.
- Twisted-pair cable to the camera (e.g. one Cat5e pair), up to ~1200 m at 2400 baud

## Wiring

| ESP32 (esp32dev) | RS-485 module | Camera |
|---|---|---|
| GPIO17 (TX) | DI | |
| GPIO16 (RX) | RO | |
| GPIO4 | DE + RE (bridged) | |
| 3.3 V / GND | VCC / GND | |
| | A | RX+ / Data+ |
| | B | RX− / Data− |

S3 pins: TX 17, RX 18, DE 8. C3 pins: TX 5, RX 4, DE 6. Change them in `platformio.ini`.
For auto-direction modules without a DE pin, set `-DPTZ_DE_PIN=-1`.

Leave the camera's **TX+ / TX−** unconnected: they only carry replies for full-duplex
RS-422 controllers, and this firmware only sends commands.

If the camera does not respond, swap A and B. On long runs, enable the camera's 120 Ω
termination.

## Camera settings

Set the camera's DIP switches to **Pelco-D**, **2400 baud**, **address 1** (see the camera
manual). If you use other values, change `PTZ_BAUD` / `PTZ_ADDRESS` in `platformio.ini`.

## Setup

```bash
cp include/secrets.example.h include/secrets.h   # then edit Wi-Fi + web login
```

The build fails until `WEB_PASSWORD` is changed from `change-me` and has at least 8 characters.

```bash
pio run -e esp32dev -t upload
pio device monitor -e esp32dev                    # prints the IP address
```

Open `http://ptz.local` or the printed IP address and log in with `WEB_USER` / `WEB_PASSWORD`.

## Use

- Hold an arrow, zoom, focus or iris button to move; release to stop.
- Presets 1–8 move to a saved position. Tick **Save current position** and tap a preset
  to store it. Some Pelco cameras reserve certain preset numbers for special functions.
- Safety: the ESP32 stops the camera if the page stops sending keep-alives for 0.8 s
  (closed tab, lost Wi-Fi).

## Remote access

Do **not** port-forward the ESP32 to the internet. The page uses HTTP basic auth without
TLS. Use a VPN instead (e.g. Tailscale on a home device or WireGuard on the router), then
open the ESP32's local address from your phone.

## HTTP API

All control endpoints require basic auth and `POST` with form-encoded parameters.

| Endpoint | Parameters |
|---|---|
| `POST /api/move` | `pan`, `tilt`, `zoom`, `focus`, `iris` (-1/0/1), `speed` (1–63) |
| `POST /api/stop` | |
| `POST /api/preset` | `action` = `go` / `set` / `clear`, `id` (1–255) |
| `GET /api/status` | |

## Tests

```bash
pio test -e native
```
