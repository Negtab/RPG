#include "lang.h"

#include <array>
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <SDL_log.h>

namespace {
    Language current = Language::Ru;
    std::array<std::unordered_map<std::string, std::string>, kLanguageCount> tables;   // английский -> перевод
}

const char* languageCode(Language lang)
{
    return lang == Language::Ru ? "ru" : "en";
}

const char* languageName(Language lang)
{
    return lang == Language::Ru ? "Русский" : "English";
}

void loadLanguages(const std::filesystem::path& dir)
{
    for (int i = 0; i < kLanguageCount; ++i)
    {
        const auto lang = static_cast<Language>(i);
        tables[i].clear();
        if (lang == Language::En) continue;                      // английский — сами ключи
        const auto path = dir / (std::string(languageCode(lang)) + ".json");
        std::ifstream f(path);
        if (!f) { SDL_Log("No translation file %s", path.string().c_str()); continue; }
        try
        {
            const auto j = nlohmann::json::parse(f, nullptr, true, true);      // комментарии разрешены
            for (const auto& [key, value] : j.items())
                if (value.is_string()) tables[i][key] = value.get<std::string>();
        }
        catch (const nlohmann::json::exception& e)
        {
            SDL_Log("Bad translation file %s: %s", path.string().c_str(), e.what());
        }
    }
}

void setLanguage(Language lang) { current = lang; }
Language currentLanguage() { return current; }

std::string tr(std::string_view english)
{
    const auto& table = tables[static_cast<int>(current)];
    const auto it = table.find(std::string(english));
    return it != table.end() ? it->second : std::string(english);
}

std::string trf(std::string_view english, std::initializer_list<std::string> args)
{
    std::string s = tr(english);
    int i = 0;
    for (const auto& a : args)
    {
        const std::string key = "{" + std::to_string(i++) + "}";
        for (size_t pos = s.find(key); pos != std::string::npos; pos = s.find(key, pos + a.size()))
            s.replace(pos, key.size(), a);
    }
    return s;
}
