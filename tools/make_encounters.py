#!/usr/bin/env python3
"""
Строит encounters.json из списка «область -> враги».
  python tools/make_encounters.py --enemies resources/Data/enemies.json --areas tools/areas.json --out resources/Data/encounters.json
Использует только тех врагов, которые есть в enemies.json (иначе игра не запустится).

У области могут быть:
  "near": ["Cornelia", ...] — порталы карты мира, рядом с которыми группа водится. Вне зон Tiled
          игра берёт группу ближайшего портала;
  "terrain": "land" | "sea" | "river" — пешком, на корабле или на каноэ (по умолчанию land).
"""
import argparse
import json


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--enemies", required=True)
    ap.add_argument("--areas", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    with open(args.enemies, encoding="utf-8") as f:
        enemies = {e["id"]: e for e in json.load(f)}
    with open(args.areas, encoding="utf-8") as f:
        areas = json.load(f)

    exps = sorted(e["exp"][0] for e in enemies.values())

    def group_size(eid):
        """Сколько таких врагов в группе: чем слабее по награде относительно остальных, тем больше."""
        rank = sum(1 for x in exps if x <= enemies[eid]["exp"][0]) / len(exps)
        return 4 if rank <= 0.25 else 3 if rank <= 0.5 else 2 if rank <= 0.75 else 1

    result, report = [], []
    for area_id, area in areas.items():
        if area_id.startswith("_"):                                # "_comment" и подобное — не область
            continue
        wanted = area["enemies"]
        ids = sorted((i for i in wanted if i in enemies), key=lambda i: enemies[i]["exp"][0])
        missing = [i for i in wanted if i not in enemies]
        if not ids:
            report.append(f"{area_id}: пропущена, нет ни одного врага в enemies.json")
            continue
        if missing:
            report.append(f"{area_id}: нет врагов {missing}")

        formations = []
        for k, eid in enumerate(ids):
            n = group_size(eid)
            formations.append({"weight": 4, "enemies": [eid] * n})
            if n > 2:
                formations.append({"weight": 2, "enemies": [eid] * (n - 1)})
            if k > 0:                                              # сильный + слабый сосед
                weaker = ids[k - 1]
                strong_n = max(1, n - 1)
                weak_n = min(4 - strong_n, group_size(weaker))
                if weak_n >= 1:
                    formations.append({"weight": 2, "enemies": [eid] * strong_n + [weaker] * weak_n})
        group = {"id": area_id, "formations": formations}
        if area.get("near"):
            group["near"] = area["near"]
        if area.get("terrain", "land") != "land":
            group["terrain"] = area["terrain"]
        if area.get("background"):
            group["background"] = area["background"]
        result.append(group)

    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2)
    print(f"Групп: {len(result)} -> {args.out}")
    for line in report:
        print("  ", line)


if __name__ == "__main__":
    main()