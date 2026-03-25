#include <iostream>
#include <fstream>
#include <windows.h>
#include <filesystem>
#include <iomanip>
#include <regex> 
#include <memory>
#include "Directory.h"
#include "File.h"

using namespace std;
namespace fs = std::filesystem;

const uint32_t MAGIC = 0x41524348;

// Валидация имени (запрет спецсимволов)
bool isValidName(const string& name) {
    regex pattern(R"([^\\/:*?"<>|]+)");
    return regex_match(name, pattern);
}

// Сохранение всего дерева в бинарный файл
static void saveAll(shared_ptr<Directory> root) {
    ofstream out("archive.dat", ios::binary);
    if (!out) return;
    out.write((char*)&MAGIC, sizeof(MAGIC));
    root->serialize(out);
    out.close();
}

// Загрузка дерева с проверкой Magic Number
static void loadAll(shared_ptr<Directory> root) {
    ifstream in("archive.dat", ios::binary);
    if (!in) return;
    uint32_t m = 0;
    in.read((char*)&m, sizeof(m));
    if (m == MAGIC) {
        root->deserialize(in, "root");
    }
    else {
        // Тест-кейс №10 из отчета сработает здесь
        cout << "\n!!! ОШИБКА ЦЕЛОСТНОСТИ: Неверный заголовок файла !!!" << endl;
    }
    in.close();
}

int main() {
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251); SetConsoleOutputCP(1251);

    int roleChoice;
    cout << "Выберите роль (1. GUEST, 2. ADMIN): ";
    cin >> roleChoice;
    AccessLevel currentUserLevel = (roleChoice == 2) ? AccessLevel::ADMIN : AccessLevel::GUEST;

    string workingPath = "root";
    if (!fs::exists(workingPath)) fs::create_directory(workingPath);

    auto root = make_shared<Directory>("root", AccessLevel::ADMIN);
    loadAll(root);

    Directory* current = root.get();
    int ch = -1;

    while (ch != 0) {
        cout << "\n------------------------------------------------";
        cout << "\nВАША РОЛЬ: " << (currentUserLevel == AccessLevel::ADMIN ? "ADMIN" : "GUEST");
        cout << "\nПУТЬ: " << workingPath << " [Размер: " << current->calculateSize() << " байт]" << endl;
        cout << "1. Папка | 2. Файл | 3. ВОЙТИ | 4. Дерево | 5. Корень | 6. УДАЛИТЬ\n";
        cout << "7. ПОИСК | 8. АУДИТ | 9. ПО ДАТЕ | 10. СОРТИРОВКА | 11. ЭКСПОРТ CSV | 0. Выход\nВыбор: ";

        if (!(cin >> ch)) { cin.clear(); cin.ignore(1000, '\n'); continue; }

        if (ch == 1 || ch == 2) {
            string n; cout << "Имя: "; cin >> n;
            if (!isValidName(n)) {
                cout << "Ошибка: Недопустимые символы в имени!" << endl;
                Directory::logOperation("Ошибка имени: " + n, false);
                continue;
            }

            if (ch == 1) {
                // Создание папки
                current->addResource(make_unique<Directory>(n, AccessLevel::USER), workingPath);
                cout << "Папка создана." << endl;
            }
            else {
                // Создание файла
                string fullPath = workingPath + "/" + n + ".txt";

                // Физически создаем пустой файл на диске
                ofstream f(fullPath); f.close();

                // Определяем его реальный размер (будет 0, если файл только создан)
                size_t actualSize = 0;
                if (fs::exists(fullPath)) {
                    actualSize = (size_t)fs::file_size(fullPath);
                }

                // Добавляем в наше дерево с реальным размером
                current->addResource(make_unique<File>(n, ".txt", actualSize, AccessLevel::USER), workingPath);
                cout << "Файл создан [Размер: " << actualSize << " байт]" << endl;
            }

            Directory::logOperation("Создано: " + n, true);
            saveAll(root);
        }
        else if (ch == 3) {
            string n; cout << "Имя папки: "; cin >> n;
            bool found = false;
            for (auto& item : current->getItems()) {
                if (Directory* d = dynamic_cast<Directory*>(item.get())) {
                    if (d->getName() == n) {
                        current = d;
                        workingPath += "/" + n;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) cout << "Ошибка: Папка не найдена!" << endl;
        }
        else if (ch == 4) {
            cout << "\n--- СТРУКТУРА АРХИВА ---" << endl;
            root->print(0);
        }
        else if (ch == 5) {
            current = root.get();
            workingPath = "root";
            cout << "Возврат в корень." << endl;
        }
        else if (ch == 6) {
            string n; cout << "Имя для удаления: "; cin >> n;
            bool found = false, denied = false;
            for (auto& item : current->getItems()) {
                if (item->getName() == n) {
                    found = true;
                    if (currentUserLevel < item->getAccess()) denied = true;
                    break;
                }
            }
            if (!found) cout << "Ошибка: Объект не найден!" << endl;
            else if (denied) cout << "ОШИБКА: Недостаточно прав для удаления (нужен ADMIN)!" << endl;
            else {
                current->removeResource(n, workingPath);
                Directory::logOperation("Удалено: " + n, true);
                saveAll(root);
                cout << "Удалено успешно." << endl;
            }
        }
        else if (ch == 7) {
            string p, e; cout << "Часть имени: "; cin >> p;
            cout << "Расширение (или 0 для любого): "; cin >> e;
            if (e == "0") e = "";
            vector<const Resource*> res;
            root->findResources(p, e, res);
            if (res.empty()) cout << "Ничего не найдено." << endl;
            for (auto r : res) cout << "Найдено: " << r->getName() << endl;
        }
        else if (ch == 8) {
            Directory::AuditStats s;
            root->getAuditStats(s);
            cout << "\n=== СТАТИСТИКА (АУДИТ) ===\n";
            cout << "Всего папок: " << s.dirCount << endl;
            cout << "Всего файлов: " << s.fileCount << endl;
            cout << "Общий объем данных: " << s.totalSize << " байт" << endl;
        }
        else if (ch == 9) {
            Date s, e;
            cout << "Диапазон поиска (формат: ДД ММ ГГГГ)\nОт: "; cin >> s.day >> s.month >> s.year;
            cout << "До: "; cin >> e.day >> e.month >> e.year;
            vector<const Resource*> res;
            root->findByDate(s, e, res);
            if (res.empty()) cout << "За этот период файлов не найдено." << endl;
            for (auto r : res) cout << "Найдено: " << r->getName() << endl;
        }
        else if (ch == 10) {
            int m; cout << "Сортировать по: 1-Имя, 2-Размер, 3-Дата: "; cin >> m;
            root->sortChildren(m);
            cout << "Сортировка выполнена рекурсивно." << endl;
        }
        else if (ch == 11) {
            ofstream csv("export.csv");
            if (csv.is_open()) {
                csv << "Path,Type,Size,Date\n";
                root->exportToCSV(csv, "root");
                csv.close();
                cout << "Успешно: Данные экспортированы в export.csv" << endl;
                Directory::logOperation("Экспорт CSV", true);
            }
            else {
                cout << "Ошибка: Не удалось создать файл экспорта!" << endl;
            }
        }
    }

    saveAll(root); // Сохраняем перед выходом
    return 0;
}