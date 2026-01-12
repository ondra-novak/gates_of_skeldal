#include <platform/platform.h>
#include "strlite.h"
#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "memman.h"

#include <string.h>
TSTR_LIST create_list(int count)
  {
  TSTR_LIST p;
  int i;

  size_t *s=(size_t *)malloc(count*sizeof(*p)+sizeof(size_t));
  if (s==NULL) return NULL;
  *s = count;
  p = (TSTR_LIST)(s+1);
  for(i=0;i<count;i++) p[i]=NULL;
  return p;
  }

int find_ptr(TSTR_LIST source,void *_ptr,int _size)
  {
   for (int i = 0; i < _size; ++i)
       if (source[i] ==_ptr) return i;
   return _size;
  }

const char *str_replace(TSTR_LIST *list,int line,const char *text)
  {
  int count;
  char *c;

  count=str_count(*list);
  if (line>=count)
     {
     int new_count = count * 2;
     if (new_count <= line) new_count = line+1;
     TSTR_LIST new_list = create_list(new_count);
     for (int i = 0; i < count; ++i) {
         new_list[i] = (*list)[i];
         (*list)[i] = NULL;
     }
     release_list(*list);
     *list = new_list;
     }
  if ((*list)[line]!=NULL) free((*list)[line]);
  if (text!=NULL)
     {
     c=(char *)getmem(strlen(text)+1);
     if (c==NULL) return NULL;
     strcpy(c,text);
     }
  else
     c=NULL;
  (*list)[line]=c;
  return c;
  }

int str_add(TSTR_LIST *list,const char *text)
  {
  int count,i;


  count=str_count(*list);
  i=find_ptr(*list,NULL,count);
  str_replace(list,i,text);
  return i;
  }

int str_move_list(TSTR_LIST to, TSTR_LIST from) {
  int cnt1 = str_count(from);
  int cnt2 = str_count(to);
  int cnt = MIN(cnt1,cnt2);
  for (int i = 0; i < cnt; ++i) {
    if (to[i]) free(to[i]);
    to[i] = from[i];
    from[i] = NULL;
  }
  return cnt;
}

const char *str_insline(TSTR_LIST *list,int before,const char *text)
  {
  int i,count,punkt;


  count=str_count(*list);
  punkt=find_ptr(*list,NULL,count);
  str_replace(list,punkt,NULL);
  for(i=punkt;i>before;i--) (*list)[i]=(*list)[i-1];
  (*list)[before]=NULL;
  return str_replace(list,before,text);
  }

void str_remove(TSTR_LIST *list,int line)
  {
  str_replace(list,line,NULL);
  }

void str_delfreelines(TSTR_LIST *list)
  {
  int count,i,j;

  count=str_count(*list);
  j=0;
  for(i=0;i<count;i++)
     if ((*list)[i]!=NULL) (*list)[j++]=(*list)[i];
  for(i=j;i<count;i++) (*list)[i]=NULL;
  }

int str_count(TSTR_LIST p)
  {


  if (p==NULL) return 0;
  return *((size_t *)p-1);
  }

void release_list(TSTR_LIST list)
  {
  int i,j;

  if (list==NULL) return;
  j=str_count(list);
  for(i=0;i<j;i++)
     str_remove(&list, i);
  size_t *s = (size_t *)list-1;
  free(s);
  }



static int cmp_list_forward(const void *pa, const void *pb) {
    return strcmp(*(char *const *)pa, *(char *const *)pb);
}
static int cmp_list_backward(const void *pa, const void *pb) {
    return -strcmp(*(char *const *)pa, *(char * const *)pb);
}

TSTR_LIST sort_list(TSTR_LIST list,int direction)
  {
    if (direction > 0) {
        qsort(list, str_count(list), sizeof(char *), cmp_list_forward);
    } else if (direction < 0) {
        qsort(list, str_count(list), sizeof(char *), cmp_list_backward);
    }
  return list;
  }

void pl_add_data(PTRMAP **p,void *data,int datasize)
  {
  PTRMAP *q;

  q=(PTRMAP *)getmem(sizeof(PTRMAP));
  q->data=(void *)getmem(datasize);
  memcpy(q->data,data,datasize);
  q->next=*p;
  *p=q;
  }

void pl_search(PTRMAP *p,void *key,int keysize,PTRMAP **find,PTRMAP **last)
  {
  *find=p;
  *last=NULL;
  while (*find!=NULL && memcmp((*find)->data,key,keysize)!=0)
     {
     *last=*find;
     *find=(*find)->next;
     }
  }

void *pl_get_data(PTRMAP **p,void *key,int keysize)
  {
  PTRMAP *find, *last;

  pl_search(*p,key,keysize,&find,&last);
  if (find!=NULL) return find->data;
  return NULL;
  }

PTRMAP *pl_find_item(PTRMAP **p,void *key,int keysize)
  {
  PTRMAP *find,*last;

  pl_search(*p,key,keysize,&find,&last);
  return find;
  }

void pl_delete_item(PTRMAP **p,void *key,int keysize)
  {
  PTRMAP *find,*last,*q;

  pl_search(*p,key,keysize,&find,&last);
  q=find;
  if (q==NULL) return;
  if (last==NULL) *p=find->next ;else last->next=find->next;
  if (q->data!=NULL) free(q->data);
  free(q);
  }

void pl_delete_all(PTRMAP **p)
  {
  PTRMAP *q;

  while (*p!=NULL)
     {
     q=*p;
     *p=q->next;
     if (q->data!=NULL) free(q->data);
     free(q);
     }
  }


int load_string_list(TSTR_LIST *list,const char *filename)
  {
  char c[1024],*p;
  int i,j,lin=0;
  FILE *f;

  f=fopen_icase(filename,"r");
  if (*list==NULL) *list=create_list(256);
  if (f==NULL) return -1;
  do
     {
     lin++;
       do
        {
        j=fgetc(f);
        if (j==';') while ((j=fgetc(f))!='\n' && j!=EOF);
        if (j=='\n') lin++;
        }
       while (j=='\n');
      ungetc(j,f);
     j=fscanf(f,"%d",&i);
     if (j==EOF)
        {
        fclose(f);
        return -2;
        }
     if (j!=1)
        {
        fclose(f);
        return lin;
        }
     if (i==-1) break;
     while ((j=fgetc(f))<33 && j!=EOF);
     if (j!=EOF) ungetc(j,f);
     if (fgets(c,1022,f)==NULL)
        {
        fclose(f);
        return lin;
        }
     p=strchr(c,'\n');if (p!=NULL) *p=0;
     for(p=c;*p;p++) *p=*p=='|'?'\n':*p;
     if (str_replace(list,i,c)==NULL)
        {
        fclose(f);
        return -3;
        }
     }
  while (1);
  fclose(f);
  return 0;
  }

void strlist_cat(TSTR_LIST *org, TSTR_LIST add)
  {
  int cnt=str_count(add);
  int i;
  for (i=0;i<cnt;i++) str_add(org,add[i]);
  }
