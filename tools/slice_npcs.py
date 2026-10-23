#!/usr/bin/env python3
"""
NPC FF1 Pixel Remaster (Spriters Resource, "Miscellaneous - NPCs.png") -> спрайты NPC игры.

  python tools/slice_npcs.py "C:/.../Downloads/PC _ Computer - Final Fantasy (Pixel Remaster) - Miscellaneous - NPCs.png"

На листе у персонажа 6 кадров 16x16 подряд: вниз, вниз, влево, влево, вверх, вверх.
Каждый становится resources/Images/Npcs/Npc<Имя>.png в формате Walk-листов героя:
4 столбца (вверх, вниз, влево, вправо — как enum Direction) x 2 кадра; вправо — отражённое влево.

Ещё пишется resources/Maps/npcs.tsx — тайлсет для Tiled: NPC ставится на слой объектов "npcs"
как тайл и виден в редакторе (тайл показывает кадр «вниз» из листа).
"""
import os
import sys

from PIL import Image, ImageOps

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "resources", "Images", "Npcs")
TSX = os.path.join(ROOT, "resources", "Maps", "npcs.tsx")
F = 16

# Имя: (ряд, столбец первого кадра) в клетках 16x16 листа
NPCS = {
    "Townsman":      (0, 0),
    "TownsmanTeal":  (1, 0),
    "Merchant":      (2, 0),
    "OldMan":        (3, 0),
    "Dancer":        (4, 0),
    "Girl":          (5, 0),
    "Hooded":        (6, 0),
    "Woman":         (7, 0),
    "RedMage":       (9, 0),
    "Farmer":        (10, 0),
    "WhiteHair":     (11, 0),
    "Scholar":       (12, 0),
    "Elf":           (13, 0),
    "Child":         (0, 6),
    "BlackMage":     (1, 6),
    "Priest":        (2, 6),
    "Sage":          (3, 6),
    "Fairy":         (4, 6),
    "Wizard":        (5, 6),
    "GirlBlonde":    (6, 6),
    "DancerBlonde":  (7, 6),
    "PunkGreen":     (8, 6),
    "PunkPink":      (9, 6),
    "PunkRed":       (10, 6),
    "Garland":       (13, 6),
    "King":          (0, 12),
    "Prince":        (1, 12),
    "ElfKing":       (2, 12),
    "ElfGuard":      (3, 12),
    "Dwarf":         (4, 12),
    "Princess":      (6, 12),
    "Nun":           (7, 12),
    "Witch":         (8, 12),
    "Mermaid":       (9, 12),
    "Boy":           (10, 12),
    "GirlGreen":     (11, 12),
    "DwarfGold":     (12, 12),
    "Pirate":        (13, 12),
}


def frame(sheet, row, col):
    return sheet.crop((col * F, row * F, col * F + F, row * F + F))


def main(src):
    sheet = Image.open(src).convert("RGBA")
    os.makedirs(OUT_DIR, exist_ok=True)

    tiles = []
    for i, (name, (row, col)) in enumerate(NPCS.items()):
        down = [frame(sheet, row, col), frame(sheet, row, col + 1)]
        left = [frame(sheet, row, col + 2), frame(sheet, row, col + 3)]
        up = [frame(sheet, row, col + 4), frame(sheet, row, col + 5)]
        right = [ImageOps.mirror(f) for f in left]

        out = Image.new("RGBA", (4 * F, 2 * F), (0, 0, 0, 0))
        for c, frames in enumerate([up, down, left, right]):
            for r, f in enumerate(frames):
                out.paste(f, (c * F, r * F))
        file = f"Npc{name}.png"
        out.save(os.path.join(OUT_DIR, file))
        tiles.append(
            f' <tile id="{i}" x="{F}" y="0" width="{F}" height="{F}">\n'
            f'  <image source="../Images/Npcs/{file}" width="{4 * F}" height="{2 * F}"/>\n'
            f' </tile>\n')

    with open(TSX, "w", encoding="utf-8", newline="\n") as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        f.write(f'<tileset version="1.10" tiledversion="1.12.2" name="npcs" tilewidth="{F}" tileheight="{F}" '
                f'tilecount="{len(tiles)}" columns="0">\n')
        f.write(' <grid orientation="orthogonal" width="1" height="1"/>\n')
        f.writelines(tiles)
        f.write('</tileset>\n')
    print(f"{len(tiles)} NPC -> {OUT_DIR}, {TSX}")


if __name__ == "__main__":
    main(sys.argv[1])
