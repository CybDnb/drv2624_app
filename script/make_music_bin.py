#!/usr/bin/env python3

import argparse
import math
import struct
from pathlib import Path

MAGIC = b"SBN1"


def append_rest(samples: list[int], duration_ms: int, interval_us: int) -> None:
    if duration_ms < 0:
        raise ValueError("rest duration must be >= 0")
    count = max(1, (duration_ms * 1000) // interval_us)
    samples.extend([0] * count)


def append_tone(
    samples: list[int],
    freq_hz: float,
    duration_ms: int,
    amplitude: int,
    interval_us: int,
) -> None:
    if freq_hz <= 0:
        raise ValueError("tone frequency must be > 0")
    if duration_ms <= 0:
        raise ValueError("tone duration must be > 0")
    if amplitude < 0:
        amplitude = -amplitude
    if amplitude > 127:
        amplitude = 127

    count = max(1, (duration_ms * 1000) // interval_us)
    sample_rate = 1_000_000.0 / interval_us
    for i in range(count):
        t = i / sample_rate
        value = int(round(amplitude * math.sin(2.0 * math.pi * freq_hz * t)))
        if value < -128:
            value = -128
        elif value > 127:
            value = 127
        samples.append(value)


def parse_music(path: Path, interval_us: int) -> list[int]:
    samples: list[int] = []
    for lineno, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        cmd = parts[0]
        if cmd == "rest":
            if len(parts) != 2:
                raise ValueError(f"{path}:{lineno}: rest format: rest <duration_ms>")
            append_rest(samples, int(parts[1]), interval_us)
            continue
        if cmd == "tone":
            if len(parts) not in (3, 4):
                raise ValueError(
                    f"{path}:{lineno}: tone format: tone <freq_hz> <duration_ms> [amplitude]"
                )
            freq_hz = float(parts[1])
            duration_ms = int(parts[2])
            amplitude = int(parts[3]) if len(parts) == 4 else 70
            append_tone(samples, freq_hz, duration_ms, amplitude, interval_us)
            continue
        raise ValueError(f"{path}:{lineno}: unknown command: {cmd}")
    return samples


def write_bin(samples: list[int], interval_us: int, out_path: Path) -> None:
    header = struct.pack("<4sII", MAGIC, interval_us, len(samples))
    payload = bytes((sample & 0xFF) for sample in samples)
    out_path.write_bytes(header + payload)


def main() -> None:
    parser = argparse.ArgumentParser(description="Compile music text into music.bin")
    parser.add_argument("input", type=Path, help="input music text file")
    parser.add_argument("output", type=Path, help="output .bin path")
    parser.add_argument("--interval-us", type=int, default=650, help="sample interval in microseconds")
    args = parser.parse_args()

    if args.interval_us <= 0:
        raise SystemExit("interval-us must be > 0")

    samples = parse_music(args.input, args.interval_us)
    if not samples:
        raise SystemExit("no music samples generated")

    write_bin(samples, args.interval_us, args.output)
    print(f"wrote {args.output}")
    print(f"  samples:     {len(samples)}")
    print(f"  interval_us: {args.interval_us}")


if __name__ == "__main__":
    main()
