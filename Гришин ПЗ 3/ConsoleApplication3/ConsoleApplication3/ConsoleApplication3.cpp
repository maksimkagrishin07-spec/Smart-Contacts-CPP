#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <windows.h>
#include <sqlext.h>
#include <iomanip>
#include <fstream>
#include <regex>
#include <ctime>

using namespace std;

// Цвета для индикации (Группа В.12)
#define RED 12
#define GREEN 10
#define YELLOW 14
#define WHITE 7

void printColor(string t, int c) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, (WORD)c);
    cout << t;
    SetConsoleTextAttribute(h, WHITE);
}

int getValidInt() {
    int n;
    while (!(cin >> n)) {
        printColor("Ошибка! Введите число: ", RED);
        cin.clear(); cin.ignore(1000, '\n');
    }
    return n;
}

// --- БАЗА ДАННЫХ ---

void LogAction(SQLHDBC hDbc, string act) {
    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    string q = "INSERT INTO Logs (Description, LogDate) VALUES (?, GETDATE())";
    SQLPrepareA(h, (SQLCHAR*)q.c_str(), SQL_NTS);
    SQLBindParameter(h, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0, (SQLPOINTER)act.c_str(), 0, NULL);
    SQLExecute(h); SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// 1. ПОИСК
void Search(SQLHDBC hDbc) {
    string mask; cout << "Поиск по имени: "; cin >> mask; mask = "%" + mask + "%";
    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    string q = "SELECT ResourceID, Name, Size FROM Resources WHERE Name LIKE ? AND isDeleted = 0";
    SQLPrepareA(h, (SQLCHAR*)q.c_str(), SQL_NTS);
    SQLBindParameter(h, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0, (SQLPOINTER)mask.c_str(), 0, NULL);
    if (SQLExecute(h) == SQL_SUCCESS) {
        SQLINTEGER id; SQLCHAR n[256]; SQLINTEGER s;
        cout << "\nID   | Имя             | Размер\n--------------------------------\n";
        while (SQLFetch(h) == SQL_SUCCESS) {
            SQLGetData(h, 1, SQL_C_LONG, &id, 0, NULL);
            SQLGetData(h, 2, SQL_C_CHAR, n, 256, NULL);
            SQLGetData(h, 3, SQL_C_LONG, &s, 0, NULL);
            string nStr = (char*)n; if (nStr.length() > 15) nStr = nStr.substr(0, 12) + "...";
            cout << left << setw(4) << id << " | " << setw(15) << nStr << " | " << s << " KB" << endl;
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// 2. ДОБАВИТЬ
void AddResource(SQLHDBC hDbc) {
    string name; int catID, ownerID;
    cout << "Имя файла (.txt, .pdf, .exe): "; cin >> name;
    if (name.find(".txt") == string::npos && name.find(".pdf") == string::npos && name.find(".exe") == string::npos) {
        printColor("Ошибка: расширение запрещено!\n", RED); return;
    }
    long realSize = rand() % 8000 + 150;
    cout << "ID категории (1-Документы, 2-Медиа): "; catID = getValidInt();
    cout << "ID владельца: "; ownerID = getValidInt();

    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    string q = "INSERT INTO Resources (Name, Size, CategoryID, OwnerID, isDeleted) VALUES (?, ?, ?, ?, 0)";
    SQLPrepareA(h, (SQLCHAR*)q.c_str(), SQL_NTS);
    SQLBindParameter(h, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0, (SQLPOINTER)name.c_str(), 0, NULL);
    SQLBindParameter(h, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &realSize, 0, NULL);
    SQLBindParameter(h, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &catID, 0, NULL);
    SQLBindParameter(h, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &ownerID, 0, NULL);
    if (SQLExecute(h) == SQL_SUCCESS) {
        printColor("Создано! Размер (авто): " + to_string(realSize) + " KB\n", GREEN);
        LogAction(hDbc, "Added: " + name + " (ID auto-inc)");
    }
    SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// 3. СТАТИСТИКА
void ShowStats(SQLHDBC hDbc) {
    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    const char* q = "SELECT COUNT(*), SUM(Size) FROM Resources WHERE isDeleted = 0";
    if (SQLExecDirectA(h, (SQLCHAR*)q, SQL_NTS) == SQL_SUCCESS && SQLFetch(h) == SQL_SUCCESS) {
        long cnt; long long sz = 0;
        SQLGetData(h, 1, SQL_C_LONG, &cnt, 0, NULL);
        SQLGetData(h, 2, SQL_C_SBIGINT, &sz, 0, NULL);
        cout << "\nФайлов в базе: " << cnt << " | Общий вес: " << sz << " KB\n";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// 4. В КОРЗИНУ (Удаление по конкретному ID)
void MoveToTrash(SQLHDBC hDbc) {
    cout << "Введите ID файла для удаления в корзину: ";
    int id = getValidInt();

    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    string q = "UPDATE Resources SET isDeleted = 1 WHERE ResourceID = ?";
    SQLPrepareA(h, (SQLCHAR*)q.c_str(), SQL_NTS);
    SQLBindParameter(h, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, 0, NULL);

    if (SQLExecute(h) == SQL_SUCCESS) {
        SQLLEN rows; SQLRowCount(h, &rows);
        if (rows > 0) {
            printColor("Файл ID " + to_string(id) + " перемещен в корзину!\n", YELLOW);
            LogAction(hDbc, "Moved to trash ID: " + to_string(id));
        }
        else {
            printColor("Файл с таким ID не найден!\n", RED);
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// 5. ПРОСМОТР КОРЗИНЫ
void ShowTrash(SQLHDBC hDbc) {
    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    SQLExecDirectA(h, (SQLCHAR*)"SELECT ResourceID, Name FROM Resources WHERE isDeleted = 1", SQL_NTS);
    cout << "\n--- КОРЗИНА ---\nID | Имя\n";
    SQLINTEGER id; SQLCHAR n[256];
    while (SQLFetch(h) == SQL_SUCCESS) {
        SQLGetData(h, 1, SQL_C_LONG, &id, 0, NULL);
        SQLGetData(h, 2, SQL_C_CHAR, n, 256, NULL);
        cout << id << " | " << n << endl;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// 6. ВОССТАНОВИТЬ ИЗ КОРЗИНЫ
void Restore(SQLHDBC hDbc) {
    cout << "Введите ID для восстановления: "; int id = getValidInt();
    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    string q = "UPDATE Resources SET isDeleted = 0 WHERE ResourceID = ?";
    SQLPrepareA(h, (SQLCHAR*)q.c_str(), SQL_NTS);
    SQLBindParameter(h, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &id, 0, NULL);
    if (SQLExecute(h) == SQL_SUCCESS) {
        SQLLEN rows; SQLRowCount(h, &rows);
        if (rows > 0) printColor("Файл успешно восстановлен!\n", GREEN);
        else printColor("ID не найден в корзине.\n", RED);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// 7. ЭКСПОРТ CSV
void ExportCSV(SQLHDBC hDbc) {
    ofstream f("export.csv"); f << "ID;Name;Size\n";
    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    SQLExecDirectA(h, (SQLCHAR*)"SELECT ResourceID, Name, Size FROM Resources WHERE isDeleted = 0", SQL_NTS);
    SQLINTEGER id; SQLCHAR n[256]; SQLINTEGER s;
    while (SQLFetch(h) == SQL_SUCCESS) {
        SQLGetData(h, 1, SQL_C_LONG, &id, 0, NULL);
        SQLGetData(h, 2, SQL_C_CHAR, n, 256, NULL);
        SQLGetData(h, 3, SQL_C_LONG, &s, 0, NULL);
        f << id << ";" << (char*)n << ";" << s << "\n";
    }
    f.close(); printColor("Экспортировано в export.csv\n", GREEN);
    SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// 8. ОЧИСТКА СТАРЫХ (30+ дней)
void CleanOld(SQLHDBC hDbc) {
    SQLHSTMT h; SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &h);
    if (SQLExecDirectA(h, (SQLCHAR*)"DELETE FROM Resources WHERE CreatedAt < DATEADD(day, -30, GETDATE())", SQL_NTS) == SQL_SUCCESS) {
        SQLLEN rows; SQLRowCount(h, &rows);
        printColor("Удалено старых записей: " + to_string(rows) + "\n", RED);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, h);
}

// --- MAIN ---
int main() {
    srand((unsigned int)time(0)); setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251); SetConsoleOutputCP(1251);
    SQLHENV hEnv; SQLHDBC hDbc;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);

    string connStr = "DRIVER={SQL Server};SERVER=.\\SQLEXPRESS;DATABASE=ByteKeeperDB;Trusted_Connection=yes;";
    if (SQLDriverConnectA(hDbc, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT) < 0) {
        printColor("Ошибка подключения к ByteKeeperDB!\n", RED);
        return -1;
    }

    int choice = -1;
    while (choice != 0) {
        cout << "\n==== BYTEKEEPER PRO (8 ФУНКЦИЙ) ====\n";
        cout << "1. Поиск       | 2. Добавить    | 3. Статистика  | 4. В корзину\n";
        cout << "5. Корзина (Т) | 6. Из корзины  | 7. Экспорт CSV | 8. Чистка старых\n";
        cout << "0. Выход\nВыбор: ";
        choice = getValidInt();
        switch (choice) {
        case 1: Search(hDbc); break;
        case 2: AddResource(hDbc); break;
        case 3: ShowStats(hDbc); break;
        case 4: MoveToTrash(hDbc); break;
        case 5: ShowTrash(hDbc); break;
        case 6: Restore(hDbc); break;
        case 7: ExportCSV(hDbc); break;
        case 8: CleanOld(hDbc); break;
        }
    }

    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    return 0;
}