# Servo-Driver-with-ESP32 (RikisuT Fork)

This repository is an independent fork focused on reliable, low-latency serial control for robot motion workloads.

## Lineage and Credits

- Maintainer fork: [RikisuT/Servo-Driver-with-ESP32](https://github.com/RikisuT/Servo-Driver-with-ESP32)
- Original upstream (Waveshare): [waveshare/Servo-Driver-with-ESP32](https://github.com/waveshare/Servo-Driver-with-ESP32)
- Reference fork used for selective ideas and patches: [berickson/Servo-Driver-with-ESP32](https://github.com/berickson/Servo-Driver-with-ESP32)

## What This Fork Is Optimized For

- Host-to-ESP32 serial command bridge for motion control.
- Mixed servo support (SC and STS).
- Stronger runtime observability and performance counters.
- Practical behavior for real robots: robust scanning, missing-servo tolerance, and deterministic command handling.

## Current Status Compared to Waveshare and berickson

### Active in this fork now

- Serial bridge command path (text + binary).
- Binary fast path for control commands.
- Queue-aware command processing and performance telemetry.
- Motion-control-first workflow used by external ROS tooling.

### Present in tree but not currently wired into the active firmware path

- Wi-Fi and web control implementation files are still present in `src/connect.h` and related headers.
- Current `src/servo_driver.ino` build path uses serial bridge modules and does not include `connect.h`.

This means Wi-Fi/web code is currently inactive by default in this branch's active runtime path.

## Why This Differs From Waveshare and berickson

- Waveshare baseline: general board feature set with original control flow.
- berickson fork: important extension work and compatibility improvements.
- This fork: narrowed around high-rate serial robot control and real-world tuning/measurement workflows.

In short, this branch prioritizes motion command determinism and diagnostics over web-first operation.

## Dependency Policy (nerd-bus-servo)

This project uses `lib/nerd-bus-servo` as a pinned git submodule commit.

- Users do not automatically receive newer library commits.
- Library behavior changes only after this repo updates the submodule pointer and commits it.

Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/RikisuT/Servo-Driver-with-ESP32
```

Update the library intentionally:

```bash
git submodule update --init --recursive
git -C lib/nerd-bus-servo fetch origin
git -C lib/nerd-bus-servo checkout <commit-or-tag>
git add lib/nerd-bus-servo
git commit -m "chore: bump nerd-bus-servo"
```

## Sync Policy

- `origin`: RikisuT fork (source of truth for this branch behavior)
- `upstream`: Waveshare (baseline reference)
- `berickson`: fetch-only reference for selective cherry-picks/adaptations

## Hardware Reference Links

- Product page: [Waveshare Product](https://www.waveshare.net/shop/Servo-Driver-with-ESP32.htm)
- Waveshare wiki: [Servo Driver with ESP32 Wiki](https://www.waveshare.net/wiki/Servo_Driver_with_ESP32)
