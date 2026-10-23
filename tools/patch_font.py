"""Готовит шрифт игры из Pixelify Sans (Google Fonts, лицензия OFL).

В исходном шрифте нет заглавных «О» и «П», у «К» лишний штрих, а «5» не отличить от «S». «О» берём у латинской O,
«П» рисуем по строчной «п», вытянутой до высоты заглавных.
Переменный шрифт фиксируем на обычной толщине (wght 400) — SDL_ttf так надёжнее.

    python tools/patch_font.py <PixelifySans[wght].ttf> resources/Fonts/PixelifySans.ttf
"""
import sys

from fontTools.pens.recordingPen import RecordingPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer


def main(src: str, dst: str) -> None:
    font = instancer.instantiateVariableFont(TTFont(src), {"wght": 400})
    glyf, hmtx = font["glyf"], font["hmtx"]

    def add(name: str, glyph, metrics) -> None:
        glyf[name] = glyph                              # glyf сам дописывает имя в порядок глифов
        hmtx[name] = metrics

    add("uni041E", glyf["O"], hmtx["O"])

    # У «К» в исходнике лишний штрих над буквой (по сути это «Ќ»): оставляем только саму букву
    rec = RecordingPen()
    font.getGlyphSet()["uni041A"].draw(rec)
    first = rec.value[:[op for op, _ in rec.value].index("closePath") + 1]
    pen = TTGlyphPen(None)
    clean = RecordingPen()
    clean.value = first
    clean.replay(pen)
    glyf["uni041A"] = pen.glyph()

    # «5» в исходнике не отличить от «S» — рисуем свою в той же сетке, что «3»: прямой верх, круглый низ
    pen = TTGlyphPen(None)
    pen.moveTo((60, 631))
    for pt in [(525, 631), (525, 540), (161, 540), (161, 359), (433, 359), (433, 270), (525, 270), (525, 78),
               (433, 78), (433, -12), (151, -12), (151, 78), (60, 78), (60, 179), (161, 179), (161, 88),
               (423, 88), (423, 260), (60, 260)]:
        pen.lineTo(pt)
    pen.closePath()
    glyf["five"] = pen.glyph()

    # Те же размеры, что у «Н»: стойки 60..161 и 423..525, верх 631, перекладина толщиной ~100
    pen = TTGlyphPen(None)
    pen.moveTo((60, -12))
    for pt in [(60, 631), (525, 631), (525, -12), (423, -12), (423, 531), (161, 531), (161, -12)]:
        pen.lineTo(pt)
    pen.closePath()
    add("uni041F", pen.glyph(), hmtx["uni041D"])

    for table in font["cmap"].tables:
        if table.isUnicode():
            table.cmap[0x041E] = "uni041E"
            table.cmap[0x041F] = "uni041F"
    font.save(dst)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
