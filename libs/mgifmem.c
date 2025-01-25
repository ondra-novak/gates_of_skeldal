#include <skeldal_win.h>
#include <stdio.h>
#include "types.h"
#include "memman.h"
#include "mem.h"
#include "mgifmem.h"

#define MGIF "MGIF"
#define LZW_MAX_CODES  16384
#define LZW_BUFFER 64000

MGIF_PROC show_proc;
static int mgif_frames;
static int cur_frame;


typedef struct double_s
  {
  short group,chr,first,next;
  }DOUBLE_S;

typedef DOUBLE_S CODE_TABLE[LZW_MAX_CODES];

DOUBLE_S *compress_dic=NULL;
static void *lzw_buffer=NULL;
static int clear_code;
static int end_code;
static int free_code;
static int nextgroup;
static int bitsize,init_bitsize;
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

  if (compress_dic==NULL) compress_dic=(DOUBLE_S *)getmem(sizeof(CODE_TABLE));
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
  if (strncmp(c,MGIF,4)) return NULL;
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


int input_code(void *source,int32_t *bitepos,int bitsize,int mask)
  {
    int32_t ofs = *bitepos >> 3;
    int32_t shf = *bitepos & 0x7;
    uint8_t *src = source;
    uint8_t val1 = src[ofs];
    uint8_t val2 = src[ofs+1];
    uint16_t val = val1 + val2 * 256;
    *bitepos+=bitsize;
    return val >> shf;
/*
  __asm
    {
    mov esi,source
    mov edi,bitepos
    mov ebx,bitsize
    mov edx,mask

    mov     ecx,[edi]
    mov     eax,ecx
    shr     eax,3
    mov     eax,[esi+eax]
    and     cl,7
    shr     eax,cl
    and     eax,edx
    add     [edi],ebx
    }
    */
  }
//#pragma aux input_code parm [esi][edi][ebx][edx]=\    value[eax] modify [ecx];


int de_add_code(int group,int chr,int mask)
  {
  DOUBLE_S *q;

  q=&compress_dic[nextgroup];q->group=group;q->chr=chr;q->first=compress_dic[group].first+1;
  nextgroup++;
  if (nextgroup==mask)
     {
     mask=(mask<<1)+1;
     bitsize++;
     }
  return mask;
  }


int fast_expand_code(DOUBLE_S *compress_dic, int code,uint8_t **target, uint8_t *old_value)
  {

    uint8_t out;
    uint8_t w;
    if (code >= 256) {

        DOUBLE_S *pos = compress_dic+code;
        uint8_t *t = *target + pos->first;
        (**target) += pos->first+1;
        short len = pos->first;
        short group = pos->group;
        do{
            *t = pos->chr;
            --t;
            group = pos->group;
            pos = compress_dic+group;
        } while (group >= 256);
        w=(uint8_t)group;
        out = w;
        w += *old_value;
        *t = w;
        while (len) {
            ++t;
            w = w + *t;
            *t = w;
            --len;
        }
        *old_value = w;
    } else {
        out = (uint8_t) code;
        w = out + *old_value;
        *old_value = w;
        **target = out;
        (*target)++;
        return code;
    }
/*
  _asm
    {
     mov     eax,code
     mov     edi,target

     cmp     eax,256
     jnc     expand
     mov     esi,[edi]
     inc     dword ptr [edi]
     mov     bl,al
     add     al,old_value
     mov     [esi],al           //esi - target ptr
     mov     old_value,al
     jmp     end
expand:
     mov     ebx,compress_dic
     lea     ecx,[eax*8+ebx]
     movzx   eax,short ptr [ecx+4]  // first
     add     [edi],eax
     push    eax
     mov     esi,[edi]          //esi - target ptr
eloop:movzx   eax,short ptr [ecx+2] // chr
     mov     [esi],al
     dec     esi
     movzx   eax,short ptr [ecx]    //group
     lea     ecx,[eax*8+ebx]
     cmp     eax,256
     jnc     eloop
     mov     bl,al
     add     al,old_value
     mov     [esi],al
     inc     dword ptr [edi]
     pop     ecx
elp2:inc     esi
     add     al,[esi]
     mov     [esi],al
     dec     ecx
     jnz     elp2
     mov     old_value,al
end:
     movzx   eax,bl
    }
    */
  }


void lzw_decode(void *source,char *target)
  {
  int32_t bitpos=0;
  register int code;
  int old,i;
  //int group,chr;
  int old_first;
  register int mask=0xff;


  for(i=0;i<LZW_MAX_CODES;i++) compress_dic[i].first=0;
  clear:
  uint8_t old_value=0;
  nextgroup=free_code;
  bitsize=init_bitsize;
  mask=(1<<bitsize)-1;
  code=input_code(source,&bitpos,bitsize,mask);
  uint8_t *t = target;
  old_first=fast_expand_code(compress_dic,code,&t,&old_value);
  old=code;
  while ((code=input_code(source,&bitpos,bitsize,mask))!=end_code)
     {
     if (code==clear_code)
        {
        goto clear;
        }
     else if (code<nextgroup)
        {
        old_first=fast_expand_code(compress_dic,code,&t,&old_value);
        //group=old;
        //chr=old_first;
        mask=de_add_code(old,old_first,mask);
        old=code;
        }
     else
        {
        //p.group=old;
        //p.chr=old_first;
        mask=de_add_code(old,old_first,mask);
        old_first=fast_expand_code(compress_dic,code,&t, &old_value);
        old=code;
        }
     }
    }

void *mgif_play(void *mgif) //dekoduje a zobrazi frame
  {
  char *pf,*pc,*ff;
  int acts,size,act,csize;
  void *scr_sav;
  int scr_act=-1;

  pf=mgif;
  acts=*pf++;
  size=(*(int *)pf) & 0xffffff;
  pf+=3;pc=pf;pf+=size;
  if (acts)
  do
     {
     act=*pc++;csize=(*(int *)pc) & 0xffffff;pc+=3;
     if (act==MGIF_LZW || act==MGIF_DELTA)
        {
        ff=lzw_buffer;
        lzw_decode(pc,ff);
        scr_sav=ff;
        scr_act=act;
        }
     else if (act==MGIF_COPY)
        {
        scr_sav=ff;scr_act=act;
        }
     else
        {
        ff=pc;
        show_proc(act,ff,csize);
        }
     pc+=csize;
     }
  while (--acts);
  if (scr_act!=-1) show_proc(scr_act,scr_sav,csize);
  cur_frame+=1;
  if (cur_frame==mgif_frames) return NULL;
  return pf;
  }







