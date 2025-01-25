#include <skeldal_win.h>
#include "strlite.h"
#include <stdlib.h>
#include <stdio.h>
#include <mem.h>
#include <malloc.h>
#include <dos.h>
#include "types.h"
#include "memman.h"

TSTR_LIST create_list(int count)
  {
  register TSTR_LIST p;int i,j;

  p=(TSTR_LIST)malloc(count*sizeof(*p));
  if (p==NULL) return NULL;
  j=_msize(p)/sizeof(*p);
  for(i=0;i<j;i++) p[i]=NULL;
  return p;
  }

TSTR_LIST find_ptr(TSTR_LIST source,void *_ptr,int _size)
  {
  __asm
    {
    mov edi, source
    mov eax, _ptr
    mov ecx, _size

    cld
    repnz scasd
    jnz  skok
    sub  edi,4
    skok:
    mov eax, edi
    }
  }
    //parm [edi][eax][ecx] value[edi];

const char *str_replace(TSTR_LIST *list,int line,const char *text)
  {
  int count,i,j;
  TSTR_LIST p;
  char *c;

  count=str_count(*list);
  if (line>=count)
     {
     int plus;

        plus=count-line;
        plus=(plus/STR_REALLOC_STEP+1)*STR_REALLOC_STEP;
        p=getmem((count+plus)*sizeof(*p));
        memcpy(p,*list,count*sizeof(*p));
        free(*list);
        j=_msize(p)/sizeof(*p);
        i=count;
        for(;i<j;i++) p[i]=NULL;
        i=count;count=j;
        *list=p;
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
  TSTR_LIST p;

  count=str_count(*list);
  p=find_ptr(*list,NULL,count);
  i=p-*list;
  str_replace(list,i,text);
  return i;
  }

const char *str_insline(TSTR_LIST *list,int before,const char *text)
  {
  int i,count,punkt;
  TSTR_LIST p;

  count=str_count(*list);
  p=find_ptr(*list,NULL,count);
  punkt=p-*list;
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
  TSTR_LIST p;

  count=_msize(*list)/sizeof(*p);
  j=0;
  for(i=0;i<count;i++)
     if ((*list)[i]!=NULL) (*list)[j++]=(*list)[i];
  if (j==0) j++;
  p=(TSTR_LIST)realloc(*list,j*sizeof(*p));
  if (p!=NULL) *list=p;
  count=_msize(*list)/sizeof(*p);
  for(i=j;i<count;i++) (*list)[i]=NULL;
  }

int str_count(TSTR_LIST p)
  {
  int count;

  if (p==NULL) return 0;
  count=_msize(p)/sizeof(*p);
  return count;
  }

void release_list(TSTR_LIST list)
  {
  int i,j;

  if (list==NULL) return;
  j=str_count(list);
  for(i=0;i<j;i++)
     str_remove(&list, i);
  free(list);
  }

typedef struct tuzel
  {
  char *data;
  struct tuzel *levy,*pravy,*spatky;
  }
  TUZEL;


int sort_add_to_tree(TUZEL *uzel,const char *text, int dir)
  {
  TUZEL *q;
  if (uzel->data==NULL)
     {
     uzel->data=text;
     return 0;
     }
  q=(TUZEL *)getmem(sizeof(TUZEL));
  if (q==NULL) return -1;
  q->data=text;
  q->levy=NULL;q->pravy=NULL;
  while (uzel!=NULL)
     if (strcmp(text,uzel->data)==dir)
        {
        if (uzel->levy==NULL)
           {
           uzel->levy=q;
           q->spatky=uzel;
           uzel=NULL;
           }
        else
           uzel=uzel->levy;
        }
     else
        {
        if (uzel->pravy==NULL)
           {
           uzel->pravy=q;
           q->spatky=uzel;
           uzel=NULL;
           }
        else
           uzel=uzel->pravy;
        }
  return 0;
  }

void sort_read_list(TUZEL *uzel,TSTR_LIST list)
  {
  int counter=0;
  TUZEL *ptr,*last;
  int c;

  if (uzel->data==NULL) return;
  last=NULL;
  while (uzel!=NULL)
     {
     if (last==NULL)
        {
        ptr=uzel;
        uzel=uzel->levy;
        last=NULL;
        c=1;
        }
     else if (last==uzel->levy || (int)last==1)
        {
        ptr=uzel;
        list[counter++]=uzel->data;
        uzel=uzel->pravy;
        last=NULL;
        c=2;
        }
     else if (last==uzel->pravy || (int)last==2)
        {
        last=uzel;
        uzel=uzel->spatky;
        continue;
        }
     if (uzel==NULL)
        {
        last=(TUZEL *)c;
        uzel=ptr;
        }
     }
  }
void sort_release_tree(TUZEL *uzel)
  {
  TUZEL *ptr,*last;
  int c;

  if (uzel->data==NULL) return;
  last=NULL;
  while (uzel!=NULL)
     {
     if (last==NULL)
        {
        ptr=uzel;
        uzel=uzel->levy;
        last=NULL;
        c=1;
        }
     else if (last==uzel->levy || (int)last==1)
        {
        ptr=uzel;
        uzel=uzel->pravy;
        last=NULL;
        c=2;
        }
     else if (last==uzel->pravy || (int)last==2)
        {
        last=uzel;
        uzel=uzel->spatky;
        if (last->spatky!=NULL) free(last);
        continue;
        }
     if (uzel==NULL)
        {
        last=(TUZEL *)c;
        uzel=ptr;
        }
     }
  }
TSTR_LIST sort_list(TSTR_LIST list,int direction)
  {
  TUZEL uz;
  int i,j;

  uz.data=NULL;uz.levy=NULL;uz.pravy=NULL;
  uz.spatky=NULL;
  j=str_count(list);
  for(i=0;i<j;i++)
     if (list[i]!=NULL) if (sort_add_to_tree(&uz,list[i],direction))
                             {
                             sort_release_tree(&uz);
                             return NULL;
                             }
  sort_read_list(&uz,list);
  sort_release_tree(&uz);
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

  f=fopen(filename,"r");
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