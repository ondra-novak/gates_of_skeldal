#include  "ddl_ar.h"
#include  "ddl_ar_class.h"
#include <iostream>


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
    std::puts("");
    std::puts("-h         this help\n"
              "file.ddl   input ddl file\n"
              "-l         list of files\n"
              "-x         extract files\n");
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

    DDLArchive arch;
    if (!arch.open(ddl_file)) {
        std::cerr << "Failed to read ddl file: " << ddl_file;
        return 1;
    }


    std::string_view swch(argv[2]);
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
