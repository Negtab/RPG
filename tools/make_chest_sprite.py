#!/usr/bin/env python3
"""
Открытый сундук для resources/Images/Props/ChestOpen.png.

  python tools/make_chest_sprite.py

Закрытые сундуки нарисованы прямо в картах Pixel Remaster (клетка 16x16: крышка сверху, ящик с замком снизу).
Готового открытого в листах нет, поэтому собираем его из закрытого: крышка откинута назад (видна узкой полосой
сверху), внутри темно, замка нет. Спрайт рисуется поверх клетки сундука, когда тот открыт.
"""
import os

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "resources", "Images", "Dungeons", "ChaosShrine", "temple_of_chaos_rooms.png")
CHEST_CELL = (4, 5)                      # левый сундук северо-западной комнаты храма
OUT = os.path.join(ROOT, "resources", "Images", "Props", "ChestOpen.png")


def main():
    img = Image.open(SRC).convert("RGBA")
    x0, y0 = CHEST_CELL[0] * 16, CHEST_CELL[1] * 16
    closed = img.crop((x0, y0, x0 + 16, y0 + 16))
    src = closed.load()
    out = closed.copy()
    px = out.load()

    # Крышка (строки 0..8) откинута: от неё видна верхняя кромка — берём строки 0, 4 и 8 закрытой крышки
    for dst, row in enumerate((0, 4, 8)):
        for x in range(16):
            px[x, dst] = src[x, row]
    # Внутри темно: строки 3..9 между железными стойками по краям
    for y in range(3, 10):
        for x in range(16):
            if x in (0, 1, 14, 15):
                px[x, y] = src[x, 9]                       # стойки — цвет оковки
            else:
                shade = 18 + (y - 3) * 4                   # к дну чуть светлее: видно глубину
                px[x, y] = (shade + 14, shade + 4, shade, 255)
    # Замок снят: золото на ящике закрашиваем деревом соседних пикселей
    for y in range(10, 16):
        for x in range(16):
            r, g, b, a = src[x, y]
            if r > 180 and g > 150 and b < 90:             # золото замка
                px[x, y] = src[x - 3 if x >= 3 else x + 3, y]
    out.save(OUT)
    print("saved", os.path.relpath(OUT, ROOT))


if __name__ == "__main__":
    main()
