#include  "ddl_ar.h"
#include  "ddl_ar_class.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>


void show_licence_header() {
std::puts("Copyright (c) 2025 Ondrej Novak. All rights reserved.\n"
          "This work is licensed under the terms of the MIT license.\n"
          "For a copy, see <https://opensource.org/licenses/MIT>\n");
}

void show_short_help() {

    show_licence_header();
    std::puts("Use -h for help");
}

void show_help() {
    show_licence_header();
    std::puts("ddl_ar -h");
    std::puts("ddl_ar <file.ddl> -l");
    std::puts("ddl_ar <file.ddl> -x <files...>");
    std::puts("ddl_ar <file.ddl> -n <directory>");
    std::puts("");
    std::puts("-h         this help\n"
              "file.ddl   input ddl file\n"
              "-l         list of files\n"
              "-x         extract files\n"
              "-n         pack directory to new file\n"
    );
}

struct DirEntry {
    std::filesystem::path path;
    std::string name;
    uint32_t size;
    uint32_t offset;
};

void new_ddl(std::string ddl_file, std::string path) {
    std::vector<DirEntry> dir;

    std::filesystem::directory_iterator iter(path);
    std::filesystem::directory_iterator end;
    while (iter != end) {
        const auto &entry = *iter;
        if (entry.is_regular_file()) {
            uint32_t sz = entry.file_size();
            std::string fname = entry.path().filename().string();
            if (fname.size() <= 12) {
                std::transform(fname.begin(), fname.end(), fname.begin(),[](char c)->char{return std::toupper(c);});
                dir.push_back(DirEntry{entry.path(), fname, sz,0});
            }
        }
        ++iter;

    }

    if (dir.empty()) {
        std::cerr << "No files found, nothing changed" << std::endl;
    }

    std::size_t dataofs = 8 + dir.size()*(12+4);

    for (auto &x: dir) {
        x.offset = dataofs;
        dataofs+=4+x.size;
    }

    uint32_t group = 0;
    uint32_t group_offset = 8;

    std::ofstream f(ddl_file, std::ios::out|std::ios::trunc|std::ios::binary);

    f.write(reinterpret_cast<const char *>(&group),4);
    f.write(reinterpret_cast<const char *>(&group_offset),4);

    for (const auto &x: dir) {
        char n[13];
        std::strncpy(n,x.name.c_str(), 12);
        n[12] = 0;
        f.write(n,12);
        f.write(reinterpret_cast<const char *>(&x.offset),4);
    }
    for (const auto &x: dir) {
        f.write(reinterpret_cast<const char *>(&x.size),4);
        std::ifstream in(x.path, std::ios::in|std::ios::binary);
        char *buff = new char[x.size];
        if (!in) {
            std::cerr << "Failed to open " << x.path << std::endl;
        } else {
            in.read(buff,x.size);
            f.write(buff,x.size);
            std::cout << "Added " << x.name << " from file " << x.path << std::endl;
        }
        delete [] buff;
    }


}

int main(int argc, char **argv) {
    if (argc <= 1) {
        show_short_help();
        return 1;
    }

    std::string_view ddl_file = argv[1];
    if (ddl_file == "-h") {
        show_help();
        return 1;
    }

    if (argc <= 2) {
        show_short_help();
        return 1;
    }

    std::string_view swch(argv[2]);
    if (swch == "-n") {
        if (argc <= 3) {
            show_short_help();
            return 1;
        }
        new_ddl(std::string(ddl_file), argv[3]);
        return 0;
    }


    DDLArchive arch;
    if (!arch.open(ddl_file)) {
        std::cerr << "Failed to read ddl file: " << ddl_file;
        return 1;
    }

    if (swch == "-l") {
        for (const auto &x: arch.get_directory()) {
            std::puts(x.first.c_str());
        }
    } else if (swch == "-x") {
        std::vector<std::string_view> fls;
        fls.reserve(argc);
        for (int i = 3; i < argc; ++i) fls.push_back(argv[i]);
        arch.extract(fls.begin(), fls.end(), [](const DDLArchive::Extracted &x){
            if (!x.found) {
                std::cout << "Not found: " << x.name << "\n";
            } else {
                std::ofstream out(std::string(x.name), std::ios::out|std::ios::binary);
                out.write(x.data.data(), x.data.size());
                std::cout << "Extracted: " << x.name << " (" << x.data.size() << " bytes)\n";
            }
        });
    } else {
        std::cerr << "Unknown switch " << swch << std::endl;
    }

    return 0;
}
