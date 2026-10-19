#!/usr/bin/env python3
"""
Страница для сопоставления спрайтов с именами врагов.
  python tools/name_picker.py sprites_out tools/enemies.csv name_picker.html
"""
import base64
import csv
import html
import os
import sys

# Мои предположения по силуэтам: подсвечены жёлтым, проверь и поправь
GUESS = {0: "Goblin", 1: "Goblin Guard", 5: "Winter Wolf", 6: "Skeleton", 7: "Bloodbones",
         63: "Crazy Horse", 64: "Nightmare", 71: "Shark", 72: "White Shark",
         122: "Lich", 123: "Marilith", 124: "Kraken", 125: "Tiamat", 126: "Garland", 127: "Chaos"}

PAGE = """<!doctype html>
<html lang="ru"><meta charset="utf-8"><title>Имена спрайтов</title>
<style>
body { font-family: sans-serif; background: #2b3a55; color: #fff; margin: 16px; }
.grid { display: flex; flex-wrap: wrap; gap: 8px; }
.cell { background: #46628f; padding: 6px; border-radius: 6px; width: 170px; text-align: center; }
.cell img { image-rendering: pixelated; display: block; margin: 6px auto; }
.idx { font-size: 12px; opacity: .85; }
.cell input { width: 160px; }
.cell input.guess { background: #fff3b0; }
textarea { width: 100%; height: 160px; margin-top: 8px; }
</style>
<h2>Сопоставление спрайтов с врагами</h2>
<p>Впиши имя (подсказка появляется при наборе). Жёлтые поля: мои предположения, проверь их.
Пустое поле: спрайт сохранится как sprite_###. «-» пропускает спрайт (дубликат).</p>
<button onclick="exportNames()">Собрать names.txt</button>
<button onclick="copyNames()">Копировать</button>
<textarea id="out" readonly></textarea>
<datalist id="names">__OPTIONS__</datalist>
<div class="grid">
__CELLS__
</div>
<script>
const KEY = "ffNamePicker";
let saved = {};
try { saved = JSON.parse(localStorage.getItem(KEY) || "{}"); } catch (e) {}
document.querySelectorAll("input[data-i]").forEach(inp => {
  const i = inp.dataset.i;
  if (saved[i] !== undefined) inp.value = saved[i];
  inp.addEventListener("input", () => {
    saved[i] = inp.value;
    try { localStorage.setItem(KEY, JSON.stringify(saved)); } catch (e) {}
  });
});
function exportNames() {
  const arr = [];
  document.querySelectorAll("input[data-i]").forEach(x => { arr[+x.dataset.i] = x.value.trim(); });
  document.getElementById("out").value = Array.from(arr, v => v || "").join("\\n");
}
function copyNames() {
  exportNames();
  const t = document.getElementById("out");
  t.select();
  try { navigator.clipboard.writeText(t.value); } catch (e) { document.execCommand("copy"); }
}
</script>
</html>
"""


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    folder, csv_path, out_path = sys.argv[1:]

    with open(os.path.join(folder, "index.csv"), encoding="utf-8", newline="") as f:
        rows = list(csv.DictReader(f))

    enemy_names = []
    with open(csv_path, encoding="utf-8-sig", newline="") as f:
        for r in csv.DictReader(f):
            if r["name"] not in enemy_names:
                enemy_names.append(r["name"])

    cells = []
    for r in rows:
        idx = int(r["index"])
        path = os.path.join(folder, r["name"] + ".png")
        if not os.path.exists(path):
            continue
        with open(path, "rb") as f:
            b64 = base64.b64encode(f.read()).decode()
        w, h = int(r["out_w"]), int(r["out_h"])
        scale = max(1, min(4, 150 // max(w, h)))
        guess = GUESS.get(idx, "")
        cls = ' class="guess" title="предположение: проверь"' if guess else ""
        cells.append(
            f'<div class="cell"><div class="idx">#{idx} &nbsp; {w}x{h}</div>'
            f'<img src="data:image/png;base64,{b64}" style="width:{w * scale}px">'
            f'<input list="names" data-i="{idx}" value="{html.escape(guess)}" placeholder="имя"{cls}></div>')

    options = "".join(f'<option value="{html.escape(n)}">' for n in enemy_names)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(PAGE.replace("__OPTIONS__", options).replace("__CELLS__", "\n".join(cells)))
    print(f"Готово: {out_path} ({len(cells)} спрайтов, {len(enemy_names)} имён в списке)")


if __name__ == "__main__":
    main()