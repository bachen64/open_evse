# OpenEVSE — System Overview & Developer Reference

OpenEVSE is an open-source electric vehicle supply equipment (EVSE) platform consisting of three tightly integrated components: an AVR-based charging controller, an ESP32 WiFi gateway, and a Svelte web interface. Together they deliver a fully featured, locally hosted EV charging station with cloud integration, energy management, and smart-home connectivity.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                   Web Browser / App                 │
│            openevse-gui-v2  (Svelte / Vite)         │
└────────────────────┬────────────────────────────────┘
                     │  HTTP REST / WebSocket
┌────────────────────▼────────────────────────────────┐
│          ESP32 WiFi Gateway Firmware                │
│    openevse_esp32_firmware  (C / C++ / ESP-IDF)     │
│  WiFi · Ethernet · MQTT · OCPP · Home Assistant     │
└────────────────────┬────────────────────────────────┘
                     │  Serial RAPI API (UART) 115200
┌────────────────────▼────────────────────────────────┐
│      OpenEVSE EV Safety Controller Firmware         │
│      open_evse  (C++, ATmega328P/ARM SAMD21)        │
│   J1772 · GMI/GFI · Relay · Pilot · Proximity       │
└─────────────────────────────────────────────────────┘
```
---

## 1. OpenEVSE Controller Firmware

**Repository:** <https://github.com/OpenEVSE/open_evse>  
**MCU:** ATmega328P / ATSAMD21G18  
**Language:** C++ (83.8%), Roff  
**Standard:** SAE J1772 (Level 1 & Level 2)

### Key Capabilities

- GFI (Ground Fault Interrupter) protection
- Pilot signal generation and state machine (A/B/C/D/E/F)
- Relay control with stuck-relay and no-ground fault detection
- Time-based scheduled charging
- Display with 3.5" TFT or Text RGB backlight
- Temperature monitoring (MCP9808)
- Ammeter with calibration
- Charge time and energy limits
- Heartbeat supervision for gateway watchdog
- Mennekes lock control (EU Type 2)

### Build & Programming

- Pre-compiled binaries available on GitHub Releases
- Compile with VSCode / PlatformIO using bundled library versions
- Flash via `avrdude` with USBASP or compatible ISP programmer
- Hardware definitions in `/boards/`, firmware source in `/firmware/open_evse/`

---

## 2. RAPI Protocol

The Remote API (RAPI) is a serial ASCII protocol used by the ESP32 gateway (and any host) to control and query the OpenEVSE controller over UART.

**Format:** `$<cmd> [args...]*<checksum>\r\n`  
**Response:** `$OK [values...]` or `$NK` (not OK)

### Function Commands

| Command | Description | Parameters |
|---|---|---|
| `FD` | Disable EVSE | — |
| `FE` | Enable EVSE | — |
| `FS` | Sleep EVSE | — |
| `FR` | Restart EVSE | — |
| `F0` | Enable/disable display updates | `1`=enable, `0`=disable |
| `F1` | Simulate front-panel button press | — |
| `FB` | Set LCD backlight color | `0`=OFF `1`=RED `2`=GREEN `3`=YELLOW `4`=BLUE `5`=VIOLET `6`=TEAL `7`=WHITE |
| `FP` | Print text on LCD | x, y, text |
| `FF` | Enable/disable feature flag | feature_id (`B/D/E/F/G/R/T/V`), `0`/`1` |

### Set Commands

| Command | Description | Parameters |
|---|---|---|
| `SC` | Set current capacity | amps, `[V\|M]` |
| `SL` | Set service level | `1`=L1, `2`=L2, `A`=Auto |
| `ST` | Set timer | starthr, startmin, endhr, endmin |
| `S3` | Set charge time limit | cnt × 15 min |
| `SH` | Set charge limit | kWh |
| `SK` | Set accumulated Wh | value |
| `SV` | Set voltage for power calc | millivolts |
| `SA` | Set ammeter calibration | currentscalefactor, currentoffset |
| `SM` | Set voltmeter calibration | voltscalefactor, voltoffset |
| `S1` | Set RTC clock | yr, mo, day, hr, min, sec |
| `S2` | Enable/disable ammeter cal mode | `0`/`1` |
| `S4` | Set auth lock | `0`=unlocked, `1`=locked |
| `S5` | Mennekes lock setting | `A`=auto, `M`=manual, `0`=unlock, `1`=lock |
| `SB` | Clear boot lock | — |
| `SY` | Set heartbeat supervision | interval, currentlimit |
| `S0` | Set LCD type | `0`=monochrome, `1`=RGB |

### Get Commands

| Command | Description | Response |
|---|---|---|
| `GS` | Get EVSE state | evsestate, elapsed, pilotstate, vflags |
| `GG` | Get charging current & voltage | milliamps, millivolts |
| `GU` | Get energy usage | Wattseconds, Whacc |
| `GC` | Get current capacity info | minamps, hmaxamps, pilotamps, cmaxamps |
| `GE` | Get settings | amps, flags |
| `GV` | Get firmware & protocol version | firmware_version, protocol_version |
| `GT` | Get RTC time | yr, mo, day, hr, min, sec |
| `GP` | Get temperature sensors | ds3231temp, mcp9808temp, tmp007temp |
| `GO` | Get overtemperature thresholds | ambientthresh, irthresh |
| `GH` | Get charge limit | kWh |
| `G3` | Get charge time limit | cnt |
| `GD` | Get delay timer | starthr, startmin, endhr, endmin |
| `GF` | Get fault counters | gfitripcnt, nogndtripcnt, stuckrelaytripcnt |
| `GA` | Get ammeter settings | currentscalefactor, currentoffset |
| `GM` | Get voltmeter settings | voltscalefactor, voltoffset |
| `G0` | Get EV connect state | connectstate (`0`/`1`/`2`) |
| `G4` | Get auth lock state | lockstate |
| `G5` | Get Mennekes settings | state, mode |
| `GI` | Get MCU ID | mcuid |
| `GY` | Get heartbeat supervision status | interval, currentlimit, trigger |

---

## 3. ESP32 WiFi Gateway Firmware

**Repository:** <https://github.com/OpenEVSE/openevse_esp32_firmware>  
**MCU:** ESP32  
**Language:** C (73%), C++ (24.7%)  
**License:** GPL v3  
**Latest Release:** v5.1.5 (October 2025)  
**Minimum OpenEVSE firmware:** v7.1.3

### Connectivity

- 802.11 b/g/n WiFi (AP + STA modes)
- Wired Ethernet via ESP32 Gateway board
- UART to OpenEVSE controller (RAPI)

### Features

| Feature | Status |
|---|---|
| Web UI control (start/pause, current, schedules, limits) | Stable |
| MQTT status & control | Stable |
| Emoncms logging (`data.openevse.com` or self-hosted) | Stable |
| Eco / Solar divert mode | Stable |
| Power shaper (grid capacity management) | Stable |
| OTA firmware updates | Stable |
| OCPP v1.6J | Stable |
| Home Assistant integration | Stable |
| Tesla vehicle API | Available |

### REST API

Base URL: `http://<device-ip>` (default hostname: `openevse.local`)

#### Status

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/status` | Current EVSE operational state |
| `POST` | `/status` | Push external data (voltage, power, solar, grid, battery) |
| `GET` | `/ws` | WebSocket — real-time state stream |

#### Configuration

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/config` | Read WiFi/EVSE settings |
| `POST` | `/config` | Write config (EmonCMS, solar divert, charging mode, Tesla credentials) |

#### Manual Override

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/override` | Read current override |
| `POST` | `/override` | Set override with custom parameters |
| `PATCH` | `/override` | Toggle override state |
| `DELETE` | `/override` | Clear override |

#### Claims (multi-client current arbitration)

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/claims` | List all client claims |
| `GET` | `/claims/{client}` | Get specific client claim |
| `POST` | `/claims/{client}` | Create/update claim |
| `DELETE` | `/claims/{client}` | Release claim |

#### Scheduling

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/schedule` | List all schedule events |
| `POST` | `/schedule` | Batch update events |
| `GET` | `/schedule/{id}` | Get event by ID |
| `POST` | `/schedule/{id}` | Update event |
| `DELETE` | `/schedule/{id}` | Remove event |
| `GET` | `/schedule/plan` | Planned events by day |

#### Limits

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/limit` | Get active charge limit |
| `POST` | `/limit` | Set charge limit |
| `DELETE` | `/limit` | Clear charge limit |

#### System

| Method | Endpoint | Description |
|---|---|---|
| `POST` | `/restart` | Restart gateway or EVSE (`device` param) |
| `GET` | `/time` | Get current time and timezone offset |
| `POST` | `/time` | Set time and timezone |
| `DELETE` | `/emeter` | Reset energy meter |
| `GET` | `/logs` | Log block index |
| `GET` | `/logs/{index}` | Fetch log events for block |

#### Tesla

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/tesla/vehicles` | List vehicles linked to Tesla account |

#### Certificates

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/certificates` | List uploaded TLS certificates |
| `POST` | `/certificates` | Upload certificate |
| `GET` | `/certificates/{id}` | Get certificate by ID |
| `DELETE` | `/certificates/{id}` | Delete certificate |

### Full API Spec

OpenAPI / Swagger specification: `api.yml` in the firmware repository root.  
Interactive docs: <https://openevse.stoplight.io/docs/openevse-wifi>

---

## 4. Web Interface (openevse-gui-v2)

**Repository:** <https://github.com/OpenEVSE/openevse-gui-v2>  
**License:** BSD-2-Clause  
**Stack:** Svelte, Vite, Bulma CSS  
**Language:** Svelte (54.9%), JavaScript (44.2%)

### Features

- Dashboard: live state, current draw, energy session
- Start / pause charging
- Current capacity slider
- Scheduler: create, edit, and delete timed charging events
- Eco/Solar divert mode controls
- System configuration (WiFi, MQTT, EmonCMS, limits)
- Internationalization (i18n)

### Development

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/OpenEVSE/openevse-gui-v2.git
cd openevse-gui-v2

npm install

# Set target device (defaults to openevse.local)
export VITE_OPENEVSEHOST=192.168.x.x

npm run dev        # Dev server on :5173 — proxies API to VITE_OPENEVSEHOST
npm run build      # Production build → dist/
```

---

## 5. Integration Notes

### MQTT

The ESP32 gateway publishes EVSE state and subscribes to control topics. Configure broker, username, password, and base topic via `/config` or the web UI. Commonly used with Home Assistant MQTT discovery.

### Emoncms

Data can be logged to `enoncms.org` or a self-hosted Emoncms instance. Set `emoncms_apikey`, `emoncms_server`, and `emoncms_node` in `/config`.

### Eco / Solar Divert Mode

The gateway accepts real-time solar generation and grid import data via `POST /status` (fields: `solar`, `grid_ie`, `voltage`) and adjusts the pilot current to charge only from surplus renewable energy.

### Home Assistant

Beta native integration. Alternatively, use MQTT with HA's MQTT integration. The gateway's auto-discovery payloads populate entities automatically.

### OCPP v1.6

Beta support for communicating with a central management system (CMS/CSMS). Configure OCPP server URL and identity via `/config`.

---

## 6. Hardware References

| Component | Link |
|---|---|
| OpenEVSE Plus v5 Controller | <https://github.com/OpenEVSE/OpenEVSE_PLUS> |
| ESP32 WiFi Kit | <https://www.openevse.com> |
| OpenEVSE Hardware Repo | <https://github.com/OpenEVSE/OpenEVSE_PLUS> |

---

## 7. Useful Links

| Resource | URL |
|---|---|
| Main firmware repo | <https://github.com/OpenEVSE/open_evse> |
| ESP32 gateway firmware | <https://github.com/OpenEVSE/openevse_esp32_firmware> |
| Web interface | <https://github.com/OpenEVSE/openevse-gui-v2> |
| Interactive API docs | <https://openevse.stoplight.io/docs/openevse-wifi> |
| OpenEnergyMonitor | <https://openenergymonitor.org> |

Tip Jar: Lincomatic developed/maintains this firmware on a volunteer basis. Any donation, no matter how small, is greatly appreciated.

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.me/lincomatic)

```text
Open EVSE is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

Open EVSE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Open EVSE; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

* Open EVSE is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
