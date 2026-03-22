#!/usr/bin/env python3
import argparse
import csv
import glob
import math
import struct
import sys
import time
from datetime import datetime

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


def wait_for_firmware_ready(client: "MotorSerialClient", timeout_s: float = 3.5) -> bool:
    end_t = time.time() + timeout_s
    while time.time() < end_t:
        out = client.request("HELP", wait_s=0.25)
        for ln in out:
            if ln.startswith("OK commands:"):
                return True
        time.sleep(0.1)
    return False


class MotorSerialClient:
    def __init__(self, port: str, baud: int, timeout: float = 0.15):
        self.ser = serial.Serial(port=port, baudrate=baud, timeout=timeout)
        time.sleep(0.25)
        self.ser.reset_input_buffer()

    def close(self) -> None:
        self.ser.close()

    def send(self, cmd: str) -> None:
        self.ser.write((cmd.strip() + "\n").encode("ascii", errors="ignore"))

    def read_lines(self, window_s: float = 0.5, stop_on_prefix: tuple[str, ...] = ()) -> list[str]:
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
        wait_s: float = 0.5,
        stop_on_prefix: tuple[str, ...] = (),
    ) -> list[str]:
        self.send(cmd)
        return self.read_lines(wait_s, stop_on_prefix=stop_on_prefix)

    def write_raw(self, data: bytes) -> None:
        self.ser.write(data)


BIN_SOF = 0x7E
BIN_SET = 0x01
BIN_GETP = 0x02
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
        sof = client.ser.read(1)
        if not sof:
            continue
        if sof[0] != BIN_SOF:
            continue
        hdr = client.ser.read(3)
        if len(hdr) != 3:
            continue
        cmd, seq, length = hdr[0], hdr[1], hdr[2]
        payload = client.ser.read(length) if length else b""
        if len(payload) != length:
            continue
        crc_b = client.ser.read(1)
        if len(crc_b) != 1:
            continue
        if crc_b[0] != bin_crc_xor(cmd, seq, payload):
            continue
        return cmd, seq, payload
    raise TimeoutError("Timed out waiting for binary frame")


def send_bin_fastcfg(client: MotorSerialClient, seq: int, noack_set: bool) -> bool:
    payload = struct.pack("<B", 0x01 if noack_set else 0x00)
    client.write_raw(build_bin_frame(BIN_FASTCFG, seq, payload))
    cmd, rsp_seq, rsp = read_bin_frame(client)
    if cmd != (BIN_FASTCFG | 0x80) or rsp_seq != (seq & 0xFF) or len(rsp) < 1:
        return False
    return rsp[0] == 0


def send_bin_set(client: MotorSerialClient, seq: int, motor_id: int, pos: int, speed: int, acc: int) -> None:
    payload = struct.pack("<BHHB", motor_id & 0xFF, pos & 0xFFFF, speed & 0xFFFF, acc & 0xFF)
    client.write_raw(build_bin_frame(BIN_SET, seq, payload))


def send_bin_setn(client: MotorSerialClient, seq: int, entries: list[tuple[int, int, int, int]]) -> None:
    payload = b"".join(struct.pack("<BHHB", mid & 0xFF, pos & 0xFFFF, spd & 0xFFFF, a & 0xFF) for mid, pos, spd, a in entries)
    client.write_raw(build_bin_frame(BIN_SETN, seq, payload))


def bin_getp(client: MotorSerialClient, seq: int, motor_id: int) -> int | None:
    payload = struct.pack("<B", motor_id & 0xFF)
    client.write_raw(build_bin_frame(BIN_GETP, seq, payload))
    cmd, rsp_seq, rsp = read_bin_frame(client)
    if cmd != (BIN_GETP | 0x80) or rsp_seq != (seq & 0xFF) or len(rsp) < 1:
        return None
    if rsp[0] != 0 or len(rsp) < 4:
        return None
    return int(struct.unpack("<H", rsp[2:4])[0])


def parse_getp_text(lines: list[str]) -> int | None:
    for ln in lines:
        if not ln.startswith("FBP id="):
            continue
        parts = ln.split()
        for p in parts:
            if p.startswith("pos="):
                raw = p.split("=", 1)[1]
                if raw.lstrip("-").isdigit():
                    return int(raw)
    return None


def save_trace_csv(path: str, rows: list[tuple[float, int, int, int]]) -> None:
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["t_s", "commanded", "actual", "error"])
        for t_s, cmd, actual, err in rows:
            w.writerow([f"{t_s:.6f}", cmd, actual, err])


def save_trace_plot(path: str, rows: list[tuple[float, int, int, int]]) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("Plot skipped: missing matplotlib (pip install matplotlib)")
        return

    t = [r[0] for r in rows]
    cmd = [r[1] for r in rows]
    act = [r[2] for r in rows]
    err = [r[3] for r in rows]

    fig, axes = plt.subplots(2, 1, figsize=(10, 6), sharex=True)
    axes[0].plot(t, cmd, label="commanded", linewidth=1.2)
    axes[0].plot(t, act, label="actual", linewidth=1.2)
    axes[0].set_ylabel("position")
    axes[0].legend(loc="best")
    axes[0].grid(True, alpha=0.3)

    axes[1].plot(t, err, color="tab:red", linewidth=1.0)
    axes[1].set_xlabel("time [s]")
    axes[1].set_ylabel("error")
    axes[1].grid(True, alpha=0.3)

    fig.tight_layout()
    fig.savefig(path, dpi=140)
    plt.close(fig)


def parse_ids(lines: list[str]) -> list[int]:
    for ln in lines:
        if ln.startswith("OK ids="):
            payload = ln.split("=", 1)[1].strip()
            if not payload:
                return []
            out: list[int] = []
            for tok in payload.split(","):
                tok = tok.strip()
                if tok.isdigit():
                    out.append(int(tok))
            return out
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
    # Some boards reboot on serial-open and need extra time before scan state is valid.
    for attempt in range(1, 4):
        if attempt > 1:
            time.sleep(0.6)

        client.request("SCAN", wait_s=0.5, stop_on_prefix=("OK scan_started", "ERR"))
        # Give firmware enough time to complete scan cycle and publish list.
        time.sleep(1.1)

        out = client.request("LIST", wait_s=0.4, stop_on_prefix=("OK ids=",))
        ids = parse_ids(out)
        if not ids:
            ids = wait_for_ids(client, timeout_s=4.5)
        if ids:
            return ids

    # Fallback when scan list is stale: probe directly.
    return probe_ids_by_ping(client, max_id=20)


def choose_motor_id(ids: list[int], desired: int | None) -> int:
    if desired is not None:
        if desired not in ids:
            raise ValueError(f"Requested id {desired} not found in {ids}")
        return desired
    if len(ids) == 1:
        return ids[0]

    print("Detected IDs:", ", ".join(str(x) for x in ids))
    while True:
        raw = input("Select motor ID: ").strip()
        if raw.isdigit() and int(raw) in ids:
            return int(raw)
        print("Invalid ID. Pick from detected list.")


def ramp_points(min_pos: int, max_pos: int, step: int):
    up = list(range(min_pos, max_pos + 1, step))
    if up[-1] != max_pos:
        up.append(max_pos)
    down = list(range(max_pos, min_pos - 1, -step))
    if down[-1] != min_pos:
        down.append(min_pos)
    while True:
        for p in up:
            yield p
        for p in down:
            yield p


def sine_position(min_pos: int, max_pos: int, cycle_s: float, t_rel_s: float) -> int:
    mid = 0.5 * (min_pos + max_pos)
    amp = 0.5 * (max_pos - min_pos)
    phase = 2.0 * math.pi * (t_rel_s / cycle_s)
    pos = int(round(mid + amp * math.sin(phase)))
    if pos < min_pos:
        return min_pos
    if pos > max_pos:
        return max_pos
    return pos


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Continuously sway motor position between two limits using SET commands."
    )
    parser.add_argument("--port", default=None, help="Serial port, e.g. /dev/ttyUSB0")
    parser.add_argument(
        "--baud",
        default="auto",
        help="Serial baudrate or 'auto' (tries 500000 then 1000000)",
    )
    parser.add_argument("--id", type=int, default=None, help="Motor ID (if omitted, choose after scan)")
    parser.add_argument("--min-pos", type=int, default=1000, help="Minimum position")
    parser.add_argument("--max-pos", type=int, default=3000, help="Maximum position")
    parser.add_argument("--step", type=int, default=50, help="Position step per command")
    parser.add_argument("--interval-ms", type=int, default=10, help="Delay between SET commands")
    parser.add_argument(
        "--waveform",
        choices=("sine", "triangle"),
        default="sine",
        help="Command profile shape (default: sine for smoother motion)",
    )
    parser.add_argument(
        "--cycle-s",
        type=float,
        default=0.0,
        help="Cycle period for sine waveform in seconds (0 = auto-derive from step/interval)",
    )
    parser.add_argument("--speed", type=int, default=0, help="SET speed parameter (0 = max)")
    parser.add_argument("--acc", type=int, default=0, help="SET acc parameter (0 = max)")
    parser.set_defaults(binary=True)
    parser.set_defaults(no_ack_set=True)
    parser.add_argument("--binary", dest="binary", action="store_true", help="Use binary fast path (default)")
    parser.add_argument("--text", dest="binary", action="store_false", help="Force text command mode")
    parser.add_argument("--no-ack-set", dest="no_ack_set", action="store_true", help="Use no-ACK binary SET mode (default)")
    parser.add_argument("--ack-set", dest="no_ack_set", action="store_false", help="Require ACK on binary SET frames")
    parser.add_argument("--batch-size", type=int, default=1, help="Binary mode: SET commands per frame via SETN (1..16)")
    parser.add_argument(
        "--exec-period-ms",
        type=int,
        default=5,
        help="Enable staged fixed-rate execution on firmware at this period in ms (set 0 to disable)",
    )
    parser.add_argument(
        "--duration-s",
        type=float,
        default=0.0,
        help="Run sway for a fixed duration in seconds (0 = run until Ctrl+C)",
    )
    parser.add_argument(
        "--trace",
        action="store_true",
        help="Capture commanded vs actual trace during sway",
    )
    parser.add_argument(
        "--trace-sample-hz",
        type=float,
        default=40.0,
        help="Sampling rate for actual-position reads when --trace is enabled",
    )
    parser.add_argument(
        "--trace-csv",
        default="",
        help="Output CSV path for trace (default: sway_trace_<timestamp>.csv)",
    )
    parser.add_argument(
        "--trace-plot",
        default="",
        help="Output PNG path for trace plot (default: sway_trace_<timestamp>.png)",
    )
    args = parser.parse_args()

    if args.min_pos < 0 or args.max_pos > 4095 or args.min_pos >= args.max_pos:
        print("Invalid range. Require 0 <= min-pos < max-pos <= 4095")
        return 2
    if args.step <= 0:
        print("step must be > 0")
        return 2
    if args.interval_ms < 1:
        print("interval-ms must be >= 1")
        return 2
    if args.batch_size < 1 or args.batch_size > 16:
        print("batch-size must be in range 1..16")
        return 2
    if args.duration_s < 0:
        print("duration-s must be >= 0")
        return 2
    if args.trace_sample_hz <= 0:
        print("trace-sample-hz must be > 0")
        return 2
    if args.cycle_s < 0:
        print("cycle-s must be >= 0")
        return 2

    runtime_batch_size = args.batch_size
    if args.binary and args.exec_period_ms > 0 and args.batch_size > 1:
        print(
            "note: executor mode coalesces same-motor batched SETN updates; "
            "forcing batch-size=1 to preserve trajectory fidelity"
        )
        runtime_batch_size = 1

    speed = max(0, min(7500, args.speed))
    acc = max(0, min(255, args.acc))
    sleep_s = args.interval_ms / 1000.0

    cycle_s = args.cycle_s
    if args.waveform == "sine" and cycle_s <= 0:
        # Match sine max slope to prior triangle command slope for comparable aggressiveness.
        amp = 0.5 * (args.max_pos - args.min_pos)
        cmd_slope = args.step / sleep_s
        if amp <= 0 or cmd_slope <= 0:
            cycle_s = 4.0
        else:
            cycle_s = (2.0 * math.pi * amp) / cmd_slope

    port = args.port or find_default_port()
    if not port:
        print("No serial port found. Use --port /dev/ttyUSB0")
        return 2

    try:
        baud_candidates = parse_baud_candidates(args.baud)
    except ValueError as exc:
        print(str(exc))
        return 2

    client = None
    motor_id: int | None = None
    seq = 1

    try:
        ids: list[int] = []
        active_baud: int | None = None
        for baud in baud_candidates:
            print(f"Opening {port} @ {baud}")
            trial = MotorSerialClient(port=port, baud=baud)
            # Allow MCU reboot/USB-serial settle before handshake.
            time.sleep(1.2)
            ready = wait_for_firmware_ready(trial, timeout_s=6.0)
            if not ready:
                print("  warning: HELP handshake not seen; trying discovery anyway")
            trial.request("STREAM 0", wait_s=0.15, stop_on_prefix=("OK stream", "ERR"))
            trial.request("TMODE POS", wait_s=0.15, stop_on_prefix=("OK tmode", "ERR"))
            trial.request(f"BIN {1 if args.binary else 0}", wait_s=0.15, stop_on_prefix=("OK bin=", "ERR"))
            if args.exec_period_ms > 0:
                trial.request(f"EXEC 1 {args.exec_period_ms}", wait_s=0.15, stop_on_prefix=("OK exec_mode", "ERR"))
            else:
                trial.request("EXEC 0", wait_s=0.15, stop_on_prefix=("OK exec_mode", "ERR"))
            if args.binary:
                try:
                    trial.ser.reset_input_buffer()
                    _ = send_bin_fastcfg(trial, 0, args.no_ack_set)
                except TimeoutError:
                    pass

            print("Scanning motors...")
            ids = discover_ids_with_retries(trial)
            if ids:
                print("  ", "OK ids=" + ",".join(str(i) for i in ids))
            else:
                print("  no IDs discovered on this baud")

            if ids:
                client = trial
                active_baud = baud
                break

            trial.close()

        if not ids:
            print("No motors detected on tested baud rates.")
            return 3

        if len(baud_candidates) > 1:
            print(f"Using baud {active_baud}")

        motor_id = choose_motor_id(ids, args.id)
        print(f"Using motor ID {motor_id}")
        if args.binary:
            print(f"Binary mode: no_ack_set={1 if args.no_ack_set else 0}, batch_size={runtime_batch_size}")
        if args.exec_period_ms > 0:
            print(f"Executor mode: enabled period_ms={args.exec_period_ms}")
        else:
            print("Executor mode: disabled")

        client.request(f"TORQUE {motor_id} 1", wait_s=0.2)
        print(
            f"Starting sway: id={motor_id}, range={args.min_pos}-{args.max_pos}, "
            f"step={args.step}, interval={args.interval_ms}ms, waveform={args.waveform}, speed={speed}, acc={acc}"
        )
        if args.waveform == "sine":
            print(f"Sine cycle period: {cycle_s:.3f}s")
        if args.duration_s > 0:
            print(f"Run duration: {args.duration_s:.2f}s")
        else:
            print("Press Ctrl+C to stop.")

        trace_rows: list[tuple[float, int, int, int]] = []
        next_trace_t = 0.0
        trace_period_s = 1.0 / args.trace_sample_hz
        t0 = time.perf_counter()
        t_end = (t0 + args.duration_s) if args.duration_s > 0 else None
        if args.trace:
            print(f"Trace capture enabled at {args.trace_sample_hz:.1f} Hz")

        point_iter = ramp_points(args.min_pos, args.max_pos, args.step)
        while True:
            now = time.perf_counter()
            if t_end is not None and now >= t_end:
                break

            if args.waveform == "triangle":
                pos = next(point_iter)
            else:
                pos = sine_position(args.min_pos, args.max_pos, cycle_s, now - t0)

            if args.binary:
                if runtime_batch_size <= 1:
                    send_bin_set(client, seq, motor_id, pos, speed, acc)
                else:
                    entries: list[tuple[int, int, int, int]] = [(motor_id, pos, speed, acc)]
                    for _ in range(runtime_batch_size - 1):
                        if args.waveform == "triangle":
                            next_pos = next(point_iter)
                        else:
                            next_now = now + (sleep_s * (len(entries)))
                            next_pos = sine_position(args.min_pos, args.max_pos, cycle_s, next_now - t0)
                        entries.append((motor_id, next_pos, speed, acc))
                    send_bin_setn(client, seq, entries)
                seq = (seq + 1) & 0xFF
            else:
                cmd = f"SET {motor_id} {pos} {speed} {acc}"
                client.send(cmd)

            if args.trace and now >= next_trace_t:
                actual_pos = None
                if args.binary:
                    try:
                        actual_pos = bin_getp(client, seq, motor_id)
                    except TimeoutError:
                        actual_pos = None
                    finally:
                        seq = (seq + 1) & 0xFF
                else:
                    out = client.request(f"GETP {motor_id}", wait_s=0.15, stop_on_prefix=("FBP id=", "ERR"))
                    actual_pos = parse_getp_text(out)

                if actual_pos is not None:
                    t_rel = now - t0
                    err = actual_pos - pos
                    trace_rows.append((t_rel, pos, actual_pos, err))
                next_trace_t = now + trace_period_s

            time.sleep(sleep_s * (runtime_batch_size if args.binary else 1))

        if args.trace:
            stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            csv_path = args.trace_csv or f"sway_trace_{stamp}.csv"
            png_path = args.trace_plot or f"sway_trace_{stamp}.png"
            if trace_rows:
                save_trace_csv(csv_path, trace_rows)
                save_trace_plot(png_path, trace_rows)
                abs_err = [abs(r[3]) for r in trace_rows]
                mean_abs = sum(abs_err) / len(abs_err)
                max_abs = max(abs_err)
                print(f"Trace saved: {csv_path}")
                print(f"Plot saved: {png_path}")
                print(f"Tracking error |mean|={mean_abs:.2f} |max|={max_abs:.2f} (ticks)")
            else:
                print("Trace capture requested, but no samples were collected.")

    except KeyboardInterrupt:
        print("\nStopping sway...")
        try:
            if motor_id is not None:
                client.request(f"STOP {motor_id}", wait_s=0.2)
        except Exception:
            pass
        return 0
    finally:
        if client is not None:
            client.close()


if __name__ == "__main__":
    raise SystemExit(main())
