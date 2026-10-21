#!/usr/bin/env python3
"""
Карта регионов встреч на карте мира: какая группа из encounters.json выпадет в каждой клетке.

  python tools/preview_encounters.py            -> tools/encounter_regions.png

Как в игре: вне зон Tiled группа берётся по ближайшему порталу ("near" в areas.json) и по местности
(суша / море / река). Реки — узкая вода дальше 8 клеток от открытого моря; они обведены белым.
Зоны из Tiled рисуются рамками с подписью.
"""
import colorsys
import json
import os

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCALE = 4


def main():
    tmj = json.load(open(os.path.join(ROOT, "resources", "Maps", "overworld.tmj"), encoding="utf-8"))
    W, H = tmj["width"], tmj["height"]
    solid, portals, zones = None, [], []
    for L in tmj["layers"]:
        if L["name"] == "collision":
            solid = np.array(L["data"]).reshape(H, W) != 0
        if L["name"] == "portals":
            portals += [(o["name"], int(o["x"] // 16), int(o["y"] // 16)) for o in L["objects"] if o["name"]]
        if L["name"] == "zones":
            zones += [o for o in L["objects"]]

    img = Image.open(os.path.join(ROOT, "resources", "Images", "Backgrounds", "MapBackground.png")).convert("RGB")
    a = np.asarray(img)[:H * 16, :W * 16].reshape(H, 16, W, 16, 3)
    water = ((a[..., 2] >= 0xC0) & (a[..., 0] <= 0x80) & (a[..., 1] <= 0x50)).mean(axis=(1, 3)) >= 0.5
    to_land = ndimage.distance_transform_cdt(water, metric="chessboard")
    to_open = ndimage.distance_transform_cdt(~(water & (to_land >= 4)), metric="chessboard")
    river = water & (to_open > 8)
    lab, n = ndimage.label(river, structure=np.ones((3, 3)))
    sizes = ndimage.sum(river, lab, range(1, n + 1))
    river = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s >= 12])

    groups = json.load(open(os.path.join(ROOT, "resources", "Data", "encounters.json"), encoding="utf-8"))
    def group(name, terrain):
        for g in groups:
            if g.get("terrain", "land") == terrain and name in g.get("near", []):
                return g["id"]
        return None

    ys, xs = np.mgrid[0:H, 0:W]
    px = np.array([p[1] for p in portals]); py = np.array([p[2] for p in portals])
    nearest = ((xs[..., None] - px) ** 2 + (ys[..., None] - py) ** 2).argmin(axis=2)
    ids = sorted({g["id"] for g in groups if g.get("near")})
    color = {}
    for i, g in enumerate(ids):
        r, gg, b = colorsys.hsv_to_rgb((i * 0.61803) % 1, 0.75, 1.0)
        color[g] = (int(r * 255), int(gg * 255), int(b * 255))

    over = np.zeros((H, W, 3), np.uint8)
    for y in range(H):
        for x in range(W):
            if solid[y, x] and not water[y, x]:
                continue
            terrain = "river" if river[y, x] else "sea" if water[y, x] else "land"
            g = group(portals[nearest[y, x]][0], terrain)
            if g:
                over[y, x] = color[g]

    base = img.resize((W * SCALE, H * SCALE), Image.BILINEAR)
    ov = Image.fromarray(over).resize(base.size, Image.NEAREST)
    mask = Image.fromarray(((over.sum(2) > 0) * 120).astype(np.uint8)).resize(base.size, Image.NEAREST)
    base.paste(ov, (0, 0), mask)
    d = ImageDraw.Draw(base)
    edge = river & ~ndimage.binary_erosion(river)
    for y, x in zip(*np.nonzero(edge)):
        d.rectangle([x * SCALE, y * SCALE, x * SCALE + SCALE - 1, y * SCALE + SCALE - 1], outline=(255, 255, 255))
    for z in zones:
        x0, y0 = z["x"] / 16 * SCALE, z["y"] / 16 * SCALE
        d.rectangle([x0, y0, x0 + z["width"] / 16 * SCALE, y0 + z["height"] / 16 * SCALE], outline=(255, 255, 0), width=2)
        d.text((x0 + 3, y0 + 2), z.get("name", ""), fill=(255, 255, 0))
    for name, x, y in portals:
        d.ellipse([x * SCALE - 4, y * SCALE - 4, x * SCALE + 4, y * SCALE + 4], fill=(255, 255, 255))
        d.text((x * SCALE + 6, y * SCALE - 6), name, fill=(255, 255, 255))

    legend = Image.new("RGB", (240, len(ids) * 18 + 8), (20, 20, 30))
    dl = ImageDraw.Draw(legend)
    for i, g in enumerate(ids):
        dl.rectangle([6, 6 + i * 18, 20, 18 + i * 18], fill=color[g])
        dl.text((26, 6 + i * 18), g, fill=(255, 255, 255))
    out = Image.new("RGB", (base.width + legend.width, base.height), (20, 20, 30))
    out.paste(base, (0, 0))
    out.paste(legend, (base.width, 0))
    path = os.path.join(ROOT, "tools", "encounter_regions.png")
    out.save(path)
    print("сохранено:", path, "| клеток реки:", int(river.sum()))


if __name__ == "__main__":
    main()
