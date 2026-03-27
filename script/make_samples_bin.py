#!/usr/bin/env python3

import argparse
import struct
from pathlib import Path

MAGIC = b"SBN1"


def parse_samples(path: Path) -> list[int]:
    samples: list[int] = []
    for lineno, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue
        for token in line.split():
            try:
                value = int(token, 10)
            except ValueError as exc:
                raise ValueError(f"{path}:{lineno}: invalid integer sample: {token}") from exc
            if value < -128 or value > 127:
                raise ValueError(f"{path}:{lineno}: sample out of range [-128, 127]: {value}")
            samples.append(value)
    return samples


def write_bin(samples: list[int], interval_us: int, out_path: Path) -> None:
    header = struct.pack("<4sII", MAGIC, interval_us, len(samples))
    payload = bytes((sample & 0xFF) for sample in samples)
    out_path.write_bytes(header + payload)


def main() -> None:
    parser = argparse.ArgumentParser(description="Compile raw RTP samples text into samples.bin")
    parser.add_argument("input", type=Path, help="input text file with whitespace-separated samples")
    parser.add_argument("output", type=Path, help="output .bin path")
    parser.add_argument("--interval-us", type=int, default=650, help="sample interval in microseconds")
    args = parser.parse_args()

    if args.interval_us <= 0:
        raise SystemExit("interval-us must be > 0")

    samples = parse_samples(args.input)
    if not samples:
        raise SystemExit("no samples found")

    write_bin(samples, args.interval_us, args.output)
    print(f"wrote {args.output}")
    print(f"  samples:     {len(samples)}")
    print(f"  interval_us: {args.interval_us}")


if __name__ == "__main__":
    main()
