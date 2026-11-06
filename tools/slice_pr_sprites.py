#!/usr/bin/env python3
"""
Нарезка листов FF1 Pixel Remaster (Spriters Resource) в форматы игры.

  python tools/slice_pr_sprites.py --vehicles "...Vehicles.png" \
                                   --overworld "...Overworld Sprites.png" \
                                   --battle "...Battle Sprites.png"

Листы: кадры в ячейках на розовом фоне #ff00ff, фон кадров прозрачный.

Что получается (всё в resources/):
  Images/Vehicles/Ship.png, Canoe.png, Airship.png — 4 колонки (Up, Down, Left, Right) x 4 строки:
      строки 0-1 — стоит (два кадра), 2-3 — движется (два кадра); кадр 16x16
  Images/Vehicles/AirshipShadow.png — тень летящего дирижабля
  Images/Overworld/Walk<Набор>.png — герой на карте: 4 колонки (Up, Down, Left, Right) x 2 кадра, 16x16
  Images/Battle/Battle<Набор>.png — все позы в бою как в листе: 2 колонки x 5 строк по 32x32
      (0 — стойка/шаг, 1 — шаг, 2 — удар, 3 — победа, 4 — ранен/пал)
  Images/Characters/Image{N}.png, Image{N}Dead.png и Animations/Hero{N}Animation/ — то, что
      сейчас использует бой, для шести классов игры (номер N — heroLook в types.h)
Наборы (префиксы Walk/Battle, чтобы имена текстур не совпадали): Warrior, Thief, Monk, RedMage, WhiteMage, BlackMage и повышенные Knight, Ninja, Master,
RedWizard, WhiteWizard, BlackWizard.
"""
import argparse
import os

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "resources")

SETS = ["Warrior", "Thief", "Monk", "RedMage", "WhiteMage", "BlackMage",
        "Knight", "Ninja", "Master", "RedWizard", "WhiteWizard", "BlackWizard"]
# Класс игры -> набор FF1 и номер картинок (heroLook в types.h)
GAME_CLASSES = {1: "RedMage", 2: "BlackMage", 3: "Thief", 4: "Warrior", 5: "Monk", 6: "WhiteMage"}


def load(path):
    return Image.open(path).convert("RGBA")


def cells_of(img):
    """Ячейки листа: связные области не розового цвета, отсортированы сверху вниз, слева направо."""
    a = np.asarray(img)
    magenta = (a[..., 0] == 255) & (a[..., 1] == 0) & (a[..., 2] == 255)
    lab, _ = ndimage.label(~magenta)
    boxes = [(s[0].start, s[1].start, s[1].stop - s[1].start, s[0].stop - s[0].start) for s in ndimage.find_objects(lab)]
    return sorted(boxes)                                   # (y, x, w, h)


def crop(img, box):
    y, x, w, h = box
    return img.crop((x, y, x + w, y + h))


def grid(cells, size):
    """Ячейки нужного размера, разложенные в таблицу [строка][колонка] по их координатам."""
    cs = [c for c in cells if (c[2], c[3]) == size]
    ys = sorted({c[0] for c in cs})
    xs = sorted({c[1] for c in cs})
    table = [[None] * len(xs) for _ in ys]
    for c in cs:
        table[ys.index(c[0])][xs.index(c[1])] = c
    return table


def save(img, *parts):
    path = os.path.join(RES, *parts)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path)
    return path


def sheet(frames, cols, rows, size):
    """frames[row][col] -> один PNG."""
    out = Image.new("RGBA", (cols * size[0], rows * size[1]), (0, 0, 0, 0))
    for r in range(rows):
        for c in range(cols):
            out.paste(frames[r][c], (c * size[0], r * size[1]))
    return out


def mirror(img):
    return img.transpose(Image.FLIP_LEFT_RIGHT)


# ── Транспорт ────────────────────────────────────────

# Номера ячеек в листе (по порядку сверху вниз, слева направо) — сверено с автором:
# корабль — колонки 0-1, каноэ — 2-3, дирижабль — 4-5 (+6), тени — 38/40/48.
# В каждой колонке три вида (вниз, вверх, бок влево) в двух состояниях.
VEHICLES = {
    #            idle: (down, up, left)               moving: (down, up, left)
    "Ship":    {"idle": [(0, 1), (8, 9), (16, 17)],   "move": [(24, 25), (32, 33), (41, 42)]},
    "Canoe":   {"idle": [(26, 27), (34, 35), (43, 44)], "move": [(2, 3), (10, 11), (18, 19)]},
    "Airship": {"idle": [(28, 29), (36, 37), (45, 46)], "move": [(4, 5), (12, 13), (20, 21)]},
}


def slice_vehicles(path):
    img = load(path)
    cells = cells_of(img)
    out = []
    for name, states in VEHICLES.items():
        rows = []
        for state in ("idle", "move"):
            down, up, left = [[crop(img, cells[i]) for i in pair] for pair in states[state]]
            for f in range(2):                             # два кадра — две строки листа
                rows.append([up[f], down[f], left[f], mirror(left[f])])   # порядок enum Direction: Up, Down, Left, Right
        out.append(save(sheet(rows, 4, 4, (16, 16)), "Images", "Vehicles", name + ".png"))
    out.append(save(crop(img, cells[38]), "Images", "Vehicles", "AirshipShadow.png"))
    return out


# ── Персонажи ────────────────────────────────────────

def character_sets(path, size):
    """Для каждого из 12 наборов: таблица 5 строк x 2 кадра."""
    table = grid(cells_of(load(path)), size)
    img = load(path)
    sets = []
    for block in range(2):                                 # верх — базовые классы, низ — повышенные
        for group in range(6):
            frames = [[crop(img, table[block * 5 + r][group * 2 + c]) for c in range(2)] for r in range(5)]
            sets.append(frames)
    return sets


def slice_overworld(path):
    out = []
    for name, f in zip(SETS, character_sets(path, (16, 16))):
        down, left, up = f[0], f[1], f[2]                  # строки листа: вниз, влево, вверх
        rows = [[up[i], down[i], left[i], mirror(left[i])] for i in range(2)]
        out.append(save(sheet(rows, 4, 2, (16, 16)), "Images", "Overworld", "Walk" + name + ".png"))
    return out


def slice_battle(path):
    out = []
    sets = dict(zip(SETS, character_sets(path, (32, 32))))
    for name, f in sets.items():
        out.append(save(sheet(f, 2, 5, (32, 32)), "Images", "Battle", "Battle" + name + ".png"))

    # То, что бой берёт сейчас: стойка, павший и удар (шаг вперёд, два кадра удара, снова стойка)
    for look, name in GAME_CLASSES.items():
        f = sets[name]
        out.append(save(f[0][0], "Images", "Characters", f"Image{look}.png"))
        out.append(save(f[4][1], "Images", "Characters", f"Image{look}Dead.png"))
        anim_dir = os.path.join(RES, "Animations", f"Hero{look}Animation")
        os.makedirs(anim_dir, exist_ok=True)
        for old in os.listdir(anim_dir):                   # старые кадры другого размера убираем
            if old.lower().endswith(".png"):
                os.remove(os.path.join(anim_dir, old))
        for i, frame in enumerate([f[0][1], f[2][0], f[2][1], f[0][0]]):
            out.append(save(frame, "Animations", f"Hero{look}Animation", f"Frame{i}.png"))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vehicles")
    ap.add_argument("--overworld")
    ap.add_argument("--battle")
    args = ap.parse_args()
    written = []
    if args.vehicles:  written += slice_vehicles(args.vehicles)
    if args.overworld: written += slice_overworld(args.overworld)
    if args.battle:    written += slice_battle(args.battle)
    for p in written:
        print("written", os.path.relpath(p, ROOT))


if __name__ == "__main__":
    main()
