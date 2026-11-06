#!/usr/bin/env python3
"""
Задники боя из листа FF1 Pixel Remaster ("Battle Effects - Battle Backgrounds.png").

  python tools/slice_pr_backgrounds.py SHEET

Сохраняет resources/Images/BattleBackgrounds/BB<Имя>.png (31 штука, 472x232).
Имя задаётся полем "background" у зоны в Tiled и у группы встреч в tools/areas.json;
на карте мира без него задник выбирается по местности (Plains, Forest, Marsh, Desert, Sea, River).
"""
import argparse
import os

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "resources", "Images", "BattleBackgrounds")

# Порядок как в листе: ряды сверху вниз, по 4 слева направо
NAMES = [
    "Plains", "Forest", "DeepForest", "Wasteland",
    "Marsh", "Swamp", "Waterfall", "River",
    "Sea", "Ruins", "SunkenRuins", "CastleHall",
    "CastleBridge", "BlueHall", "GreenHall", "BlueShrine",
    "DesertTemple", "GoldHall", "Cave", "CaveLight",
    "GreenCave", "PurpleCave", "Lava", "LavaShrine",
    "IceCave", "SandCave", "Badlands", "Desert",
    "Town", "ShipDeck", "Space",
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sheet")
    args = ap.parse_args()
    img = Image.open(args.sheet).convert("RGBA")
    a = np.asarray(img).astype(int)
    fg = (a[..., :3].max(axis=2) > 8) & (a[..., 3] > 0)        # фон листа чёрный
    lab, _ = ndimage.label(fg)
    boxes = [(s[0].start, s[1].start, s[1].stop - s[1].start, s[0].stop - s[0].start)
             for s in ndimage.find_objects(lab) if s[1].stop - s[1].start > 100]
    # ряды по верхнему краю (с допуском), в ряду слева направо
    boxes.sort(key=lambda b: (round(b[0] / 50), b[1]))
    if len(boxes) != len(NAMES):
        print(f"! задников {len(boxes)}, а имён {len(NAMES)}")
    os.makedirs(OUT, exist_ok=True)
    for name, (y, x, w, h) in zip(NAMES, boxes):
        img.crop((x, y, x + w, y + h)).save(os.path.join(OUT, f"BB{name}.png"))
    print("сохранено:", min(len(boxes), len(NAMES)))


if __name__ == "__main__":
    main()
