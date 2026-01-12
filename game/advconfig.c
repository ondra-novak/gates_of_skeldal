#include <platform/platform.h>
#include "advconfig.h"

#include <ctype.h>

#include <stdlib.h>
#include <string.h>


static void process_row(INI_CONFIG_SECTION *sec, const char *row) {

    int l = strlen(row);
    char *buff = strcpy((char *)alloca(l+1), row);
    while (l && isspace(buff[l-1])) {
        --l;
        buff[l] = 0;
    }
    if (!l) return;

    char *sep = strchr(buff,' ');
    if (sep == NULL) return;
    *sep = 0;

    const char *key = buff;
    const char *value = sep+1;



    if (istrcmp(key,"CESTA_GRAFIKA") == 0) {
        ini_replace_key(sec, "graphics", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_ZVUKY") == 0) {
        ini_replace_key(sec, "sounds", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_FONTY") == 0) {
        ini_replace_key(sec, "fonts", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_MAPY") == 0) {
        ini_replace_key(sec, "maps", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_MUSIC") == 0) {
        ini_replace_key(sec, "music_adv", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_BGRAFIKA") == 0) {
        ini_replace_key(sec, "gui", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_ITEMY") == 0) {
        ini_replace_key(sec, "items", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_ENEMY") == 0) {
        ini_replace_key(sec, "enemy", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_VIDEO") == 0) {
        ini_replace_key(sec, "video", file_icase_find(value));
    } else if (istrcmp(key, "CESTA_DIALOGY") == 0) {
        ini_replace_key(sec, "dialogs", file_icase_find(value));
    } else if (istrcmp(key, "DEFAULT_MAP") == 0) {
        ini_replace_key(sec, "default_map", value);
    } else if (istrcmp(key, "PATCH_FILE") == 0) {
        ini_replace_key(sec, "patch_file", file_icase_find(value));
    } else if (istrcmp(key, "PATCH") == 0 && istrcmp(value,"1") == 0) {
        ini_replace_key(sec, "patch_mode", "1");
    }
}



void adv_patch_config(INI_CONFIG *srcconfig,  TSTR_LIST lst) {
    INI_CONFIG_SECTION *sect = ini_create_section(srcconfig, "paths");
    int cnt = str_count(lst);
    for (int i = 0; i < cnt; ++i) {
        const char *row = lst[i];
        if (row) {
            process_row(sect, row);
        }
    }
}


