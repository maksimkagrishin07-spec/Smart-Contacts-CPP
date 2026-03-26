#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <windows.h>
#include <filesystem> // Теперь это не будет выдавать ошибку

using namespace std;
namespace fs = std::filesystem;

// Цветовая индикация
inline void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// Валидация имени (RegEx)
inline bool isValidFileName(const string& name) {
    // Запрещаем системные символы по ТЗ [cite: 29]
    regex pattern(R"([^/\\:\?\*\"<>\|]+)");
    return regex_match(name, pattern);
}

// Белый список расширений [cite: 27]
inline bool isAllowedExtension(const string& filename) {
    vector<string> whitelist = { ".exe", ".txt", ".pdf" };
    size_t pos = filename.find_last_of(".");
    if (pos == string::npos) return false;
    string ext = filename.substr(pos);
    for (const auto& w : whitelist) if (ext == w) return true;
    return false;
}

// Форматирование вывода (обрезание > 15 символов) [cite: 45]
inline string formatOutput(string text) {
    if (text.length() > 15) return text.substr(0, 12) + "...";
    return text;
}

#endif