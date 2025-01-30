#include <platform/platform.h>
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

void do_clear_code(void)
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

void reinit_lzw(void)
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


void done_lzw_compressor(void)
  {
  free(compress_dic);
  compress_dic=NULL;
  }

void mgif_install_proc(MGIF_PROC proc)
  {
  show_proc=proc;
  }

struct mgif_header load_mgif_header(char **mgif) {
    struct mgif_header r;
    char *iter = *mgif;
    memcpy(r.sign, iter, 4); iter+=4;
    memcpy(r.year, iter, 2); iter+=2;
    r.eof = *iter++;
    memcpy(&r.ver, iter, 2); iter+=2;
    memcpy(&r.frames, iter, 4); iter+=4;
    memcpy(&r.snd_chans, iter, 2); iter+=2;
    memcpy(&r.snd_freq, iter, 4); iter+=4;
    memcpy(r.ampl_table, iter, sizeof(r.ampl_table)); iter+=sizeof(r.ampl_table);
    iter += 64;
    *mgif = iter;
    return r;
}

void *open_mgif(void *mgif) //vraci ukazatel na prvni frame
  {
  char *c = mgif;
  struct mgif_header mgh = load_mgif_header(&c);

  if (strncmp(mgh.sign,MGIF,4)) return NULL;
  mgif_frames=mgh.frames;
  cur_frame=0;
  init_lzw_compressor(8);
  if (lzw_buffer==NULL) lzw_buffer=getmem(LZW_BUFFER);
  return c;
  }

void close_mgif(void)           //dealokuje buffery pro prehravani
  {
  done_lzw_compressor();
  free(lzw_buffer);
  lzw_buffer=NULL;
  }


int input_code(void *source,int32_t *bitepos,int bitsize,int mask)
  {
      uint8_t *esi = source;            //    mov esi,source
      int32_t *edi = bitepos;           //    mov edi,bitepos
      int ebx = bitsize;                //    mov ebx,bitsize
      int edx = mask;                   //    mov edx,mask

      int ecx = *edi;                   //    mov     ecx,[edi]
      int eax = ecx;                    //    mov     eax,ecx
      eax >>=3;                         //    shr     eax,3
      eax = esi[eax] | (esi[eax+1] << 8) | (esi[eax+2] << 16) | (esi[eax+3] << 24);
                                        //    mov     eax,[esi+eax]
      ecx &= 7;                         //    and     cl,7
      eax >>= ecx;                      //    shr     eax,cl
      eax &= edx;                       //    and     eax,edx
      (*edi) += ebx;                    //    add     [edi],ebx
      return eax;
  }


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

int fast_expand_code(const DOUBLE_S *compress_dic, int code,uint8_t **target, uint8_t *old_value)
  {


    int  eax = code;              //mov     eax,code
    uint8_t ** edi = target;      //mov     edi,target
                                  //cmp     eax,256
    if (eax < 256) {              //jnc     expand
      uint8_t *esi = *edi;        //mov     esi,[edi]
       ++(*edi);                  //inc     dword ptr [edi]
      uint8_t bl = (uint8_t)eax;  //mov     bl,al
      uint8_t al = bl + *old_value; //add     al,old_value
      *esi = al;                  //  mov     [esi],al
      *old_value = al;            // mov     old_value,al
      return bl;                  // jmp     end
    }

                                    //expand:
    const DOUBLE_S *ebx = compress_dic;   //   mov     ebx,compress_dic
    const DOUBLE_S *ecx = ebx+eax;        // lea     ecx,[eax*8+ebx]
    eax = ecx->first;               //  movzx   eax,short ptr [ecx+4]  // first
    *target += eax;                 //   add     [edi],eax
    int save_eax = eax;             // push    eax
    uint8_t *esi = *edi;            // mov     esi,[edi]          //esi - target ptr

    do {                            //eloop:

        eax = ecx->chr;             //movzx   eax,short ptr [ecx+2] // chr
        *esi = (uint8_t)(eax);      //mov     [esi],al
        --esi;                      //dec     esi
        eax = ecx->group;           //movzx   eax,short ptr [ecx]    //group
        ecx = ebx+eax;              //lea     ecx,[eax*8+ebx]
                                    //cmp     eax,256
    }    while (eax >= 256 );       //jnc     eloop
       uint8_t bl = (uint8_t)eax;   //mov     bl,al
       uint8_t al = bl + *old_value; //add     al,old_value
       *esi =  al;                   //mov     [esi],al
       ++(*edi);                     //inc     dword ptr [edi]
       int ecx2 = save_eax;          //pop     ecx
   do {                              //elp2
       ++esi;                        //inc     esi
       al = al + *esi;               //add     al,[esi]
       *esi = al;                    //mov     [esi],al
       --ecx2;                       //dec     ecx
   } while (ecx2 > 0);                //jnz     elp2
         *old_value =al;             //mov     old_value,al
         return bl;                  //movzx   eax,bl

    //  }
#if 0
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
        **target = w;
        (*target)++;
    }
    return out;
#endif
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
  int code;
  int old,i;
  int old_first;

  uint8_t old_value;

  for(i=0;i<LZW_MAX_CODES;i++) compress_dic[i].first=0;
  uint8_t *t = (uint8_t *)target;
  clear:
  old_value=0;
  int mask=0xff;
  nextgroup=free_code;
  bitsize=init_bitsize;
  mask=(1<<bitsize)-1;
  code=input_code(source,&bitpos,bitsize,mask);
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


typedef struct frame_header_t {
    uint32_t size;
    uint8_t count;
} FRAME_HEADER_T;

typedef struct chunk_header_t {
    uint32_t size;
    uint8_t type;
} CHUNK_HEADER_T;

CHUNK_HEADER_T read_chunk_header(char **iter) {
    CHUNK_HEADER_T ret;
    ret.type = *(uint8_t *)(*iter)++;
    ret.size = *(uint8_t *)(*iter)++;
    ret.size |= *(uint8_t *)(*iter)++ << 8;
    ret.size |= *(uint8_t *)(*iter)++ << 16;
    return ret;
}

FRAME_HEADER_T read_frame_header(char **iter) {
    FRAME_HEADER_T ret;
    ret.count = *(uint8_t *)(*iter)++;
    ret.size = *(uint8_t *)(*iter)++;
    ret.size |= *(uint8_t *)(*iter)++ << 8;
    ret.size |= *(uint8_t *)(*iter)++ << 16;
    return ret;
}

void *mgif_play(void *mgif) //dekoduje a zobrazi frame
  {
  char *pf,*pc,*ff;
//  int acts,size,act,csize;
  void *scr_sav;
  int scr_act=-1;



  pf=mgif;
  FRAME_HEADER_T frame_hdr = read_frame_header(&pf);
  pc = pf;
  pf += frame_hdr.size;
  for (uint8_t i = 0; i < frame_hdr.count; ++i) {
      CHUNK_HEADER_T chunk_hdr = read_chunk_header(&pc);
      if (chunk_hdr.type == MGIF_LZW || chunk_hdr.type == MGIF_DELTA) {
          ff=lzw_buffer;
          lzw_decode(pc,ff);
          scr_sav=ff;
          scr_act=chunk_hdr.type;
      } else if (chunk_hdr.type==MGIF_COPY) {
          //scr_sav=ff;scr_act=act;
          //strange code
          scr_sav = pc;
          scr_act = chunk_hdr.type;
      } else {
          show_proc(chunk_hdr.type,pc,chunk_hdr.size);
      }
      pc+=chunk_hdr.size;
  }
  if (scr_act!=-1) show_proc(scr_act,scr_sav,0);
  cur_frame+=1;
  if (cur_frame==mgif_frames) return NULL;
  return pf;
  }
/*
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



*/



