#include <skeldal_win.h>
#include "types.h"
#include "bgraph.h"
#include <debug.h>


void bar32(int x1,int y1, int x2, int y2)
  {
  word *begline;
  int i,j;

  if (x1>x2) swap_int(x1,x2);
  if (y1>y2) swap_int(y1,y2);
  if (x1<0) x1=0;
  if (y1<0) y1=0;
  if (x2>639) x2=639;
  if (y2>479) y2=479;
  for (i=y1,begline=screen+scr_linelen2*y1;i<=y2;i++,begline+=scr_linelen2)
    {
    for (j=x1;j<=x2;j++) begline[j]=curcolor;
    }
  }

void hor_line32(int x1,int y1,int x2)
  {
  word *begline;
  int i;
  unsigned long curcolor2=curcolor | (curcolor<<16);

  if (y1<0 || y1>479) return;
  if (x1>x2) swap_int(x1,x2);  
  if (x1<0) x1=0;
  if (x2>639) x2=639;
  begline=screen+scr_linelen2*y1;
  for (i=x1;i<x2;i+=2) *(unsigned long *)(begline+i)=curcolor2;
  if (i==x2) begline[i]=curcolor;
  }

void ver_line32(int x1,int y1,int y2)
  {
  word *begline;
  int i;
  if (y1>y2) swap_int(y1,y2);
  if (x1<0 || x1>639) return;
  if (y1<0) y1=0;
  if (y2>479) y2=479;
  begline=screen+scr_linelen2*y1+x1;
  for (i=y1;i<=y2;i++,begline+=scr_linelen2) *begline=curcolor;
  }
  
void hor_line_xor(int x1,int y1,int x2)
  {
  word *begline;
  int i;
  unsigned long curcolor2=curcolor | (curcolor<<16);

  if (y1<0 || y1>479) return;
  if (x1>x2) swap_int(x1,x2);  
  if (x1<0) x1=0;
  if (x2>639) x2=639;
  begline=screen+scr_linelen2*y1;
  for (i=x1;i<x2;i+=2) *(unsigned long *)(begline+i)^=curcolor2;
  if (i==x2) begline[i]^=curcolor;
  }

void ver_line_xor(int x1,int y1,int y2)
  {
  word *begline;
  int i;
  if (y1>y2) swap_int(y1,y2);
  if (x1<0 || x1>639) return;
  if (y1<0) y1=0;
  if (y2>479) y2=479;
  begline=screen+scr_linelen2*y1+x1;
  for (i=y1;i<=y2;i++,begline+=scr_linelen2) *begline^=curcolor;
  }

void line_32(int x,int y,int xs,int ys)
  {
  if (xs==0) {ver_line32(x,y,y+ys);return;}
  if (ys==0) {hor_line32(x,y,x+xs);return;}
  STOP();
  }

void char_32(word *posit,word *font,char znak)
//#pragma aux char_32 parm [edi] [esi] [eax] modify [eax ebx ecx edx]
  {
  __asm
    {
        mov     edi,posit;
        mov     esi,font;
        mov     al,znak
                        ;edi - pozice na obrazovce
                        ;esi - ukazatel na font
                        ;al - znak
        and     eax,0ffh
        mov     ax,[esi][eax*2]
        or      ax,ax
        jz      chrend
        add     esi,eax
        lodsw
        xor     dl,dl   ;dl - je citac transparetnich pozic
        mov     cx,ax  ;cl - XRES, ch - YRES
chr6:   mov     ebx,edi ;ebx - ukazuje po radcich v jednom sloupci
        mov     dh,ch   ;dh - bude citac radku
chr5:   or      dl,dl   ;pokud je dl = 0 pak se cte dalsi bajt
        jnz     chr1    ;jinak je dalsi bod jenom transparetni
        lodsb           ;cti barvu
        or      al,al   ;pokud je 0 pak je transparetni
        jz      chr2    ;preskoc kresleni
        cmp     al,8    ;8 a vice jsou informace o opakovanych transparetnich bodech
        jnc     chr3    ;(viz FONTEDIT.DOC). Pak se podle toho zarid
        and     eax,0ffh;v eax jen dolnich 8 bitu
        dec     al
        mov     ax,short ptr charcolors[EAX*2] ;vyjmi barvu
        cmp     ax,0xffff ;0xffff je barva ktera se nekresli;
        jz      chr4    ;
        mov     [ebx],ax;zobraz ji na obrazovce
        jmp     chr4    ;a skoc na konec smycky
chr3:   cmp     al,0ffh ;pokud je al=255 pak jsme narazily na terminator.
        jz      chrend  ;V tom pripade KONEC
        sub     al,6    ;odecti do al 6. Ziskas pocet transparetnich pozic
        mov     dl,al   ;uloz je do citace
chr1:   dec     dl      ;pro kazdou pozici to dl odecti dokud neni 0
chr2:
chr4:   add     ebx,scr_linelen;dalsi radka
        dec     dh      ;odecti citac radek
        jnz     chr5    ;dokud neni nula
        add     edi,2   ;dalsi sloupec
        dec     cl      ;odecti citac sloupcu
        jnz     chr6    ;dokud neni nula
chrend:                 ;konec
    }

  }
void char2_32(word *posit,word *font,char znak)
//#pragma aux char2_32 parm [edi] [esi] [eax] modify [eax ebx ecx edx]
  {
  __asm
    {
        mov edi,posit
        mov esi,font
        mov al,znak
    
                        ;edi - pozice na obrazovce
                        ;esi - ukazatel na font
                        ;al - znak
        and     eax,0ffh
        mov     ax,[esi][eax*2]
        or      ax,ax
        jz      chr2end
        add     esi,eax
        lodsw
        xor     dl,dl   ;dl - je citac transparetnich pozic
        mov     cx,ax  ;cl - XRES, ch - YRES
chr26:   mov     ebx,edi ;ebx - ukazuje po radcich v jednom sloupci
        mov     dh,ch   ;dh - bude citac radku
chr25:   or      dl,dl   ;pokud je dl = 0 pak se cte dalsi bajt
        jnz     chr21    ;jinak je dalsi bod jenom transparetni
        lodsb           ;cti barvu
        or      al,al   ;pokud je 0 pak je transparetni
        jz      chr22    ;preskoc kresleni
        cmp     al,8    ;8 a vice jsou informace o opakovanych transparetnich bodech
        jnc     chr23    ;(viz FONTEDIT.DOC). Pak se podle toho zarid
        and     eax,0ffh;v eax jen dolnich 8 bitu
        dec     al
        mov     ax,charcolors[EAX*2] ;vyjmi barvu
        push    ebx
        mov     [ebx],ax;zobraz ji na obrazovce
        mov     [ebx+2],ax;zobraz ji na obrazovce
        add     ebx,scr_linelen
        mov     [ebx],ax;zobraz ji na obrazovce
        mov     [ebx+2],ax;zobraz ji na obrazovce
        pop     ebx
        jmp     chr24    ;a skoc na konec smycky
chr23:   cmp     al,0ffh ;pokud je al=255 pak jsme narazily na terminator.
        jz      chr2end  ;V tom pripade KONEC
        sub     al,6    ;odecti do al 6. Ziskas pocet transparetnich pozic
        mov     dl,al   ;uloz je do citace
chr21:   dec     dl      ;pro kazdou pozici to dl odecti dokud neni 0
chr22:
chr24:  add     ebx,scr_linelen;dalsi radka
        add     ebx,scr_linelen
        dec     dh      ;odecti citac radek
        jnz     chr25    ;dokud neni nula
        add     edi,4   ;dalsi sloupec
        dec     cl      ;odecti citac sloupcu
        jnz     chr26    ;dokud neni nula
chr2end:              ;konec
    }

  }


/*          public  charsize_
charsize_:              ;esi - ukazatel na font
                        ;al - znak
        and     eax,0ffh
        mov     ax,[esi][eax*2]
        or      ax,ax
        jz      chsend
        add     esi,eax
        lodsw
chsend: and     eax,0ffffh
        ret*/

