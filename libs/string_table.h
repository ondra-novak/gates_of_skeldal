#pragma once

#ifdef __cplusplus
extern "C" {
#endif


typedef struct stringtable_struct_tag TSTRINGTABLE;

TSTRINGTABLE *stringtable_load(const char *filename);

const char *stringtable_find(const TSTRINGTABLE *st, int id, const char *default_value);
void stringtable_free(TSTRINGTABLE *st);


#ifdef __cplusplus
}
#endif
