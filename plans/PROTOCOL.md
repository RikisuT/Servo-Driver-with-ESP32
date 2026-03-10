# Servo Driver ESP32 — Protocol & Codebase Reference

This document catalogs every HTTP route, servo library call, AT/serial command, and known limitation in the current codebase. Produced as pre-work before the UI rebuild.

---

## 1. HTTP Routes (Web Server)

The web server uses `WebServer.h` (synchronous, port 80). Routes are registered in `CONNECT.h::webCtrlServer()`.

| Route | Method | Handler | Description |
|-------|--------|---------|-------------|
| `/` | GET | `handleRoot()` | Serves the full HTML page from `index_html` PROGMEM string (defined in `WEBPAGE.h`) |
| `/readID` | GET | `handleID()` | Returns plain text list of detected servo IDs (e.g., `"ID:11 12 13 "`) or `"Searching..."` if a scan is in progress |
| `/readSTS` | GET | `handleSTS()` | Returns plain text with active servo telemetry: Active ID, Position, Device Mode (Normal/Leader/Follower), Voltage, Load, Speed, Temperature, Speed Set, ID to Set, Mode (Servo/Motor), Torque (On/Off). Returns `"FeedBack err"` on read failure. |
| `/cmd` | GET | lambda | Command dispatch. Reads 4 query params: `inputT` (command type), `inputI` (instruction), `inputA`, `inputB` (unused currently). Dispatches via `cmdT` switch — see Command Dispatch below. |

### Command Dispatch (`/cmd?inputT=T&inputI=I&inputA=A&inputB=B`)

**cmdT = 0 → `activeID(cmdI)`** — Select servo by cycling through the detected list  
- `cmdI = 1` → next servo in list  
- `cmdI = -1` → previous servo in list  

**cmdT = 1 → `activeCtrl(cmdI)`** — Servo control commands:

| cmdI | JS constant | Action | Implementation |
|------|-------------|--------|----------------|
| 1 | `servo_cmd.middle` | Move to middle position | `st.WritePosEx(id, ServoDigitalMiddle, speed, acc)` — middle is 511 for SC servos |
| 2 | `servo_cmd.stop` | Stop movement | Servo mode: `servoStop()` (disable+re-enable torque). Motor mode: `st.WritePos(id, 0, 0, 0)` |
| 3 | `servo_cmd.release` | Release (torque off) | `servoTorque(id, 0)` → `st.EnableTorque(id, 0)` |
| 4 | `servo_cmd.torque` | Torque on | `servoTorque(id, 1)` → `st.EnableTorque(id, 1)` |
| 5 | `servo_cmd.position_inc` | Position+ (continuous) | Servo: `WritePosEx(id, range-1, speed, acc)`. Motor: direction-dependent WritePosEx/WritePos |
| 6 | `servo_cmd.position_dec` | Position− (continuous) | Servo: `WritePosEx(id, 0, speed, acc)`. Motor: reverse direction |
| 7 | `servo_cmd.speed_inc` | Speed +100 | `activeSpeed(100)` — increments `activeServoSpeed` by 100, capped at `ServoMaxSpeed` (1500) |
| 8 | `servo_cmd.speed_dec` | Speed −100 | `activeSpeed(-100)` — decrements by 100, floor at 0 |
| 9 | `servo_cmd.id_to_set_inc` | ID-to-set +1 | Increments `servotoSet`, wraps at 250→0 |
| 10 | `servo_cmd.id_to_set_dec` | ID-to-set −1 | Decrements `servotoSet`, floor at 0 |
| 11 | `servo_cmd.set_middle` | Set middle position | `setMiddle(id)` → `st.CalibrationOfs(id)` — **NOTE: returns -1 for SCSCL (not implemented)** |
| 12 | `servo_cmd.set_servo_mode` | Set Servo mode | `setMode(id, 0)` — unlocks EEPROM, writes angle limits, locks EEPROM |
| 13 | `servo_cmd.set_motor_mode` | Set Motor mode | `setMode(id, 3)` — unlocks EEPROM, writes 0 to angle limits, locks EEPROM |
| 14 | `servo_cmd.serial_fwd_on` | Start serial forwarding | Sets `SERIAL_FORWARDING = true` — USB ↔ servo bus passthrough |
| 15 | `servo_cmd.serial_fwd_off` | Stop serial forwarding | Sets `SERIAL_FORWARDING = false` |
| 16 | `servo_cmd.set_new_id` | Set new servo ID | `setID(activeID, servotoSet)` — unlocks EEPROM, writes new ID byte, locks EEPROM |
| 17 | `servo_cmd.role_normal` | Normal role | `DEV_ROLE = 0` |
| 18 | `servo_cmd.role_leader` | Leader role | `DEV_ROLE = 1` — sends position data via ESP-NOW |
| 19 | `servo_cmd.role_follower` | Follower role | `DEV_ROLE = 2` — receives position data via ESP-NOW |
| 20 | `servo_cmd.rainbow_on` | Rainbow LED on | `RAINBOW_STATUS = 1` |
| 21 | `servo_cmd.rainbow_off` | Rainbow LED off | `RAINBOW_STATUS = 0` |

**cmdT = 9** → Sets `searchCmd = true` — triggers a servo scan on the next loop iteration

### Browser JS Polling

- `get_data()` — polls `/readSTS` every **300ms**, updates `#STSValue` innerHTML
- `get_servo_id()` — polls `/readID` every **1500ms**, updates `#IDValue` innerHTML

---

## 2. Servo Library

### Current: nerd-bus-servo (✅ MIGRATED)

**Repo:** https://github.com/berickson/nerd-bus-servo  
**Location:** `lib/nerd-bus-servo/` (cloned into project)

The user's own library, handling both SC and STS servo families on the same bus with a clean polymorphic API. Replaced the old `lib/SCServo/` library (now removed).

**Key classes:**
- `ServoBusApi` (global `servo_bus`) — low-level bus protocol, automatic byte-order handling per servo type
- `Servo` (base) — common operations, type inference via `infer_servo_type()`
- `SCServo` — SC-series wrapper (big-endian, 0–1023)
- `STSServo` — STS-series wrapper (little-endian, 0–4095) + acceleration, torque limit, wheel mode

**Waveshare board adaptation:** Added `set_echo_enabled(bool)` to `ServoBusApi` — set `false` for the Waveshare Servo Driver board which has hardware TX/RX isolation (no echo). Called in `servoInit()`.

**Build requirements:** C++17 (`-std=gnu++17` in `platformio.ini` build_flags, `-std=gnu++11` unflagged) — required for `std::optional`, `std::vector`, `std::map` used by the library.

**Compatibility wrappers** (defined in `STSCTRL.h`):
- `servoWritePosEx(id, position, speed, acc)` — replaces old `st.WritePosEx()`
- `servoWritePos(id, position, time, speed)` — replaces old `st.WritePos()`
- `currentServoType()` — returns `ServoBusApi::ServoType` based on global `SERVO_TYPE_SELECT`

See [PROGRESS.md](PROGRESS.md) "Architecture Notes" for usage examples.

### Legacy Methods Reference (old SCServo — removed)

The following documents the old SCServo library methods that are used in the current firmware. Each has a nerd-bus-servo equivalent.

Serial bus: `Serial1` at 1,000,000 baud, GPIO 18 (RX), GPIO 19 (TX).

### High-Level Methods Used in Firmware

| Method | Signature | Description | Used In |
|--------|-----------|-------------|---------|
| `Ping` | `Ping(u8 ID)` → int | Checks if servo responds. Returns -1 on failure. | `pingAll()` — scans IDs 0 to MAX_ID |
| `FeedBack` | `FeedBack(int ID)` → int | Reads bulk telemetry (pos, speed, load, voltage, current, temp) into internal buffer. Returns -1 on failure. | `getFeedBack()` |
| `ReadPos` | `ReadPos(int ID)` → int | Read position. Pass -1 to read from FeedBack buffer. | `getFeedBack()` |
| `ReadSpeed` | `ReadSpeed(int ID)` → int | Read speed. Signed (bit 15 = direction). | `getFeedBack()` |
| `ReadLoad` | `ReadLoad(int ID)` → int | Read load (0–1000, bit 10 = direction). | `getFeedBack()` |
| `ReadVoltage` | `ReadVoltage(int ID)` → int | Read voltage (raw, ×0.1V). | `getFeedBack()` |
| `ReadCurrent` | `ReadCurrent(int ID)` → int | Read current (signed, 1 unit = 6.5mA). | `getFeedBack()` |
| `ReadTemper` | `ReadTemper(int ID)` → int | Read temperature (°C). | `getFeedBack()` |
| `ReadMode` | `ReadMode(int ID)` → int | Read mode. SCSCL implementation: reads min angle limit — 0 means motor mode (3), >0 means servo mode (0). | `getFeedBack()` |
| `ReadMove` | `ReadMove(int ID)` → int | Read moving status (1=moving, 0=stopped). | Not used in firmware currently |
| `ReadInfoValue` | `ReadInfoValue(int ID, int addr)` → int | Generic 2-byte read at any register address. | Not used in firmware currently |
| `WritePosEx` | `WritePosEx(u8 ID, s16 Pos, u16 Speed, u8 ACC)` → int | Write position with speed. **Note: ACC is force-set to 0 in SCSCL implementation.** | `activeCtrl()` for position commands |
| `WritePos` | `WritePos(u8 ID, u16 Pos, u16 Time, u16 Speed)` → int | Write position with time and speed. Used for motor mode stop command. | Motor stop (`WritePos(id, 0, 0, 0)`) |
| `EnableTorque` | `EnableTorque(u8 ID, u8 Enable)` → int | Enable (1) or disable (0) torque. | `servoTorque()`, `servoStop()` |
| `unLockEprom` | `unLockEprom(u8 ID)` → int | Unlock EEPROM for writing persistent settings. Writes 0 to LOCK register. | `setMode()`, `setID()` |
| `LockEprom` | `LockEprom(u8 ID)` → int | Lock EEPROM (saves settings to flash). Writes 1 to LOCK register. | `setMode()`, `setID()` |
| `CalibrationOfs` | `CalibrationOfs(u8 ID)` → int | Calibrate offset / set middle. **Returns -1 in SCSCL (stub — not implemented).** | `setMiddle()` |
| `writeByte` | `writeByte(u8 ID, u8 Addr, u8 Val)` → int | Write 1 byte to register. | `setMode()`, `setID()`, lock/unlock |
| `writeWord` | `writeWord(u8 ID, u8 Addr, u16 Val)` → int | Write 2 bytes to register. | `setMode()` (angle limits) |
| `readByte` | `readByte(u8 ID, u8 Addr)` → int | Read 1 byte from register. | `ReadVoltage`, `ReadTemper`, `ReadMove` |
| `readWord` | `readWord(u8 ID, u8 Addr)` → int | Read 2 bytes from register. | `ReadPos`, `ReadSpeed`, etc. |
| `PWMMode` | `PWMMode(u8 ID)` → int | Set PWM mode (writes 0 to all angle limits). | Not used in firmware |
| `WritePWM` | `WritePWM(u8 ID, s16 pwmOut)` → int | Write PWM output value. | Not used in firmware |
| `SyncWritePos` | `SyncWritePos(u8 IDs[], u8 N, ...)` | Sync write position to multiple servos in one packet. | Not used in firmware |
| `RegWritePos` | `RegWritePos(u8 ID, u16 Pos, u16 Time, u16 Speed)` | Async write (deferred until `RegWriteAction`). | Not used in firmware |

### SCSCL Register Map (from `SCSCL.h`)

| Register | Address | Size | R/W | Description |
|----------|---------|------|-----|-------------|
| VERSION_L/H | 3–4 | 2 | R | Firmware version |
| ID | 5 | 1 | RW | Servo ID (EEPROM) |
| BAUD_RATE | 6 | 1 | RW | Baud rate setting (EEPROM) |
| MIN_ANGLE_LIMIT | 9–10 | 2 | RW | Min angle limit (EEPROM). 0 = motor mode. |
| MAX_ANGLE_LIMIT | 11–12 | 2 | RW | Max angle limit (EEPROM). 0 = motor mode. |
| CW_DEAD | 26 | 1 | RW | Clockwise dead zone (EEPROM) |
| CCW_DEAD | 27 | 1 | RW | Counter-clockwise dead zone (EEPROM) |
| TORQUE_ENABLE | 40 | 1 | RW | Torque on/off (SRAM) |
| GOAL_POSITION | 42–43 | 2 | RW | Target position (SRAM) |
| GOAL_TIME | 44–45 | 2 | RW | Time-based move in ms (SRAM) |
| GOAL_SPEED | 46–47 | 2 | RW | Target speed (SRAM) |
| LOCK | 48 | 1 | RW | EEPROM lock (0=unlocked, 1=locked) (SRAM) |
| PRESENT_POSITION | 56–57 | 2 | R | Actual position |
| PRESENT_SPEED | 58–59 | 2 | R | Actual speed (signed, bit 15 = dir) |
| PRESENT_LOAD | 60–61 | 2 | R | Actual load (signed, bit 10 = dir) |
| PRESENT_VOLTAGE | 62 | 1 | R | Voltage (×0.1V) |
| PRESENT_TEMPERATURE | 63 | 1 | R | Temperature (°C) |
| MOVING | 66 | 1 | R | Moving flag (1=in motion) |
| PRESENT_CURRENT | 69–70 | 2 | R | Current (signed, 1 unit = 6.5mA) |

**Note:** The SCSCL register map has fewer EEPROM registers than the STS (SMS_STS) series. Notably missing from SCSCL vs STS: Mode register (addr 33), ACC register (addr 41), Offset registers (addr 31-32), Torque Limit registers (addr 48-49), and various protection/PID registers. The SCSCL determines mode by checking if angle limits are 0 (motor) or >0 (servo).

### SMS_STS Register Map (from `SMS_STS.h`) — For Reference

The implementation plan targets STS servos. Key differences from SCSCL:

| Register | Address | Size | Not in SCSCL | Description |
|----------|---------|------|--------------|-------------|
| OFS (Offset) | 31–32 | 2 | Yes | Position offset (EEPROM) |
| MODE | 33 | 1 | Yes | 0=Servo, 3=Motor (EEPROM) |
| ACC | 41 | 1 | Yes | Acceleration (SRAM) |
| TORQUE_LIMIT | 48–49 | 2 | Yes* | Torque limit (SRAM). *SCSCL uses addr 48 for LOCK instead. |
| LOCK | 55 | 1 | Yes* | EEPROM lock. *Different address than SCSCL (48). |

**CRITICAL — Byte Order:** SCSCL is **big-endian** (`End=1`), SMS_STS is **little-endian** (`End=0`). Reading a 2-byte register with the wrong class will return a byte-swapped value. Always dispatch through the correct class for each servo ID.

**CRITICAL — LOCK Register:** SCSCL LOCK is at address **48**, STS LOCK is at address **55**. Each class handles this correctly in its own `unLockEprom`/`LockEprom` methods. Address 48 in STS is the Torque Limit register — writing to the wrong address would set torque limit instead of locking EEPROM.

**Mixed Servo Support:** The firmware will use the [nerd-bus-servo](https://github.com/berickson/nerd-bus-servo) library, which provides a polymorphic `Servo` base class with `SCServo` and `STSServo` subclasses. The library handles byte order, register address differences, and type detection automatically via `Servo::infer_servo_type()`. See [PROGRESS.md](PROGRESS.md) for the full integration plan.

---

## 3. Serial / AT Commands

There is no dedicated AT command parser. The "serial forwarding" mode (`SERIAL_FORWARDING = true`) creates a transparent byte-level passthrough between the USB serial (`Serial`) and the servo bus (`Serial1`). This allows an external host (Raspberry Pi, Jetson, etc.) to send raw Feetech protocol packets directly to the servos.

When serial forwarding is active:
- USB RX → Serial1 TX (to servos)
- Serial1 RX → USB TX (from servos)
- The web server continues to handle HTTP requests
- The OLED displays "SERIAL_FORWARDING"
- Telemetry polling stops

---

## 4. Threading / Task Architecture

Two FreeRTOS tasks run on separate contexts:

| Task | Stack | Priority | Core | Interval | Function |
|------|-------|----------|------|----------|----------|
| `InfoUpdateThreading` | 4000 | 5 | ARDUINO_RUNNING_CORE (1) | 600ms | Calls `getFeedBack()` for active servo, `getWifiStatus()`, `screenUpdate()`. Also handles `pingAll(searchCmd)` when scan requested, and rainbow mode. |
| `clientThreading` | 4000 | 5 | Any | 10ms | Calls `server.handleClient()`. Also handles serial forwarding loop and ESP-NOW leader data send. |

The main `loop()` does nothing (sleeps 300 seconds).

**Important:** The servo bus (Serial1) is NOT thread-safe. `InfoUpdateThreading` reads telemetry while `clientThreading` dispatches servo write commands. There is no mutex. This can cause bus contention and corrupt packets. The current code "mostly works" because the timing makes collisions infrequent.

---

## 5. ESP-NOW (Leader/Follower)

- Leader mode (`DEV_ROLE=1`): Sends the active servo's ID, position, and speed to a hardcoded broadcast MAC address every ~200ms via ESP-NOW.
- Follower mode (`DEV_ROLE=2`): Receives ESP-NOW packets and writes the received position to the specified servo ID.
- Normal mode (`DEV_ROLE=0`): ESP-NOW callbacks registered but not actively used.

---

## 6. Known Limitations & Gotchas

### Servo Scan
- **Default MAX_ID = 20.** Servos with IDs > 20 will not be found. `setID()` bumps `MAX_ID` if the new ID exceeds it, but this is not persisted across reboots.
- Scan is sequential (0 to MAX_ID), takes ~1-2 seconds for 20 IDs.
- Scan is triggered by `searchCmd` flag, executed in the InfoUpdate thread.

### Servo Type
- **Both SC-series (SCSCL) and STS-series (SMS_STS) are supported**, even mixed on the same bus.
- SC servos: range 0–1023, middle=511, angle range 210°, big-endian byte order.
- STS servos: range 0–4095, middle=2047, angle range 360°, little-endian byte order.
- Type is detected per-servo during scan and stored in `servoInfo[id]`.
- The firmware instantiates both `SCSCL sc;` and `SMS_STS sts;` sharing `Serial1`.
- All servo operations (read/write) must dispatch through the correct class for the servo's detected type.

### CalibrationOfs (Set Middle)
- Returns `-1` (not implemented) for SCSCL. The "Set Middle" button in the current UI does nothing for SC servos.

### SCSCL WritePosEx
- The `ACC` parameter is **force-set to 0** inside `SCSCL::WritePosEx()` regardless of what you pass. Acceleration is not supported on SC-series.

### Memory
- HTML page is stored in PROGMEM (~4KB currently). The new UI will be significantly larger.
- ESP32 has ~320KB RAM. Task stacks are 4000 bytes each.
- Consider LittleFS for serving HTML if the new page exceeds ~10KB.

### Thread Safety
- No mutex on the servo bus. Concurrent reads (telemetry) and writes (commands) from different tasks can corrupt packets.

### WiFi
- Tries STA first (connects to home network from `wifi_credentials.h`), falls back to AP mode (`ESP32_DEV` / `12345678`) if STA fails.
- IP: `192.168.4.1` in AP mode.

### Position±  Behavior
- Position+ and Position− don't increment/decrement by a step. They command the servo to move to the max or min position at the current speed. The effect is continuous motion until Stop is pressed. This is re-implemented in the plan as proper jog buttons with configurable step sizes.

---

## 7. File Map

| File | Purpose |
|------|---------|
| `src/ServoDriver.ino` | Main sketch — pin defs, constants, WiFi config, `setup()`, `loop()`, includes all headers |
| `src/WEBPAGE.h` | HTML string (PROGMEM) — full web UI page |
| `src/CONNECT.h` | WiFi setup, HTTP server routes, ESP-NOW init, command dispatch |
| `src/STSCTRL.h` | Servo control layer — init, feedback, mode set, ID set, torque, stop |
| `src/BOARD_DEV.h` | OLED display, RGB LED, FreeRTOS tasks, servo scan (`pingAll`) |
| `src/RGB_CTRL.h` | NeoPixel RGB LED control (10 LEDs, GPIO 23) |
| `src/PreferencesConfig.h` | Stub — `Preferences` init only, not used yet |
| `src/wifi_credentials.h` | STA WiFi credentials (gitignored) |
| `lib/SCServo/` | Feetech SC/STS servo library |
