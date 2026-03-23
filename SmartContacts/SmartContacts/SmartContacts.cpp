#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <limits>
#include <sstream>
#include <windows.h>
#include <chrono>
#include <thread>

#ifdef max
#undef max
#endif

using namespace std;

// Цвета для интерфейса
const int WHITE = 7, CYAN = 11, RED = 12, YELLOW = 14, GREEN = 10;

enum Group { WORK, FAMILY, FRIENDS, OTHERS };

struct Date {
    int day = 1, month = 1, year = 2000;
    // Для удобной сортировки дат
    long toInt() const { return year * 10000 + month * 100 + day; }
};

struct Phone { int countryCode = 0, cityCode = 0; long long number = 0; };

struct Contact {
    string lastName, firstName, middleName;
    Phone phone; Date birthday; string email;
    Group category = OTHERS;
};

// --- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ---

void setColor(int color) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color); }

static int getSafeInt() {
    int value;
    while (!(cin >> value)) {
        cin.clear();
        cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        setColor(RED); cout << "Ошибка! Введите число: "; setColor(WHITE);
    }
    return value;
}

static void playLoadingAnimation(string message) {
    setColor(YELLOW);
    const char spinner[] = { '.', 'o', 'O', '@', '*' };
    for (int i = 0; i < 10; ++i) {
        cout << "\r" << message << " " << spinner[i % 5] << flush;
        this_thread::sleep_for(chrono::milliseconds(80));
    }
    setColor(GREEN); cout << " [Готово]" << endl; setColor(WHITE);
}

// --- СОХРАНЕНИЕ И ЗАГРУЗКА (Пункт 7 ТЗ) ---

static void saveToFile(const vector<Contact>& db) {
    ofstream out("database.txt");
    for (const auto& c : db) {
        out << c.lastName << ";" << c.firstName << ";" << c.middleName << ";"
            << c.phone.countryCode << ";" << c.phone.cityCode << ";" << c.phone.number << ";"
            << c.birthday.day << ";" << c.birthday.month << ";" << c.birthday.year << ";"
            << c.email << ";" << (int)c.category << endl;
    }
}

static void loadFromFile(vector<Contact>& db) {
    ifstream in("database.txt");
    if (!in.is_open()) return;
    db.clear();
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        stringstream ss(line); string temp; Contact c;
        getline(ss, c.lastName, ';'); getline(ss, c.firstName, ';'); getline(ss, c.middleName, ';');
        getline(ss, temp, ';'); c.phone.countryCode = stoi(temp);
        getline(ss, temp, ';'); c.phone.cityCode = stoi(temp);
        getline(ss, temp, ';'); c.phone.number = stoll(temp);
        getline(ss, temp, ';'); c.birthday.day = stoi(temp);
        getline(ss, temp, ';'); c.birthday.month = stoi(temp);
        getline(ss, temp, ';'); c.birthday.year = stoi(temp);
        getline(ss, c.email, ';');
        getline(ss, temp, ';'); c.category = (Group)stoi(temp);
        db.push_back(c);
    }
}

// --- ПРОСМОТР (Пункт 2 ТЗ) ---

static void showTable(const vector<Contact>& db) {
    if (db.empty()) { setColor(RED); cout << "База пуста.\n"; setColor(WHITE); return; }
    setColor(CYAN);
    cout << setfill('=') << setw(145) << "=" << setfill(' ') << endl;
    cout << "| " << left << setw(15) << "Фамилия" << "| " << setw(12) << "Имя" << "| " << setw(15) << "Отчество"
        << "| " << setw(18) << "Телефон" << "| " << setw(12) << "Дата рожд." << "| " << setw(25) << "Email" << " | " << setw(10) << "Группа" << " |" << endl;
    cout << setfill('-') << setw(145) << "-" << setfill(' ') << endl;

    for (const auto& c : db) {
        if (c.category == WORK) setColor(CYAN);
        else if (c.category == FAMILY) setColor(RED);
        else if (c.category == FRIENDS) setColor(GREEN);
        else setColor(WHITE);

        string bday = to_string(c.birthday.day) + "." + to_string(c.birthday.month) + "." + to_string(c.birthday.year);
        string gName = (c.category == WORK) ? "WORK" : (c.category == FAMILY) ? "FAMILY" : (c.category == FRIENDS) ? "FRIENDS" : "OTHERS";

        cout << "| " << left << setw(15) << c.lastName << "| " << setw(12) << c.firstName
            << "| " << setw(15) << c.middleName << "| +" << c.phone.countryCode << "(" << c.phone.cityCode << ")" << c.phone.number
            << " | " << setw(12) << bday << " | " << setw(25) << c.email << " | " << setw(10) << gName << " |" << endl;
    }
    setColor(CYAN); cout << setfill('=') << setw(145) << "=" << setfill(' ') << endl; setColor(WHITE);
}

// --- ФУНКЦИИ МЕНЮ (Пункты 1, 3, 4, 5, 6 ТЗ) ---

static void addContact(vector<Contact>& db) {
    Contact c;
    cout << "Фамилия: "; cin >> c.lastName; cout << "Имя: "; cin >> c.firstName; cout << "Отчество: "; cin >> c.middleName;
    cout << "Дата (ДД ММ ГГГГ): "; c.birthday.day = getSafeInt(); c.birthday.month = getSafeInt(); c.birthday.year = getSafeInt();
    cout << "Код страны: "; c.phone.countryCode = getSafeInt(); cout << "Код города: "; c.phone.cityCode = getSafeInt();
    cout << "Номер: "; cin >> c.phone.number; cout << "Email: "; cin >> c.email;
    cout << "Группа (0-WORK, 1-FAMILY, 2-FRIENDS, 3-OTHERS): "; c.category = (Group)(getSafeInt() % 4);
    db.push_back(c);
}

static void searchMask(const vector<Contact>& db) {
    cout << "Введите фрагмент фамилии или имени: "; string q; cin >> q;
    vector<Contact> res;
    for (const auto& c : db)
        if (c.lastName.find(q) != string::npos || c.firstName.find(q) != string::npos) res.push_back(c);
    showTable(res);
}

static void deleteContact(vector<Contact>& db) {
    cout << "Введите фамилию для удаления: "; string ln; cin >> ln;
    auto it = remove_if(db.begin(), db.end(), [&](const Contact& c) { return c.lastName == ln; });
    if (it != db.end()) { db.erase(it, db.end()); playLoadingAnimation("Удаление..."); }
    else cout << "Контакт не найден.\n";
}

static void editContact(vector<Contact>& db) {
    cout << "Введите фамилию для редактирования: "; string ln; cin >> ln;
    for (auto& c : db) {
        if (c.lastName == ln) {
            cout << "Новый номер телефона: "; cin >> c.phone.number;
            cout << "Новый Email: "; cin >> c.email;
            playLoadingAnimation("Обновление..."); return;
        }
    }
    cout << "Контакт не найден.\n";
}

static void sortContacts(vector<Contact>& db) {
    cout << "1. По алфавиту\n2. По дате рождения\n> ";
    int s = getSafeInt();
    if (s == 1) sort(db.begin(), db.end(), [](const Contact& a, const Contact& b) {
        return (a.lastName + a.firstName) < (b.lastName + b.firstName);
        });
    else sort(db.begin(), db.end(), [](const Contact& a, const Contact& b) {
        return a.birthday.toInt() < b.birthday.toInt();
        });
    playLoadingAnimation("Сортировка...");
}

// --- ГЛАВНЫЙ ЦИКЛ ---

int main() {
    SetConsoleCP(1251); SetConsoleOutputCP(1251); setlocale(LC_ALL, "Russian");
    vector<Contact> database;
    loadFromFile(database); // Пункт 7: Авточтение

    int choice = -1;
    do {
        setColor(YELLOW);
        cout << "\n1.Добавить 2.Просмотр 3.Поиск 4.Удалить 5.Править 6.Сорт 0.Выход\n> ";
        setColor(WHITE);
        choice = getSafeInt();
        switch (choice) {
        case 1: addContact(database); break;
        case 2: showTable(database); break;
        case 3: searchMask(database); break;
        case 4: deleteContact(database); break;
        case 5: editContact(database); break;
        case 6: sortContacts(database); break;
        }
    } while (choice != 0);

    saveToFile(database); // Пункт 7: Автосохранение
    return 0;
}