#include "Directory.h"
#include "File.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;

Directory::Directory(std::string n, AccessLevel a) : Resource(n, a) {}

// --- Системные функции ---

// 14. Журналирование ( history.log )
void Directory::logOperation(const std::string& msg, bool success) {
    std::ofstream log("history.log", std::ios::app);
    if (log.is_open()) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm* timeinfo = std::localtime(&now);
        log << "[" << std::put_time(timeinfo, "%d.%m.%Y %H:%M:%S") << "] "
            << (success ? "SUCCESS: " : "ERROR: ") << msg << std::endl;
    }
}

// 15. Сортировка функторами
void Directory::sortChildren(int mode) {
    std::sort(children.begin(), children.end(), [mode](const std::unique_ptr<Resource>& a, const std::unique_ptr<Resource>& b) {
        if (mode == 1) return a->getName() < b->getName();
        if (mode == 2) return a->calculateSize() < b->calculateSize();
        if (mode == 3) {
            Date d1 = a->getCreationDate();
            Date d2 = b->getCreationDate();
            if (d1.year != d2.year) return d1.year < d2.year;
            if (d1.month != d2.month) return d1.month < d2.month;
            return d1.day < d2.day;
        }
        return false;
        });

    for (auto& child : children) {
        if (Directory* d = dynamic_cast<Directory*>(child.get())) {
            d->sortChildren(mode);
        }
    }
}

// 16. Экспорт в CSV
void Directory::exportToCSV(std::ofstream& out, const std::string& path) const {
    for (const auto& child : children) {
        std::string currentFullPath = path + "/" + child->getName();
        std::string type = dynamic_cast<Directory*>(child.get()) ? "Directory" : "File";
        Date d = child->getCreationDate();

        out << currentFullPath << ","
            << type << ","
            << child->calculateSize() << ","
            << d.day << "." << d.month << "." << d.year << "\n";

        if (Directory* dSub = dynamic_cast<Directory*>(child.get())) {
            dSub->exportToCSV(out, currentFullPath);
        }
    }
}

// --- Базовая логика ---

size_t Directory::calculateSize() const {
    size_t total = 0;
    for (const auto& child : children) total += child->calculateSize();
    return total;
}

void Directory::getAuditStats(AuditStats& stats) const {
    for (const auto& child : children) {
        if (Directory* d = dynamic_cast<Directory*>(child.get())) {
            stats.dirCount++;
            d->getAuditStats(stats);
        }
        else if (File* f = dynamic_cast<File*>(child.get())) {
            stats.fileCount++;
            stats.totalSize += f->calculateSize();
        }
    }
}

void Directory::findByDate(Date start, Date end, std::vector<const Resource*>& results) const {
    for (const auto& child : children) {
        if (child->getCreationDate() >= start && child->getCreationDate() <= end) {
            results.push_back(child.get());
        }
        if (Directory* d = dynamic_cast<Directory*>(child.get())) {
            d->findByDate(start, end, results);
        }
    }
}

void Directory::findResources(const std::string& namePart, const std::string& ext, std::vector<const Resource*>& results) const {
    for (const auto& child : children) {
        bool nameMatch = (child->getName().find(namePart) != std::string::npos);
        bool extMatch = true;
        if (!ext.empty()) {
            if (File* f = dynamic_cast<File*>(child.get())) {
                extMatch = (f->getName().length() >= ext.length() &&
                    f->getName().substr(f->getName().length() - ext.length()) == ext);
            }
            else extMatch = false;
        }
        if (nameMatch && extMatch) results.push_back(child.get());
        if (Directory* d = dynamic_cast<Directory*>(child.get())) d->findResources(namePart, ext, results);
    }
}

void Directory::addResource(std::unique_ptr<Resource> res, const std::string& currentPath) {
    fs::path fullPath = fs::path(currentPath) / res->getName();
    if (Directory* dir = dynamic_cast<Directory*>(res.get())) {
        if (!fs::exists(fullPath)) fs::create_directories(fullPath);
        for (const auto& child : dir->children) dir->addResource(child->clone(), fullPath.string());
    }
    else {
        // Здесь можно реализовать физическое создание файла нужного размера, 
        // но для логики архива мы просто фиксируем объект в памяти
        std::ofstream f(fullPath.string() + ".txt");
        if (f.is_open()) f.close();
    }
    children.push_back(std::move(res));
}

bool Directory::removeResource(const std::string& name, const std::string& currentPath) {
    fs::path fullPath = fs::path(currentPath) / name;
    auto it = std::remove_if(children.begin(), children.end(), [&](const std::unique_ptr<Resource>& r) {
        if (r->getName() == name) {
            if (fs::exists(fullPath)) fs::remove_all(fullPath);
            return true;
        }
        return false;
        });
    if (it != children.end()) { children.erase(it, children.end()); return true; }
    return false;
}

void Directory::print(int indent) const {
    for (int i = 0; i < indent; ++i) std::cout << "  ";
    std::cout << (indent > 0 ? "|-- " : "") << "[D] " << getName()
        << " [" << calculateSize() << " байт]" << std::endl;
    for (const auto& c : children) c->print(indent + 1);
}

std::unique_ptr<Resource> Directory::clone() const {
    auto copy = std::make_unique<Directory>(getName(), getAccess());
    for (const auto& c : children) copy->children.push_back(c->clone());
    return copy;
}

// ИСПРАВЛЕННАЯ СЕРИАЛИЗАЦИЯ
void Directory::serialize(std::ostream& out) const {
    size_t count = children.size();
    out.write((char*)&count, sizeof(count));
    for (const auto& c : children) {
        int type = (dynamic_cast<Directory*>(c.get())) ? 2 : 1;
        out.write((char*)&type, sizeof(type));

        size_t nLen = c->getName().size();
        out.write((char*)&nLen, sizeof(nLen));
        out.write(c->getName().c_str(), nLen);

        // Сохраняем реальный размер ресурса
        size_t rSize = c->calculateSize();
        out.write((char*)&rSize, sizeof(rSize));

        c->serialize(out);
    }
}

// ИСПРАВЛЕННАЯ ДЕСЕРИАЛИЗАЦИЯ
void Directory::deserialize(std::istream& in, const std::string& currentPath) {
    size_t count = 0;
    if (!in.read((char*)&count, sizeof(count))) return;
    for (size_t i = 0; i < count; ++i) {
        int type = 0; in.read((char*)&type, sizeof(type));
        size_t nLen = 0; in.read((char*)&nLen, sizeof(nLen));
        std::string n(nLen, ' '); in.read(&n[0], nLen);

        // Читаем сохраненный размер
        size_t rSize = 0;
        in.read((char*)&rSize, sizeof(rSize));

        if (type == 1) {
            // Создаем файл с ПРАВИЛЬНЫМ размером из архива
            auto file = std::make_unique<File>(n, ".txt", rSize, AccessLevel::USER);
            file->deserialize(in, currentPath);
            children.push_back(std::move(file));
        }
        else {
            auto sub = std::make_unique<Directory>(n, AccessLevel::USER);
            sub->deserialize(in, currentPath + "/" + n);
            children.push_back(std::move(sub));
        }
    }
}