#ifndef PROJECT_NAME_UTF8_H
#define PROJECT_NAME_UTF8_H

#include <string>

// Тексты в UTF-8: русская буква занимает два байта, поэтому считаем и режем по символам, а не по байтам
namespace utf8 {

inline bool isContinuation(unsigned char c) { return (c & 0xC0) == 0x80; }

inline size_t length(const std::string& s)
{
    size_t n = 0;
    for (unsigned char c : s) if (!isContinuation(c)) ++n;
    return n;
}

inline size_t prevBoundary(const std::string& s, size_t pos)       // начало символа перед pos
{
    if (pos == 0) return 0;
    do --pos; while (pos > 0 && isContinuation(static_cast<unsigned char>(s[pos])));
    return pos;
}

inline size_t nextBoundary(const std::string& s, size_t pos)       // начало символа после pos
{
    if (pos >= s.size()) return s.size();
    do ++pos; while (pos < s.size() && isContinuation(static_cast<unsigned char>(s[pos])));
    return pos;
}

inline std::string prefix(const std::string& s, size_t chars)      // первые chars символов
{
    size_t i = 0;
    while (i < s.size() && chars-- > 0) i = nextBoundary(s, i);
    return s.substr(0, i);
}

inline void popBack(std::string& s) { s.erase(prevBoundary(s, s.size())); }

// Строчные буквы для поиска: латиница и кириллица (А-Я, Ё)
inline std::string lower(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i)
    {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c == 0xD0 && i + 1 < s.size())
        {
            const auto d = static_cast<unsigned char>(s[i + 1]);
            if (d >= 0x90 && d <= 0x9F) { out += '\xD0'; out += static_cast<char>(d + 0x20); ++i; continue; }   // А-П
            if (d >= 0xA0 && d <= 0xAF) { out += '\xD1'; out += static_cast<char>(d - 0x20); ++i; continue; }   // Р-Я
            if (d == 0x81)              { out += '\xD1'; out += '\x91'; ++i; continue; }                          // Ё
        }
        out += c < 0x80 ? static_cast<char>(std::tolower(c)) : static_cast<char>(c);
    }
    return out;
}

// Символ можно ввести с клавиатуры: печатный ASCII или любой многобайтный (кириллица)
inline bool printable(unsigned char lead) { return (lead >= 32 && lead <= 126) || lead >= 0xC0; }

}

#endif //PROJECT_NAME_UTF8_H
