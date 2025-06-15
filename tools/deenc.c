#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.ENC>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("Can't open file");
        return 1;
    }

    // Získání velikosti souboru
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    // Alokace bufferu
    unsigned char *data = malloc(size);
    if (!data) {
        perror("Memory alloc error");
        fclose(f);
        return 1;
    }

    fread(data, 1, size, f);
    fclose(f);

    int last=0;
    for (int i = 0; i < size; ++i) {
        last = (last + data[i]) & 0xFF;
        data[i] = last;
    }

    // Výstup na stdout
    fwrite(data, 1, size, stdout);
    free(data);

    return 0;
}
