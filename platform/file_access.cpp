#include "platform.h"
#include <filesystem>


std::filesystem::path break_and_compose_path(const std::string_view &pathname, char sep) {
    auto p = pathname.rfind(sep);
    if (p == pathname.npos) {
        if (pathname == "." || pathname == "..") return std::filesystem::canonical(".");
        else if (pathname.empty()) return std::filesystem::current_path().root_path();
        else if (pathname == std::filesystem::current_path().root_name())
            return pathname;
        else return std::filesystem::current_path()/pathname;

    }
    return break_and_compose_path(pathname.substr(0,p), sep) / pathname.substr(p+1);
}


std::filesystem::path convert_pathname_to_path(const std::string_view &pathname) {
    auto p = pathname.find('\\');
    if (p == pathname.npos) {
        p = pathname.find('/');
        if (p == pathname.npos) {
            return std::filesystem::current_path()/pathname;
        }
        return break_and_compose_path(pathname, '/');
    }
    return break_and_compose_path(pathname, '\\');
}

std::filesystem::path try_to_find_file(const std::filesystem::path &p) {
    if (!std::filesystem::exists(p)) {
        std::filesystem::path parent;
        if (p.has_relative_path()) {
            parent = try_to_find_file(p.parent_path());
        }
        if (std::filesystem::exists(parent)) {
            auto iter = std::filesystem::directory_iterator(parent);
            auto end = std::filesystem::directory_iterator();
            std::filesystem::path n = p.filename();
            while (iter != end) {
                const std::filesystem::directory_entry &e = *iter;
                auto fn = e.path().filename();
                if (stricmp(n.c_str(), fn.c_str()) == 0) {
                    return e.path();
                }
                ++iter;
            }
        }
        return p;
    }
    else return p;
}


char check_file_exists(const char *pathname) {
    std::filesystem::path path = try_to_find_file(convert_pathname_to_path(pathname));
    return std::filesystem::exists(path)?1:0;

}
FILE *fopen_icase(const char *pathname, const char *mode) {
    std::filesystem::path path = try_to_find_file(convert_pathname_to_path(pathname));
    return fopen(path.c_str(), mode);
}
