#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "Resource.h"
#include <vector>
#include <string>

class Directory : public Resource {
private:
    std::vector<std::unique_ptr<Resource>> children;
public:
    struct AuditStats {
        size_t fileCount = 0;
        size_t dirCount = 0;
        size_t totalSize = 0;
    };

    Directory(std::string n, AccessLevel a);

    size_t calculateSize() const override;
    void print(int indent) const override;
    void serialize(std::ostream& out) const override;
    void deserialize(std::istream& in, const std::string& currentPath) override;
    std::unique_ptr<Resource> clone() const override;

    void addResource(std::unique_ptr<Resource> res, const std::string& currentPath);
    bool removeResource(const std::string& name, const std::string& currentPath);

    void findResources(const std::string& namePart, const std::string& ext, std::vector<const Resource*>& results) const;
    void findByDate(Date start, Date end, std::vector<const Resource*>& results) const;
    void getAuditStats(AuditStats& stats) const;

    // НОВЫЕ МЕТОДЫ
    void sortChildren(int mode); // 1-имя, 2-размер, 3-дата
    void exportToCSV(std::ofstream& out, const std::string& path) const;
    static void logOperation(const std::string& msg, bool success);

    const std::vector<std::unique_ptr<Resource>>& getItems() const { return children; }
};

#endif