#include <platform.h>
#include "advconfig.h"


#include <alloca.h>
#include <string.h>


static void process_row(INI_CONFIG_SECTION *sec, const char *row) {

    char *buff = strcpy((char *)alloca(strlen(row)+1), row);
    char *sep = strchr(buff,' ');
    if (sep == NULL) return;
    *sep = 0;

    const char *key = buff;
    const char *value = sep+1;


    if (stricmp(key,"CESTA_GRAFIKA") == 0) {
        ini_replace_key(sec, "graphics", value);
    } else if (stricmp(key, "CESTA_ZVUKY") == 0) {
        ini_replace_key(sec, "sounds", value);
    } else if (stricmp(key, "CESTA_FONTY") == 0) {
        ini_replace_key(sec, "fonts", value);
    } else if (stricmp(key, "CESTA_MAPY") == 0) {
        ini_replace_key(sec, "maps", value);
    } else if (stricmp(key, "CESTA_MUSIC") == 0) {
        ini_replace_key(sec, "music_adv", value);
    } else if (stricmp(key, "CESTA_BGRAFIKA") == 0) {
        ini_replace_key(sec, "gui", value);
    } else if (stricmp(key, "CESTA_ITEMY") == 0) {
        ini_replace_key(sec, "items", value);
    } else if (stricmp(key, "CESTA_ENEMY") == 0) {
        ini_replace_key(sec, "enemy", value);
    } else if (stricmp(key, "CESTA_VIDEO") == 0) {
        ini_replace_key(sec, "video", value);
    } else if (stricmp(key, "CESTA_DIALOGY") == 0) {
        ini_replace_key(sec, "dialogs", value);
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


