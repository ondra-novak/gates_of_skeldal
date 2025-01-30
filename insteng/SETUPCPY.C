#include <stdio.h>
#include <stdlib.h>
#include <libs/memman.h>
#include <string.h>
#include "setupcpy.h"

#define DATASIZE 32768
#define INFOBLOCK 1
#define DATABLOCK 2
#define ERRORBLOCK 3

typedef struct tdatablock
  {
  void *next;
  char block_type;
  unsigned short load_size;
  char data[DATASIZE];
  }TDATABLOCK;

typedef struct tinfoblock
  {
  void *next;
  char block_type;
  char pathname[2];
  }TINFOBLOCK;

static void *start=NULL;
static void *end=NULL;
static FILE *cpyout=NULL;
static long progress=0;

void (*cpy_error)(int,char *);
void (*mem_error_next)(long);

void (*cpy_progress)(long);


static void *alloc_file(char *target_name)
  {
  TINFOBLOCK *p;

  p=(TINFOBLOCK *)getmem(sizeof(TINFOBLOCK)+strlen(target_name));
  strcpy(p->pathname,target_name);
  p->next=NULL;
  p->block_type=INFOBLOCK;
  return p;
  }

static void *alloc_data()
  {
  TDATABLOCK *p;

  p=New(TDATABLOCK);
  p->next=NULL;
  p->load_size=0;
  p->block_type=DATABLOCK;
  return p;
  }


static void *load_data(FILE *f)
  {
  TDATABLOCK *p;
  int rc;
  int retry=0;
  void *end_line;

  p=end_line=alloc_data();
  again:
  errno=0;
  rc=fread(p->data,1,sizeof(p->data),f);
  p->load_size=rc;
  if (ferror(f) || rc!=sizeof(p->data) && errno!=0)
     {
     if (retry) retry--;
     else  cpy_error(CPERR_READ,NULL);
     fseek(f,-rc,SEEK_CUR);
     errno=0;
     goto again;
     }
  progress+=p->load_size;
  cpy_progress(progress);
  return end_line;
  }

static error_mem(long size)
  {
  TDATABLOCK *p;
  TINFOBLOCK *q;
  int rc;

  size;
  if (start==NULL) mem_error_next(size);
  while (start!=NULL)
     {
     p=start;
     q=start;
     if (q->block_type==INFOBLOCK)
        {
        if (cpyout!=NULL) fclose(cpyout);
        again:
        cpyout=fopen(q->pathname,"wb");
        if (cpyout==NULL)
           {
           cpy_error(CPERR_OPEN,q->pathname);
           goto again;
           }
        start=q->next;
        free(q);
        }
     else if (p->block_type==DATABLOCK)
        {
        again2:
        rc=fwrite(p->data,1,p->load_size,cpyout);
        if (rc!=p->load_size)
           {
           cpy_error(CPERR_WRITE,q->pathname);
           fseek(cpyout,-rc,SEEK_CUR);
           goto again2;
           }
        progress+=p->load_size;
        cpy_progress(progress);
        start=p->next;
        free(p);
        }
     }
   end=NULL;
  }

void cpy_file(char *source,char *target)
  {
  FILE *f;
  TDATABLOCK *p;
  TINFOBLOCK *q;

  again:
  errno=0;
  f=fopen(source,"rb");
  if (f==NULL)
     {
     cpy_error(CPERR_OPEN,source);
     goto again;
     }
  q=alloc_file(target);
  if (start==NULL) start=end=q;else end=(((TDATABLOCK *)end)->next=q);
  while (!feof(f))
     {
     p=load_data(f);
     if (start==NULL) start=end=p;else end=(((TDATABLOCK *)end)->next=p);
     }
  fclose(f);
  }

void cpy_flush()
  {
  if (start!=NULL)error_mem(0);
  if (cpyout!=NULL)fclose(cpyout);
  cpyout=NULL;
  }

void install_cpy()
  {
  mem_error_next=mem_error;
  mem_error=error_mem;
  }
