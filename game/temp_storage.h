#include <stdint.h>

///store to temp storage
void temp_storage_store(const char *name, const void *data, int32_t size);
///find in temp storage - returns -1 = not found, otherwise size of data
int32_t temp_storage_find(const char *name);
///retrieve from temp storage (returns same as find)
int32_t temp_storage_retrieve(const char *name, void *data, int32_t size);

void temp_storage_list(void (*callback)(const char *, void *), void *context);

void temp_storage_clear();

typedef struct _temp_storage_file_rd TMPFILE_RD;
typedef struct _temp_storage_file_wr TMPFILE_WR;

TMPFILE_RD *temp_storage_open(const char *name);
TMPFILE_WR *temp_storage_create(const char *name);
TMPFILE_WR *temp_storage_append(const char *name);
void temp_storage_delete(const char *name);
void temp_storage_close_rd(TMPFILE_RD *f);
void temp_storage_close_wr(TMPFILE_WR *f);
int temp_storage_getc(TMPFILE_RD *f);
void temp_storage_ungetc(TMPFILE_RD *f);
void temp_storage_write(const void *data, uint32_t size, TMPFILE_WR *f);
uint32_t temp_storage_read(void *data, uint32_t size, TMPFILE_RD *f);
void temp_storage_skip(TMPFILE_RD *f, int bytes);

int *temp_storage_internal_skip_ptr(TMPFILE_RD *f);
void temp_storage_internal_begin_scanf(TMPFILE_RD *f, const char *format, ... );
int temp_storage_internal_end_scanf(TMPFILE_RD *f);


#define temp_storage_scanf(f, format, ...) (temp_storage_internal_begin_scanf(f, format "%n", __VA_ARGS__, temp_storage_internal_skip_ptr(f)),temp_storage_internal_end_scanf(f))



