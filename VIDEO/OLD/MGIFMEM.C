#include <stdio.h>
#include <libs/memman.h>
#include <mem.h>

#define MGIF "MGIF"
#define LZW_MAX_CODES  4096
#define LZW_BUFFER 64000


MGIF_PROC show_proc;
int mgif_frames;int cur_frame;

typedef struct mgif_header
    {
    char sign[4];
    char year[2];
    char eof;
    word ver;
    long frames;
    word snd_chans;
    int snd_freq;
    short ampl_table[256];
    short reserved[32];
    };


typedef struct double_s
  {
  short group,chr,first,next;
  }DOUBLE_S;

typedef DOUBLE_S CODE_TABLE[LZW_MAX_CODES];

DOUBLE_S *compress_dic=NULL;
void *lzw_buffer=NULL;
int clear_code;
int end_code;
int free_code;
int nextgroup;
int bitsize,init_bitsize;
char old_value=0;

void do_clear_code()
  {
  int i;

  old_value=0;
  nextgroup=free_code;
  bitsize=init_bitsize;
  for(i=0;i<clear_code;i++)
     {
     DOUBLE_S *p;
     p=&compress_dic[i];
     p->group=i;p->chr=-1;p->next=-1;p->first=-1;
     }
  }

void reinit_lzw()
  {
  do_clear_code();
  }

void init_lzw_compressor(int dic_size)
  {

  if (compress_dic!=NULL)compress_dic=(CODE_TABLE *)getmem(sizeof(CODE_TABLE));
  clear_code=1<<dic_size;
  end_code=clear_code+1;
  free_code=end_code+1;
  nextgroup=free_code;
  init_bitsize=bitsize=dic_size+1;
  do_clear_code();
  }


void done_lzw_compressor()
  {
  free(compress_dic);
  compress_dic=NULL;
  }

void mgif_install_proc(MGIF_PROC proc)
  {
  show_proc=proc;
  }

void *open_mgif(void *mgif) //vraci ukazatel na prvni frame
  {
  char *c;
  struct mgif_header *mgh;

  c=mgif;
  if (strncmp(c,MGIF)) return NULL;
  mgh=mgif;
  mgif_frames=mgh->frames;
  cur_frame=0;
  c+=sizeof(*mgh);
  init_lzw_compressor(8);
  if (lzw_buffer==NULL) lzw_buffer=getmem(LZW_BUFFER);
  return c;
  }

void close_mgif()           //dealokuje buffery pro prehravani
  {
  done_lzw_compressor();
  free(lzw_buffer);
  lzw_buffer=NULL;
  }


int input_code(void *source,long *bitepos,int bitsize,int mask);
#pragma aux input_code parm [esi][edi][ebx][edx]=\
     "mov     ecx,[edi]"\
     "mov     eax,ecx"\
     "shr     eax,3"\
     "mov     eax,[esi+eax]"\
     "and     cl,7"\
     "shr     eax,cl"\
     "and     eax,edx"\
     "add     [edi],ebx"\
    value[eax] modify [ecx];


void de_add_code(DOUBLE_S *p,int *mask)
  {
  DOUBLE_S *q;

  q=&compress_dic[nextgroup];q->group=p->group;q->chr=p->chr;q->first=compress_dic[p->group].first+1;
  nextgroup++;
  if (nextgroup==*mask)
     {
     *mask=(*mask<<1)+1;
     bitsize++;
     }
  }


char fast_expand_code(int code,char **target);
#pragma aux fast_expand_code parm[eax][edi] modify [esi ecx] value [bl]

void lzw_decode(void *source,char *target)
  {
  long bitpos=0;
  int code,old,i;
  DOUBLE_S p;
  int old_first;
  int mask=0xff;


  for(i=0;i<LZW_MAX_CODES;i++) compress_dic[i].first=0;
  clear:
  old_value=0;
  nextgroup=free_code;
  bitsize=init_bitsize;
  mask=(1<<bitsize)-1;
  code=input_code(source,&bitpos,bitsize,mask);
  old_first=fast_expand_code(code,&target);
  old=code;
  while ((code=input_code(source,&bitpos,bitsize,mask))!=end_code)
     {
     if (code==clear_code)
        {
        goto clear;
        }
     else if (code<nextgroup)
        {
        old_first=fast_expand_code(code,&target);
        p.group=old;
        p.chr=old_first;
        de_add_code(&p,&mask);
        old=code;
        }
     else
        {
        p.group=old;
        p.chr=old_first;
        de_add_code(&p,&mask);
        old_first=fast_expand_code(code,&target);
        old=code;
        }
     }
    }

void *mgif_play(void *mgif) //dekoduje a zobrazi frame
  {
  char *pf,*pc,*ff;
  char acts,size,act,csize;

  pf=mgif;
  acts=*pf++;
  size=*(int *)pf & 0xffffff;
  pf+=3;pc=pf;pf+=size;
  do
     {
     act=*pc++;csize=*(int *)pc & 0xffffff;pc+=3;
     if (act==MGIF_LZW || act==MGIF_DELTA)
        {
        ff=lzw_buffer;
        lzw_decode(pc,ff);
        }
     else
        ff=pc;
     show_proc(act,ff);
     pc+=csize;
     }
  while (--acts);
  return pf;
  }

