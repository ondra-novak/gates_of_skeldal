#include "platform.h"
#include <string.h>
#include <wchar.h>
#include <cstdarg>
#include <chrono>
#include <filesystem>
#include "../libs/logfile.h"


std::filesystem::path break_and_compose_path(const std::string_view &pathname, char sep) {
    auto p = pathname.rfind(sep);
    if (p == pathname.npos) {
        if (pathname == "." || pathname == "..") {
            return std::filesystem::canonical(".");
        } else if (pathname.empty()) {
            return std::filesystem::current_path().root_path();
        } else if (pathname == std::filesystem::current_path().root_name()) {
            return std::filesystem::current_path().root_path();
        } else {
            return std::filesystem::current_path()/pathname;
        }

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
                #ifdef _WIN32
                if (_wcsicmp(n.c_str(), fn.c_str()) == 0) {
                #else
                if (istrcmp(n.c_str(), fn.c_str()) == 0) {
                #endif
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

const char *file_icase_find(const char *pathname) {
    static std::string p;
    std::filesystem::path path = try_to_find_file(convert_pathname_to_path(pathname));
    p = path.string();
    return p.c_str();
}

FILE *fopen_icase(const char *pathname, const char *mode) {
    std::filesystem::path path = try_to_find_file(convert_pathname_to_path(pathname));
    return fopen(path.string().c_str(), mode);
}

static thread_local std::string build_pathname_buffer;



const char * build_pathname(size_t nparts, const char *part1, ...) {
    va_list lst;
    va_start(lst, part1);

    std::filesystem::path p = part1;
    for (size_t i = 1; i < nparts; ++i) {
        p = p / va_arg(lst, const char *);
    }
    build_pathname_buffer = p.string();
    SEND_LOG("(BUILD_PATHNAME) %s", build_pathname_buffer.c_str());
    return build_pathname_buffer.c_str();
}

char create_directories(const char *path) {
    std::filesystem::path p(path);
    std::error_code ec;
    return std::filesystem::create_directories(p, ec)?1:0;
}


char change_current_directory(const char *path) {
    std::error_code ec;
    std::filesystem::current_path(std::filesystem::path(path), ec);
    return ec == std::error_code{}?1:0;
}


std::chrono::system_clock::time_point from_file_time(std::filesystem::file_time_type::clock::time_point tp) {
    return  std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            tp - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );

}

int list_files(const char *directory, int type, LIST_FILES_CALLBACK cb, void *ctx) {
    std::error_code ec;
    std::filesystem::directory_iterator iter(std::string(directory), ec);
    if (ec == std::error_code{}) {
        while (iter != std::filesystem::directory_iterator()) {
            int r = 0;
            const auto &entry = *iter;
            const char *name;
            size_t szortm = 0;
            std::string tmp;
            if (type & file_type_just_name) {
                tmp = entry.path().filename().string();
            } else {
                tmp = entry.path().string();
            }
            if (type & file_type_need_timestamp) {
                auto tm = from_file_time(entry.last_write_time(ec));
                szortm = std::chrono::system_clock::to_time_t(tm);
            } else {
                szortm = entry.file_size(ec);
            }
             name = tmp.c_str();
            if (entry.is_regular_file(ec) && (type & file_type_normal)) {
                r = cb(name, file_type_normal, szortm, ctx);
            } else if (entry.is_directory(ec)) {
                int dot = entry.path().filename() == "." || entry.path().filename() == "..";
                if (!dot && (type & file_type_directory)) {
                    r = cb(name, file_type_directory, szortm, ctx);
                } else if (dot & (type & file_type_dot)) {
                    r = cb(name, file_type_dot, szortm, ctx);
                }
            }
            if (r) return r;
            ++iter;
        }
    }
    return 0;
}
