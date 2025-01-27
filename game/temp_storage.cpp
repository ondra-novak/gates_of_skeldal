#include <cstdarg>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

extern "C" {
#include "temp_storage.h"
}

typedef struct _temp_storage_file_wr {
    std::vector<uint8_t> *_data;
} TMPFILE_WR;

typedef struct _temp_storage_file_rd {
    std::basic_string_view<uint8_t> _data;
    int skp = 0;
    int scan_ret = 0;
} TMPFILE_RD;

using FileSystem = std::map<std::string, std::vector<uint8_t>, std::less<> >;
static FileSystem temp_fsystem;


void temp_storage_store(const char *name, const void *data, int32_t size) {
    auto b = reinterpret_cast<const uint8_t *>(data);
    auto e = b+size;
    temp_fsystem[std::string(name)] = {b,e};
}

int32_t temp_storage_find(const char *name) {
    auto iter = temp_fsystem.find(std::string_view(name));
    if (iter == temp_fsystem.end()) return -1;
    return iter->second.size();
}

int32_t temp_storage_retrieve(const char *name, void *data, int32_t size) {
    auto iter = temp_fsystem.find(std::string_view(name));
    if (iter == temp_fsystem.end()) return -1;
    size = std::min<int32_t>(size, iter->second.size());
    std::copy(iter->second.begin(), iter->second.end(), reinterpret_cast<uint8_t *>(data));
    return iter->second.size();

}

void temp_storage_list(void (*callback)(const char*, void*), void *context) {
    for (const auto &[k,v]: temp_fsystem) {
        callback(k.c_str(), context);
    }
}

void temp_storage_clear() {
    temp_fsystem.clear();
}

TMPFILE_RD* temp_storage_open(const char *name) {
    auto iter = temp_fsystem.find(std::string_view(name));
    if (iter == temp_fsystem.end()) return NULL;
    return new TMPFILE_RD{std::basic_string_view<uint8_t>(iter->second.data(), iter->second.size())};
}

TMPFILE_WR* temp_storage_create(const char *name) {
    auto &v = temp_fsystem[std::string(name)] = {};
    return new TMPFILE_WR{&v};
}

TMPFILE_WR* temp_storage_append(const char *name) {
    auto &v = temp_fsystem[std::string(name)];
    return new TMPFILE_WR{&v};
}

void temp_storage_close_rd(TMPFILE_RD *f) {
    delete f;
}

void temp_storage_close_wr(TMPFILE_WR *f) {
    delete f;
}

void temp_storage_write(const void *data, uint32_t size, TMPFILE_WR *f) {
    auto b = reinterpret_cast<const uint8_t *>(data);
    auto e = b+size;
    std::copy(b,e, std::back_inserter(*f->_data));
}

uint32_t temp_storage_read(void *data, uint32_t size, TMPFILE_RD *f) {
    auto &d = f->_data;
    auto p = d.substr(0,size);
    d = d.substr(p.size());
    auto b = reinterpret_cast<uint8_t *>(data);
    std::copy(d.begin(), d.end(), b);
    return p.size();
}

void temp_storage_skip(TMPFILE_RD *f, int bytes) {
    auto &d = f->_data;
    auto p = d.substr(0,bytes);
    d = d.substr(p.size());
}

void temp_storage_delete(const char *name) {
    temp_fsystem.erase(std::string(name));
}


int temp_storage_getc(TMPFILE_RD *f) {
    if (f->_data.empty()) return -1;
    int r = f->_data[0];
    f->_data = f->_data.substr(1);
    return r;
}

char *temp_storage_gets(char *buff, size_t sz, TMPFILE_RD *f) {
    auto &d =f->_data;
    auto pos = d.find('\n');
    if (pos > d.size()) pos = d.size();
    if (pos == 0) return NULL;
    if (pos > sz - 1) pos = sz - 1;
    temp_storage_read(buff, pos, f);
    buff[pos] = 0;
    return buff;
}


void temp_storage_internal_begin_scanf(TMPFILE_RD *f, const char *format, ...) {
    if (f->_data.empty()) {
        f->scan_ret = -1;
    }
    va_list lst;
    va_start(lst, format);
    f->scan_ret =  vsscanf(reinterpret_cast<const char *>(f->_data.data()), format, lst);
    va_end(lst);
}

int *temp_storage_internal_skip_ptr(TMPFILE_RD *f) {
    return &f->skp;
}

int temp_storage_internal_end_scanf(TMPFILE_RD *f) {
    temp_storage_skip(f, f->skp);
    return f->scan_ret;
}

void temp_storage_ungetc(TMPFILE_RD *f) {
    f->_data = std::basic_string_view<uint8_t>(f->_data.data()-1, f->_data.size()+1);
}
