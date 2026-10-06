#!/usr/bin/env python3
"""Writes docs/screenshots/swatches.png from the panel palette (python3 standard library only).

Usage: python3 docs/tools/make_swatches.py
Keep PALETTE in step with app/Source/ui/Theme.h and docs/DESIGN_SWATCHES.md.
"""
import struct
import zlib
from pathlib import Path

# name, hex, note
PALETTE = [
    ("panel cream",     "d8c7a0", "matte plastic, main faceplate"),
    ("frame putty",     "e4d4ae", "tube frame highlight"),
    ("frame body",      "cdb88c", "tube frame mid-tone"),
    ("shadow",          "9b8559", "cream in shade"),
    ("ink black",       "17120b", "knobs, screen-printed legends"),
    ("legend black",    "1c1710", "outline boxes"),
    ("lamp green",      "4cf29a", "pushbutton LED"),
    ("metal gray",      "cfc8b6", "painted-metal module plates"),
    ("metal gray dark", "aaa392", "plate shade"),
    ("teal glass",      "4be3d6", "read-outs on dark glass"),
]

CELL_W, CELL_H, PAD = 220, 150, 12
COLUMNS = 5

FONT = {  # 5x7 bitmap, uppercase + digits + a few symbols
    "A": "01110 10001 10001 11111 10001 10001 10001", "B": "11110 10001 10001 11110 10001 10001 11110",
    "C": "01110 10001 10000 10000 10000 10001 01110", "D": "11110 10001 10001 10001 10001 10001 11110",
    "E": "11111 10000 10000 11110 10000 10000 11111", "F": "11111 10000 10000 11110 10000 10000 10000",
    "G": "01110 10001 10000 10111 10001 10001 01111", "H": "10001 10001 10001 11111 10001 10001 10001",
    "I": "01110 00100 00100 00100 00100 00100 01110", "J": "00111 00010 00010 00010 00010 10010 01100",
    "K": "10001 10010 10100 11000 10100 10010 10001", "L": "10000 10000 10000 10000 10000 10000 11111",
    "M": "10001 11011 10101 10101 10001 10001 10001", "N": "10001 11001 10101 10011 10001 10001 10001",
    "O": "01110 10001 10001 10001 10001 10001 01110", "P": "11110 10001 10001 11110 10000 10000 10000",
    "Q": "01110 10001 10001 10001 10101 10010 01101", "R": "11110 10001 10001 11110 10100 10010 10001",
    "S": "01111 10000 10000 01110 00001 00001 11110", "T": "11111 00100 00100 00100 00100 00100 00100",
    "U": "10001 10001 10001 10001 10001 10001 01110", "V": "10001 10001 10001 10001 10001 01010 00100",
    "W": "10001 10001 10001 10101 10101 11011 10001", "X": "10001 10001 01010 00100 01010 10001 10001",
    "Y": "10001 10001 01010 00100 00100 00100 00100", "Z": "11111 00001 00010 00100 01000 10000 11111",
    "0": "01110 10001 10011 10101 11001 10001 01110", "1": "00100 01100 00100 00100 00100 00100 01110",
    "2": "01110 10001 00001 00010 00100 01000 11111", "3": "11110 00001 00001 01110 00001 00001 11110",
    "4": "00010 00110 01010 10010 11111 00010 00010", "5": "11111 10000 11110 00001 00001 10001 01110",
    "6": "00110 01000 10000 11110 10001 10001 01110", "7": "11111 00001 00010 00100 01000 01000 01000",
    "8": "01110 10001 10001 01110 10001 10001 01110", "9": "01110 10001 10001 01111 00001 00010 01100",
    "#": "01010 01010 11111 01010 11111 01010 01010", "-": "00000 00000 00000 11111 00000 00000 00000",
    ",": "00000 00000 00000 00000 00110 00100 01000", " ": "00000 00000 00000 00000 00000 00000 00000",
}


def rgb(hex_text):
    return tuple(int(hex_text[i:i + 2], 16) for i in (0, 2, 4))


def draw_text(canvas, width, x, y, text, colour, scale=2):
    for ch in text.upper():
        rows = FONT.get(ch, FONT[" "]).split()
        for ry, row in enumerate(rows):
            for rx, bit in enumerate(row):
                if bit == "1":
                    for sy in range(scale):
                        for sx in range(scale):
                            px, py = x + rx * scale + sx, y + ry * scale + sy
                            canvas[py * width + px] = colour
        x += 6 * scale


def write_png(path, width, height, pixels):
    raw = b"".join(b"\x00" + b"".join(bytes(pixels[y * width + x]) for x in range(width)) for y in range(height))

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    Path(path).write_bytes(png)


def main():
    rows = (len(PALETTE) + COLUMNS - 1) // COLUMNS
    width = COLUMNS * CELL_W + (COLUMNS + 1) * PAD
    height = rows * CELL_H + (rows + 1) * PAD
    canvas = [(40, 36, 30)] * (width * height)
    for i, (name, hex_text, note) in enumerate(PALETTE):
        col, row = i % COLUMNS, i // COLUMNS
        x0, y0 = PAD + col * (CELL_W + PAD), PAD + row * (CELL_H + PAD)
        for y in range(CELL_H - 44):
            for x in range(CELL_W):
                canvas[(y0 + y) * width + x0 + x] = rgb(hex_text)
        for y in range(CELL_H - 44, CELL_H):
            for x in range(CELL_W):
                canvas[(y0 + y) * width + x0 + x] = (238, 232, 220)
        draw_text(canvas, width, x0 + 8, y0 + CELL_H - 40, name, (23, 18, 11))
        draw_text(canvas, width, x0 + 8, y0 + CELL_H - 20, "#" + hex_text, (23, 18, 11))
    out = Path(__file__).resolve().parents[1] / "screenshots" / "swatches.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    write_png(out, width, height, canvas)
    print("wrote", out)


if __name__ == "__main__":
    main()
