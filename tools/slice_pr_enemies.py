#!/usr/bin/env python3
"""
Враги из листа FF1 Pixel Remaster (Spriters Resource, "Enemies & Bosses").

  python tools/slice_pr_enemies.py SHEET --preview out.png   # сводный лист: номер и имя под каждым спрайтом
  python tools/slice_pr_enemies.py SHEET --apply             # PNG в resources/Images/Enemies + srcRect в enemies.json

Имена берутся из tools/pr_enemy_names.txt ("номер = имя"). Номера — ряды листа сверху вниз,
в ряду слева направо; чёрные тени-силуэты пропускаются. Спрайты без имени не трогают врага:
у него остаётся старая картинка. srcRect потом совпадёт и с генерацией из CSV
(enemies_csv_to_json.py берёт размер из PNG).
"""
import argparse
import json
import os

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = os.path.join(ROOT, "tools", "pr_enemy_names.txt")
ENEMIES_JSON = os.path.join(ROOT, "resources", "Data", "enemies.json")
OUT_DIR = os.path.join(ROOT, "resources", "Images", "Enemies")


def sprites(img):
    """Спрайты листа по порядку: ячейки на розовом фоне, без теней, сгруппированные в ряды по нижнему краю."""
    a = np.asarray(img)
    magenta = (a[..., 0] == 255) & (a[..., 1] == 0) & (a[..., 2] == 255)
    lab, _ = ndimage.label(~magenta)
    found = []
    for s in ndimage.find_objects(lab):
        y, x, h, w = s[0].start, s[1].start, s[0].stop - s[0].start, s[1].stop - s[1].start
        cell = a[y:y + h, x:x + w]
        opaque = cell[..., 3] > 0
        if not opaque.any():
            continue
        black = (opaque & (cell[..., :3].max(axis=2) < 40)).sum() / opaque.sum()
        if black > 0.9:                                     # тень под врагом
            continue
        found.append((y, x, w, h))

    found.sort(key=lambda c: c[0] + c[3])
    rows = []
    for c in found:                                         # ряд — ячейки с почти одинаковым нижним краем
        bottom = c[0] + c[3]
        if rows and abs(rows[-1][0] - bottom) <= 12:
            rows[-1][1].append(c)
        else:
            rows.append([bottom, [c]])
    ordered = []
    for _, cells in rows:
        ordered += sorted(cells, key=lambda c: c[1])
    return ordered


def trimmed(img, cell):
    y, x, w, h = cell
    part = img.crop((x, y, x + w, y + h))
    box = part.getbbox()                                    # по непрозрачным пикселям
    return part.crop(box) if box else part


def read_names():
    names = {}
    with open(NAMES, encoding="utf-8") as f:
        for line in f:
            line = line.split("#", 1)[0].strip()
            if "=" not in line:
                continue
            idx, name = (p.strip() for p in line.split("=", 1))
            if name and name != "-":
                names[int(idx)] = name
    return names


def preview(img, cells, names, out):
    cw, ch, cols = 128, 150, 10
    rows = (len(cells) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cw, rows * ch), (40, 48, 70, 255))
    d = ImageDraw.Draw(sheet)
    for i, cell in enumerate(cells):
        t = trimmed(img, cell)
        scale = max(1, min(2, (cw - 8) // max(t.width, 1), (ch - 40) // max(t.height, 1)))
        t = t.resize((t.width * scale, t.height * scale), Image.NEAREST)
        t.thumbnail((cw - 8, ch - 40))
        px, py = (i % cols) * cw, (i // cols) * ch
        sheet.paste(t, (px + (cw - t.width) // 2, py + 16 + (ch - 40 - t.height) // 2), t)
        name = names.get(i)
        d.text((px + 4, py + 2), str(i), fill=(255, 230, 90, 255))
        d.text((px + 4, py + ch - 20), name or "?", fill=(255, 255, 255, 255) if name else (255, 110, 110, 255))
    sheet.save(out)


def apply(img, cells, names):
    with open(ENEMIES_JSON, encoding="utf-8") as f:
        enemies = json.load(f)
    known = {e["name"] for e in enemies}
    used = {}
    for i, name in sorted(names.items()):
        if i >= len(cells):
            print(f"! {i} = {name}: в листе нет такого номера")
            continue
        if name in used:
            print(f"! {i} = {name}: имя уже взято спрайтом {used[name]}")
            continue
        used[name] = i
        t = trimmed(img, cells[i])
        t.save(os.path.join(OUT_DIR, name + ".png"))       # PNG нужен и новому врагу: генератор из CSV возьмёт его размер
        if name not in known:
            print(f"  {i} = {name}: PNG сохранён, но врага нет в enemies.json — добавьте его в enemies.csv")
            continue
        for e in enemies:                                   # Kraken и Tiamat встречаются дважды
            if e["name"] == name:
                e["texture"] = name
                e["srcRect"] = [0, 0, t.width, t.height]
    with open(ENEMIES_JSON, "w", encoding="utf-8") as f:
        json.dump(enemies, f, ensure_ascii=False, indent=2)
        f.write("\n")
    missing = sorted(known - set(used))
    print(f"обновлено {len(set(used) & known)} врагов; со старой картинкой {len(missing)}: {', '.join(missing)}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sheet")
    ap.add_argument("--preview")
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()
    img = Image.open(args.sheet).convert("RGBA")
    cells = sprites(img)
    names = read_names()
    print(f"спрайтов в листе: {len(cells)}, с именем: {len(names)}")
    if args.preview:
        preview(img, cells, names, args.preview)
        print("превью:", args.preview)
    if args.apply:
        apply(img, cells, names)


if __name__ == "__main__":
    main()
