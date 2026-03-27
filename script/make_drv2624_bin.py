#!/usr/bin/env python3
import json
import struct
import sys
from pathlib import Path


DRV2624_FIRMWARE_MAGIC = 0x2624
DRV2624_MAX_LIBRARY_SIZE = 1024
DRV2624_MAX_EFFECT_SIZE = 0x1F


def usage(prog: str) -> None:
    print(
        "usage:\n"
        f"  {prog} <input.json> [output.bin]\n"
        "\n"
        "json format:\n"
        "  {\n"
        '    "fw_date": 20260325,\n'
        '    "revision": 0,\n'
        '    "effects": [\n'
        '      { "id": 1, "name": "click", "repeat": 0, "data": [63, 8, 0, 8] },\n'
        '      { "id": 2, "name": "double_click", "repeat": 0, "data": [63, 6, 0, 6, 63, 6, 0, 8] }\n'
        "    ]\n"
        "  }\n"
    )


def firmware_checksum(buffer: bytes) -> int:
    checksum = 0
    for index, value in enumerate(buffer):
        if 11 < index < 16:
            continue
        checksum += value
    return checksum


def require_int(name: str, value, minimum: int, maximum: int) -> int:
    if not isinstance(value, int):
        raise ValueError(f"{name} must be an integer")
    if value < minimum or value > maximum:
        raise ValueError(f"{name} must be in range [{minimum}, {maximum}]")
    return value


def build_library(spec: dict) -> tuple[bytes, int]:
    revision = require_int("revision", spec.get("revision", 0), 0, 255)
    raw_effects = spec.get("effects")
    if not isinstance(raw_effects, list) or not raw_effects:
        raise ValueError("effects must be a non-empty list")

    effects = []
    for item in raw_effects:
        if not isinstance(item, dict):
            raise ValueError("each effect must be an object")

        effect_id = require_int("effect.id", item.get("id"), 1, 255)
        repeat = require_int("effect.repeat", item.get("repeat", 0), 0, 7)
        data = item.get("data")
        if not isinstance(data, list) or not data:
            raise ValueError(f"effect {effect_id}: data must be a non-empty list")
        if len(data) % 2 != 0:
            raise ValueError(f"effect {effect_id}: data length must be even")
        if len(data) > DRV2624_MAX_EFFECT_SIZE:
            raise ValueError(
                f"effect {effect_id}: data length must be <= {DRV2624_MAX_EFFECT_SIZE}"
            )

        checked_data = bytes(
            require_int(f"effect {effect_id} data", value, 0, 255) for value in data
        )
        effects.append(
            {
                "id": effect_id,
                "name": item.get("name", f"effect_{effect_id}"),
                "repeat": repeat,
                "data": checked_data,
            }
        )

    effects.sort(key=lambda item: item["id"])
    for expected_id, effect in enumerate(effects, start=1):
        if effect["id"] != expected_id:
            raise ValueError(
                "effect ids must be contiguous and start at 1 "
                f"(expected {expected_id}, got {effect['id']})"
            )

    header_count = len(effects)
    start_addr = 1 + header_count * 3

    library = bytearray()
    library.append(revision)

    payload = bytearray()
    current_addr = start_addr
    for effect in effects:
        size = len(effect["data"])
        config = ((effect["repeat"] & 0x07) << 5) | (size & 0x1F)
        library.extend([(current_addr >> 8) & 0xFF, current_addr & 0xFF, config])
        payload.extend(effect["data"])
        current_addr += size

    library.extend(payload)

    if len(library) > DRV2624_MAX_LIBRARY_SIZE:
        raise ValueError(
            f"library size {len(library)} exceeds {DRV2624_MAX_LIBRARY_SIZE} bytes"
        )

    return bytes(library), len(effects)


def main(argv: list[str]) -> int:
    if len(argv) < 2 or len(argv) > 3:
        usage(argv[0])
        return 1

    input_path = Path(argv[1])
    output_path = Path(argv[2]) if len(argv) == 3 else Path("drv2624.bin")

    with input_path.open("r", encoding="utf-8") as handle:
        spec = json.load(handle)

    library, effect_count = build_library(spec)
    fw_date = require_int("fw_date", spec.get("fw_date", 20260325), 0, 0x7FFFFFFF)

    header = struct.pack(
        "<iiiii",
        DRV2624_FIRMWARE_MAGIC,
        20 + len(library),
        fw_date,
        0,
        effect_count,
    )
    image = bytearray(header + library)
    checksum = firmware_checksum(image)
    image[12:16] = struct.pack("<i", checksum)

    with output_path.open("wb") as handle:
        handle.write(image)

    print(f"wrote {output_path}")
    print(f"  total_size: {len(image)} bytes")
    print(f"  library:    {len(library)} bytes")
    print(f"  effects:    {effect_count}")
    print(f"  checksum:   0x{checksum:x}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
