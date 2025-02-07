#include <platform/platform.h>
#include <libs/event.h>
#include "globals.h"
#include "lang.h"
#include <libs/strlite.h>


static char *lang_folder = NULL;

static void free_lang_folder(void) {
    free(lang_folder);
}

const char *lang_get_folder(void) {
    return lang_folder;
}

void lang_set_folder(const char *path) {
    if (lang_folder == NULL) atexit(free_lang_folder);
    lang_folder = strdup(path);
}
void lang_patch_stringtable(TSTR_LIST *lst, const char *object_name, const char *prefix) {
    if (lang_folder == NULL) return;
    const char *fname = set_file_extension(object_name, ".csv");
    fname = concat2(prefix, fname);
    const char *path = build_pathname(2, lang_folder, fname);
    TSTRINGTABLE *st =  stringtable_load(path);
    if (!st) return;
    for (int i = 0, cnt = str_count(*lst); i<cnt; ++i) {
        const char *newstr = stringtable_find(st, i, NULL);
        if (newstr) str_replace(lst, i, newstr);
    }
    stringtable_free(st);

}
TSTRINGTABLE *lang_load(const char *object_name) {
    if (lang_folder == NULL) return NULL;
    const char *fname = set_file_extension(object_name, ".csv");
    const char *path = build_pathname(2, lang_folder, fname);
    TSTRINGTABLE *st =  stringtable_load(path);
    return st;
}

const char *lang_replace_path_if_exists(const char *file) {
    if (lang_folder == NULL) return NULL;
    const char *path = build_pathname(2, lang_folder, file);
    if (check_file_exists(path)) return path;
    return NULL;
}
