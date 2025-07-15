
extern "C" {
#include "map_file.h"
}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdexcept>
#include <filesystem>

// Funkce pro mapování souboru do paměti
const void* map_file_to_memory_cpp(const char *name, size_t *sz) {
    if (!name || !sz) {
        return NULL;
    }

    std::filesystem::path p(reinterpret_cast<const char8_t *>(name));

    HANDLE h = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (h == INVALID_HANDLE_VALUE) throw std::runtime_error(std::string("Failed to open file for mapping: ")+p.string());

    LARGE_INTEGER fsize;
    if (!GetFileSizeEx(h, &fsize)) {
        CloseHandle(h);
        throw std::runtime_error(std::string("failed to get size of file:").append(name));
    }



    HANDLE hMapping = CreateFileMapping(h,NULL,PAGE_READONLY,0,0,NULL);
    if (hMapping == NULL || hMapping == INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        throw std::runtime_error(std::string("Failed to create mapping of file:").append(name));
    }

    void *mappedData = MapViewOfFile(hMapping,FILE_MAP_READ,0,0,0);
    CloseHandle(h);
    CloseHandle(hMapping);
    if (mappedData == NULL) {
        throw std::runtime_error(std::string("Failed to map file:").append(name));
    }

    *sz = fsize.LowPart;
    return mappedData;
}

const void* map_file_to_memory(const char *name, size_t *sz) {
    return map_file_to_memory_cpp(name, sz);
}

// Funkce pro zrušení mapování
void unmap_file(const void *ptr, size_t) {
    UnmapViewOfFile((void *)ptr);
}
