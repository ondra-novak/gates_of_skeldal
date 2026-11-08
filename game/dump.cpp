#include <platform/platform.h>
#include <iostream>
#include <filesystem>
#include <regex>
#include <string>
#include <optional>
#include <libs/memman.h>

int findMaxDumpNumber(const std::string &directoryPath) {
    namespace fs = std::filesystem;
    std::regex pattern(R"(dump(\d{4})\.bmp)");
    int maxNumber = -1;

    for (const auto &entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::smatch match;
            std::string fileName = entry.path().filename().string();

            if (std::regex_match(fileName, match, pattern)) {
                int number = std::stoi(match[1]); // Extrahujeme číslo NNNN
                if (number > maxNumber) {
                    maxNumber = number; // Aktualizujeme maximální číslo
                }
            }
        }
    }

    return maxNumber;
}

extern "C" {
void save_dump(const uint16_t *screen_addr,
        unsigned int width,
        unsigned int height,
        unsigned int linelen);
}

void save_dump(const uint16_t *screen_addr,
        unsigned int width,
        unsigned int height,
        unsigned int linelen) {
    static int dump_counter = -1;
    FILE *f;
    int i, r, g, b, x, y;
    const uint16_t *a;
    char c[20];

    if (dump_counter == -1) {
        dump_counter = findMaxDumpNumber(".");
        SEND_LOG("(DUMP) Dump counter sets to %d", dump_counter);
    }
    snprintf(c, sizeof(c), "dump%04d.bmp", ++dump_counter);
    SEND_LOG("(DUMP) Saving screen shot named '%s'", c);
    f = fopen_icase(c, "wb");
    fputc('B', f);
    fputc('M', f);
    i = width * height * 3 + 0x36;
    fwrite(&i, 1, 4, f);
    i = 0;
    fwrite(&i, 1, 4, f);
    i = 0x36;
    fwrite(&i, 1, 4, f);
    i = 0x28;
    fwrite(&i, 1, 4, f);
    i = width;
    fwrite(&i, 1, 4, f);
    i = height;
    fwrite(&i, 1, 4, f);
    i = 1;
    fwrite(&i, 1, 2, f);
    i = 24;
    fwrite(&i, 1, 2, f);
    i = 0;
    fwrite(&i, 1, 4, f);
    i = width * height * 3;
    fwrite(&i, 1, 4, f);
    for (i = 4, r = 0; i > 0; i--)
        fwrite(&r, 1, 4, f);
    for (y = height; y > 0; y--) {
        const uint16_t *scr = screen_addr;
        a = scr + (y - 1) * linelen;
        for (x = 0; (unsigned int)x < width; x++) {
            i = a[x];
            b = (i & 0x1f) << 3;
            g = (i & 0x7ff) >> 3;
            r = i >> 8;
            i = ((r * 256) + g) * 256 + b;
            fwrite(&i, 1, 3, f);
        }
    }
    fclose(f);
}
