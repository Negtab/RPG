#ifndef PROJECT_NAME_SCRIPTARGS_H
#define PROJECT_NAME_SCRIPTARGS_H

#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "../main/types.h"

// Разбор аргументов сценариев отдельно от Lua: так его можно проверить тестами

// "up" / "down" / "left" / "right" или первая буква; иначе пусто
inline std::optional<Direction> parseDirection(const std::string& s)
{
    if (s == "u" || s == "up")    return Direction::Up;
    if (s == "d" || s == "down")  return Direction::Down;
    if (s == "l" || s == "left")  return Direction::Left;
    if (s == "r" || s == "right") return Direction::Right;
    return std::nullopt;
}

// Путь npcWalk: буквы u d l r, после буквы — сколько шагов (без числа — один). "l3 u" = три влево, один вверх.
// Пробелы и запятые пропускаются; любой другой символ — ошибка, путь пустой
inline std::optional<std::vector<Direction>> parseWalkPath(const std::string& path)
{
    std::vector<Direction> steps;
    for (size_t i = 0; i < path.size();)
    {
        const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(path[i])));
        if (c == ' ' || c == ',') { ++i; continue; }
        const auto dir = parseDirection(std::string(1, c));
        if (!dir) return std::nullopt;
        ++i;
        int count = 0;
        while (i < path.size() && std::isdigit(static_cast<unsigned char>(path[i])))
        {
            count = count * 10 + (path[i] - '0');
            if (count > 99) return std::nullopt;                 // опечатка вроде "l1000": не уводим жителя за край карты
            ++i;
        }
        steps.insert(steps.end(), count == 0 ? 1 : count, *dir);
    }
    return steps;
}

// heroClass(): имя класса для сценариев — латиница в нижнем регистре, не зависит от языка интерфейса
inline const char* classScriptName(SpecializationNames c)
{
    switch (c)
    {
        case SpecializationNames::Warrior:   return "warrior";
        case SpecializationNames::Thief:     return "thief";
        case SpecializationNames::Monk:      return "monk";
        case SpecializationNames::RedMage:   return "red_mage";
        case SpecializationNames::WhiteMage: return "white_mage";
        case SpecializationNames::BlackMage: return "black_mage";
        case SpecializationNames::None:      break;
    }
    return "";
}

#endif //PROJECT_NAME_SCRIPTARGS_H
