#ifndef PROJECT_NAME_LANG_H
#define PROJECT_NAME_LANG_H

#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>

// Язык интерфейса. Строки в коде и названия в data/*.json — английские, они же ключи перевода.
// Перевод лежит в resources/Lang/<код>.json: { "Items": "Предметы", ... }. Нет перевода — показываем английский
enum class Language { En, Ru };
constexpr int kLanguageCount = 2;

void loadLanguages(const std::filesystem::path& dir);
void setLanguage(Language lang);
[[nodiscard]] Language currentLanguage();
[[nodiscard]] const char* languageCode(Language lang);         // "en", "ru" — так пишется в settings.ini
[[nodiscard]] const char* languageName(Language lang);         // как язык называется на нём самом

[[nodiscard]] std::string tr(std::string_view english);
// Перевод с подстановкой: trf("Obtained {0} Gil", {"120"}) -> «Получено 120 гил»
[[nodiscard]] std::string trf(std::string_view english, std::initializer_list<std::string> args);

#endif //PROJECT_NAME_LANG_H
