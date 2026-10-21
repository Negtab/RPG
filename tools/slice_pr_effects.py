#!/usr/bin/env python3
"""
Боевые эффекты из материалов FF1 Pixel Remaster (Spriters Resource).

  python tools/slice_pr_effects.py --effects "...Miscellaneous - Effects" --weapons "...Battle Effects - Weapons.png"

Что получается в resources/Images/Effects:
  EF_*.png      — нужные текстуры эффектов (список EFFECT_FILES); как их проигрывать — resources/Data/effects.json
  SwingNN.png   — замах оружием: 4 кадра в строку. NN — номер группы в листе оружия
                  (ряды сверху вниз, в ряду по 4 ячейки слева направо); у оружия в items.json это поле "swing"
"""
import argparse
import os
import shutil

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "resources", "Images", "Effects")

# Текстуры, на которые ссылается effects.json. Добавили эффект — добавьте файл сюда
EFFECT_FILES = [
    "EF_Hit01_SA", "EF_Hit03_SA", "EF_Hit06_SA", "EF_Hit08_SA", "EF_Light00_SA",
    "EF_Explosion00_SA", "EF_Thunder01_SA", "EF_Ice07_MS", "EF_Sphere01_SA",
    "EF_Star01_ST", "EF_Bubble00_SA", "EF_Ring00_SA", "EF_Ring01_SA",
]


def copy_effects(src_dir):
    out = []
    for name in EFFECT_FILES:
        src = os.path.join(src_dir, name + ".png")
        if not os.path.exists(src):
            print("! нет файла", src)
            continue
        dst = os.path.join(OUT, name + ".png")
        shutil.copyfile(src, dst)
        out.append(dst)
    return out


def swing_groups(sheet):
    """Ячейки листа оружия по 4 в группе: ряды по верхнему краю, в ряду слева направо."""
    a = np.asarray(sheet)
    magenta = (a[..., 0] == 255) & (a[..., 1] == 0) & (a[..., 2] == 255)
    lab, _ = ndimage.label(~magenta)
    cells = [(s[0].start, s[1].start, s[1].stop - s[1].start, s[0].stop - s[0].start) for s in ndimage.find_objects(lab)]
    groups = []
    for top in sorted({c[0] for c in cells}):
        row = sorted((c for c in cells if c[0] == top), key=lambda c: c[1])
        groups += [row[k:k + 4] for k in range(0, len(row), 4)]
    return groups


def slice_swings(path):
    sheet = Image.open(path).convert("RGBA")
    out = []
    for n, group in enumerate(swing_groups(sheet)):
        if len(group) < 4:                                  # неполные группы — не замахи
            continue
        w = max(c[2] for c in group)
        h = max(c[3] for c in group)
        strip = Image.new("RGBA", (4 * w, h), (0, 0, 0, 0))
        for k, (y, x, cw, ch) in enumerate(group):         # кадры одного размера по центру ячейки
            strip.paste(sheet.crop((x, y, x + cw, y + ch)), (k * w + (w - cw) // 2, (h - ch) // 2))
        dst = os.path.join(OUT, f"Swing{n:02d}.png")
        strip.save(dst)
        out.append(dst)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--effects", help="папка с EF_*.png")
    ap.add_argument("--weapons", help="лист Battle Effects - Weapons")
    args = ap.parse_args()
    os.makedirs(OUT, exist_ok=True)
    written = []
    if args.effects: written += copy_effects(args.effects)
    if args.weapons: written += slice_swings(args.weapons)
    print("файлов:", len(written))


if __name__ == "__main__":
    main()
