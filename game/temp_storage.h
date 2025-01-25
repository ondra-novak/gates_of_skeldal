

///store to temp storage
void temp_storage_store(const char *name, const void *data, long size);
///find in temp storage - returns -1 = not found, otherwise size of data
long temp_storage_find(const char *name);
///retrieve from temp storage (returns same as find)
long temp_storage_retrieve(const char *name, void *data, long size);

void temp_storage_list(void (*callback)(const char *, void *), void *context);

void temp_storage_clear();

typedef struct _temp_storage_file_rd TMPFILE_RD;
typedef struct _temp_storage_file_wr TMPFILE_WR;

TMPFILE_RD *temp_storage_open(const char *name);
TMPFILE_WR *temp_storage_create(const char *name);
TMPFILE_WR *temp_storage_append(const char *name);
void temp_storage_close_rd(TMPFILE_RD *f);
void temp_storage_close_wr(TMPFILE_WR *f);
void temp_storage_write(const void *data, unsigned long size, TMPFILE_WR *f);
unsigned long temp_storage_read(void *data, unsigned long size, TMPFILE_RD *f);
void temp_storage_skip(TMPFILE_RD *f, int bytes);


