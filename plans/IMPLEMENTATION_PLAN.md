# Servo Driver ESP32 — UI Rebuild Implementation Plan

## Context

We are rebuilding the web UI for the Waveshare Servo Driver with ESP32 board. The fork is at `https://github.com/berickson/Servo-Driver-with-ESP32`.

The reference design is in `servo-driver-ui.jsx` (React prototype). The actual implementation will be **vanilla HTML/JS/CSS**.

### Existing Architecture (IMPORTANT — read before coding)

The fork has been restructured from the original Waveshare Arduino sketch into a **PlatformIO project**:
- `src/` — main source (was `ServoDriver/ServoDriverST.ino` + header files)
- `lib/SCServo/` — Feetech servo library (was `SCServo/` at root)
- `platformio.ini` — PlatformIO build config
- `todo.md` — existing notes from the fork author

The original Waveshare code has this web architecture:
- **No WebSocket.** Uses `WebServer.h` (synchronous ESP32 HTTP server)
- **No separate HTML files.** The HTML is embedded as C string literals in header files (likely `WEB_CTRL.h` or equivalent). There is no `data/` folder, no SPIFFS/LittleFS file serving.
- **Full page reload on every action.** Each button is an HTTP GET to a URL like `/position_plus`. The handler moves the servo and re-serves the entire HTML page with new values baked into the string.
- **Servo scan defaults to IDs 0–20** (`MAX_ID`). IDs above 20 require changing this constant and reflashing.
- Servo control logic is in `STSCTRL.h` using the SCServo/Feetech library
- RGB LED control in `RGB_CTRL.h`
- WiFi AP mode by default: SSID `ESP32_DEV`, password `12345678`, UI at `192.168.4.1`
- There is also an **AT command set** for serial/UART communication with an external host computer (Raspberry Pi, Jetson, etc.) — this is separate from the web UI

**Key constraints:**
- Minimal JS footprint — no frameworks, no build step
- Must work on mobile Chrome (primary use case is phone connected to ESP32 WiFi AP, no internet)
- External font imports (Google Fonts) will NOT work — must use system font stacks
- ESP32 has limited RAM — the HTML string must fit in memory

---

## Pre-work: Understand the Existing Codebase

Before any implementation, do this first:

1. Read all source files in `src/` — identify where the web server routes are defined, where the HTML string is built, and where servo commands are dispatched
2. Read the servo control layer (originally `STSCTRL.h`) to understand which SCServo library calls are available — `WritePos`, `ReadPos`, `ReadLoad`, `ReadVoltage`, `ReadTemper`, `ReadCurrent`, `ReadMode`, `WriteByte`, `unLockEprom`, `LockEprom`, etc.
3. Read `lib/SCServo/` to understand the register addresses and read/write methods
4. Map every existing `server.on("/...", handler)` route to understand the current command set
5. Check `platformio.ini` for board config, partition scheme, and any PSRAM settings
6. Read `todo.md` for the fork author's existing notes and intentions

**Verification:** Produce a `PROTOCOL.md` file listing:
- Every HTTP route the web server handles, what it does, and what parameters it reads
- Every servo library function called and its purpose
- The AT command set (from serial UART) if present
- Any known limitations (MAX_ID, memory, etc.)

Do not proceed until this is complete and reviewed.

### Architecture Decision: Keep HTTP or Add WebSocket?

The existing code uses full-page-reload HTTP. For our redesigned UI, we have two options:

**Option A: Keep HTTP, use AJAX (fetch/XMLHttpRequest)**
- Least disruptive to the existing codebase
- Add JSON API endpoints alongside the existing HTML routes (e.g., `GET /api/status` returns JSON with all servo states)
- The browser JS polls for updates or fetches after each command
- Simpler, but not real-time — telemetry updates only on poll/action

**Option B: Add AsyncWebServer + WebSocket**
- Requires replacing `WebServer.h` with `ESPAsyncWebServer` (available via PlatformIO)
- Enables real-time push of telemetry updates to the browser
- More work upfront, but better UX (live load/temp/position updates)
- Can serve HTML from PROGMEM or SPIFFS

**Recommendation:** Start with Option A (AJAX + JSON API) to get the UI working quickly. Migrate to WebSocket in a later phase if real-time telemetry matters enough. Note this in the plan and let the developer decide.

---

## Phase 1: Card Layout + Position Control

Replace the existing single-servo button-stack UI with the card-per-servo layout.

### 1.1 — Scan and Card Rendering

- On page load (or "Scan" button press), scan for servos and render one card per detected servo
- Each card shows: `#ID`, mode badge (Servo/Motor), torque toggle
- Cards stack vertically, max-width ~680px centered
- Dark theme matching the prototype colors
- **NOTE:** The existing firmware scans IDs 0–20 by default (`MAX_ID`). Consider increasing this or making it configurable. Scanning all 253 IDs takes several seconds.

**Verification:**
- [ ] Power on the board with 1-3 servos connected
- [ ] Open the web UI on your phone
- [ ] Each connected servo gets its own card with correct ID
- [ ] Unplugging a servo and re-scanning updates the list

### 1.2 — Position Slider + Direct Input (Servo Mode)

- Full-width range slider for position (0–4095)
- Text input field next to it — type a value, hit enter or blur, servo moves
- Slider and input stay in sync
- Jog buttons (−/+) flanking the input, small (28×28) for servo mode
- Step size selector: ±1, ±10, ±100

**Verification:**
- [ ] Drag slider → servo physically moves, input updates
- [ ] Type `2048` in input, press enter → servo moves to center, slider updates
- [ ] Tap `+` with step=10 → position increments by 10, servo moves
- [ ] Change step to 100, tap `−` → position decrements by 100

### 1.3 — Position Jog-Only (Motor Mode)

- If the servo is in motor mode, hide the slider entirely
- Show large (48×48) jog buttons with the value centered between them
- Step size selector below

**Verification:**
- [ ] Connect a servo configured in motor mode
- [ ] No slider visible — only large −/+ buttons and value
- [ ] Jog buttons move the motor, step size works correctly

### 1.4 — Setpoint vs Actual Display

- Above the slider, show `POSITION` label and `Actual: {value}` 
- Actual value updates via polling or after each command (fetch `/api/status` and update the display)
- Color: green if |actual − setpoint| ≤ 20, orange if larger

**Verification:**
- [ ] With torque on, set position — actual tracks closely, shown in green
- [ ] Turn torque off, push servo by hand — actual changes, goes orange since it diverges from setpoint
- [ ] Turn torque back on — servo returns to setpoint, actual goes green again

---

## Phase 2: Telemetry + Torque

### 2.1 — Telemetry Row

- In the card header, show inline: Voltage, Load, Temperature, Current (mA)
- Values update from periodic polling (e.g., fetch `/api/status` every 500ms–1s) or after each user action
- Orange color if: load > 50, temp > 50°C, current > 80 (raw units)
- Show a pulsing "Moving" badge when the servo reports movement

**Verification:**
- [ ] All four values display and update in real-time
- [ ] Apply load to a servo by hand — Load and Current values increase, turn orange if threshold crossed
- [ ] Command a move — "Moving" badge appears during travel, disappears on arrival

### 2.2 — Torque Toggle

- Toggle pill button: green "Torque" when on, grey when off
- Clicking toggles torque enable/disable on the servo
- "Release" button in quick actions also turns torque off

**Verification:**
- [ ] Torque on → servo holds position, resists movement
- [ ] Torque off → servo is free to move by hand
- [ ] "Release" button has same effect as toggling torque off

---

## Phase 3: Quick Actions + Speed + Torque Limit

### 3.1 — Quick Action Buttons

- Row of buttons: Center (position=2048), Min (position=minAngle), Max (position=maxAngle), Stop, Release
- Min/Max use the configured angle limits (defaults to 0 and 4095 until changed)

**Verification:**
- [ ] "Center" moves servo to 2048
- [ ] Set angle limits (e.g., 500/3500 in Phase 5), then "Min"/"Max" go to those values
- [ ] "Stop" halts servo movement immediately

### 3.2 — Speed + Torque Limit Inline

- Below quick actions, show: `Speed [___]` and `Torque Limit [___] /1000`
- Simple text input fields, no sliders
- Torque limit text turns orange when at factory default (1000)
- These values are sent to the servo when changed

**Verification:**
- [ ] Change speed to 50 → servo moves noticeably slower on next position command
- [ ] Change torque limit to 300 → servo is weaker, can be overpowered by hand more easily
- [ ] Torque limit field shows orange at 1000, normal color at other values

---

## Phase 4: Servo Naming + Batch Actions

### 4.1 — Servo Naming

- Each card header shows an editable name next to the `#ID` badge
- Default name: "Servo 5", "Servo 11", etc.
- Click the name to edit inline, blur or enter to save
- Names stored in localStorage (keyed by servo ID)
- Names persist across page reloads on the same browser

**Verification:**
- [ ] Card shows default name "Servo 5" on first visit
- [ ] Click name, type "Left Shoulder", press enter → name updates
- [ ] Refresh page → name persists from localStorage
- [ ] Scan new servo → gets default name
- [ ] ID still gets shown



---

## Phase 5: Config Section (Collapsible)

### 5.1 — Collapsible Container

- At the bottom of each card, a `▶ CONFIG` toggle
- Clicking expands a section with grouped settings
- Show `EEPROM UNLOCKED` warning badge when EEPROM is unlocked

**Verification:**
- [ ] Config section is hidden by default
- [ ] Click toggles open/closed
- [ ] EEPROM badge appears when unlocked

### 5.2 — EEPROM Lock/Unlock

- First element in config: "Unlock EEPROM" / "Lock EEPROM (Save)" button
- Helper text explains what it does
- Sends the appropriate lock/unlock command to the servo

**Verification:**
- [ ] Unlock → change a persistent setting (e.g., angle limit) → Lock → power cycle → setting persists
- [ ] Without unlocking, persistent settings do not save across power cycles

### 5.3 — Mode Settings

Group label: **MODE**
- Operating mode: Servo / Motor toggle buttons (both SC and STS)
- Phase (direction reversal): 0 or 253 (STS only, register 18) — *needs library register addition*
- Offset: numeric input −2048 to 2047 (STS only, register 31-32)

**Verification:**
- [ ] Switch mode from Servo to Motor → card UI switches to jog-only for position
- [ ] Set phase to 253 on STS → servo direction reverses
- [ ] Set offset on STS → position reference shifts

### 5.4 — Safety Limits (with Capture + Go)

Group label: **SAFETY LIMITS**
- Min Angle: numeric input + `← Capture` button + `Go →` button
- Max Angle: numeric input + `← Capture` button + `Go →` button  
- Max Torque: numeric input (0–1023, STS only) — also editable from the main card
- Min/Max Voltage: numeric inputs (STS only, registers 14-15, ×0.1V) — *needs library register addition*
- Max Temperature: numeric input (STS only, register 13, °C) — *needs library register addition*

`← Capture` reads the current actual position and writes it into the field.
`Go →` sends the servo to that limit value to verify it.

**Verification:**
- [ ] Turn torque off, manually move servo to a hard stop
- [ ] Tap "← Capture" on Min Angle → field populates with the actual position
- [ ] Move to other hard stop, capture as Max Angle
- [ ] Tap "Go →" on Min Angle → servo travels to that position
- [ ] Tap "Go →" on Max Angle → servo travels to that position

### 5.5 — Motion Settings

Group label: **MOTION**
- Acceleration: numeric input (0–254, STS only, register 41)
- CW / CCW Dead Zones: numeric inputs (0–255, both SC and STS, registers 26-27)

**Verification:**
- [ ] Set acceleration to 100 → servo ramps up/down smoothly instead of jerking
- [ ] Set acceleration to 0 → motion is immediate/jerky (factory behavior)

### 5.6 — PID Tuning (STS only)

Group label: **PID TUNING**
- P, I, D: numeric inputs (0–255, registers 21-23) — *needs library register addition*
- Speed P, Speed I: numeric inputs (0–255, registers 37, 39) — *needs library register addition*

**Verification:**
- [ ] Change P coefficient → servo response changes (more/less aggressive)
- [ ] Write values, read back → confirms register access works

### 5.7 — Protection Settings (STS only)

Group label: **PROTECTION**
- Protection Current: numeric input (register 28-29, 2 bytes) — *needs library register addition*
- Protective Torque: numeric input (register 34) — *needs library register addition*
- Protection Time: numeric input (register 35) — *needs library register addition*
- Overload Torque: numeric input (register 36) — *needs library register addition*

**Verification:**
- [ ] Set protection current low → servo disables under moderate load
- [ ] Reset to default → servo operates normally under same load

### 5.8 — Actions (Set ID, Set Middle)

Group label: **ACTIONS**
- Set Middle: button (STS only — uses `set_offset()` to set current position as center)
- Set New ID: text input (1–253) + "Apply" button with orange warning color (both SC and STS)

**Verification:**
- [ ] Connect single servo (ID 1), type 11 in Set New ID, tap Apply → servo now responds as ID 11
- [ ] Scan → servo shows up as #11
- [ ] Set Middle while STS servo is off-center → offset adjusts so current position becomes center

### Library work needed

Several STS registers need to be added to the nerd-bus-servo `Register` enum before the UI can use them. SC servos do not have these registers.

| Register | Addr | Size | Needs Adding |
|----------|------|------|--------------|
| Max Temp Limit | 13 | 1 | Yes |
| Max Voltage | 14 | 1 | Yes |
| Min Voltage | 15 | 1 | Yes |
| Max Torque (EPROM) | 16 | 2 | Yes |
| Phase | 18 | 1 | Yes |
| P Coefficient | 21 | 1 | Yes |
| D Coefficient | 22 | 1 | Yes |
| I Coefficient | 23 | 1 | Yes |
| Protection Current | 28 | 2 | Yes |
| Protective Torque | 34 | 1 | Yes |
| Protection Time | 35 | 1 | Yes |
| Overload Torque | 36 | 1 | Yes |
| Speed P | 37 | 1 | Yes |
| Speed I | 39 | 1 | Yes |

---

## Phase 6: Check Button

### 6.1 — Per-Card Health Check

- "Check" button in the quick actions row (orange/warn style)
- Clicking runs a client-side check against the servo's current config
- Displays an orange warning banner below the quick actions with bullet points

**Warnings to check:**
- ID is 1 (factory default)
- Min/Max angle at 0/4095 (no limits set)
- Max torque at 1000 (no protection)
- Acceleration is 0 with servo mode (jerky motion)
- EEPROM left unlocked
- Temperature > 55°C
- Voltage < 6.0V
- Min angle ≥ max angle (invalid config)

**Verification:**
- [ ] Fresh factory servo → Check shows multiple warnings (ID, limits, torque, acceleration)
- [ ] Configure the servo properly → Check shows no warnings (or fewer)
- [ ] Dismiss button hides the banner

---

## Phase 7: Polish + Mobile Optimization

### 7.1 — Responsive Layout

- Test on phone screens (360px–414px width typical)
- Cards should not horizontally overflow
- Telemetry row should wrap gracefully
- Config fields should stack if needed

### 7.2 — Connection Handling

- Show a status indicator (green/red dot) based on whether the last API fetch succeeded or failed
- If a fetch fails, show "Disconnected" and retry periodically
- "Scanning…" state on the Scan button

### 7.3 — Font Cleanup for ESP32

- Remove Google Fonts import (ESP32 can't proxy external fonts)
- Use system font stack: `-apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif`
- For monospace values: `'SF Mono', 'Fira Code', 'Consolas', monospace`

**Verification:**
- [ ] UI works fully in airplane mode (phone connected to ESP32 AP only, no internet)
- [ ] Disconnect/reconnect WiFi → UI reconnects and resumes updating
- [ ] UI is usable on a phone in portrait orientation without horizontal scrolling

---

## Implementation Notes for the Developer

### Servo Communication

The Feetech SC/ST servo register map (STS series shown):

| Register | Address | Size | Description |
|----------|---------|------|-------------|
| ID | 5 | 1 | Servo ID (1–253) |
| Min Angle Limit | 9 | 2 | Minimum position |
| Max Angle Limit | 11 | 2 | Maximum position |
| Max Temp Limit | 13 | 1 | Max temperature (°C) |
| Max Voltage | 14 | 1 | Max voltage (×0.1V) |
| Min Voltage | 15 | 1 | Min voltage (×0.1V) |
| Max Torque | 16 | 2 | Max torque (0–1000) |
| Phase | 18 | 1 | Direction (0 or 253) |
| P Coefficient | 21 | 1 | Position P gain |
| D Coefficient | 22 | 1 | Position D gain |
| I Coefficient | 23 | 1 | Position I gain |
| CW Dead Zone | 26 | 1 | Clockwise dead zone |
| CCW Dead Zone | 27 | 1 | Counter-CW dead zone |
| Protection Current | 28 | 2 | Over-current threshold |
| Offset | 31 | 2 | Position offset |
| Mode | 33 | 1 | 0=Servo, 3=Motor |
| Protective Torque | 34 | 1 | Protection torque level |
| Protection Time | 35 | 1 | Protection time (ms) |
| Overload Torque | 36 | 1 | Overload torque level |
| Speed P | 37 | 1 | Speed loop P gain |
| Speed I | 39 | 1 | Speed loop I gain |
| Torque Enable | 40 | 1 | 0=off, 1=on |
| Acceleration | 41 | 1 | Accel ramp (0–254) |
| Goal Position | 42 | 2 | Target position |
| Goal Time | 44 | 2 | Time-based move (ms) |
| Goal Speed | 46 | 2 | Target speed |
| Lock | 55 | 1 | EEPROM lock (0=unlocked) |
| Present Position | 56 | 2 | Actual position (read) |
| Present Speed | 58 | 2 | Actual speed (read) |
| Present Load | 60 | 2 | Actual load (read) |
| Present Voltage | 62 | 1 | Voltage ×0.1V (read) |
| Present Temp | 63 | 1 | Temperature °C (read) |
| Moving | 66 | 1 | In motion (read) |
| Present Current | 69 | 2 | Current, 1=6.5mA (read) |

### File Structure (PlatformIO project)

```
src/
  main.cpp              ← main source (refactored from ServoDriverST.ino)
  web_page.h            ← HTML string as PROGMEM const (or use SPIFFS below)
lib/
  SCServo/              ← Feetech servo library (already present)
data/                   ← (optional) if using SPIFFS/LittleFS for the HTML
  index.html
platformio.ini          ← build config (already present)
```

If the HTML gets large (>10KB), add a `data/` folder and configure LittleFS:
```ini
; in platformio.ini
board_build.filesystem = littlefs
```
Then upload filesystem with `pio run --target uploadfs`.

### Web Communication Architecture

The existing code uses `WebServer.h` with full-page-reload on every button press. For the new UI, add JSON API endpoints that the browser JS can call via `fetch()`. Keep the HTML as a single embedded string served on `GET /`.

**Suggested API endpoints to ADD (keep existing routes for backwards compatibility):**

```
GET  /api/scan                → {"ids": [11, 12, 13]}
GET  /api/status?id=12        → {"id":12, "pos":2041, "load":12, "voltage":98, "temp":29, "current":120, "moving":0, "mode":0, "torque":1}
GET  /api/status_all          → [{"id":11, ...}, {"id":12, ...}, ...]  (for polling all servos at once)
POST /api/setpos              → body: id=12&pos=2048&speed=200
POST /api/torque              → body: id=12&enable=1
POST /api/writereg            → body: id=12&addr=9&value=500
POST /api/lockeeprom          → body: id=12
POST /api/unlockeeprom        → body: id=12
POST /api/setid               → body: old_id=1&new_id=11
POST /api/setmiddle           → body: id=12
```

**The HTML page** should be served as a single embedded C string on `GET /`. The JS in that page uses `fetch()` to call the API endpoints above. This avoids full-page reloads and enables a responsive single-page UI.

**Polling strategy:** The browser JS calls `GET /api/status_all` every ~750ms to update telemetry. This is acceptable for 1-3 servos. If latency becomes an issue, consider migrating to `ESPAsyncWebServer` + WebSocket in a future phase.

**Memory note:** The HTML string must fit in ESP32 RAM. For a complex UI, consider:
- Using PROGMEM to store the HTML in flash
- Or using the PlatformIO SPIFFS/LittleFS upload to serve from filesystem (add a `data/` folder and `board_build.filesystem = littlefs` to `platformio.ini`)
- The SPIFFS approach is recommended if the HTML exceeds ~10KB
