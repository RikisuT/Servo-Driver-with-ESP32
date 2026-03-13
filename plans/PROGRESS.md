# Servo Driver ESP32 — Phase Tracking

> This document tracks implementation progress across all phases.  
> Each phase may be handled by a different agent. Update this doc as you complete work.  
> Reference: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for full specs, [PROTOCOL.md](PROTOCOL.md) for codebase reference, [servo-driver-ui.jsx](servo-driver-ui.jsx) for the React design prototype.

## ⚠️ Testing Policy — ALL AGENTS READ THIS

**Always have the user test on real hardware between implementation steps.** Do not chain multiple implementation steps without a manual test checkpoint.

After each sub-task or logical group of sub-tasks:
1. Ensure the build succeeds (user runs PlatformIO Build task from VS Code GUI)
2. Propose specific, simple by-hand tests the user can run on the board
3. Wait for the user to report results before proceeding

Tests should be concrete and quick, e.g.:
- "Flash the board, open Serial Monitor, power cycle — confirm you see `Servo 11: STS (range 0-4095)` in the log"
- "Open `http://192.168.4.1/api/scan` in your browser — confirm you see JSON with your servo IDs"
- "Drag the slider to 2048 — confirm the servo physically moves"

Never skip testing to "save time." A 30-second hardware test catches issues that no amount of code review can.

---

## Pre-Work ✅ COMPLETE

- [x] Read all source files in `src/`
- [x] Map HTTP routes, command dispatch, servo library calls
- [x] Document the AT/serial forwarding system
- [x] Identify threading model and thread-safety issues
- [x] Produce [PROTOCOL.md](PROTOCOL.md)

### Key Findings for All Agents

1. **Mixed servo support required:** The user has both SC-series and STS-series servos and may use them interchangeably or simultaneously on the same bus. This is handled by the **nerd-bus-servo** library (see below).
2. **No mutex on servo bus:** `InfoUpdateThreading` reads telemetry while `clientThreading` handles web commands. Both touch `Serial1`. No locking. This mostly works but can cause occasional corrupt packets. The nerd-bus-servo library does not add locking either — we need to handle this.
3. **CalibrationOfs (Set Middle)** — not directly exposed in nerd-bus-servo yet. Can be done via `write_byte()` to the torque_enable register with value 128 for STS servos. Not available for SC servos.
4. **ACC parameter** — STS only. Available via `STSServo::move_to_encoder_angle_with_accel()`. SC servos don't support acceleration.
5. **LOCK register address differs:** SC uses addr 48, STS uses addr 55. The nerd-bus-servo library handles this correctly with `Register::lock_sc` and `Register::lock_sts` plus runtime validation.
6. **MAX_ID = 20** by default. The plan mentions making this configurable.
7. **HTML in PROGMEM** (`WEBPAGE.h`). New UI will be larger — consider LittleFS if >10KB.

### Architecture Decision: Use nerd-bus-servo Library

**Library:** [github.com/berickson/nerd-bus-servo](https://github.com/berickson/nerd-bus-servo)

This is the user's own library that provides a clean abstraction over both SC and STS servo families on the same bus. It replaces the old `lib/SCServo/` library entirely.

**What it provides:**

- **`ServoBusApi`** — Low-level bus protocol with automatic byte-order handling (SC=big-endian, STS=little-endian), type-safe register validation, `std::optional` returns for error handling
- **`Servo`** (base class) — Common operations: `read_encoder_angle()`, `read_speed()`, `read_load()`, `read_voltage()`, `read_temperature()`, `enable_torque()`, `disable_torque()`, `is_torque_enabled()`, `move_to_encoder_angle()`, `move_to_percent()`, PWM/motor mode
- **`Servo::infer_servo_type()`** — Auto-detects SC vs STS per servo by reading angle limits with both byte orders. Also checks MODE register for STS servos in motor/wheel mode (max_angle=0) to prevent misdetection as SC.
- **`SCServo`** — SC-series wrapper (0–1023 range, big-endian)
- **`STSServo`** — STS-series wrapper with extras: `move_to_encoder_angle_with_accel()`, `set_torque_limit()`, `read_torque_limit()`, wheel mode (`enable_wheel_mode()`, `set_wheel_velocity()`)
- **`ServoBusApi::set_servo_id_permanent()`** — Full EEPROM lock/unlock/write/verify flow for ID changes
- **Sync read/write** for multiple servos at once

**Key differences between servo types (unchanged):**

| Property | SC (SCServo) | STS (STSServo) |
|----------|-------------|----------------|
| Byte order | Big endian | Little endian |
| Position range | 0–1023 | 0–4095 |
| Middle position | 511 | 2047 |
| Angle range | 210° | 360° |
| LOCK register | Address 48 | Address 55 |
| MODE register | None (inferred from angle limits=0) | Address 33 |
| Acceleration | Not supported | `move_to_encoder_angle_with_accel()` |
| Torque Limit | Not supported | `set_torque_limit()` / `read_torque_limit()` |
| Wheel mode | Not supported | `enable_wheel_mode()` / `set_wheel_velocity()` |
| Set Middle | Not supported | Write 128 to torque_enable register |

**Integration plan:**

1. **Replace `lib/SCServo/`** with nerd-bus-servo as a PlatformIO lib dependency (github URL in `platformio.ini`)
2. **Rewrite `STSCTRL.h`** to use nerd-bus-servo: one `ServoBusApi` instance on `Serial1`, store `Servo*` pointers (polymorphic — `SCServo*` or `STSServo*`) per detected ID
3. **During scan:** `Ping` each ID, then `Servo::infer_servo_type()` to detect type, then construct the correct subclass
4. **Telemetry loop:** Call `read_encoder_angle()`, `read_load()`, etc. through the `Servo*` pointer — byte order handled automatically
5. **API endpoints** call servo methods directly — no manual byte-order dispatch needed
6. **UI adapts per servo** based on type info returned by API (range, available features)

---

## Phase 1: Card Layout + Position Control — IN PROGRESS

**Goal:** Replace the single-servo button UI with a card-per-servo layout with position sliders and jog buttons.

**Files to modify:**
- `platformio.ini` — add nerd-bus-servo lib dependency, remove old SCServo
- `src/STSCTRL.h` — rewrite to use nerd-bus-servo (`ServoBusApi`, `SCServo`, `STSServo`)
- `src/BOARD_DEV.h` — update scan logic to use `Servo::infer_servo_type()` and store `Servo*` pointers
- `src/CONNECT.h` — add JSON API endpoints, update command handlers to use new servo objects
- `src/WEBPAGE.h` — replace HTML string entirely
- `lib/SCServo/` — remove (replaced by nerd-bus-servo)

**Sub-tasks:**
- [x] **1.0a** Add nerd-bus-servo to `lib/` (cloned from github), remove old SCServo, add C++17 build flags to `platformio.ini`
- [x] **1.0b** Added `set_echo_enabled(bool)` to nerd-bus-servo `ServoBusApi` for Waveshare board TX/RX isolation (no echo)
- [x] **1.0c** Rewrite `STSCTRL.h`: removed old `SCSCL st;`, using `servo_bus` global from nerd-bus-servo, rewrote `getFeedBack()`, `servoInit()`, `setMiddle()`, `setMode()`, `setID()`, `servoStop()`, `servoTorque()`. Added `servoWritePosEx()` / `servoWritePos()` compatibility wrappers.
- [x] **1.0d** Update `CONNECT.h`: replaced all `st.WritePosEx()` / `st.WritePos()` calls with `servoWritePosEx()` / `servoWritePos()` wrappers
- [x] **1.0e** Update `BOARD_DEV.h`: replaced `st.Ping()` with `servo_bus.ping()` (returns `std::optional`)
- [x] **1.0f** Verified compilation: `pio run` succeeds (RAM 15.4%, Flash 64.3%)
- [x] **1.0g** Update `pingAll()` in `BOARD_DEV.h`: after ping, call `Servo::infer_servo_type()` to detect type, construct `SCServo` or `STSServo` per ID. Also: telemetry thread now polls ALL detected servos; task stack sizes bumped to 8KB; `Torque_List` re-indexed by servo ID.
- [x] **1.0h** Add JSON API endpoint: `GET /api/scan` → returns `{"servos": [{"id":11, "type":"STS", "range":4095, "middle":2047}, ...]}`
- [x] **1.0i** Add JSON API endpoint: `GET /api/status_all` → returns per-servo telemetry (pos, speed, load, voltage, temp, current, mode, torque, range)
- [x] **1.0j** Add JSON API endpoints: `GET /api/setpos?id=X&pos=Y&speed=Z` and `GET /api/rescan`
- [x] **1.0k** Verified compilation: `pio run` succeeds (RAM 15.7%, Flash 64.0%)
- [x] **1.0-TEST** Manual hardware test — PASSED 2026-03-11

**Test Results (2026-03-11):** Board at 192.168.86.68 (STA mode). 4 servos detected: STS #5 (range 0–4095), SC #11/#12/#13 (range 0–1003).
- `/api/scan` — correct JSON with IDs, types, ranges, hasCurrent
- `/api/status_all` — telemetry OK (pos, voltage 9.6–9.9V, temp 23–30°C, torque=true)
- `/api/setpos?id=5&pos=2048&speed=500` — servo physically moved, position confirmed at 2048
- `/api/rescan` — returned ok, re-scan found same 4 servos
- **Note:** setpos fails silently under bus contention (telemetry thread). Works reliably when contention is low. Mutex needed eventually.
- [x] **1.0l** Removed legacy `/cmd`, `/readID`, `/readSTS` routes and all supporting functions (`activeCtrl`, `activeID`, `activeSpeed`, `rangeCtrl`, `handleID`, `handleSTS`). Removed unused vars `servotoSet`, `MAX_MIN_OFFSET`. Added `/api/stop` endpoint. Single REST API surface now.
- [x] **1.1** Dark theme, card layout, scan button, card rendering from JS (adapts slider range per servo type) — already implemented in WEBPAGE.h
- [x] **1.2** Position slider + direct text input + jog buttons (servo mode) — already implemented, range adapts to SC 0–1023 or STS 0–4095
- [x] **1.3** Jog-only mode for motor-mode servos — already implemented (large jog buttons, no slider)
- [x] **1.4** Setpoint vs actual display with color coding — already implemented (green=match, orange=diff)
- [x] **1.x-TEST** Manual hardware test of full Phase 1 UI — PASSED 2026-03-11

**Verification:** See IMPLEMENTATION_PLAN.md Phase 1 checklists.

---

## Phase 2: Telemetry + Torque — COMPLETE

All sub-tasks were implemented as part of Phase 1 UI work.

- [x] **2.1** Telemetry row (V, Load, Temp, mA) with orange warning colors (V<6, load>50, temp>50)
- [x] **2.1** "Moving" badge with pulse animation (shown when speed≠0)
- [x] **2.1** Periodic polling via `setInterval(pollStatus, 100)` (100ms)
- [x] **2.2** Torque toggle pill button (green=on, grey=off)
- [x] **2.2** `/api/torque` endpoint (GET, id + enable params)

---

## Phase 3: Quick Actions + Speed + Torque Limit — COMPLETE

**Goal:** Add quick action buttons (Center, Min, Max, Stop, Release) and inline speed/torque-limit fields.

**Files to modify:**
- `src/WEBPAGE.h` — add button row and inline fields to each card
- `src/CONNECT.h` — `/api/stop` already done, added `/api/torque_limit`

**Sub-tasks:**
- [x] **3.1** Quick action button row: Min, Center, Max (Stop/Release removed — torque toggle covers those)
- [x] **3.2** Speed input field per servo (sends with next position command, default 500)
- [x] **3.3** Torque Limit input field (STS only, 0–1023) with orange warning at factory default (1023). Added `/api/torque_limit` endpoint (read+write).
- [x] **3.x-TEST** Manual hardware test — PASSED 2026-03-11. Quick actions move servo correctly, speed field affects move speed, torque limit field reads/writes on STS, hidden on SC.

**Verification:** See IMPLEMENTATION_PLAN.md Phase 3 checklists.

---

## Bus Mutex — COMPLETE

**Goal:** Prevent Serial1 bus contention between telemetry polling and web command threads.

- [x] Added `SemaphoreHandle_t servo_bus_mutex` global, created in `servoInit()`
- [x] Wrapped all `servo_bus` / `Servo*` bus calls in `sts_ctrl.h` (getFeedBack, servoWritePosEx, servoWritePos, setMiddle, setMode, setID, servoStop, servoTorque)
- [x] Wrapped `pingAll()` scan loop in `board_dev.h`
- [x] Wrapped direct servo calls in `connect.h` handlers (stop/motor, torque_limit, angle_limits, set_id)
- [x] Reduced client thread delay from 10ms to 1ms
- [x] Manual hardware test — PASSED 2026-03-11

---

## Phase 4: Servo Naming + Batch Actions — NOT STARTED

**Goal:** Add editable servo names (localStorage) and batch action buttons.

**Files to modify:**
- `src/WEBPAGE.h` — add editable name in card header, batch action bar, localStorage logic

**Sub-tasks:**
- [ ] **4.1** Editable servo name in card header (click to edit, localStorage persistence)
- [ ] **4.2** Batch action bar: "Center All", "Zero All" buttons above cards

**Verification:** See IMPLEMENTATION_PLAN.md Phase 4 checklists.

---

## Phase 5: Config Section (Collapsible) — PARTIALLY COMPLETE

**Goal:** Expandable config section per servo card for EEPROM settings, safety limits, motion, PID, protection, mode, and actions.

**Files to modify:**
- `lib/nerd-bus-servo/servo_bus_api.h` — add missing STS register definitions (see table below)
- `src/WEBPAGE.h` — expand collapsible config UI per card
- `src/CONNECT.h` — add API endpoints: `/api/writereg`, `/api/lockeeprom`, `/api/unlockeeprom`, `/api/setmiddle`

**Sub-tasks:**
- [x] **5.1** Collapsible `▶ CONFIG` container (already in UI)
- [ ] **5.2** EEPROM Lock/Unlock button + warning badge
- [ ] **5.3** Mode settings: Servo/Motor toggle (both), Phase (STS, *needs reg 18*), Offset (STS)
- [x] **5.4a** Angle limit fields with Save button (already in UI, `/api/angle_limits` exists)
- [ ] **5.4b** Capture + Go buttons for angle limits
- [ ] **5.4c** Max Torque field in config (STS only)
- [x] **5.4d** Safety Limits: Max Temp (°C), Min/Max Voltage (V, ×0.1), Max Torque EPROM (/1000). STS only.
- [ ] **5.5** Motion: Acceleration (STS), CW/CCW Dead Zones (both)
- [ ] **5.6** PID Tuning: P/I/D, Speed P/I (STS, *needs regs 21-23, 37, 39*)
- [x] **5.7** Protection: current (0-65535), torque (0-255), time (0-255), overload (0-255). STS only. `/api/safety` endpoint. Lazy-loaded on config open.
- [x] **5.8a** Set New ID (already in UI, `/api/set_id` exists)
- [x] **5.8c** Servo naming — editable display name per servo, stored in ESP32 NVS, shown in card header and config. Names migrate on ID change.
- [ ] **5.8b** Set Middle button (STS only, uses `set_offset()`)

**Library registers to add** (all STS-only, EPROM unless noted):

| Register | Addr | Size | Status |
|----------|------|------|--------|
| Max Temp Limit | 13 | 1 | ✅ Added |
| Max Voltage | 14 | 1 | ✅ Added |
| Min Voltage | 15 | 1 | ✅ Added |
| Max Torque (EPROM) | 16 | 2 | ✅ Added |
| Phase | 18 | 1 | |
| P Coefficient | 21 | 1 | |
| D Coefficient | 22 | 1 | |
| I Coefficient | 23 | 1 | |
| Protection Current | 28 | 2 | ✅ Added |
| Protective Torque | 34 | 1 | ✅ Added |
| Protection Time | 35 | 1 | ✅ Added |
| Overload Torque | 36 | 1 | ✅ Added |
| Speed P | 37 | 1 | |
| Speed I | 39 | 1 | |

**Verification:** See IMPLEMENTATION_PLAN.md Phase 5 checklists.

---

## Phase 6: Check Button — NOT STARTED

**Goal:** Per-card health check that displays configuration warnings.

**Files to modify:**
- `src/WEBPAGE.h` — add Check button and warning banner (client-side logic only)

**Sub-tasks:**
- [ ] **6.1** "Check" button in quick actions row
- [ ] **6.1** Client-side warning checks (ID=1, full-range limits, max torque, no acceleration, EEPROM unlocked, high temp, low voltage)
- [ ] **6.1** Orange warning banner with dismiss button

**Verification:** See IMPLEMENTATION_PLAN.md Phase 6 checklists.

---

## Phase 7: Polish + Mobile Optimization — MOSTLY COMPLETE

**Goal:** Responsive layout, connection status, font cleanup.

**Files to modify:**
- `src/WEBPAGE.h` — responsive CSS, status indicator, system font stacks

**Sub-tasks:**
- [x] **7.1** Responsive dark theme layout (already implemented, max-width ~800px)
- [x] **7.2** Connection status indicator (green/red dot in header, already implemented)
- [x] **7.3** System font stacks only, no Google Fonts (already implemented)
- [ ] **7.x-TEST** Verify full functionality in airplane mode (ESP32 AP only, no internet)

**Verification:** See IMPLEMENTATION_PLAN.md Phase 7 checklists.

---

## Architecture Notes for All Agents

### nerd-bus-servo Library

**Repo:** https://github.com/berickson/nerd-bus-servo  
**Add to `platformio.ini`:**
```ini
lib_deps =
    https://github.com/berickson/nerd-bus-servo.git
    adafruit/Adafruit SSD1306
    adafruit/Adafruit NeoPixel
```

**Key files in the library:**
- `servo_bus_api.h` — low-level bus protocol, register definitions, read/write methods
- `servo.h` — base `Servo` class with `infer_servo_type()`, common read/write, PWM mode
- `sc_servo.h` — `SCServo` subclass (SC-series, big-endian)
- `sts_servo.h` — `STSServo` subclass with acceleration, torque limit, wheel mode

**Usage pattern in firmware:**
```cpp
#include "servo_bus_api.h"
#include "sc_servo.h"
#include "sts_servo.h"

// Global bus API (defined in servo_bus_api.h as `servo_bus`)
// Or create your own:
// ServoBusApi bus;

// Store detected servos as polymorphic pointers
Servo* servos[253] = {nullptr};
uint8_t servoIDs[253];
uint8_t servoCount = 0;

// During scan:
void scanServos() {
  // Clean up old servos
  for(int i = 0; i < servoCount; i++) {
    delete servos[servoIDs[i]];
    servos[servoIDs[i]] = nullptr;
  }
  servoCount = 0;
  
  for(int id = 0; id <= MAX_ID; id++) {
    servo_bus.set_servo_type(ServoBusApi::ServoType::STS); // either works for ping
    auto ping_result = servo_bus.ping(id);
    if(ping_result) {
      auto type = Servo::infer_servo_type(&servo_bus, id);
      if(type) {
        if(*type == ServoBusApi::ServoType::SC) {
          servos[id] = new SCServo(&servo_bus, id);
        } else {
          servos[id] = new STSServo(&servo_bus, id);
        }
        servos[id]->read_info(); // loads min/max encoder angles
        servoIDs[servoCount++] = id;
      }
    }
  }
}

// Reading telemetry:
auto pos = servos[id]->read_encoder_angle();  // returns std::optional<int>
auto voltage = servos[id]->read_voltage();     // returns std::optional<float> (already /10)
auto temp = servos[id]->read_temperature();     // returns std::optional<float>
auto load = servos[id]->read_load();            // returns std::optional<int16_t>

// Moving:
servo_bus.set_servo_type(servos[id]->type());
servo_bus.write_position(id, position, 0, speed);
// Or for STS with acceleration:
((STSServo*)servos[id])->move_to_encoder_angle_with_accel(position, speed, acc);
```

### Adding API Endpoints

Add new routes in `CONNECT.h::webCtrlServer()` before `server.begin()`. Example pattern:

```cpp
server.on("/api/scan", [](){
  String json = "{\"servos\":[";
  for(int i = 0; i < servoCount; i++){
    if(i > 0) json += ",";
    uint8_t id = servoIDs[i];
    bool isSTS = (servos[id]->type() == ServoBusApi::ServoType::STS);
    json += "{\"id\":" + String(id);
    json += ",\"type\":\"" + String(isSTS ? "STS" : "SC") + "\"";
    json += ",\"range\":" + String(servos[id]->max_encoder_angle());
    json += ",\"middle\":" + String(servos[id]->max_encoder_angle() / 2);
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
});
```

### Serving the HTML

The HTML is a PROGMEM C string in `WEBPAGE.h`. Replace the entire `index_html` variable. If the new UI exceeds ~10KB, consider switching to LittleFS:
1. Create `data/index.html`
2. Add `board_build.filesystem = littlefs` to `platformio.ini`
3. Upload with `pio run --target uploadfs`
4. Serve with `server.serveStatic("/", LittleFS, "/index.html")`

### Thread Safety Reminder

All servo bus operations happen on `Serial1`. The `InfoUpdateThreading` task reads telemetry every 600ms. Web command handlers run in `clientThreading`. There is NO mutex. For the new API endpoints that read servo registers, consider:
- Keeping cached telemetry arrays that the info thread populates, and having API endpoints read from cache
- Only scan and write commands need to touch the bus directly from the web handler thread
- The nerd-bus-servo library has a 2ms timeout per operation, so bus contention will manifest as occasional `std::nullopt` returns rather than long blocks

### Cached Telemetry Arrays (to be defined in `STSCTRL.h`)

Keep cached arrays that the telemetry polling thread populates. API endpoints read from these instead of hitting the bus:
- `cachedPos[id]` — position (from `servo->read_encoder_angle()`)
- `cachedSpeed[id]` — speed (from `servo->read_speed()`)
- `cachedLoad[id]` — load (from `servo->read_load()`)
- `cachedVoltage[id]` — voltage as float (from `servo->read_voltage()`, already ÷10)
- `cachedCurrent[id]` — current (from bus `read_register` for current registers)
- `cachedTemp[id]` — temperature as float (from `servo->read_temperature()`)
- `cachedTorque[id]` — torque enabled (from `servo->is_torque_enabled()`)
- `servoIDs[n]` — ID of nth detected servo
- `servoCount` — number of detected servos
- `servos[id]` — `Servo*` pointer (polymorphic), nullptr if not detected
  - `servos[id]->type()` — `ServoBusApi::ServoType::SC` or `::STS`
  - `servos[id]->max_encoder_angle()` — 1023 or 4095
  - `servos[id]->min_encoder_angle()` — min angle limit
