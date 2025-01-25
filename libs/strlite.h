#ifndef _STRLITE_H_
#define _STRLITE_H_
typedef char **TSTR_LIST;

typedef struct ptrmap
  {
  struct ptrmap *next;
  void *data;
  }PTRMAP;

#define STR_REALLOC_STEP 256


TSTR_LIST create_list(int count);
int str_add(TSTR_LIST *list,const char *text);
const char *str_insline(TSTR_LIST *list,int before,const char *text);
const char *str_replace(TSTR_LIST *list,int line,const char *text);
void str_remove(TSTR_LIST *list,int line);
void str_delfreelines(TSTR_LIST *list);
int str_count(TSTR_LIST p);
void release_list(TSTR_LIST list);
TSTR_LIST sort_list(TSTR_LIST list,int direction);
TSTR_LIST read_directory(const char *mask,int view_type,int attrs);
//void name_conv(const char *c);
void strlist_cat(TSTR_LIST *org, TSTR_LIST add);

void pl_add_data(PTRMAP **p,void *data,int datasize);
void *pl_get_data(PTRMAP **p,void *key,int keysize);
PTRMAP *pl_find_item(PTRMAP **p,void *key,int keysize);
void pl_delete_item(PTRMAP **p,void *key,int keysize);
void pl_delete_all(PTRMAP **p);

int load_string_list(TSTR_LIST *list,const char *filename);

#endif
