#include <platform/platform.h>

#include <stddef.h>
#include <stdint.h>
#include <libs/event.h>
#include <libs/strlite.h>
#include "globals.h"
#include <libs/cztable.h>



static void csv_output(FILE *f, const char *txt) {
    while (*txt) {
        if (*txt == '"') {
            fputc('"',f);
            fputc('"',f);
        } else {
            fputc(kamenik2windows_chr(*txt),f);
        }
        ++txt;
    }
}

static char store_to_csv(TSTR_LIST lst, const char *target_path)  {
    int cnt = str_count(lst);
    char to_store = 0;
    for (int i = 0; i < cnt; ++i) if (lst[i]) to_store = 1;
    if (!to_store) {
        fprintf(stdout, "Empty string table not stored: %s\n", target_path);
        return 1;
    }
    FILE *out = fopen_icase(target_path, "w");
    if (!out) {
        fprintf(stderr, "Failed to open %s for writing\n",target_path);
        return 0;
    }
    printf("Writing file: %s\n", target_path);
    fprintf(out, "id,string\n");
    for (int i = 0; i < cnt; ++i) {
        if (lst[i]) {
            fprintf(out, "%d,\"",i);
            csv_output(out, lst[i]);
            fprintf(out, "\"\n");
        }
    }
    fclose(out);
    return 1;
}

static int find_mapedit(TSTR_LIST lst) {
    for (int i = 0; i < str_count(lst); ++i) {
        if (lst[i] && imatch(lst[i], "TOTO JE MAGIC SLOUP MAPEDIT")) return i;
    }
    return 0;
}

void cti_texty(void);

static int convert_map_strings_1(const char *source_name, LIST_FILE_TYPE type, size_t sz, void *context) {
    int l = strlen(source_name);
    if (istrcmp(source_name+l-4,".map")) return 0;
    source_name = set_file_extension(source_name, ".txt");
    const char *target_name = set_file_extension(concat2("map_", source_name),".csv");
    const char *target_path = *(const char **)context;
    TSTR_LIST lst = create_list(100);
    int err = load_string_list_ex(&lst, build_pathname(2, gpathtable[SR_MAP], source_name));
    if (err) {
        release_list(lst);
        fprintf(stderr,"Failed to read: %s, error code: %d\n",source_name, err);
        return 1;
    }
    str_replace(&lst, 0, NULL);
    int toremove = find_mapedit(lst);
    if (toremove) {
        str_replace(&lst, toremove, NULL);
    }
    err = store_to_csv(lst, build_pathname(2, target_path, target_name));
    release_list(lst);
    return !err;
    ;

}

static char convert_map_strings(const char *target_path) {
    return !list_files(build_pathname(1, gpathtable[SR_MAP]), file_type_normal|file_type_just_name,convert_map_strings_1 , &target_path);
}


static char convert_file_to(const char *src_file, const char *target_file) {
    TMPFILE_RD *rd = enc_open(src_file);
    size_t sz = temp_storage_find("__enc_temp");
    char *buff = malloc(sz);
    temp_storage_retrieve("__enc_temp", buff, sz);
    FILE *out = fopen_icase(target_file, "w");
    if (!out) {
        fprintf(stderr, "Failed to open %s for writing\n",target_file);
        return 0;
    }
    printf("Writing %s\n", target_file);
    fwrite(buff,1,sz,out);
    fclose(out);
    enc_close(rd);
    return 1;
}

static char convert_book(const char *target_path) {
    const char *path = build_pathname(2, target_path, "book.txt");
    path = local_strdup(path);
    return convert_file_to(build_pathname(2, gpathtable[SR_MAP], "kniha.txt"), path);
}

static char convert_end_titles(const char *target_path) {
    const char *path = build_pathname(2, target_path, "end_titles.txt");
    path = local_strdup(path);

    return convert_file_to(build_pathname(2, gpathtable[SR_DATA], "titulky.txt"), path);
}

static char convert_epilog(const char *target_path) {
    const char *path = build_pathname(2, target_path, "epilog.txt");
    path = local_strdup(path);
    return convert_file_to(build_pathname(2, gpathtable[SR_DATA], "endtext.txt"), path);
}
static char convert_intro_titles(const char *target_path) {
    TSTR_LIST lst = create_list(100);
    int err = load_string_list_ex(&lst, build_pathname(2, gpathtable[SR_VIDEO], "intro.txt"));
    if (err) {
        release_list(lst);
        fprintf(stderr,"Failed to read: %s, error code: %d\n","intro.txt", err);
        return 1;
    }
    err = store_to_csv(lst, build_pathname(2, target_path, "intro.csv"));
    release_list(lst);
    return err;
}

char generate_items_strings(const char *path) {
    load_items();
    TSTR_LIST lst = create_list(item_count);
    for (int i = 0; i < item_count;++i) {
        str_replace(&lst, i, concat3(glob_items[i].jmeno,"\n",glob_items[i].popis));
    }
    int r = store_to_csv(lst, build_pathname(2,path,"items.csv"));
    release_list(lst);
    return r;
}
char generate_spell_strings(const char *path) {
    TSTR_LIST lst = create_list(255);
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 7; j++) {
            int idx = (i * 7 + j)*3;
            str_replace(&lst, idx, get_rune_name(idx));
        }

    int r = store_to_csv(lst, build_pathname(2,path,"spells.csv"));
    release_list(lst);
    return r;
}

extern TSHOP **shop_list;
extern int max_shops;


char generate_shop_strings(const char *path) {
    TSTR_LIST lst = create_list(255);
    load_shops();
    for (int i = 0; i< max_shops; ++i) {
        str_replace(&lst, shop_list[i]->shop_id, shop_list[i]->keeper);
    }
    int r = store_to_csv(lst, build_pathname(2,path,"shops.csv"));
    release_list(lst);
    return r;
}

char generate_dialog_append_pgf(TSTR_LIST *lst,const char *beg, const char *pc, const char *end) {

    while (pc < end) {
        char type = *pc;
        ++pc;
        if (type == 1) {
            str_replace(lst, pc - beg, pc);
            pc += strlen(pc)+1;
        } else {
            pc+=2;
        }
    }
    return 1;

}

char generate_dialog_table(const char *path) {
    const int32_t *data = (const int32_t *)ablock(H_DIALOGY_DAT);
    size_t total_sz = get_handle_size(H_DIALOGY_DAT);
    const char *beg = (const char *)data;

    size_t count = data[0];
    TSTR_LIST lst = create_list(256);
    data+=2;
    for (size_t i = 0; i < count; i++) {
        int pos = data[1] + 8*(count+1);
        int end = i + 1 == count? total_sz : data[3] + 8*(count+1);

        generate_dialog_append_pgf(&lst, beg, beg+pos, beg+end);

        data+=2;
    }
    int r = store_to_csv(lst, build_pathname(2,path,"dialogs.csv"));
    release_list(lst);
    return r;
}



char generate_string_tables(const char *path) {
    create_directories(path);
    cti_texty();
    if (!store_to_csv(texty, build_pathname(2, path, "ui.csv"))) return 0;
    if (!convert_map_strings(path)) return 0;
    if (!convert_book(path)) return 0;
    if (!convert_end_titles(path)) return 0;
    if (!convert_epilog(path)) return 0;
    if (!generate_items_strings(path)) return 0;
    if (!generate_spell_strings(path)) return 0;
    if (!generate_dialog_table(path)) return 0;
    if (!convert_intro_titles(path)) return 0;
    if (!generate_shop_strings(path)) return 0;
    return 1;
}


