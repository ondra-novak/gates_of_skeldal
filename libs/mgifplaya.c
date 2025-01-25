#include <skeldal_win.h>
#include "types.h"
#include <mgfplay.h>
#include <bgraph.h>


void show_full_lfb12e(void *target, void *buff, void *paleta) {
    uint16_t *edi = (uint16_t *)target;
    uint8_t *esi = (uint8_t *)buff;
    uint16_t *ebx = (uint16_t *)paleta;
    uint8_t dl = 180;

    do {
        uint32_t ecx = 320;
        do {
            uint8_t value = *esi++;
            *edi++ = ebx[value];
            ecx--;
        } while (ecx != 0);
        dl--;
    } while (dl != 0);
}


/*
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
*/
/*
void show_delta_lfb12e(void *target, void *buff, void *paleta) {
    uint8_t *edi = (uint8_t *)target;
    uint8_t *esi = (uint8_t *)buff + 4; // skip pointer
    uint8_t *ebx = (uint8_t *)paleta;
    uint8_t cl = 180; // remaining lines
    uint8_t ch, al;
    uint16_t *edx = (uint16_t *)esi; // start of delta map
    esi += *(uint32_t *)(esi - 4); // start of data

    while (cl > 0) {
        uint8_t *line_address = edi; // save line address
        ch = *edx++; // read _skip_ value
        al = ch;
        al |= 0x3F; // test if the two highest bits are set
        al++;
        if (al == 0) { // if yes - skip lines
            ch &= 0x3F; // mask the upper 2 bits
            edi += (ch == 0) ? 640 : 0; // skip line if ch is 0
            cl--; // decrement line counter
            continue; // continue to next iteration
        }

        uint32_t eax = (uint32_t)ch; // expand _skip_ value to eax
        edi += eax * 4; // calculate new position on screen
        ch = *edx++; // read _copy_ value

        for (uint8_t i = 0; i < ch; i++) {
            uint16_t ebp = *(uint16_t *)(ebx + (al * 2)); // get color from palette
            uint16_t color = *(uint16_t *)(ebx + (eax * 2)); // get color from palette
            eax = (color << 16) | ebp; // combine colors
            *(uint32_t *)edi = eax; // store the color
            edi += 4; // move to the next pixel
        }
        edi += 640; // skip line
        cl--; // decrement line counter
    }
}
*/
static __inline uint32_t read_uint32(uint8_t *from) {
    uint32_t a = from[0];
    uint32_t b = from[1];
    uint32_t c = from[2];
    uint32_t d = from[2];
    return a | (b << 8) | (c << 16) | (d << 24);
}

//#pragma aux show_full_lfb12e parm[edi][esi][ebx] modify [eax ecx]
void show_delta_lfb12e(void *target,void *buff,void *paleta)
 {
//  __asm
//    {
    uint16_t *edi = target;
//        mov     edi,target
    uint8_t  *esi = buff;
//        mov     esi,buff
    uint16_t *ebx = paleta;
//        mov     ebx,paleta
//                        ;edi - target
//                        ;esi - buff
//                        ;ebx - paleta
//        push    ebp             ;uchovej ebp
    uint8_t  cl = 180;
//        mov     cl,180          ;cl pocet zbyvajicich radek
    uint32_t offset = read_uint32(esi);
//        add     esi,4           ;preskoc ukazatel
    esi += 4;
    uint8_t *edx = esi;
//        mov     edx,esi         ;edx - zacatek delta mapy
    uint8_t *data = (esi + offset);
//        add     esi,[esi-4]     ;esi - zacatek dat
    uint16_t *line_beg = edi;
//shdl6: push    edi             ;uloz adresu radku
    while (cl>0) {
        uint8_t ch = *edx++;
//shdl2: mov     ch,[edx]        ;cti _skip_ hodnotu
//        mov     al,ch
//        inc     edx
//        or      al,03fh         ;test zda jsou 2 nejvyssi bity nastaveny
//        inc     al
        if ((ch & 0xC0) == 0) {
//        jz      shdl3          ;ano - preskakovani radku
            edi += ((uint32_t)ch) << 1;
//        movzx   eax,ch          ;expanduj _skip_ hodnotu do eax
//        lea     edi,[eax*4+edi] ;vypocti novou pozici na obrazovce
            ch = *edx++;
//        mov     ch,[edx]        ;cti _copy_ hodnotu
//        inc     edx

            while (ch) {
                uint8_t a = *data++;
                *edi++ = ebx[a];
                a = *data++;
                *edi++ = ebx[a];
                --ch;
            }

//shdl1: lodsw
//        movzx   ebp,al
//        movzx   ebp,short ptr ds:[ebp*2+ebx]
//        movzx   eax,ah
//        movzx   eax,short ptr ds:[eax*2+ebx]
//        shl     eax,16
//        or      eax,ebp
//        stosd
//        dec     ch              ;odecti _copy_ hodnotu
//        jnz     shdl1          ;dokud neni 0
//        jmp     shdl2          ;pokracuj _skip_ hodnotou
        } else {
            ch &= 0x3F;
            ++ch;
            if (ch > cl) ch = cl;
            line_beg += 320*ch;
            cl -= ch;
            edi = line_beg;
        }

//shdl3: and     ch,3fh          ;odmaskuj hodni 2 bity
//        pop     edi             ;obnov edi
//        jnz     shdl4          ;pokud je ch=0 preskoc jen jeden radek;
//        add     edi,640        ;preskoc radek
//        dec     cl              ;odecti citac radku
//        jnz     shdl6          ;skok pokud neni konec
//        pop     ebp
//        jmp    konec
//shdl4: inc     ch              ;pocet radek je ch+1
//        sub     cl,ch           ;odecti ch od zbyvajicich radek
//        jz      shdl5          ;je-li nula tak konec
//shdl7: add     edi,640        ;preskoc radek
//        dec     ch              ;odecti ch
//        jnz     shdl7          ;preskakuj dokud neni 0
//        jmp     shdl6          ;cti dalsi _skip_
//shdl5: pop     ebp
    }
//konec:
//    }
//  }
 }
//#pragma aux show_delta_lfb12e parm[edi][esi][ebx] modify [eax ecx]

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
