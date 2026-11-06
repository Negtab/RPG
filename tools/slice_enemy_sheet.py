#!/usr/bin/env python3
"""
Нарезает лист врагов (NES FF1, Spriters Resource) на отдельные PNG с прозрачным фоном.
  python tools/slice_enemy_sheet.py sheet.png out_dir [--outline 1] [--names names.txt]
Создаёт: out_dir/sprite_000.png ..., contact_sheet.png (с номерами), index.csv
"""
import argparse
import csv
import os
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

Image.MAX_IMAGE_PIXELS = None
BLACK = 8                                    # канал <= 8 считаем чёрным


def find_boxes(rgb, min_size, max_size):
    """Чёрные квадраты со спрайтами: прямоугольные компоненты, не равные цвету листа, с чёрной рамкой."""
    frame = np.concatenate([rgb[:2].reshape(-1, 3), rgb[-2:].reshape(-1, 3),
                            rgb[:, :2].reshape(-1, 3), rgb[:, -2:].reshape(-1, 3)])
    vals, counts = np.unique(frame, axis=0, return_counts=True)
    bg = vals[counts.argmax()]                                   # цвет самого листа

    fg = np.abs(rgb.astype(int) - bg).max(axis=2) > 6
    lab, _ = ndimage.label(fg, structure=np.ones((3, 3)))

    boxes = []
    for i, sl in enumerate(ndimage.find_objects(lab), start=1):
        ys, xs = sl
        bh, bw = ys.stop - ys.start, xs.stop - xs.start
        if not (min_size <= bw <= max_size and min_size <= bh <= max_size):
            continue                                              # чипы палитры, подписи, мусор
        if not (0.55 <= bw / bh <= 1.8):
            continue                                              # легенда справа внизу
        if (lab[sl] == i).mean() < 0.85:
            continue                                              # не сплошной прямоугольник
        crop = rgb[sl]
        ring = np.concatenate([crop[0], crop[-1], crop[:, 0], crop[:, -1]])
        if (ring <= BLACK).all(axis=1).mean() < 0.7:
            continue                                              # рамка не чёрная: это панели «Enemy placement»
        boxes.append((xs.start, ys.start, bw, bh))
    return boxes


def sort_reading_order(boxes):
    rows = []                                                     # [[y0, y1, [boxes]]]
    for b in sorted(boxes, key=lambda b: b[1]):
        x, y, w, h = b
        for row in rows:
            overlap = min(row[1], y + h) - max(row[0], y)
            if overlap > 0.5 * min(h, row[1] - row[0]):
                row[2].append(b)
                row[0], row[1] = min(row[0], y), max(row[1], y + h)
                break
        else:
            rows.append([y, y + h, [b]])
    rows.sort(key=lambda r: r[0])
    return [b for r in rows for b in sorted(r[2], key=lambda b: b[0])]


def make_transparent(rgb, outline):
    h, w = rgb.shape[:2]
    pad = outline + 1
    img = np.zeros((h + 2 * pad, w + 2 * pad, 4), np.uint8)
    img[pad:pad + h, pad:pad + w, :3] = rgb
    img[pad:pad + h, pad:pad + w, 3] = 255

    black = np.ones(img.shape[:2], bool)                          # поля вокруг считаем фоном
    black[pad:pad + h, pad:pad + w] = (rgb <= BLACK).all(axis=2)

    lab, _ = ndimage.label(black)
    edge = np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))
    background = np.isin(lab, edge[edge > 0])                     # чёрное, связанное с краем: фон + старая окантовка
    foreground = ~background

    out = img.copy()
    out[background] = (0, 0, 0, 0)
    if outline > 0:
        ring = ndimage.binary_dilation(foreground, iterations=outline) & ~foreground
        out[ring] = (0, 0, 0, 255)                                # новая тонкая чёрная рамка вокруг силуэта

    ys, xs = np.nonzero(out[..., 3])
    return out[ys.min():ys.max() + 1, xs.min():xs.max() + 1]


def contact_sheet(images, path, scale=2, cols=10, pad=6):
    cw = max(i.width for i in images) * scale + pad
    ch = max(i.height for i in images) * scale + pad + 10
    rows = (len(images) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cw, rows * ch), (70, 110, 190, 255))
    d = ImageDraw.Draw(sheet)
    for n, im in enumerate(images):
        x, y = (n % cols) * cw + pad // 2, (n // cols) * ch + pad // 2
        d.text((x, y), str(n), fill=(255, 255, 255, 255), stroke_width=1, stroke_fill=(0, 0, 0, 255))
        sheet.alpha_composite(im.resize((im.width * scale, im.height * scale), Image.NEAREST), (x, y + 10))
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sheet")
    ap.add_argument("out")
    ap.add_argument("--outline", type=int, default=1)
    ap.add_argument("--min", type=int, default=20, help="минимальный размер квадрата")
    ap.add_argument("--max", type=int, default=400, help="максимальный размер квадрата")
    ap.add_argument("--names", help="файл: имя спрайта на строку по порядку; '-' пропускает спрайт")
    ap.add_argument("--count", type=int, help="взять только первые N квадратов (128: без панелей Enemy placement)")
    args = ap.parse_args()

    rgb = np.array(Image.open(args.sheet).convert("RGB"))
    boxes = sort_reading_order(find_boxes(rgb, args.min, args.max))
    if not boxes:
        sys.exit("Не нашёл ни одного квадрата со спрайтом. Попробуй изменить --min / --max")

    names = []
    if args.names:
        with open(args.names, encoding="utf-8") as f:
            names = [line.strip() for line in f]

    os.makedirs(args.out, exist_ok=True)
    images, rows = [], []
    used = set()
    for n, (x, y, w, h) in enumerate(boxes):
        name = names[n] if n < len(names) and names[n] else f"sprite_{n:03d}"
        if name == "-":
            continue
        if name in used:
            name = f"{name}_{n:03d}"
        used.add(name)
        im = Image.fromarray(make_transparent(rgb[y:y + h, x:x + w], args.outline), "RGBA")
        im.save(os.path.join(args.out, name + ".png"))
        images.append(im)
        rows.append([n, name, x, y, w, h, im.width, im.height])

    contact_sheet(images, os.path.join(args.out, "contact_sheet.png"))
    with open(os.path.join(args.out, "index.csv"), "w", newline="", encoding="utf-8") as f:
        csv.writer(f).writerows([["index", "name", "x", "y", "w", "h", "out_w", "out_h"]] + rows)
    print(f"Найдено квадратов: {len(boxes)}, сохранено: {len(images)} -> {args.out}")


if __name__ == "__main__":
    main()