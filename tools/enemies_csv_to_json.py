#!/usr/bin/env python3
"""
  python tools/enemies_csv_to_json.py enemies.csv --sprites resources/Images/Enemies ^
         --skills resources/Data/skills.json --out resources/Data/enemies.json
  Необязательно: --hp-scale 4 --atk-scale 2 --reward-scale 3
"""
import argparse
import csv
import json
import os
import sys

from PIL import Image

ELEMENTS = {"Fire", "Water", "Earth", "Air", "Ice", "Lava", "Dark", "Light",
            "None", "Physical", "Poison", "Lightning"}


def growth(text, scale):
    base, _, per = (text or "0").strip().partition("+")
    return [round(int(base) * scale), round(int(per or 0) * scale)]


def elements(text, who, column):
    names = [n.strip() for n in (text or "").split("|") if n.strip()]
    for n in names:
        if n not in ELEMENTS:
            sys.exit(f"{who}, колонка {column}: неизвестная стихия '{n}' (допустимо: {', '.join(sorted(ELEMENTS))})")
    return names


def abilities(text, who, skill_ids):
    out = []
    for part in (p.strip() for p in (text or "attack:100").split(";") if p.strip()):
        key, _, weight = part.partition(":")
        w = int(weight) if weight else 1
        if key == "attack":
            out.append({"attack": True, "weight": w})
        else:
            sid = int(key)
            if skill_ids is not None and sid not in skill_ids:
                sys.exit(f"{who}: умения {sid} нет в skills.json")
            out.append({"skill": sid, "weight": w})
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv")
    ap.add_argument("--sprites", required=True, help="папка с PNG врагов: размер srcRect берётся из файла")
    ap.add_argument("--skills", help="skills.json для проверки номеров умений")
    ap.add_argument("--out", default="enemies.json")
    ap.add_argument("--hp-scale", type=float, default=1.0)
    ap.add_argument("--atk-scale", type=float, default=1.0, help="для attack и defence")
    ap.add_argument("--reward-scale", type=float, default=1.0, help="для exp и gold")
    ap.add_argument("--default-agility", type=int, default=8, help="если колонка agility пуста")
    ap.add_argument("--skip-missing", action="store_true", help="пропускать врагов, для которых нет PNG")
    args = ap.parse_args()

    skill_ids = None
    if args.skills:
        with open(args.skills, encoding="utf-8") as f:
            skill_ids = {s["id"] for s in json.load(f)}

    missing = []
    result, seen = [], set()
    with open(args.csv, encoding="utf-8-sig", newline="") as f:
        for row in csv.DictReader(f):
            eid = int(row["id"])
            who = f"Враг {eid} ({row.get('name', '?')})"
            if eid in seen:
                sys.exit(f"{who}: повторяющийся id")
            seen.add(eid)

            texture = (row.get("texture") or row["name"]).strip()
            png = os.path.join(args.sprites, texture + ".png")
            if not os.path.exists(png):
                missing.append(f"{eid} {row['name']} -> {texture}.png")
                continue
            w, h = Image.open(png).size

            result.append({
                "id": eid, "name": row["name"].strip(), "texture": texture, "srcRect": [0, 0, w, h],
                "hp": growth(row["hp"], args.hp_scale),
                "attack": growth(row["attack"], args.atk_scale),
                "defence": growth(row["defence"], args.atk_scale),
                "agility": growth(row.get("agility") or str(args.default_agility), 1.0),
                "exp": growth(row["exp"], args.reward_scale),
                "gold": growth(row["gold"], args.reward_scale),
                "hits": int(row.get("hits") or 1),
                "resists": elements(row.get("resists"), who, "resists"),
                "immunities": elements(row.get("immunities"), who, "immunities"),
                "vulnerabilities": elements(row.get("vulnerabilities"), who, "vulnerabilities"),
                "abilities": abilities(row.get("abilities"), who, skill_ids),
            })
    if missing:
        print("Нет спрайтов для:\n  " + "\n  ".join(missing))
        if not args.skip_missing:                       # выходим, только если чего-то правда не хватает
            sys.exit("Добавь файлы или запусти с --skip-missing")
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=2)
    print(f"Записано врагов: {len(result)} -> {args.out}")


if __name__ == "__main__":
    main()