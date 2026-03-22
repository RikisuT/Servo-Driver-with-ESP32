# Servo-Driver-with-ESP32 Advanced

### Fork and Credits:

This repository is maintained independently by RikisuT.
Based on the original Waveshare project:
https://github.com/waveshare/Servo-Driver-with-ESP32

Credit to berickson for contributions and updates in their fork:
https://github.com/berickson/Servo-Driver-with-ESP32

### Repo Sync Policy:

- origin: https://github.com/RikisuT/Servo-Driver-with-ESP32
- upstream: https://github.com/waveshare/Servo-Driver-with-ESP32
- berickson: fetch-only source for selective changes

### nerd-bus-servo Dependency Policy:

This project uses `lib/nerd-bus-servo` as a pinned git submodule commit.

- Users do not automatically receive newer library commits.
- New library behavior only ships after this repo updates the submodule pointer and commits it.
- Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/RikisuT/Servo-Driver-with-ESP32
```

- Update the library only when intentionally tested:

```bash
git submodule update --init --recursive
git -C lib/nerd-bus-servo fetch origin
git -C lib/nerd-bus-servo checkout <commit-or-tag>
git add lib/nerd-bus-servo
git commit -m "chore: bump nerd-bus-servo"
```

### Description:

An enhanced Servo Driver with ESP32 fork with expanded APIs, richer web UI controls, mixed SC/STS support, and additional configuration workflows beyond the original upstream projects.

### Website:

https://www.waveshare.net/shop/Servo-Driver-with-ESP32.htm

### WIKI:

https://www.waveshare.net/wiki/Servo_Driver_with_ESP32
