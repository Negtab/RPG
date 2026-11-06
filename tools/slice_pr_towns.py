#!/usr/bin/env python3
"""
Города FF1 Pixel Remaster (Spriters Resource, "Maps - <город>.png") -> карты игры.

  python tools/slice_pr_towns.py "C:/.../Downloads/Maps"

Лист — город и его интерьеры на розовом или чёрном фоне. Каждый кусок становится картой:
  resources/Images/Towns/<Город>/<кусок>.png
  resources/Maps/Towns/<карта>.tmj   (карта: cornelia, cornelia_inn, cornelia_church, ...)
В .tmj: фон, пустой слой collision (рисуется в Tiled), zones, portals, props, markers со spawn.
Свойства карты:
  mapId    — номер, начиная с FIRST_MAP_ID;
  exit     — "back": шаг за край карты возвращает туда, откуда пришли;
  entrance — у самого города: имя портала на карте мира, который ведёт сюда.
Существующие .tmj не перезаписываются (там уже могут быть ваши коллизии), пока не указан --force.
"""
import argparse
import json
import os

import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG_DIR = os.path.join(ROOT, "resources", "Images", "Towns")
MAP_DIR = os.path.join(ROOT, "resources", "Maps", "Towns")
FIRST_MAP_ID = 100
CELL = 16

# Город: (имя файла, имя портала на карте мира, имена кусков после самого города — по убыванию площади).
# Интерьеры опознаны по виду: гостиница — кровати, церковь — дорожка и свечи, магия — розовый (белая)
# и синий (чёрная) ковёр, лавки — прилавок. Если перепутано — поправьте здесь и перезапустите с --force.
TOWNS = [
    ("Cornelia", "Cornelia", ["inn", "church", "armor_shop", "weapon_shop", "black_magic", "white_magic", "item_shop"]),
    ("Pravoka", "Pravoca", ["inn", "church", "white_magic", "weapon_shop", "black_magic", "armor_shop", "item_shop"]),
    ("Elfheim", "Elfheim", ["inn", "church", "item_shop", "white_magic", "white_magic_2", "armor_shop", "weapon_shop",
                            "black_magic", "black_magic_2"]),
    ("Melmond", "Melmond", ["inn", "armor_shop", "white_magic", "weapon_shop", "black_magic"]),
    ("Crescent Lake", "Crescent Lake", ["inn", "church", "item_shop", "weapon_shop", "armor_shop", "white_magic",
                                        "black_magic"]),
    ("Onrac", "Onrac", ["inn", "church", "white_magic", "black_magic", "item_shop"]),
    ("Gaia", "Gaia", ["inn", "shop", "magic", "church", "white_magic", "black_magic", "item_shop"]),
    ("Lufenia", "Lufenia", ["magic"]),
]


def pieces(img):
    """Куски листа: связные области не цвета фона (розовый, если он есть, иначе чёрный), крупнее 64 px."""
    a = np.asarray(img).astype(int)
    magenta = (a[..., 0] == 255) & (a[..., 1] == 0) & (a[..., 2] == 255)
    background = magenta if magenta.mean() > 0.01 else a[..., :3].max(axis=2) <= 12
    lab, _ = ndimage.label(~background)
    boxes = []
    for s in ndimage.find_objects(lab):
        w, h = s[1].stop - s[1].start, s[0].stop - s[0].start
        if w >= 64 and h >= 64:
            boxes.append((s[1].start, s[0].start, w, h))
    boxes.sort(key=lambda b: -b[2] * b[3])                 # самый большой — сам город
    return boxes


def snap(box):
    """Обрезаем до кратного 16: у листов на чёрном фоне вокруг интерьеров рамка в пару пикселей."""
    x, y, w, h = box
    dw, dh = w % CELL, h % CELL
    return x + dw // 2, y + dh // 2, w - dw, h - dh


def object_group(layer_id, name, objects):
    return {"draworder": "topdown", "id": layer_id, "name": name, "objects": objects, "opacity": 1,
            "type": "objectgroup", "visible": True, "x": 0, "y": 0}


def write_tmj(path, image_rel, w, h, map_id, spawn, entrance=None):
    props = [{"name": "mapId", "type": "int", "value": map_id},
             {"name": "exit", "type": "string", "value": "back"}]
    if entrance:
        props.append({"name": "entrance", "type": "string", "value": entrance})
    sx, sy = spawn
    tmj = {
        "compressionlevel": -1, "height": h, "infinite": False, "orientation": "orthogonal",
        "renderorder": "right-down", "tiledversion": "1.12.2", "tileheight": CELL, "tilewidth": CELL,
        "type": "map", "version": "1.10", "width": w, "nextlayerid": 7, "nextobjectid": 2,
        "properties": props,
        "tilesets": [{"firstgid": 1, "source": "../collision.tsx"}, {"firstgid": 2, "source": "../props.tsx"}],
        "layers": [
            {"id": 1, "image": image_rel, "imageheight": h * CELL, "imagewidth": w * CELL, "locked": True,
             "name": "background", "opacity": 1, "type": "imagelayer", "visible": True, "x": 0, "y": 0},
            {"data": [0] * (w * h), "height": h, "id": 2, "name": "collision", "opacity": 0.6,
             "type": "tilelayer", "visible": True, "width": w, "x": 0, "y": 0},
            object_group(3, "zones", []),
            object_group(4, "portals", []),
            object_group(5, "props", []),
            object_group(6, "markers", [{"height": 0, "id": 1, "name": "spawn", "point": True, "rotation": 0,
                                         "type": "", "visible": True, "width": 0,
                                         "x": sx * CELL + CELL / 2, "y": sy * CELL + CELL / 2}]),
        ],
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(tmj, f, indent=1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("maps_dir", help="папка с 'PC _ Computer - Final Fantasy (Pixel Remaster) - Maps - *.png'")
    ap.add_argument("--force", action="store_true", help="перезаписать существующие .tmj")
    args = ap.parse_args()
    os.makedirs(MAP_DIR, exist_ok=True)

    map_id = FIRST_MAP_ID
    for town, entrance, names in TOWNS:
        src = os.path.join(args.maps_dir, f"PC _ Computer - Final Fantasy (Pixel Remaster) - Maps - {town}.png")
        img = Image.open(src).convert("RGBA")
        boxes = pieces(img)
        if len(boxes) != len(names) + 1:
            print(f"! {town}: кусков {len(boxes)}, а имён {len(names) + 1} — проверьте TOWNS")
        key = town.lower().replace(" ", "_")
        folder = town.replace(" ", "")
        os.makedirs(os.path.join(IMG_DIR, folder), exist_ok=True)

        for i, box in enumerate(boxes[:len(names) + 1]):
            piece = "town" if i == 0 else names[i - 1]
            x, y, w, h = snap(box)
            png = os.path.join(IMG_DIR, folder, piece + ".png")
            img.crop((x, y, x + w, y + h)).save(png)

            cw, ch = w // CELL, h // CELL
            map_name = key if i == 0 else f"{key}_{piece}"
            tmj = os.path.join(MAP_DIR, map_name + ".tmj")
            # Появление: у города — внизу по центру (въезд с дороги), у интерьера — над дверью внизу
            spawn = (cw // 2, ch - 1) if i == 0 else (cw // 2, ch - 2)
            if os.path.exists(tmj) and not args.force:
                print("  есть, не трогаю:", os.path.relpath(tmj, ROOT))
            else:
                rel = os.path.relpath(png, MAP_DIR).replace("\\", "/")
                write_tmj(tmj, rel, cw, ch, map_id, spawn, entrance if i == 0 else None)
                print(f"  {map_name}: {cw}x{ch}, mapId {map_id}")
            map_id += 1


if __name__ == "__main__":
    main()
