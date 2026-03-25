#include "Resource.h"
class File : public Resource {
private:
    std::string extension;
    size_t size;
public:
    File(std::string n, std::string ext, size_t s, AccessLevel a);

    size_t calculateSize() const override; // Должно быть!
    void print(int indent) const override;
    void serialize(std::ostream& out) const override;
    void deserialize(std::istream& in, const std::string& currentPath) override;
    std::unique_ptr<Resource> clone() const override;
};