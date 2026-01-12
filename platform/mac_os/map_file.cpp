#include "../platform.h"
#include "../error.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Function for mapping a file into memory
void* map_file_to_memory(const char *name, size_t *sz) {
    if (!name || !sz) {
        return NULL;
    }

    // Open file for reading
    int fd = open(name, O_RDONLY);
    if (fd == -1) {
        display_error("Failed to open file: %s %s", name, strerror(errno));
        return NULL;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        display_error("Failed to fstat file: %s %s", name, strerror(errno));
        close(fd);
        return NULL;
    }
    *sz = st.st_size;

    // Map file into memory
    void *mapped = mmap(NULL, *sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        display_error("Failed to map file: %s %s", name, strerror(errno));
        close(fd);
        return NULL;
    }

    // Close file descriptor (no longer needed after mmap)
    close(fd);

    return mapped;
}

// Function for unmapping
void unmap_file(void *ptr, size_t sz) {
    if (!ptr || sz == 0) {
        return;
    }

    // Unmap memory
    if (munmap(ptr, sz) == -1) {
        perror("Error during unmapping");
    }
}
