#include <platform/platform.h>
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include "memman.h"
#include <time.h>
//#include <i86.h>
#include "swaper.c"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>


#define DPMI_INT 0x31
#define LOAD_BUFFER 4096

#undef malloc

void bonz_table();

#define NON_GETMEM_RESERVED (4*1024)
static const char **  mman_pathlist=NULL;


char mman_patch=0;

static int max_handle=0;
//static FILE *log;

void (*mman_action)(int action)=NULL;

int32_t last_load_size;

void def_mman_group_table(const char ** p) {
    mman_pathlist = p;
}

void standard_mem_error(size_t size)
  {
  display_error("memory allocation error %lu", (unsigned long)size);
  exit(1);
  }

void load_error(const char *filename)
  {
  char buff[256];
  SEND_LOG("(ERROR) Load error detected, system can't load file: %s",filename);
  sprintf(buff,"Load error while loading file: %s", filename);
  display_error(buff);
  exit(1);
  }

void standard_swap_error()
  {
  char buff[256];
  sprintf(buff,"Swap error. Maybe disk is full");
  display_error(buff);
  exit(1);
  }


void (*mem_error)(size_t)=standard_mem_error;
void (*swap_error)()=standard_swap_error;

void *getmem(int32_t size)
  {
  void *p,*res;

  if (!size) return NULL;
  do
     {
     res=malloc(NON_GETMEM_RESERVED);
     if (res!=NULL)
        {
        p=(void *)malloc(size);
        free(res);
        }
     else p=NULL;
     if (p==NULL) mem_error(size);
     }
  while (p==NULL);
//  SEND_LOG("(ALLOC) **** Alloc: %p size %d",p,*((int32_t *)p-1));
  return p;
  }


void *load_file(const char *filename, size_t *sz)
  {
  FILE *f;
  int32_t *p;
  size_t size;

  if (mman_action!=NULL) mman_action(MMA_READ);
  SEND_LOG("(LOAD) Loading file '%s'",filename);
  f=fopen_icase(filename, "rb");
  if (f==NULL) {
      load_error(filename);
      return NULL;
  }
  fseek(f,0, SEEK_END);
  size=ftell(f);
  fseek(f,0, SEEK_SET);
  p=(void *)getmem(size);
  if (fread(p,1,size,f)!=size) load_error(filename);
  fclose(f);
  *sz=size;
  return p;
  }

//--------------- BLOCK MASTER OPERATION SYSTEM --------------------------
//--------------- part: MEMORY MANAGER -----------------------------------
typedef struct tnametable
        {
        char name[12];
        int32_t seek;
        }TNAMETABLE;

typedef struct tnametable_ref {
    const TNAMETABLE *data;
    uint32_t count;
} TNAMETABLE_REF;

typedef struct ddlmap_info {
    const void *ptr;
    size_t size;
    TNAMETABLE_REF nametable;
} TDDLMAP_INFO;

#define MAX_PATCHES 4

static TDDLMAP_INFO ddlmap[MAX_PATCHES];


static int next_name_read=0;
static int last_group;

handle_groups _handles;
uint32_t bk_global_counter=0;
char *swap_path;


static int test_file_exist_DOS(int group,char *filename)
  {
     const char *f = build_pathname(2, mman_pathlist[group], filename);
     if (!check_file_exists(f)) return 0;
     return 1;
  }




#if 0
void load_grp_table()
  {
  int32_t i = 0;;

  SEND_LOG("(LOAD) Loading Group Table");
  const uint32_t *src_table = (const uint32_t *)bmf_m;
                   //fseek(bmf,4,SEEK_SET);
  i = src_table[1];//fread(&i,4,1,bmf);
  grptable=(int32_t *)getmem(i+4);
                  //fseek(bmf,0,SEEK_SET);
  grptabsiz = src_table[0];
                 //fread(grptable,i,1,bmf);
  grptabsiz=i;
  for(i=0;i<(grptabsiz>>3);i++) grptable[i*2+1]=(grptable[i*2+1]-grptabsiz)>>4;
  SEND_LOG("(LOAD) Group Table Loaded");
  }
#endif

static TNAMETABLE_REF load_file_table(const void *bmf_m)
  {
  const uint32_t *src_table = (const uint32_t *)bmf_m;
  uint32_t grptabsiz = src_table[1];
  TNAMETABLE_REF out;
  out.data = (const TNAMETABLE *)((const char *)bmf_m + grptabsiz);
  out.count = (out.data[0].seek - grptabsiz)/sizeof(TNAMETABLE);
  return out;
  }


int get_file_entry_in_table(const TNAMETABLE_REF *where, char *name) {
  for(uint32_t i = 0; i< where->count; ++i) {
      if (strncmp(where->data[i].name, name, 12) == 0) {
          return where->data[i].seek;
      }
  }
  return -1;

}



char get_file_entry(int group,char *name, THANDLE_DATA *h) {
  char ex;

  ex=mman_patch && test_file_exist_DOS(group,name);
  if (!ex) {
      for (int i = 0; i < MAX_PATCHES; ++i) {
          const TDDLMAP_INFO *nfo = &ddlmap[i];
          if (nfo->ptr) {
              int sk =  get_file_entry_in_table(&nfo->nametable, name);
              if (sk >= 0) {
                  h->src_index = i;
                  h->offset = sk;
                  return 1;
              }
          }
      }
  }
  return 0;
}


THANDLE_DATA *get_handle(int handle)
  {
  int group,list;

  group=handle/BK_MINOR_HANDLES;
  list=handle % BK_MINOR_HANDLES;
  if (_handles[group]==NULL)
     {
     _handles[group]=(handle_list *)getmem(sizeof(handle_list));
     memset(_handles[group],0,sizeof(handle_list));
     }
  if (handle>max_handle) max_handle=handle;
  return ((THANDLE_DATA *)_handles[group])+list;
  }


static char need_to_be_free(const void *ptr) {
    for (int i = 0; i < MAX_PATCHES; ++i) {
        const TDDLMAP_INFO *nfo = &ddlmap[i];
        if (nfo->ptr) {
            const char *beg = (const char *)nfo->ptr;
            const char *p = (const char *)ptr;
            if (p >= beg && p < beg+nfo->size) return 0;
        }
    }
    return 1;
}


THANDLE_DATA *kill_block(int handle)
  {
  THANDLE_DATA *h;

  h=get_handle(handle);if (h->status==BK_NOT_USED) return h;
  if (h->flags & BK_LOCKED)
     {
     SEND_LOG("(ERROR) Unable to kill block! It is LOCKED! '%-.12s' (%04X)",h->src_file,handle);
     return NULL;
     }
  SEND_LOG("(KILL) Killing block '%-.12s' (%04X)",h->src_file,handle);
  if (h->status==BK_SAME_AS) return h;
  if (h->status==BK_PRESENT) {
      ablock_free(h->blockdata);
  }
  h->status=BK_NOT_LOADED;
  h->flags&=~BK_HSWAP;
  return h;
  }

THANDLE_DATA *zneplatnit_block(int handle)
  {
  THANDLE_DATA *h;

  h=kill_block(handle);
  if (h->status==BK_SAME_AS)
     return zneplatnit_block(h->offset);
  return h;
  }


static void add_patch(const void *bmf, size_t sz) {
    for (int i = 0; i < MAX_PATCHES; ++i) {
        if (ddlmap[i].ptr == NULL) {
            ddlmap[i].ptr = bmf;
            ddlmap[i].size = sz;
            ddlmap[i].nametable = load_file_table(bmf);
            return;
        }
    }
    display_error("memman: Too many patches");
    abort();
}

char add_patch_file(const char *filename) {
    size_t bmf_s;
    const void *bmf = map_file_to_memory(file_icase_find(filename), &bmf_s);
    if (bmf) {
        add_patch(bmf, bmf_s);
        return 1;
    }
    return 0;
}

void init_manager(void) {
  next_name_read=0;
  last_group=0;
  memset(_handles,0,sizeof(_handles));
  memset(ddlmap,0,sizeof(ddlmap));
}



int find_same(const char *name,ABLOCK_DECODEPROC decomp)
  {
  THANDLE_DATA *p;
  int i,j;

  decomp;
  if (name[0]==0) return -1;
  for(i=0;i<BK_MAJOR_HANDLES;i++)
     if (_handles[i]!=NULL)
     {
     p=(THANDLE_DATA *)(_handles[i]);
     for(j=0;j<BK_MINOR_HANDLES;j++)
        if ((!strncmp(p[j].src_file,name,12))&& (p[j].loadproc==decomp)) return i*BK_MINOR_HANDLES+j;
     }
  return -1;
  }

int find_handle(const char *name,ABLOCK_DECODEPROC decomp)
  {
  return find_same(name,decomp);
  }

int test_file_exist(int group,char *filename)
  {
    THANDLE_DATA h;
    if (get_file_entry(group, filename, &h) == 0)  return test_file_exist_DOS(group,filename);
    return 1;
  }

THANDLE_DATA *def_handle(int handle,const char *filename,ABLOCK_DECODEPROC decompress,char path)
  {
  THANDLE_DATA *h;
  int i;

  i=find_same(filename,decompress);
  h=get_handle(handle);
  if (i==handle) return h;
  if (kill_block(handle)==NULL)
     {
     SEND_LOG("(ERROR) File/Block can't be registred, handle is already in use '%-.12s' handle %04X",filename,handle);
     return NULL;
     }
  if (i!=-1 && i!=handle)
     {
     h->status=BK_SAME_AS;
     h->offset=i;
     return h;
     }
  strcopy_n(h->src_file,filename,sizeof(h->src_file));
  h->offset=0;
  strupper(h->src_file);
  h->loadproc=decompress;
  if (filename[0]) {
      get_file_entry(path,h->src_file,h);
  }
  SEND_LOG("(REGISTER) File/Block registred '%-.12s' handle %04X",h->src_file,handle);
  SEND_LOG("(REGISTER) Seekpos=%d",h->seekpos);
  h->flags=0;
  h->path=path;
  if (h->status!=BK_DIRLIST) h->status=BK_NOT_LOADED;
  h->counter=bk_global_counter++;
  return h;
  }

const void *afile(char *filename,int group,int32_t *blocksize)
  {
  char *d;
  char entr;
  void *p;

  d=alloca(strlen(filename)+1);
  strcpy(d,filename);
  strupper(d);
  THANDLE_DATA hd;
  entr = get_file_entry(group, d, &hd);
  if (entr!=0)
     {
		 SEND_LOG("(LOAD) Afile is loading file '%s' from group %d",d,group);
		 const TDDLMAP_INFO *nfo = &ddlmap[hd.src_index];
		 const int32_t * szptr = (const int32_t *)((const char *)nfo->ptr+hd.offset);
		 *blocksize = *szptr;
		 return szptr+1;
     }
  else if (mman_pathlist!=NULL)
     {
      const char *name = build_pathname(2,mman_pathlist[group],d);
      size_t sz;

     SEND_LOG("(LOAD) Afile is loading file '%s' from disk (group %d)",d,group);
     p=load_file(name, &sz);
     *blocksize=sz;
     }
  else return NULL;
  return p;
  }

void *afile_copy(char *filename,int group,int32_t *blocksize) {
    const void *ptr = afile(filename, group, blocksize);
    if (need_to_be_free(ptr)) return (void *)ptr;
    void *cpy = getmem(*blocksize);
    memcpy(cpy, ptr, *blocksize);
    return cpy;
}


static void decompress_data(THANDLE_DATA *h, int handle) {
    if (h->loadproc) {
        int32_t sz = h->size;
        const void *r = h->loadproc(h->blockdata, &sz, handle);
        if (r != h->blockdata) {
            ablock_free(h->blockdata);
            h->blockdata = r;
            h->size = sz;
        }
    }
}

const void *ablock(int handle)
  {
  THANDLE_DATA *h;

  sem:
  h=get_handle(handle);
  if (h->status==BK_SAME_AS)
     {
     handle=h->offset;
     goto sem;
     }
  if (h->status==BK_PRESENT) {
      return h->blockdata;
  } if (h->status==BK_DIRLIST)
     {
     ablock_free(h->blockdata);h->status=BK_NOT_LOADED;
     }
  if (h->status==BK_NOT_LOADED)
     {
        void *p;int32_t s;

        SEND_LOG("(LOAD) Loading file as block '%-.12s' %04X",h->src_file,handle);
        if (h->offset==0)
           {
           if (h->src_file[0]!=0)
              {
               size_t sz;
              if (mman_action!=NULL) mman_action(MMA_READ);
              const char *name = build_pathname(2,mman_pathlist[h->path], h->src_file);
              p=load_file(name, &sz);
              s=sz;
              }
           else
              {
              p=NULL;
              s=0;
              }
           h->blockdata=p;
           h->status=BK_PRESENT;
           h->size=s;
           decompress_data(h, handle);
           return h->blockdata;
           }
        else
           {
            const TDDLMAP_INFO *nfo = &ddlmap[h->src_index];
           if (mman_action!=NULL) mman_action(MMA_READ);
           const int32_t *szptr =(const int32_t *)((const char *)nfo->ptr+h->offset);
           s = *szptr;
           void *p = (void *)(szptr+1);
           h->blockdata=p;
           h->status=BK_PRESENT;
           h->size=s;
           decompress_data(h, handle);
           return h->blockdata;
           }
        }
  return NULL;
  }

void *ablock_copy(int handle) {
    const void *ptr = ablock(handle);
    if (need_to_be_free(ptr)) return (void *)ptr;
    THANDLE_DATA *h = get_handle(handle);
    void *cpy = getmem(h->size);
    memcpy(cpy, ptr, h->size);
    h->blockdata = cpy;
    return cpy;
}

void alock(int handle)
  {
  THANDLE_DATA *h;

  h=get_handle(handle);
  if (!h->lockcount)
     {
     h->flags|=BK_LOCKED;
     if (h->status==BK_SAME_AS) alock(h->offset);
     }
  h->lockcount++;
  //SEND_LOG("(LOCK) Handle locked %04X (count %d)",handle,h->lockcount);
  }

void aunlock(int handle)
  {
  THANDLE_DATA *h;
  h=get_handle(handle);
  if (h->lockcount) h->lockcount--;else return;
  if (!h->lockcount)
     {
     h->flags&=~BK_LOCKED;
     if (h->status==BK_SAME_AS) aunlock(h->offset);
     }
  //SEND_LOG("(LOCK) Handle unlocked %04X (count %d)",handle,h->lockcount);
  }


void undef_handle(int handle)
  {
  THANDLE_DATA *h;

  h=get_handle(handle);
  if (h->status!=BK_NOT_USED)
     {
     if (kill_block(handle)==NULL) return;
     SEND_LOG("(REGISTER) File/Block unregistred %04X (%-.12s)",handle,h->src_file);
     }
  h->src_file[0]=0;
  h->offset=0;
  h->flags=0;
  h->status=BK_NOT_USED;
  h->loadproc = 0;
  }

void close_manager()
  {
  int i,j;

  for(i=0;i<BK_MAJOR_HANDLES;i++) if (_handles[i]!=NULL)
     {
     for(j=0;j<BK_MINOR_HANDLES;j ++) undef_handle(i*BK_MINOR_HANDLES+j);
     free(_handles[i]);
     }
  for (int i = 0; i < MAX_PATCHES; ++i) {
      unmap_file((void *)ddlmap[i].ptr, ddlmap[i].size);
  }

  max_handle=0;
  }


//------------------------------------------------------------
/*static void block()
  {
  static MEMINFO inf;
  void *c;
  static counter=0;

  get_mem_info(&inf);
  c=malloc(inf.LargestBlockAvail-1024);
  counter++;
  printf("%d. %d\n",counter,inf.LargestBlockAvail);
  if (c!=NULL) block();
  counter--;
  free(c);
  }*/

void display_status()
  {
  int i,j;
  THANDLE_DATA *h;
  char names[][5]={"MISS","*ok*","SWAP","????","PTR "};
  char flags[]={"LS*PH"};
  char copys[6]="     ";
  char nname[14];
  int32_t total_data=0;
  int32_t total_mem=0;
  int ln=0;

  //block();
  //getchar();
  memset(nname,0,sizeof(nname));
  for(i=0;i<BK_MAJOR_HANDLES;i++)
     if (_handles[i]!=NULL)
     {
     for(j=0;j<BK_MINOR_HANDLES;j++)
        {
        h=get_handle(i*BK_MINOR_HANDLES+j);
        if (h->status!=BK_NOT_USED && h->status!=BK_SAME_AS)
           {
           int k;

           for(k=0;k<5;k++) copys[k]=h->flags & (1<<k)?flags[k]:'.';
           if (h->src_file[0]) strcopy_n(nname,h->src_file,sizeof(nname));else strcpy(nname,"<local>");
           printf("%04Xh ... %12s %s %s %08lXh %6d %10d %6d \n",i*BK_MINOR_HANDLES+j,
           nname,names[h->status-1],
           copys,(unsigned long)(uintptr_t)h->blockdata,h->size,h->counter,h->lockcount);
           ln++;
           total_data+=h->size;
           if(h->status==BK_PRESENT)total_mem+=h->size;
           if (ln%24==23)
              {
              char c;
              printf("Enter to continue, q-quit:");
              c=getchar();
              if (c=='q' || c=='Q')
                 {
                 while (getchar()!='\n');
                 return;
                 }
              }
           }
        }

     }
  printf("Data: %7d KB, Loaded: %7d KB",total_data/1024,total_mem/1024);
  while (getchar()!='\n');
  }

void *grealloc(void *p,int32_t size)
  {
  void *q;


  if (!size)
     {
     free(p);
     return NULL;
     }
  q=realloc(p,size);
  if (q!=NULL)
     {
//     SEND_LOG("(ALLOC) **** Realloc: New %p size %d\n",q,*((int32_t *)q-1));
     return q;
     }
  q=getmem(size);
  free(q);
  q=realloc(p,size);
  return q;
  }




#ifdef LOGFILE
/*
void free(void *c)
  {
  if (c==NULL) return;
  SEND_LOG("(ALLOC)��� Dealloc: %p size %d",c,*((int32_t *)c-1));
  free(c);
  }
*/
/*
int get_time_macro();
#pragma aux get_time_macro parm[]=\
  "mov  ah,02"\
  "int  1ah"\
  "mov  al,dh"\
  "shl  eax,16"\
  "mov  ax,cx"\
  modify[eax ecx edx ebx esi edi]

char get_bcd(char bcd);
#pragma aux get_bcd parm[al]=\
  "mov  bl,al"\
  "shr  al,4"\
  "mov  ah,10"\
  "mul  ah"\
  "and  bl,0fh"\
  "add  al,bl"\
  value[al] modify [eax ebx]
*/
char *get_time_str()
  {
  time_t long_time;
  struct tm *newtime;
  static char text[20];


  time( &long_time );                /* Get time as int32_t integer. */
  newtime = localtime( &long_time ); /* Convert to local time. */

  sprintf(text,"%02d:%02d:%02d",newtime->tm_hour,newtime->tm_min,newtime->tm_sec);
  return text;
  }


#endif

int32_t get_handle_size(int handle)
  {
  THANDLE_DATA *h;

  h=get_handle(handle);
  if (h->status==BK_NOT_USED) return -1;
  return h->size;
  }

/*void *malloc(size_t size)
  {
  static char zavora=1;
  void *c;

  c=_nmalloc(size);
  if (log!=NULL && zavora)
     {
     zavora=0;
     fprintf(log,"Alloc: %p size %d\n",c,*((int32_t *)c-1));
     zavora=1;
     }
  return c;
  }
 */

void ablock_free(const void *ptr) {
    if (need_to_be_free(ptr)) free((void *)ptr);
}
