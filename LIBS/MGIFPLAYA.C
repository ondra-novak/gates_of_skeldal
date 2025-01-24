#include <skeldal_win.h>
#include "types.h"
#include <mgfplay.h>
#include <bgraph.h>



void show_full_interl_lfb(void *source,void *target,void *palette, long linelen)
  {  
  int sslinelen=2*linelen-1280;
  __asm
    {
        mov     edi,target
        mov     esi,source
        mov     ebx,palette
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp
        push    sslinelen;
        mov     dl,180
shfif2: mov     ecx,320
shfif1: lodsb
        movzx   eax,al
        movzx   eax,short ptr [eax*2+ebx]
        mov     ebp,eax
        shl     eax,16
        or      eax,ebp
        stosd
        dec     ecx
        jnz     shfif1
        add     edi,[esp]
        dec     dl
        jnz     shfif2
        pop     eax
        pop     ebp
    }
  }
//#pragma aux show_full_interl_lfb parm [esi][edi][ebx] modify [eax ecx edx]
void show_delta_interl_lfb(void *source,void *target,void *palette, long linelen)
  {  
  int sslinelen=2*linelen;
  __asm
    {
        mov     edi,target
        mov     esi,source
        mov     ebx,palette
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp             ;uchovej ebp
        push    sslinelen
        mov     cl,180          ;cl pocet zbyvajicich radek
        add     esi,4           ;preskoc ukazatel
        mov     edx,esi         ;edx - zacatek delta mapy
        add     esi,[esi-4]     ;esi - zacatek dat
shdif6: push    edi             ;uloz adresu radku
shdif2: mov     ch,[edx]        ;cti _skip_ hodnotu
        mov     al,ch
        inc     edx
        or      al,03fh         ;test zda jsou 2 nejvyssi bity nastaveny
        inc     al
        jz      shdif3          ;ano - preskakovani radku
        movzx   eax,ch          ;expanduj _skip_ hodnotu do eax
        lea     edi,[eax*8+edi] ;vypocti novou pozici na obrazovce
        mov     ch,[edx]        ;cti _copy_ hodnotu
        inc     edx
shdif1: lodsb                   ;vem bajt z datove oblasti
        movzx   eax,al          ;expanduj do eax
        movzx   eax,short ptr[eax*2+ebx] ;expanduj hicolor barvu
        mov     ebp,eax         ;rozdvoj barvy
        shl     ebp,16
        or      eax,ebp
        stosd                   ;zapis dva body
        lodsb                   ;opakuj pro dalsi bod jeste jednou
        movzx   eax,al
        movzx   eax,short ptr[eax*2+ebx]
        mov     ebp,eax
        shl     ebp,16
        or      eax,ebp
        stosd
        dec     ch              ;odecti _copy_ hodnotu
        jnz     shdif1          ;dokud neni 0
        jmp     shdif2          ;pokracuj _skip_ hodnotou
shdif3: and     ch,3fh          ;odmaskuj hodni 2 bity
        pop     edi             ;obnov edi
        jnz     shdif4          ;pokud je ch=0 preskoc jen jeden radek;
        add     edi,[esp]       ;preskoc radek
        dec     cl              ;odecti citac radku
        jnz     shdif6          ;skok pokud neni konec
        pop     ebp
        jmp     konec          ;navrat
shdif4: inc     ch              ;pocet radek je ch+1
        sub     cl,ch           ;odecti ch od zbyvajicich radek
        jz      shdif5          ;je-li nula tak konec
shdif7: add     edi,[esp]       ;preskoc radek
        dec     ch              ;odecti ch
        jnz     shdif7          ;preskakuj dokud neni 0
        jmp     shdif6          ;cti dalsi _skip_
shdif5: pop     ebp
konec:
    }
  }
//#pragma aux show_delta_interl_lfb parm [esi][edi][ebx] modify [eax ecx edx]

void show_full_lfb12e(void *target,void *buff,void *paleta)
  {
  __asm
    {
        mov     edi,target
        mov     esi,buff
        mov     ebx,paleta
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp
        mov     dl,180
shfl2:  mov     ecx,160
shfl1:  lodsw
        movzx   ebp,al
        movzx   ebp,short ptr ds:[ebp*2+ebx]
        movzx   eax,ah
        movzx   eax,short ptr ds:[eax*2+ebx]
        shl     eax,16
        or      eax,ebp
        stosd
        dec     ecx
        jnz     shfl1
        dec     dl
        jnz     shfl2
        pop     ebp
    }
  }   
//#pragma aux show_full_lfb12e parm[edi][esi][ebx] modify [eax ecx]
void show_delta_lfb12e(void *target,void *buff,void *paleta)
  {
  __asm
    {
        mov     edi,target
        mov     esi,buff
        mov     ebx,paleta
                        ;edi - target
                        ;esi - buff
                        ;ebx - paleta
        push    ebp             ;uchovej ebp
        mov     cl,180          ;cl pocet zbyvajicich radek
        add     esi,4           ;preskoc ukazatel
        mov     edx,esi         ;edx - zacatek delta mapy
        add     esi,[esi-4]     ;esi - zacatek dat
shdl6: push    edi             ;uloz adresu radku
shdl2: mov     ch,[edx]        ;cti _skip_ hodnotu
        mov     al,ch
        inc     edx
        or      al,03fh         ;test zda jsou 2 nejvyssi bity nastaveny
        inc     al
        jz      shdl3          ;ano - preskakovani radku
        movzx   eax,ch          ;expanduj _skip_ hodnotu do eax
        lea     edi,[eax*4+edi] ;vypocti novou pozici na obrazovce
        mov     ch,[edx]        ;cti _copy_ hodnotu
        inc     edx
shdl1: lodsw
        movzx   ebp,al
        movzx   ebp,short ptr ds:[ebp*2+ebx]
        movzx   eax,ah
        movzx   eax,short ptr ds:[eax*2+ebx]
        shl     eax,16
        or      eax,ebp
        stosd
        dec     ch              ;odecti _copy_ hodnotu
        jnz     shdl1          ;dokud neni 0
        jmp     shdl2          ;pokracuj _skip_ hodnotou
shdl3: and     ch,3fh          ;odmaskuj hodni 2 bity
        pop     edi             ;obnov edi
        jnz     shdl4          ;pokud je ch=0 preskoc jen jeden radek;
        add     edi,640        ;preskoc radek
        dec     cl              ;odecti citac radku
        jnz     shdl6          ;skok pokud neni konec
        pop     ebp
        jmp    konec
shdl4: inc     ch              ;pocet radek je ch+1
        sub     cl,ch           ;odecti ch od zbyvajicich radek
        jz      shdl5          ;je-li nula tak konec
shdl7: add     edi,640        ;preskoc radek
        dec     ch              ;odecti ch
        jnz     shdl7          ;preskakuj dokud neni 0
        jmp     shdl6          ;cti dalsi _skip_
shdl5: pop     ebp
konec:
    }
  }
//#pragma aux show_delta_lfb12e parm[edi][esi][ebx] modify [eax ecx]

void show_full_lfb12e_dx(void *target,void *buff,void *paleta)
  {  
  __asm
    {
        mov     edi,target
        mov     esi,buff
        mov     ebx,paleta
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp
		mov		eax,scr_linelen
		sub		eax,640
		push	eax
        mov     dl,180
shfl2:  mov     ecx,160
shfl1:  lodsw
        movzx   ebp,al
        movzx   ebp,short ptr ds:[ebp*2+ebx]
        movzx   eax,ah
        movzx   eax,short ptr ds:[eax*2+ebx]
        shl     eax,16
        or      eax,ebp
		mov		ebp,eax
		and		ebp,0x7fe07fe0
		add		eax,ebp
        stosd
        dec     ecx
        jnz     shfl1
		add		edi,[esp]
        dec     dl
        jnz     shfl2
		pop		eax
        pop     ebp
    }
  }   
//#pragma aux show_full_lfb12e parm[edi][esi][ebx] modify [eax ecx]
void show_delta_lfb12e_dx(void *target,void *buff,void *paleta,unsigned long Pitch)
  {
  __asm
    {
        mov     edi,target
        mov     esi,buff
        mov     ebx,paleta
                        ;edi - target
                        ;esi - buff
                        ;ebx - paleta
        push    ebp             ;uchovej ebp		
		mov		eax,scr_linelen
		sub		eax,640
		push	eax
        mov     cl,180          ;cl pocet zbyvajicich radek
        add     esi,4           ;preskoc ukazatel
        mov     edx,esi         ;edx - zacatek delta mapy
        add     esi,[esi-4]     ;esi - zacatek dat
shdl6: push    edi             ;uloz adresu radku
shdl2: mov     ch,[edx]        ;cti _skip_ hodnotu
        mov     al,ch
        inc     edx
        or      al,03fh         ;test zda jsou 2 nejvyssi bity nastaveny
        inc     al
        jz      shdl3          ;ano - preskakovani radku
        movzx   eax,ch          ;expanduj _skip_ hodnotu do eax
        lea     edi,[eax*4+edi] ;vypocti novou pozici na obrazovce
        mov     ch,[edx]        ;cti _copy_ hodnotu
        inc     edx
shdl1: lodsw
        movzx   ebp,al
        movzx   ebp,short ptr ds:[ebp*2+ebx]
        movzx   eax,ah
        movzx   eax,short ptr ds:[eax*2+ebx]
        shl     eax,16
        or      eax,ebp
		mov		ebp,eax
		and		ebp,0x7fe07fe0
		add		eax,ebp
        stosd
        dec     ch              ;odecti _copy_ hodnotu
        jnz     shdl1          ;dokud neni 0
        jmp     shdl2          ;pokracuj _skip_ hodnotou
shdl3: and     ch,3fh          ;odmaskuj hodni 2 bity
        pop     edi             ;obnov edi
        jnz     shdl4          ;pokud je ch=0 preskoc jen jeden radek;
        add     edi,scr_linelen ;preskoc radek
        dec     cl              ;odecti citac radku
        jnz     shdl6          ;skok pokud neni konec
        jmp    shdl5
shdl4: inc     ch              ;pocet radek je ch+1
        sub     cl,ch           ;odecti ch od zbyvajicich radek
        jz      shdl5          ;je-li nula tak konec
shdl7: add     edi,scr_linelen  ;preskoc radek
        dec     ch              ;odecti ch
        jnz     shdl7          ;preskakuj dokud neni 0
        jmp     shdl6          ;cti dalsi _skip_
shdl5:	pop		eax		
		pop     ebp
    }
  }

char test_next_frame(void *bufpos,int size)
  {
  return 0;
  }
//#pragma aux test_next_frame parm [edi][ecx] modify [ebx] value [al]

void *sound_decompress(void *source,void *bufpos,int size,void *ampl_tab)
  {
  return NULL;
  }
//#pragma aux sound_decompress parm [esi][edi][ecx][ebx] modify [eax edx] value [edi]
