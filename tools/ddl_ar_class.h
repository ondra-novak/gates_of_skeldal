#pragma once
#include <string>
#include <filesystem>
#include <map>
#include <vector>
#include <fstream>

class DDLArchive {
public:

    struct FileRec {
        uint32_t offset;
        std::filesystem::path source;

    };


    using Directory = std::map<std::string, FileRec, std::less<> >;
    bool open(std::filesystem::path ar_file);
    const Directory &get_directory() const {return _directory;}
    const std::filesystem::path& get_ar_file() const {return _ar_file;}

    struct Extracted {
        std::string_view name;
        bool found;
        std::vector<char> data;
    };


    template<typename Iter, std::invocable<Extracted> CB>
    void extract(Iter from, Iter to, CB &&cb) {
        std::ifstream f = begin_read();
        while (from != to) {
            std::string_view n = *from;
            cb(extract_file(f, n));
            ++from;
        }
    }

protected:

    std::filesystem::path _ar_file;
    Directory _directory;

    std::ifstream begin_read();
    Extracted extract_file(std::ifstream &s, std::string_view fname);


};
