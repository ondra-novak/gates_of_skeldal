#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *map_file_to_memory(const char *name, size_t *sz);
void unmap_file(void *ptr, size_t sz);

#ifdef __cplusplus
}
#endif
