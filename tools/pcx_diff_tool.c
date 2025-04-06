// pcx_diff_tool.c
// Jednoduchý nástroj pro porovnání dvou PCX souborů (8-bit) a uložení rozdílu do nového PCX

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PCX_HEADER_SIZE 128
#define PALETTE_SIZE 768

typedef struct {
    uint8_t *data;
    int width, height;
    uint8_t palette[PALETTE_SIZE];
} PCXImage;

// ------------------------ PCX Dekodovani ------------------------

int pcx_decode_rle(FILE *f, uint8_t *out, int size) {
    int count = 0;
    while (count < size) {
        int byte = fgetc(f);
        if (byte == EOF) return -1;

        if ((byte & 0xC0) == 0xC0) {
            int reps = byte & 0x3F;
            int val = fgetc(f);
            if (val == EOF) return -1;
            for (int i = 0; i < reps && count < size; ++i) out[count++] = val;
        } else {
            out[count++] = byte;
        }
    }
    return 0;
}

PCXImage *pcx_load(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    uint8_t header[PCX_HEADER_SIZE];
    fread(header, 1, PCX_HEADER_SIZE, f);

    if (header[0] != 0x0A || header[3] != 8 || header[65] != 1) {
        fclose(f);
        return NULL; // Neplatný formát
    }

    int x1 = header[4] | (header[5] << 8);
    int y1 = header[6] | (header[7] << 8);
    int x2 = header[8] | (header[9] << 8);
    int y2 = header[10] | (header[11] << 8);

    int width = x2 - x1 + 1;
    int height = y2 - y1 + 1;

    PCXImage *img = calloc(1, sizeof(PCXImage));
    img->width = width;
    img->height = height;
    img->data = malloc(width * height);

    pcx_decode_rle(f, img->data, width * height);

    fseek(f, -PALETTE_SIZE - 1, SEEK_END); // -1 pro 0x0C
    int palette_marker = fgetc(f);
    if (palette_marker != 0x0C) {
        fclose(f);
        free(img->data);
        free(img);
        return NULL; // Paleta není přítomna
    }
    fread(img->palette, 1, PALETTE_SIZE, f);

    fclose(f);
    return img;
}

void pcx_free(PCXImage *img) {
    if (!img) return;
    free(img->data);
    free(img);
}

// ------------------------ PCX Ulozeni ------------------------

void pcx_write_rle(FILE *f, uint8_t *data, int size, int width) {
    int w = width;
    for (int i = 0; i < size;) {
        uint8_t val = data[i];
        int count = 1;
        while (count < 63 && count < w && i + count < size && data[i + count] == val) count++;
        if (count > 1 || (val & 0xC0) == 0xC0) {
            fputc(0xC0 | count, f);
            fputc(val, f);
        } else {
            fputc(val, f);
        }
        w -= count;
        i += count;
        if (w == 0) {
            w = width;
        }
    }
}

int pcx_save(const char *filename, PCXImage *img) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;

    uint8_t header[PCX_HEADER_SIZE] = {0};
    header[0] = 0x0A; // manufacturer
    header[1] = 5;    // version
    header[2] = 1;    // encoding
    header[3] = 8;    // bits per pixel
    header[4] = 0; header[5] = 0; // xmin
    header[6] = 0; header[7] = 0; // ymin
    header[8] = (img->width - 1) & 0xFF;
    header[9] = (img->width - 1) >> 8;
    header[10] = (img->height - 1) & 0xFF;
    header[11] = (img->height - 1) >> 8;
    header[12] = 72; header[13] = 0; // hres (72 dpi)
    header[14] = 72; header[15] = 0; // vres (72 dpi)
    header[65] = 1; // planes
    header[66] = img->width & 0xFF;
    header[67] = img->width >> 8; // bytes per line
    header[68] = 1; header[69] = 0; // palette type (1 = color)

    fwrite(header, 1, PCX_HEADER_SIZE, f);
    pcx_write_rle(f, img->data, img->width * img->height, img->width);

    fputc(0x0C, f); // Palette ID
    fwrite(img->palette, 1, PALETTE_SIZE, f);

    fclose(f);
    return 0;
}

// ------------------------ Hlavní funkce ------------------------

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Použití: %s <referencni.pcx> <komprimovany.pcx> <vystup.pcx>\n", argv[0]);
        return 1;
    }

    PCXImage *ref = pcx_load(argv[1]);
    PCXImage *src = pcx_load(argv[2]);

    if (!ref || !src) {
        printf("Chyba při čtení PCX souborů\n");
        return 1;
    }

    if (ref->width != src->width || ref->height != src->height) {
        printf("Rozměry nesouhlasí!\n");
        return 1;
    }

    PCXImage out = {0};
    out.width = ref->width;
    out.height = ref->height;
    out.data = malloc(out.width * out.height);
    memcpy(out.palette, src->palette, PALETTE_SIZE);

    for (int i = 0; i < out.width * out.height; i++) {
        out.data[i] = src->data[i] ^ ref->data[i];
    }

    pcx_save(argv[3], &out);

    pcx_free(ref);
    pcx_free(src);
    free(out.data);
    return 0;
}
