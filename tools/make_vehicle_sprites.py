#!/usr/bin/env python3
"""
Временные спрайты транспорта: Ship.png, Canoe.png, Airship.png в resources/Images/Vehicles.

  python tools/make_vehicle_sprites.py

Формат листа (свои спрайты кладите в том же формате и с теми же именами):
  4 колонки — направления Up, Down, Left, Right (как enum Direction в коде),
  2 строки — кадры анимации (лодка покачивается, у дирижабля крутятся винты),
  кадр любого размера, но все одинаковые: лист 64x32 = кадры 16x16, 128x64 = кадры 32x32.
  Низ кадра — «ватерлиния»: он ставится на нижний край клетки. Фон прозрачный.

Pillow не нужен: PNG пишется через zlib.
"""
import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "resources", "Images", "Vehicles")

COLORS = {
    ".": (0, 0, 0, 0),
    "K": (24, 24, 32, 255),      # контур
    "W": (240, 240, 232, 255),   # парус
    "B": (140, 80, 40, 255),     # дерево
    "b": (190, 120, 60, 255),    # светлое дерево
    "R": (200, 40, 40, 255),     # красный
    "P": (240, 190, 140, 255),   # кожа
    "Y": (230, 200, 90, 255),    # оболочка дирижабля
    "y": (180, 150, 60, 255),    # тень оболочки
    "G": (170, 170, 185, 255),   # винт
}

SHIP_RIGHT = [
    "................",
    ".......K........",
    ".......KWW......",
    ".......KWWW.....",
    ".......KWWWW....",
    ".......KWWWWW...",
    ".......KWWWWWW..",
    ".....RRK........",
    ".......K........",
    ".KKKKKKKKKKKKKK.",
    ".KBBBBBBBBBBBBK.",
    "..KBbbbbbbbbBK..",
    "...KBBBBBBBBK...",
    "....KKKKKKKK....",
    "................",
    "................",
]
SHIP_DOWN = [
    "................",
    ".......KK.......",
    "....KWWKKWWK....",
    "...KWWWKKWWWK...",
    "...KWWWKKWWWK...",
    "...KWWWKKWWWK...",
    "....KWWKKWWK....",
    ".......KK.......",
    "....KKKKKKKK....",
    "...KBBBBBBBBK...",
    "...KBbbbbbbBK...",
    "....KBBBBBBK....",
    ".....KBBBBK.....",
    "......KKKK......",
    "................",
    "................",
]
CANOE_RIGHT = [
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    ".......KK.......",
    "......KPPK......",
    "......KRRK...K..",
    "......KRRK..K...",
    ".KKKKKKKKKKKKKK.",
    ".KBbbbbbbbbbbBK.",
    "..KKKKKKKKKKKK..",
    "................",
    "................",
]
CANOE_DOWN = [
    "................",
    "................",
    ".......KK.......",
    "......KbbK......",
    "......KbbK......",
    "......KPPK......",
    ".....KKRRKK.....",
    "......KRRK......",
    "......KbbK......",
    "......KbbK......",
    "......KbbK......",
    "......KbbK......",
    ".......KK.......",
    "................",
    "................",
    "................",
]
AIRSHIP_RIGHT = [
    "................",
    "................",
    "....KKKKKKKK....",
    "...KYYYYYYYYK...",
    "..KYYYYYYYYYYK..",
    "..KYYYYYYYYYYK..",
    "..KyyyyyyyyyyK..",
    "...KyyyyyyyyK...",
    "....KKKKKKKK....",
    ".....K....K.....",
    "....KBBBBBBK....",
    "...KBbbbbbbBK...",
    "....KKKKKKKK....",
    "................",
    "................",
    "................",
]
AIRSHIP_DOWN = [
    "................",
    "................",
    ".....KKKKKK.....",
    "....KYYYYYYK....",
    "...KYYYYYYYYK...",
    "...KYYYYYYYYK...",
    "...KyyyyyyyyK...",
    "....KyyyyyyK....",
    ".....KKKKKK.....",
    "......K..K......",
    ".....KBBBBK.....",
    "....KBbbbbBK....",
    ".....KKKKKK.....",
    "................",
    "................",
    "................",
]


def mirror(grid):
    return [row[::-1] for row in grid]


def shifted(grid, dy):
    """Кадр «качнулся»: весь рисунок на dy пикселей вниз."""
    empty = "." * len(grid[0])
    return [empty] * dy + grid[:len(grid) - dy]


def with_props(grid, frame, sides):
    """Винты дирижабля: в кадре 0 вертикальная лопасть, в кадре 1 горизонтальная."""
    g = [list(r) for r in grid]
    for x in sides:
        if frame == 0:
            for y in (9, 10, 11):
                g[y][x] = "G"
        else:
            for dx in (-1, 0, 1):
                if 0 <= x + dx < 16:
                    g[10][x + dx] = "G"
    return ["".join(r) for r in g]


def sheet(frames):
    """frames[row][col] — сетки 16x16; возвращает RGBA-строки листа."""
    rows = []
    for frame_row in frames:
        for y in range(16):
            line = []
            for grid in frame_row:
                line.extend(COLORS[c] for c in grid[y])
            rows.append(line)
    return rows


def write_png(path, rows):
    h, w = len(rows), len(rows[0])
    raw = b"".join(b"\x00" + bytes(v for px in row for v in px) for row in rows)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def boat(right, down):
    left, up = mirror(right), down                       # сзади лодка выглядит как спереди
    frame0 = [up, down, left, right]
    frame1 = [shifted(g, 1) for g in frame0]             # покачивание на волне
    return [frame0, frame1]


def airship():
    right, down = AIRSHIP_RIGHT, AIRSHIP_DOWN
    out = []
    for frame in (0, 1):
        r = with_props(right, frame, [1])                # винт сзади
        d = with_props(down, frame, [1, 14])             # спереди видны оба
        out.append([d, d, mirror(r), r])
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, frames in (("Ship", boat(SHIP_RIGHT, SHIP_DOWN)),
                         ("Canoe", boat(CANOE_RIGHT, CANOE_DOWN)),
                         ("Airship", airship())):
        path = os.path.join(OUT, name + ".png")
        write_png(path, sheet(frames))
        print("written", os.path.relpath(path, ROOT))


if __name__ == "__main__":
    main()
