#ifndef RESOURCE_H
#define RESOURCE_H

#pragma warning(disable : 4996)
#include <string>
#include <memory>
#include <iostream>
#include <ctime>

enum class AccessLevel { GUEST = 0, USER = 1, ADMIN = 2 };

struct Date {
    int day, month, year;
    bool operator>=(const Date& o) const {
        if (year != o.year) return year > o.year;
        if (month != o.month) return month > o.month;
        return day >= o.day;
    }
    bool operator<=(const Date& o) const {
        if (year != o.year) return year < o.year;
        if (month != o.month) return month < o.month;
        return day <= o.day;
    }
};

class Resource {
protected:
    std::string name;
    AccessLevel access;
    Date creationDate;

public:
    Resource(std::string n, AccessLevel a) : name(n), access(a) {
        time_t t = time(0);
        tm* now = localtime(&t);
        creationDate = { now->tm_mday, now->tm_mon + 1, now->tm_year + 1900 };
    }
    virtual ~Resource() = default;

    std::string getName() const { return name; }
    AccessLevel getAccess() const { return access; }
    Date getCreationDate() const { return creationDate; }

    virtual size_t calculateSize() const = 0;
    virtual void print(int indent) const = 0;
    virtual void serialize(std::ostream& out) const = 0;
    virtual void deserialize(std::istream& in, const std::string& currentPath) = 0;
    virtual std::unique_ptr<Resource> clone() const = 0;
};

#endif