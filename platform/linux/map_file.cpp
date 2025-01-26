
extern "C" {
#include "map_file.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Funkce pro mapování souboru do paměti
void* map_file_to_memory(const char *name, size_t *sz) {
    if (!name || !sz) {
        return NULL;
    }

    // Otevření souboru pro čtení
    int fd = open(name, O_RDONLY);
    if (fd == -1) {
        perror("Chyba při otevírání souboru");
        return NULL;
    }

    // Získání velikosti souboru
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Chyba při získávání velikosti souboru");
        close(fd);
        return NULL;
    }
    *sz = st.st_size;

    // Namapování souboru do paměti
    void *mapped = mmap(NULL, *sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Chyba při mapování souboru");
        close(fd);
        return NULL;
    }

    // Zavření souborového popisovače (není již potřeba po mmap)
    close(fd);

    return mapped;
}

// Funkce pro zrušení mapování
void unmap_file(void *ptr, size_t sz) {
    if (!ptr || sz == 0) {
        return;
    }

    // Zrušení mapování
    if (munmap(ptr, sz) == -1) {
        perror("Chyba při rušení mapování");
    }
}
