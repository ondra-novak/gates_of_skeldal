#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>

#define getmem(s) malloc(s)
#define New(typ) (typ *)getmem(sizeof(typ))
#define NewArr(typ,pocet) (typ *)getmem(sizeof(typ)*pocet);
#define ClrArr(p,typ,pocet) memset(p,0,sizeof(typ)*pocet);

#define LZW_MAX_CODES  4096

typedef struct double_s
  {
  short group,chr,first,next;
  }DOUBLE_S;

typedef DOUBLE_S CODE_TABLE[LZW_MAX_CODES];

DOUBLE_S *compress_dic;
int clear_code;
int end_code;
int free_code;
int nextgroup;
int bitsize,init_bitsize;
char old_value=0;

void do_clear_code() //funkce maze slovni (clear code)
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
  //dic size je velikost slovniku(bitova)
  //pro 8 bitove hodnoty zde vloz 8.
  {
  compress_dic=(CODE_TABLE *)getmem(sizeof(CODE_TABLE));
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
  }

/*
long output_code(void *target,long bitepos,int bitsize,int data);
#pragma aux output_code parm [edi][edx][ebx][eax]=\
     "mov     ecx,edx"\
     "shr     ecx,3"\
     "add     edi,ecx"\
     "mov     ecx,edx"\
     "and     cl,7h"\
     "shl     eax,cl"\
     "or      [edi],eax"\
     "add     edx,ebx"\
    value[edx] modify [ecx];
*/
long output_code_c(void *target,long bitepos,int bitesize,int data)
  {
  unsigned max_faces *c;
  c=target;
  c+=bitepos>>3;
  data<<=bitepos & 7;
  c[0]|=data;
  c[1]=data>>8;
  c[2]=data>>16;
  return bitepos+bitesize;
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

int input_code_c(unsigned char *source,long *bitepos,int bitsize,int mask)
 {
  unsigned char *c;int x;
  c=source;
  c+=*bitepos>>3;
  x=c[0]+(c[1]<<8)+(c[2]<<16);
  x>>=*bitepos & 7;
  x &= mask;
  *bitepos=*bitepos+bitsize;
  return x;
 }


int find_code(DOUBLE_S *p)
  //hleda skupinu ve slovniku. Pokud neexistuje vraci -1;
  {
  int ps;

  ps=p->group;
  ps=compress_dic[ps].first;
  while (ps!=-1)
     {
     if (compress_dic[ps].chr==p->chr) return ps;
     ps=compress_dic[ps].next;
     }
  return -1;
  }


void add_code(DOUBLE_S *p)
  //vklada novou dvojici
  {
  p->first=-1;p->next=compress_dic[p->group].first;
  memcpy(&compress_dic[nextgroup],p,sizeof(DOUBLE_S));
  compress_dic[p->group].first=nextgroup;
  nextgroup++;
  }


long lzw_encode(unsigned char *source,void *target,int size)
  //Encode LZW. zdroj, cil a velikost dat. Vraci velikost komprimovano.
  {
  long bitpos=0;
  DOUBLE_S p;
  int f;

  clear:
  old_value=p.group=*source++;size--;
  while (size-->0)
     {
     p.chr=(int)((unsigned char)(*source++));old_value+=p.chr;
     f=find_code(&p);
     if (f<0)
        {
        bitpos=output_code_c(target,bitpos,bitsize,p.group);
        add_code(&p);
        if (nextgroup==(1<<bitsize)) bitsize++;
        p.group=p.chr;
        if (nextgroup>=LZW_MAX_CODES)
           {
           bitpos=output_code_c(target,bitpos,bitsize,p.group);
           bitpos=output_code_c(target,bitpos,bitsize,clear_code);
           do_clear_code();
           goto clear;
           }
        }
     else
        p.group=f;
     }
  bitpos=output_code_c(target,bitpos,bitsize,p.group);
  bitpos=output_code_c(target,bitpos,bitsize,end_code);
  return (bitpos+8)>>3;
  }


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



int expand_code(int code,unsigned char **target)
  {
  static int first;

  if (code>end_code)
     {
     assert(compress_dic[code].group<code);
     expand_code(compress_dic[code].group,target);
     **target=old_value=compress_dic[code].chr;
     (*target)++;
     }
  else
     {
     **target=old_value=code;
     (*target)++;
     first=code;
     }
  return first;
  }
/*
char fast_expand_code(int code,char **target);
#pragma aux fast_expand_code parm[eax][edi] modify [esi ecx] value [bl]
*/

void lzw_decode(void *source,unsigned char *target)
  //dekomprimuje lzw. Nevraci velikost, tu si musi program zajistit sam.
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
  code=input_code_c(source,&bitpos,bitsize,mask);
  old_first=expand_code(code,&target);
  old=code;
  while ((code=input_code_c(source,&bitpos,bitsize,mask))!=end_code)
     {
     if (code==clear_code)
        {
        goto clear;
        }
     else if (code<nextgroup)
        {
        old_first=expand_code(code,&target);
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
        old_first=expand_code(code,&target);
        old=code;
        }
     }
    }

