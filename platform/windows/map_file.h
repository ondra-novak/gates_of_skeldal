#include <stddef.h>
#include <stdint.h>
const void *map_file_to_memory(const char *name, size_t *sz);
void unmap_file(const void *ptr, size_t sz);
