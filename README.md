<div align="center">

# ⚔️ RPG

**Пошаговая ретро-RPG на C++20 и SDL2 в духе классической Final Fantasy**

![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![SDL2](https://img.shields.io/badge/SDL-2-1E90FF)
![CMake](https://img.shields.io/badge/CMake-3.22+-064F8C?logo=cmake&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Windows-0078D6?logo=windows&logoColor=white)
![Tiled](https://img.shields.io/badge/maps-Tiled-6DB33F)

</div>

---

## ✨ Возможности

| | |
|---|---|
| 🗺️ **Открытый мир** | Карта мира из [Tiled](https://www.mapeditor.org/) (`.tmj`), тайловые коллизии, анимированные блики на воде |
| ⛵ **Транспорт** | Корабль, каноэ и дирижабль для путешествий по суше, рекам и морю |
| 🧭 **Мини-карта** | Три уровня масштаба, переключается на лету |
| ⚔️ **Пошаговые бои** | Атаки, навыки, предметы, выбор цели, всплывающие числа урона, сопротивления и уязвимости стихиям |
| 👾 **115 врагов** | Случайные встречи, зависящие от области карты, с таблицами добычи |
| 🧙 **4 класса героев** | Воин, Маг, Лучник, Вор — со своими навыками, ростом характеристик и стартовой экипировкой |
| 🎒 **Экипировка и инвентарь** | Полевое меню, снаряжение для каждого героя |
| 💾 **Слоты сохранений** | Несколько сохранений, титульный экран и игровое меню |
| 🌐 **Онлайн** | Совместное путешествие и бои по сети (TCP, порт `54000`) |
| 🎵 **Звук** | Музыкальные темы для меню, карты, боя и магазина; отдельная громкость музыки и звуков |

## 🎮 Управление

| Клавиша | Действие |
|---|---|
| `W A S D` / `← ↑ → ↓` | Движение, навигация по меню |
| `Enter` / `Space` | Подтвердить |
| `Esc` / ПКМ | Назад / игровое меню |
| `E` | Сесть в транспорт / выйти из него |
| `M` | Мини-карта: выкл → ×1 → ×2 → ×3 |
| `H` | Включить / выключить случайные бои |
| `Q` / `E` (в меню) | Переключить героя |

## 🔧 Сборка

### Требования

- **Windows** + **MinGW-w64** (x86_64)
- **CMake** 3.22+
- SDL2, SDL2_image, SDL2_mixer и SDL2_ttf уже лежат в репозитории — ставить ничего не нужно

### Через CLion

Откройте папку проекта, выберите тулчейн MinGW и нажмите **Run**.

### Из терминала

```bash
cmake -S . -B cmake-build-debug -G Ninja
```

```bash
cmake --build cmake-build-debug
```

Исполняемый файл собирается статически; рядом с ним должны лежать `SDL2*.dll` и папка `resources/`.

## 📁 Структура проекта

```
RPG/
├── scripts/
│   ├── main/           # точка входа, игровой цикл, игрок, карта и движение
│   ├── gameLogic/      # бой, герои, враги, NPC, магазин, квесты, игровые данные
│   ├── controllers/    # ресурсы, ввод, аудио, отрисовка
│   ├── ui/             # UIManager, сцены, панели, виджеты, анимации, скины окон
│   └── online/         # сервер, клиент и сетевые пакеты
├── resources/
│   ├── Data/           # heroes / enemies / items / skills / drops / encounters (JSON)
│   ├── Maps/           # карты и тайлсеты Tiled
│   ├── Images/         # фоны, персонажи, враги, транспорт, пропсы
│   ├── Animations/     # покадровые анимации
│   ├── Music/  Sounds/ Fonts/
├── tools/              # Python-утилиты для подготовки контента
├── third_party/        # nlohmann/json
└── SDL2*/              # библиотеки SDL2 для MinGW
```

### Архитектура

```
Game
├── UIManager ── Visualizer, Scene → Panel → Button / Image / Window
├── InputManager ── InputController
├── ResourceManager ── JSON-данные, текстуры, карты
├── AudioManager
├── Battle ── Hero / Enemy ← GamePerson
└── Server / Client
```

## 🛠️ Инструменты (`tools/`)

| Скрипт | Назначение |
|---|---|
| `make_collision.py` | Анализирует картинку карты и строит слой коллизий для Tiled |
| `enemies_csv_to_json.py` | Конвертирует таблицу врагов `enemies.csv` → `enemies.json` |
| `make_encounters.py` | Собирает `encounters.json` из списка «область → враги» (`areas.json`) |
| `slice_enemy_sheet.py` | Нарезает спрайт-лист врагов на отдельные PNG с прозрачным фоном |
| `name_picker.py` | HTML-страница для сопоставления спрайтов с именами врагов |
| `make_vehicle_sprites.py` | Генерирует временные спрайты транспорта |

Пример:

```bash
python tools/make_encounters.py --enemies resources/Data/enemies.json --areas tools/areas.json --out resources/Data/encounters.json
```

## 📝 Данные игры

Весь баланс вынесен в `resources/Data/*.json` — героев, врагов, предметы, навыки и встречи можно менять без пересборки. Например, класс героя:

```json
{ "class": "Warrior",
  "base":     { "hp": 180, "mp": 30, "attack": 12, "defence": 18, "agility": 6 },
  "perLevel": { "hp": 45,  "mp": 2,  "attack": 10, "defence": 9,  "agility": 1 },
  "resists": ["Physical"], "skills": [1, 5] }
```

---

<div align="center">

Сделано с ❤️ на C++ и SDL2

</div>
