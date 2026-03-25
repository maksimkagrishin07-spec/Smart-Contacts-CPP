#include "File.h"
#include <iostream>

File::File(std::string n, std::string ext, size_t s, AccessLevel a)
    : Resource(n, a), extension(ext), size(s) {
}

size_t File::calculateSize() const {
    return size;
}

void File::print(int indent) const {
    for (int i = 0; i < indent; ++i) std::cout << "  ";
    std::cout << "|-- [F] " << getName() << extension << " (" << size << " bytes)" << std::endl;
}

void File::serialize(std::ostream& out) const {
    size_t eLen = extension.size();
    out.write((char*)&eLen, sizeof(eLen));
    out.write(extension.c_str(), eLen);
    out.write((char*)&size, sizeof(size));
}

void File::deserialize(std::istream& in, const std::string& currentPath) {
    size_t eLen = 0;
    if (in.read((char*)&eLen, sizeof(eLen))) {
        extension.resize(eLen);
        in.read(&extension[0], eLen);
    }
    in.read((char*)&size, sizeof(size));
}


std::unique_ptr<Resource> File::clone() const {
    return std::make_unique<File>(getName(), extension, size, getAccess());
}