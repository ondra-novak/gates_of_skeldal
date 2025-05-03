#include "ddl_ar_class.h"


bool DDLArchive::open(std::filesystem::path ar_file) {
    _directory.clear();
    _ar_file = std::move(ar_file);

    std::ifstream f = begin_read();
    if (!f) return false;
    f.seekg(4,std::ios::beg);
    uint32_t dir_offset;
    f.read(reinterpret_cast<char *>(&dir_offset), sizeof(dir_offset));
    if (f.gcount() != sizeof(dir_offset)) return false;
    uint32_t smallest_offset = std::numeric_limits<uint32_t>::max();
    f.seekg(dir_offset, std::ios::beg);
    while (f.tellg() < smallest_offset) {
        char name[13];
        f.read(name, 12);
        if (f.gcount() != 12) return false;
        name[12] = 0;
        uint32_t offset;
        f.read(reinterpret_cast<char *>(&offset), 4);
        if (f.gcount() != 4) return false;
        _directory.emplace(name, FileRec{offset, {}});
        smallest_offset = std::min(offset, smallest_offset);
    }
    return true;
}

std::ifstream DDLArchive::begin_read() {
    return std::ifstream(_ar_file, std::ios::in|std::ios::binary);
}

DDLArchive::Extracted DDLArchive::extract_file(std::ifstream &s,
        std::string_view fname) {

    auto iter = _directory.find(fname);
    if (iter == _directory.end()) {
        return {fname, false, {}};
    }
    s.seekg(iter->second.offset, std::ios::beg);
    uint32_t sz;
    s.read(reinterpret_cast<char *>(&sz),4);
    if (s.gcount() != 4) return {fname, false, {}};
    std::vector<char> data;
    data.resize(sz);
    s.read(data.data(), sz);
    if (static_cast<uint32_t>(s.gcount()) != sz) return {fname, false, {}};
    return {fname, true, std::move(data)};

}


