#!/usr/bin/env python3
import argparse
import glob
import statistics
import struct
import sys
import time

try:
    import serial
except ImportError:
    print("Missing dependency: pyserial")
    print("Install with: pip install pyserial")
    sys.exit(1)


def find_default_port() -> str | None:
    candidates = sorted(glob.glob("/dev/ttyUSB*")) + sorted(glob.glob("/dev/ttyACM*"))
    return candidates[0] if candidates else None


def parse_baud_candidates(raw_baud: str) -> list[int]:
    text = (raw_baud or "auto").strip().lower()
    if text == "auto":
        return [500000, 1000000]
    try:
        return [int(text)]
    except ValueError as exc:
        raise ValueError("--baud must be an integer or 'auto'") from exc


class MotorSerialClient:
    def __init__(self, port: str, baud: int, timeout: float = 0.15):
        self.ser = serial.Serial(port=port, baudrate=baud, timeout=timeout)
        # Let MCU settle and flush startup noise.
        time.sleep(0.25)
        self.ser.reset_input_buffer()

    def close(self) -> None:
        self.ser.close()

    def send(self, cmd: str) -> None:
        line = (cmd.strip() + "\n").encode("ascii", errors="ignore")
        self.ser.write(line)

    def read_lines(self, window_s: float = 0.8, stop_on_prefix: tuple[str, ...] = ()) -> list[str]:
        end_t = time.time() + window_s
        lines: list[str] = []
        while time.time() < end_t:
            raw = self.ser.readline()
            if not raw:
                continue
            txt = raw.decode("utf-8", errors="replace").strip()
            if txt:
                lines.append(txt)
                if stop_on_prefix and txt.startswith(stop_on_prefix):
                    break
        return lines

    def request(
        self,
        cmd: str,
        wait_s: float = 0.8,
        stop_on_prefix: tuple[str, ...] = (),
    ) -> list[str]:
        self.send(cmd)
        return self.read_lines(wait_s, stop_on_prefix=stop_on_prefix)

    def read_exact(self, n: int, timeout_s: float) -> bytes:
        end_t = time.time() + timeout_s
        out = bytearray()
        while len(out) < n and time.time() < end_t:
            chunk = self.ser.read(n - len(out))
            if chunk:
                out.extend(chunk)
        if len(out) != n:
            raise TimeoutError(f"Timed out waiting for {n} bytes (got {len(out)})")
        return bytes(out)

    def write_raw(self, data: bytes) -> None:
        self.ser.write(data)

    def clear_input(self) -> None:
        self.ser.reset_input_buffer()


def wait_for_firmware_ready(client: MotorSerialClient, timeout_s: float = 3.5) -> bool:
    # Opening a USB CDC/serial port often resets ESP32 boards.
    # Wait until command parser is ready before sending workload commands.
    end_t = time.time() + timeout_s
    while time.time() < end_t:
        out = client.request("HELP", wait_s=0.25)
        for ln in out:
            if ln.startswith("OK commands:"):
                return True
        time.sleep(0.1)
    return False


BIN_SOF = 0x7E
BIN_SET = 0x01
BIN_GETP = 0x02
BIN_PING = 0x03
BIN_SETN = 0x05
BIN_FASTCFG = 0x06


def bin_crc_xor(cmd: int, seq: int, payload: bytes) -> int:
    crc = 0
    crc ^= cmd & 0xFF
    crc ^= seq & 0xFF
    crc ^= len(payload) & 0xFF
    for b in payload:
        crc ^= b
    return crc & 0xFF


def build_bin_frame(cmd: int, seq: int, payload: bytes) -> bytes:
    crc = bin_crc_xor(cmd, seq, payload)
    return bytes([BIN_SOF, cmd & 0xFF, seq & 0xFF, len(payload) & 0xFF]) + payload + bytes([crc])


def read_bin_frame(client: MotorSerialClient, timeout_s: float = 0.25) -> tuple[int, int, bytes]:
    end_t = time.time() + timeout_s
    while time.time() < end_t:
        b = client.read_exact(1, timeout_s=max(0.01, end_t - time.time()))[0]
        if b != BIN_SOF:
            continue
        hdr = client.read_exact(3, timeout_s=max(0.01, end_t - time.time()))
        cmd, seq, length = hdr[0], hdr[1], hdr[2]
        payload = client.read_exact(length, timeout_s=max(0.01, end_t - time.time())) if length else b""
        crc = client.read_exact(1, timeout_s=max(0.01, end_t - time.time()))[0]
        if crc != bin_crc_xor(cmd, seq, payload):
            continue
        return cmd, seq, payload
    raise TimeoutError("Timed out waiting for binary frame")


def bin_fastcfg(client: MotorSerialClient, seq: int, noack_set: bool) -> bool:
    payload = struct.pack("<B", 0x01 if noack_set else 0x00)
    client.write_raw(build_bin_frame(BIN_FASTCFG, seq, payload))
    cmd, rsp_seq, rsp = read_bin_frame(client)
    if cmd != (BIN_FASTCFG | 0x80) or rsp_seq != (seq & 0xFF) or len(rsp) < 1:
        return False
    return rsp[0] == 0


def bin_set(
    client: MotorSerialClient,
    seq: int,
    motor_id: int,
    pos: int,
    speed: int,
    acc: int,
    wait_ack: bool = True,
) -> bool:
    payload = struct.pack("<BHHB", motor_id & 0xFF, pos & 0xFFFF, speed & 0xFFFF, acc & 0xFF)
    client.write_raw(build_bin_frame(BIN_SET, seq, payload))
    if not wait_ack:
        return True
    cmd, rsp_seq, rsp = read_bin_frame(client)
    if cmd != (BIN_SET | 0x80) or rsp_seq != (seq & 0xFF) or len(rsp) < 1:
        return False
    return rsp[0] == 0


def bin_setn(
    client: MotorSerialClient,
    seq: int,
    entries: list[tuple[int, int, int, int]],
    wait_ack: bool = True,
) -> bool:
    if not entries:
        return True
    payload = b"".join(struct.pack("<BHHB", mid & 0xFF, pos & 0xFFFF, spd & 0xFFFF, acc & 0xFF) for mid, pos, spd, acc in entries)
    client.write_raw(build_bin_frame(BIN_SETN, seq, payload))
    if not wait_ack:
        return True
    cmd, rsp_seq, rsp = read_bin_frame(client)
    if cmd != (BIN_SETN | 0x80) or rsp_seq != (seq & 0xFF) or len(rsp) < 1:
        return False
    return rsp[0] == 0


def bin_getp(client: MotorSerialClient, seq: int, motor_id: int) -> int | None:
    payload = struct.pack("<B", motor_id & 0xFF)
    client.write_raw(build_bin_frame(BIN_GETP, seq, payload))
    cmd, rsp_seq, rsp = read_bin_frame(client)
    if cmd != (BIN_GETP | 0x80) or rsp_seq != (seq & 0xFF) or len(rsp) < 1:
        return None
    if rsp[0] != 0 or len(rsp) < 4:
        return None
    return int(struct.unpack("<H", rsp[2:4])[0])


def parse_ids(lines: list[str]) -> list[int]:
    # Expected firmware output format: OK ids=1,2,3
    for ln in lines:
        if ln.startswith("OK ids="):
            payload = ln.split("=", 1)[1].strip()
            if not payload:
                return []
            ids = []
            for x in payload.split(","):
                x = x.strip()
                if x.isdigit():
                    ids.append(int(x))
            return ids
    return []


def wait_for_ids(client: MotorSerialClient, timeout_s: float = 3.0) -> list[int]:
    end_t = time.time() + timeout_s
    while time.time() < end_t:
        out = client.request("LIST", wait_s=0.25, stop_on_prefix=("OK ids=",))
        ids = parse_ids(out)
        if ids:
            return ids
        time.sleep(0.08)
    return []


def probe_ids_by_ping(client: MotorSerialClient, max_id: int = 20) -> list[int]:
    found: list[int] = []
    for sid in range(1, max_id + 1):
        out = client.request(f"PING {sid}", wait_s=0.12, stop_on_prefix=("OK ping", "ERR"))
        for ln in out:
            if ln.startswith("OK ping"):
                found.append(sid)
                break
    return found


def discover_ids_with_retries(client: MotorSerialClient) -> list[int]:
    for attempt in range(1, 4):
        if attempt > 1:
            time.sleep(0.6)

        client.request("SCAN", wait_s=0.5, stop_on_prefix=("OK scan_started", "ERR"))
        time.sleep(1.1)

        out = client.request("LIST", wait_s=0.4, stop_on_prefix=("OK ids=",))
        ids = parse_ids(out)
        if not ids:
            ids = wait_for_ids(client, timeout_s=4.5)
        if ids:
            return ids

    return probe_ids_by_ping(client, max_id=20)


def parse_fb(lines: list[str]) -> str | None:
    for ln in lines:
        if ln.startswith("FB id=") or ln.startswith("FBP id=") or ln.startswith("FBPS id="):
            return ln
    return None


def parse_perf(lines: list[str]) -> str | None:
    for ln in lines:
        if ln.startswith("PERF "):
            return ln
    return None


def parse_perf_cmd(lines: list[str]) -> str | None:
    for ln in lines:
        if ln.startswith("PERF_CMD "):
            return ln
    return None


def request_perf_snapshot(client: MotorSerialClient) -> tuple[str | None, str | None]:
    # PERF emits two lines: PERF ... and PERF_CMD ...
    out = client.request("PERF", wait_s=0.35)
    return parse_perf(out), parse_perf_cmd(out)


def percentile(values: list[float], p: float) -> float:
    if not values:
        return 0.0
    if p <= 0:
        return min(values)
    if p >= 100:
        return max(values)
    idx = (len(values) - 1) * (p / 100.0)
    lo = int(idx)
    hi = min(lo + 1, len(values) - 1)
    frac = idx - lo
    return values[lo] * (1.0 - frac) + values[hi] * frac


def choose_motor_id(ids: list[int], arg_id: int | None) -> int:
    if arg_id is not None:
        if arg_id not in ids:
            raise ValueError(f"Requested id {arg_id} not in scanned list {ids}")
        return arg_id
    if len(ids) == 1:
        return ids[0]

    print("Detected motor IDs:", ", ".join(str(i) for i in ids))
    while True:
        entry = input("Select motor ID: ").strip()
        if entry.isdigit() and int(entry) in ids:
            return int(entry)
        print("Invalid ID. Pick one from the detected list.")


def choose_position(default_pos: int | None) -> int:
    if default_pos is not None:
        return default_pos

    while True:
        entry = input("Target position (0-4095): ").strip()
        if entry.isdigit():
            pos = int(entry)
            if 0 <= pos <= 4095:
                return pos
        print("Invalid position. Use integer 0..4095.")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Scan ST3215-HS motors over serial bridge, read one motor, and set position."
    )
    parser.add_argument("--port", default=None, help="Serial port, e.g. /dev/ttyUSB0")
    parser.add_argument(
        "--baud",
        default="auto",
        help="Serial baudrate or 'auto' (tries 500000 then 1000000)",
    )
    parser.add_argument("--id", type=int, default=None, help="Motor ID to use after scan")
    parser.add_argument("--pos", type=int, default=None, help="Target position (0..4095)")
    parser.add_argument("--speed", type=int, default=0, help="SET speed parameter (0 = max)")
    parser.add_argument("--acc", type=int, default=0, help="SET acceleration parameter (0 = max)")
    parser.add_argument("--with-speed", action="store_true", help="Read position+speed instead of position-only")
    parser.set_defaults(binary=True)
    parser.add_argument("--binary", dest="binary", action="store_true", help="Use binary fast path (default)")
    parser.add_argument("--text", dest="binary", action="store_false", help="Force text command mode")
    parser.add_argument(
        "--no-ack-set",
        action="store_true",
        help="Use binary no-ACK mode for SET/SETN (higher throughput, less delivery visibility)",
    )
    parser.add_argument(
        "--batch-size",
        type=int,
        default=1,
        help="Binary stress mode only: number of SET commands per frame via SETN (1..16)",
    )
    parser.add_argument(
        "--exec-period-ms",
        type=int,
        default=5,
        help="Enable staged fixed-rate execution on firmware at this period in ms (set 0 to disable)",
    )
    parser.add_argument(
        "--bench-reads",
        type=int,
        default=0,
        help="Run N GETP/GETPS round-trip latency samples after SET",
    )
    parser.add_argument(
        "--show-perf",
        action="store_true",
        help="Query and print firmware PERF counters before and after test",
    )
    parser.add_argument(
        "--stress-seconds",
        type=float,
        default=0.0,
        help="Run sustained SET stress for N seconds (0 disables)",
    )
    parser.add_argument(
        "--stress-hz",
        type=float,
        default=180.0,
        help="Target SET command rate during stress mode",
    )
    parser.add_argument(
        "--stress-delta",
        type=int,
        default=300,
        help="Position delta around target for stress oscillation",
    )
    args = parser.parse_args()

    if args.batch_size < 1 or args.batch_size > 16:
        print("--batch-size must be in range 1..16")
        return 2

    port = args.port or find_default_port()
    if not port:
        print("No serial port found. Use --port /dev/ttyUSB0 (or ttyACM0).")
        return 2

    try:
        baud_candidates = parse_baud_candidates(args.baud)
    except ValueError as exc:
        print(str(exc))
        return 2

    client = None
    active_baud = None

    try:
        # Attempt baud candidates; some boards reset on open and may miss first handshake.
        ids: list[int] = []
        for baud in baud_candidates:
            print(f"Opening {port} @ {baud}")
            trial = MotorSerialClient(port=port, baud=baud)
            time.sleep(1.2)
            ready = wait_for_firmware_ready(trial, timeout_s=6.0)
            if not ready:
                print("  warning: HELP handshake not seen; trying discovery anyway")

            # Configure bridge mode before discovery.
            trial.request("STREAM 0", wait_s=0.15, stop_on_prefix=("OK stream", "ERR"))
            trial.request("SCANVERBOSE 0", wait_s=0.15, stop_on_prefix=("OK scan_verbose", "ERR"))
            trial.request(f"BIN {1 if args.binary else 0}", wait_s=0.15, stop_on_prefix=("OK bin=", "ERR"))
            if args.exec_period_ms > 0:
                trial.request(f"EXEC 1 {args.exec_period_ms}", wait_s=0.15, stop_on_prefix=("OK exec_mode", "ERR"))
            else:
                trial.request("EXEC 0", wait_s=0.15, stop_on_prefix=("OK exec_mode", "ERR"))
            if args.binary:
                try:
                    trial.clear_input()
                    _ = bin_fastcfg(trial, 0, args.no_ack_set)
                except TimeoutError:
                    pass
            if args.with_speed:
                trial.request("TMODE POSSPD", wait_s=0.15, stop_on_prefix=("OK tmode", "ERR"))
            else:
                trial.request("TMODE POS", wait_s=0.15, stop_on_prefix=("OK tmode", "ERR"))

            ids = discover_ids_with_retries(trial)
            if ids:
                client = trial
                active_baud = baud
                break

            trial.close()

        if client is None:
            print("Could not open serial port on tested baud rates.")
            return 3

        # Keep a clear record of selected baud in auto mode.
        if len(baud_candidates) > 1:
            print(f"Using baud {active_baud}")

        # Keep command processing focused on position-only telemetry unless user asks otherwise.
        seq = 1

        if args.show_perf:
            perf, perf_cmd = request_perf_snapshot(client)
            if perf:
                print("perf(before):", perf)
            if perf_cmd:
                print("perf_cmd(before):", perf_cmd)

        print("\n[1/4] Serial link ready")
        print("  ", f"baud={active_baud}")
        if args.binary:
            print("  ", f"binary no_ack_set={1 if args.no_ack_set else 0}")
        if args.exec_period_ms > 0:
            print("  ", f"exec_mode=1 period_ms={args.exec_period_ms}")
        else:
            print("  ", "exec_mode=0")

        print("\n[2/4] Listing IDs...")
        if ids:
            print("  ", "OK ids=" + ",".join(str(i) for i in ids))
        else:
            print("  no IDs discovered")
        if not ids:
            print("No motors detected. Check power/data wiring and try again.")
            return 3

        motor_id = choose_motor_id(ids, args.id)
        print(f"Using motor ID {motor_id}")

        print("\n[3/4] Reading motor feedback...")
        read_cmd = f"GETPS {motor_id}" if args.with_speed else f"GETP {motor_id}"
        fb = None
        if args.binary and not args.with_speed:
            client.clear_input()
            pos = bin_getp(client, seq, motor_id)
            seq = (seq + 1) & 0xFF
            if pos is not None:
                fb = f"FBP id={motor_id} pos={pos}"
                print("  ", fb)
        else:
            out = client.request(read_cmd, wait_s=0.4, stop_on_prefix=("FBP id=", "FBPS id=", "ERR"))
            for ln in out:
                print("  ", ln)
            fb = parse_fb(out)
        if not fb:
            print("Warning: no FB line returned, continuing to position command.")

        target = choose_position(args.pos)
        speed = max(0, min(7500, args.speed))
        acc = max(0, min(255, args.acc))

        print("\n[4/4] Sending position command...")
        cmd = f"SET {motor_id} {target} {speed} {acc}"
        print("  cmd:", cmd)
        if args.binary:
            client.clear_input()
            ok = bin_set(client, seq, motor_id, target, speed, acc, wait_ack=not args.no_ack_set)
            seq = (seq + 1) & 0xFF
            print("  ", f"OK set id={motor_id} pos={target} speed={speed} acc={acc}" if ok else "ERR set_failed")
        else:
            out = client.request(cmd, wait_s=0.3, stop_on_prefix=("OK set", "ERR"))
            for ln in out:
                print("  ", ln)

        if args.bench_reads > 0:
            print(f"\n[bench] Running {args.bench_reads} read round-trip samples...")
            lat_ms: list[float] = []
            for _ in range(args.bench_reads):
                t0 = time.perf_counter()
                if args.binary and not args.with_speed:
                    client.clear_input()
                    pos = bin_getp(client, seq, motor_id)
                    seq = (seq + 1) & 0xFF
                    dt_ms = (time.perf_counter() - t0) * 1000.0
                    if pos is not None:
                        lat_ms.append(dt_ms)
                else:
                    out = client.request(
                        read_cmd,
                        wait_s=0.25,
                        stop_on_prefix=("FBP id=", "FBPS id=", "ERR"),
                    )
                    dt_ms = (time.perf_counter() - t0) * 1000.0
                    if parse_fb(out):
                        lat_ms.append(dt_ms)

            lat_ms.sort()
            if lat_ms:
                print(
                    "[bench] read_rtt_ms",
                    f"n={len(lat_ms)}",
                    f"mean={statistics.mean(lat_ms):.2f}",
                    f"p50={percentile(lat_ms, 50):.2f}",
                    f"p95={percentile(lat_ms, 95):.2f}",
                    f"max={max(lat_ms):.2f}",
                )
            else:
                print("[bench] no successful samples")

        if args.stress_seconds > 0:
            if args.stress_hz <= 0:
                print("[stress] stress-hz must be > 0")
                return 2

            runtime_batch_size = args.batch_size
            if args.binary and args.exec_period_ms > 0 and args.batch_size > 1:
                print(
                    "[stress] note: executor mode coalesces same-motor batched SETN updates; "
                    "forcing batch-size=1 to preserve trajectory fidelity"
                )
                runtime_batch_size = 1

            low = max(0, target - abs(args.stress_delta))
            high = min(4095, target + abs(args.stress_delta))
            if low == high:
                high = min(4095, low + 1)

            client.request("PERF RESET", wait_s=0.2, stop_on_prefix=("OK perf_reset", "ERR"))

            duration_s = args.stress_seconds
            period_s = 1.0 / args.stress_hz
            print(
                f"[stress] Running for {duration_s:.2f}s at target {args.stress_hz:.1f}Hz, "
                f"positions {low}<->{high}, batch_size={runtime_batch_size}"
            )

            lat_ms: list[float] = []
            sent = 0
            phase = 0
            t_end = time.perf_counter() + duration_s
            t_next = time.perf_counter()
            read_every = max(1, int(args.stress_hz // 10) if args.stress_hz >= 10 else 1)

            while time.perf_counter() < t_end:
                entries: list[tuple[int, int, int, int]] = []
                for _ in range(runtime_batch_size):
                    pos = low if phase == 0 else high
                    phase ^= 1
                    entries.append((motor_id, pos, speed, acc))

                if args.binary:
                    try:
                        if not args.no_ack_set:
                            client.clear_input()
                        if len(entries) == 1:
                            _ = bin_set(
                                client,
                                seq,
                                entries[0][0],
                                entries[0][1],
                                entries[0][2],
                                entries[0][3],
                                wait_ack=not args.no_ack_set,
                            )
                        else:
                            _ = bin_setn(client, seq, entries, wait_ack=not args.no_ack_set)
                    except TimeoutError:
                        pass
                    seq = (seq + 1) & 0xFF
                else:
                    for mid, pos, spd, a in entries:
                        client.send(f"SET {mid} {pos} {spd} {a}")
                sent += len(entries)

                if (sent % read_every) == 0:
                    t0 = time.perf_counter()
                    if args.binary and not args.with_speed:
                        try:
                            client.clear_input()
                            pos_now = bin_getp(client, seq, motor_id)
                            seq = (seq + 1) & 0xFF
                            if pos_now is not None:
                                lat_ms.append((time.perf_counter() - t0) * 1000.0)
                        except TimeoutError:
                            pass
                    else:
                        out = client.request(
                            read_cmd,
                            wait_s=0.25,
                            stop_on_prefix=("FBP id=", "FBPS id=", "ERR"),
                        )
                        if parse_fb(out):
                            lat_ms.append((time.perf_counter() - t0) * 1000.0)

                t_next += period_s * len(entries)
                sleep_s = t_next - time.perf_counter()
                if sleep_s > 0:
                    time.sleep(sleep_s)

            actual_rate = sent / duration_s if duration_s > 0 else 0.0
            print(f"[stress] sent={sent} actual_hz={actual_rate:.1f}")
            if lat_ms:
                lat_ms.sort()
                print(
                    "[stress] sample_read_rtt_ms",
                    f"n={len(lat_ms)}",
                    f"mean={statistics.mean(lat_ms):.2f}",
                    f"p50={percentile(lat_ms, 50):.2f}",
                    f"p95={percentile(lat_ms, 95):.2f}",
                    f"max={max(lat_ms):.2f}",
                )

        if args.show_perf:
            perf, perf_cmd = request_perf_snapshot(client)
            if perf:
                print("perf(after):", perf)
            if perf_cmd:
                print("perf_cmd(after):", perf_cmd)

        print("\nDone.")
        return 0

    finally:
        if client is not None:
            client.close()


if __name__ == "__main__":
    raise SystemExit(main())
