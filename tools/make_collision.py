#!/usr/bin/env python3
"""
  python tools/make_collision.py analyze resources/Images/Backgrounds/MapBackground.png --out build_maps
  python tools/make_collision.py build   resources/Images/Backgrounds/MapBackground.png --solid 0,3 --out resources/Maps
"""
import argparse
import json
import os
import sys

import numpy as np
from PIL import Image, ImageDraw

Image.MAX_IMAGE_PIXELS = None
CELL = 16          # клетка в пикселях картинки (тайл NES)


def load_cells(path):
    img = Image.open(path).convert("RGB")
    w, h = img.size
    if w % CELL or h % CELL:
        sys.exit(f"Размер {w}x{h} не делится на {CELL}")
    cw, ch = w // CELL, h // CELL
    a = np.asarray(img, dtype=np.uint8)
    q = (a >> 4).astype(np.uint16)                 # 4 бита на канал: сливаем близкие оттенки
    key = (q[..., 0] << 8) | (q[..., 1] << 4) | q[..., 2]
    cells = key.reshape(ch, CELL, cw, CELL).transpose(0, 2, 1, 3).reshape(ch, cw, CELL * CELL)
    return (w, h), cw, ch, cells


def cell_modes(cells):
    """Самый частый цвет в каждой клетке."""
    ch, cw, _ = cells.shape
    base = (np.arange(cw, dtype=np.int64) * 4096)[:, None]
    modes = np.empty((ch, cw), dtype=np.int32)
    for y in range(ch):
        counts = np.bincount((cells[y] + base).ravel(), minlength=cw * 4096)
        modes[y] = counts.reshape(cw, 4096).argmax(axis=1)
    return modes


def palette(modes, limit=24):
    vals, counts = np.unique(modes, return_counts=True)
    order = np.argsort(-counts, kind="stable")
    return [(int(vals[i]), int(counts[i])) for i in order[:limit]]


def key_to_rgb(k):
    return ((k >> 8) & 15) * 17, ((k >> 4) & 15) * 17, (k & 15) * 17


def preview_image(modes, solid=None):
    ch, cw = modes.shape
    rgb = np.zeros((ch, cw, 3), dtype=np.uint8)
    rgb[..., 0] = ((modes >> 8) & 15) * 17
    rgb[..., 1] = ((modes >> 4) & 15) * 17
    rgb[..., 2] = (modes & 15) * 17
    if solid is not None:
        m = solid.astype(bool)
        rgb[m] = (rgb[m] * 0.4 + np.array([255, 0, 0]) * 0.6).astype(np.uint8)
    return Image.fromarray(rgb).resize((cw * 4, ch * 4), Image.NEAREST)


def write_palette_png(pal, total, path):
    row_h = 40
    img = Image.new("RGB", (460, row_h * len(pal)), (255, 255, 255))
    d = ImageDraw.Draw(img)
    for i, (k, n) in enumerate(pal):
        r, g, b = key_to_rgb(k)
        d.rectangle([0, i * row_h, 80, (i + 1) * row_h - 2], fill=(r, g, b))
        d.text((90, i * row_h + 14), f"#{i}   #{r:02x}{g:02x}{b:02x}   {100 * n / total:5.1f}%   ({n})", fill=(0, 0, 0))
    img.save(path)


def cmd_analyze(args):
    _, cw, ch, cells = load_cells(args.image)
    modes = cell_modes(cells)
    pal = palette(modes)
    os.makedirs(args.out, exist_ok=True)
    total = cw * ch
    print(f"Клеток: {cw}x{ch} = {total}")
    for i, (k, n) in enumerate(pal):
        r, g, b = key_to_rgb(k)
        print(f"#{i:<3} #{r:02x}{g:02x}{b:02x}  {100 * n / total:5.1f}%  {n}")
    write_palette_png(pal, total, os.path.join(args.out, "palette.png"))
    preview_image(modes).save(os.path.join(args.out, "cells_preview.png"))
    print("Записаны palette.png и cells_preview.png")


def find_spawn(solid, cw, ch):
    ys, xs = np.nonzero(solid == 0)
    if len(xs) == 0:
        return cw // 2, ch // 2
    d = (xs - cw // 2) ** 2 + (ys - ch // 2) ** 2
    i = int(d.argmin())
    return int(xs[i]), int(ys[i])


def object_group(layer_id, name, objects):
    return {"id": layer_id, "name": name, "type": "objectgroup", "draworder": "topdown",
            "x": 0, "y": 0, "opacity": 1, "visible": True, "objects": objects}


def cmd_build(args):
    if args.ref_scale not in (1, 2, 4):
        sys.exit("--ref-scale: 1, 2 или 4")
    (w, h), cw, ch, cells = load_cells(args.image)
    modes = cell_modes(cells)
    pal = palette(modes, 64)

    chosen = {int(s) for s in args.solid.split(",") if s.strip()}
    keys = [pal[i][0] for i in chosen if i < len(pal)]
    solid = np.isin(modes, keys).astype(np.uint8)

    for wall in args.wall:                         # --wall x0,y0,x1,y1 (включительно)
        x0, y0, x1, y1 = (int(v) for v in wall.split(","))
        solid[y0:y1 + 1, x0:x1 + 1] = 1

    sx, sy = (int(v) for v in args.spawn.split(",")) if args.spawn else find_spawn(solid, cw, ch)

    os.makedirs(args.out, exist_ok=True)
    ts = CELL // args.ref_scale                    # размер клетки в Tiled

    if args.ref_scale > 1:                         # уменьшенная копия фона: Tiled быстрее работает с лёгкой картинкой
        ref = Image.open(args.image).convert("RGBA").resize((w // args.ref_scale, h // args.ref_scale), Image.NEAREST)
        ref.save(os.path.join(args.out, "MapBackground_ref.png"))
        bg_rel, bg_w, bg_h = "MapBackground_ref.png", ref.size[0], ref.size[1]
    else:
        bg_rel = os.path.relpath(args.image, args.out).replace("\\", "/")
        bg_w, bg_h = w, h

    Image.new("RGBA", (ts, ts), (255, 0, 0, 110)).save(os.path.join(args.out, "collision_tile.png"))
    with open(os.path.join(args.out, "collision.tsx"), "w", encoding="utf-8") as f:
        f.write(f'<?xml version="1.0" encoding="UTF-8"?>\n'
                f'<tileset version="1.10" tiledversion="1.10.2" name="collision" tilewidth="{ts}" tileheight="{ts}" '
                f'tilecount="1" columns="1">\n <image source="collision_tile.png" width="{ts}" height="{ts}"/>\n</tileset>\n')

    tmj = {
        "type": "map", "version": "1.10", "tiledversion": "1.10.2",
        "orientation": "orthogonal", "renderorder": "right-down", "infinite": False,
        "width": cw, "height": ch, "tilewidth": ts, "tileheight": ts,
        "nextlayerid": 6, "nextobjectid": 2,
        "tilesets": [{"firstgid": 1, "source": "collision.tsx"}],
        "layers": [
            {"id": 1, "name": "background", "type": "imagelayer", "image": bg_rel,
             "imagewidth": bg_w, "imageheight": bg_h, "x": 0, "y": 0, "opacity": 1, "visible": True},
            {"id": 2, "name": "collision", "type": "tilelayer", "width": cw, "height": ch,
             "x": 0, "y": 0, "opacity": 0.6, "visible": True, "data": solid.flatten().astype(int).tolist()},
            object_group(3, "zones", []),
            object_group(4, "portals", []),
            object_group(5, "markers", [{"id": 1, "name": "spawn", "type": "", "point": True,
                                         "x": sx * ts + ts / 2, "y": sy * ts + ts / 2,
                                         "width": 0, "height": 0, "rotation": 0, "visible": True}]),
        ],
    }
    with open(os.path.join(args.out, "overworld.tmj"), "w", encoding="utf-8") as f:
        json.dump(tmj, f)

    preview_image(modes, solid).save(os.path.join(args.out, "solid_preview.png"))
    print(f"Готово: {cw}x{ch}, непроходимых клеток {100 * solid.mean():.1f}%, спавн ({sx}, {sy})")

def prop_entry(name, typ, value):
    return {"name": name, "type": typ, "value": value}


def cmd_testroom(args):
    W, H = 24, 18
    os.makedirs(args.out, exist_ok=True)
    bx, by = (int(v) for v in args.back_to.split(","))

    img = Image.new("RGB", (W * CELL, H * CELL))
    d = ImageDraw.Draw(img)
    door = (W // 2, H - 1)
    for y in range(H):
        for x in range(W):
            if (x, y) == door:      color = (200, 160, 60)
            elif x in (0, W - 1) or y in (0, H - 1): color = (60, 40, 30)
            else:                   color = (150, 130, 100) if (x + y) % 2 == 0 else (140, 120, 90)
            d.rectangle([x * CELL, y * CELL, (x + 1) * CELL - 1, (y + 1) * CELL - 1], fill=color)
    img.save(os.path.join(args.out, f"{args.name}.png"))

    solid = np.zeros((H, W), dtype=np.uint8)
    solid[0, :] = solid[-1, :] = 1
    solid[:, 0] = solid[:, -1] = 1
    solid[door[1], door[0]] = 0                                   # дверь проходима

    room_px = (W * CELL, H * CELL)
    tmj = {
        "type": "map", "version": "1.10", "tiledversion": "1.10.2", "orientation": "orthogonal",
        "renderorder": "right-down", "infinite": False, "width": W, "height": H,
        "tilewidth": CELL, "tileheight": CELL, "nextlayerid": 6, "nextobjectid": 4,
        "properties": [prop_entry("mapId", "int", args.map_id)],
        "tilesets": [{"firstgid": 1, "source": "collision.tsx"}],
        "layers": [
            {"id": 1, "name": "background", "type": "imagelayer", "image": f"{args.name}.png",
             "imagewidth": room_px[0], "imageheight": room_px[1], "x": 0, "y": 0, "opacity": 1, "visible": True},
            {"id": 2, "name": "collision", "type": "tilelayer", "width": W, "height": H, "x": 0, "y": 0,
             "opacity": 0.6, "visible": True, "data": solid.flatten().astype(int).tolist()},
            object_group(3, "zones", [{"id": 1, "name": "room", "type": "", "x": 0, "y": 0,
                                       "width": room_px[0], "height": room_px[1], "rotation": 0, "visible": True,
                                       "properties": [prop_entry("safe", "bool", True)]}]),
            object_group(4, "portals", [{"id": 2, "name": "exit", "type": "", "x": door[0] * CELL, "y": door[1] * CELL,
                                         "width": CELL, "height": CELL, "rotation": 0, "visible": True,
                                         "properties": [prop_entry("targetMap", "string", "overworld"),
                                                        prop_entry("targetX", "int", bx),
                                                        prop_entry("targetY", "int", by)]}]),
            object_group(5, "markers", [{"id": 3, "name": "spawn", "type": "", "point": True,
                                         "x": (W // 2) * CELL + CELL / 2, "y": (H - 3) * CELL + CELL / 2,
                                         "width": 0, "height": 0, "rotation": 0, "visible": True}]),
        ],
    }
    path = os.path.join(args.out, f"{args.name}.tmj")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(tmj, f)
    print(f"Готово: {path}. Вход в комнату ставь на клетку (12, 15) этой карты, выход ведёт на ({bx}, {by}) в оверворлде")


def cmd_portal(args):
    with open(args.tmj, encoding="utf-8") as f:
        tmj = json.load(f)
    ts = tmj["tilewidth"]
    x, y = (int(v) for v in args.cell.split(","))
    tx, ty = (int(v) for v in args.target.split(","))

    coll = next((l for l in tmj["layers"] if l["name"] == "collision"), None)
    if coll and coll["data"][y * tmj["width"] + x]:
        print(f"ВНИМАНИЕ: клетка ({x}, {y}) на этой карте стена: до портала не дойти")

    layer = next((l for l in tmj["layers"] if l["name"] == "portals"), None)
    if layer is None:
        sys.exit("В карте нет слоя 'portals'")

    oid = tmj.get("nextobjectid", 1)
    layer["objects"].append({
        "id": oid, "name": args.to, "type": "", "x": x * ts, "y": y * ts, "width": ts, "height": ts,
        "rotation": 0, "visible": True,
        "properties": [prop_entry("targetMap", "string", args.to),
                       prop_entry("targetX", "int", tx), prop_entry("targetY", "int", ty)]})
    tmj["nextobjectid"] = oid + 1

    with open(args.tmj, "w", encoding="utf-8") as f:
        json.dump(tmj, f)
    print(f"Портал ({x}, {y}) -> {args.to} ({tx}, {ty}) добавлен в {args.tmj}")
def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)

    a = sub.add_parser("analyze")
    a.add_argument("image")
    a.add_argument("--out", default=".")

    b = sub.add_parser("build")
    b.add_argument("image")
    b.add_argument("--solid", required=True, help="номера цветов из analyze, например 0,3")
    b.add_argument("--out", default=".")
    b.add_argument("--ref-scale", type=int, default=1)
    b.add_argument("--spawn", help="x,y клетки; по умолчанию ближайшая проходимая к центру")
    b.add_argument("--wall", action="append", default=[], help="x0,y0,x1,y1: пометить прямоугольник стеной")

    t = sub.add_parser("testroom")
    t.add_argument("--out", default=".")
    t.add_argument("--name", default="test_room")
    t.add_argument("--map-id", type=int, default=1)
    t.add_argument("--back-to", required=True, help="x,y клетка оверворлда, куда выходим из комнаты")

    pt = sub.add_parser("portal")
    pt.add_argument("tmj")
    pt.add_argument("--cell", required=True, help="x,y клетка входа на этой карте")
    pt.add_argument("--to", required=True, help="имя целевой карты без .tmj")
    pt.add_argument("--target", required=True, help="x,y клетка на целевой карте")

    args = ap.parse_args()
    {"analyze": cmd_analyze, "build": cmd_build, "testroom": cmd_testroom, "portal": cmd_portal}[args.cmd](args)


if __name__ == "__main__":
    main()