#pragma once


#include <libs/strlite.h>
#include <libs/string_table.h>

void lang_set_folder(const char *path);
const char *lang_get_folder(void);
void lang_patch_stringtable(TSTR_LIST *lst, const char *object_name, const char *prefix);
const char *lang_replace_path_if_exists(const char *file);
TSTRINGTABLE *lang_load(const char *object_name);
char *lang_load_string(const char *filename);

