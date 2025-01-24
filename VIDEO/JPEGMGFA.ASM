.model small
.386
;16 bit line (de)comprimator
;
DGROUP group _DATA

extrn   _cos_tab:word;

extrn   _line_skip:dword
extrn   _xlat_table:dword

_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

ADDC    macro   REG1,REG2,CLIP
        local   addc1
        add     REG1,REG2
        jno     addc1
        shl     REG2,1
        Mov     REG2,CLIP
        adc     REG2,0
        mov     REG1,REG2
addc1:
        endm

;cos_tab maji format:

; adresa = pqii (hex)
;   p=pozice bodu (0-15 rozdeleno na yyxx)
;   q=pozice harmonicke (0-15 je rozdeleno podle code_tabs)
;   i=amplituda harmonicke(0-255);
;
;   hodnoty jsou FIXED real (reprezentovane v short)
;   vysledek je nutne podelit 32;


unpack_quant macro
        local   upqlp1
                        ;ah - point
                        ;vraci dh - bod
                        ;cl - block_size
                        ;esi - quant_list
        lea     ebx,_cos_tab;vem adresu _cos_tab
        shl     ah,4            ;presun pozici bodu do mista p
        and     eax,0xf000h     ;odmaskuj zbytek
        xor     ebp,ebp         ;vynuluj akumulator (ebp)
upqlp1: lodsb                   ;vem hodnotu harmonicke na pozici ii
        add     ebp,[ebx+eax*4];pricti k bp obsah v cos_tab
        add     eax,0100h       ;zvys q
        dec     cl              ;kontrola konce retezce
        jnz     upqlp1
        sar     ebp,9          ;vysledek podel 128 (32*4)
        mov     eax,ebp         ;vysledek je v al
endm

public  unpack_block_
unpack_block_:          ;esi - data
                        ;edi - out
                        ;ecx - size
        push    ebp     ;uchovej ebp
        mov     ch,cl   ;uchovej velikost retezce v ch
        mov     dl,0    ;dl je pozice bodu
        mov     dh,16   ;dh je pocet bodu
upblck1:push    esi     ;uchovej esi
        mov     ah,dl   ;napln ah pozici bodu
        mov     cl,ch   ;napln cl delko
        unpack_quant    ;volej makro pro vypocet bodu
        pop     esi     ;obnov esi
        add     [edi],al;zapis vysledny bod (vysetruj jako rozdil od predchoziho)
        inc     edi
        inc     dl      ;presun se na dalsi pozici
        dec     dh      ;sniz citac
        jnz     upblck1 ;opakuj
        pop     ebp     ;obnov ebp
        ret

public konv_iyg_hicolor_ ;esi - block i
                         ;edx - block r+b
                         ;ebx - block g+b
                         ;edi - out(screen) (hicolor xRRRRRGGGGGBBBBB
konv_iyg_hicolor_:
        push    ebp
        mov     ebp,ebx
        lea     ebx,_xlat_table
        mov     cl,16           ;celkem ctverec 4x4
kiyglp1:mov     ch,[esi]        ;ch obsahuje intenzitu
        mov     al,ch           ;zkopiruj ch do registru pro R G (ch je B)
        mov     ah,ch
        sub     ah,[edx]        ;vypocet R (I-(R+B))  R+B=[ebp]
        sub     ch,[ds:ebp]        ;vypocet G
        sub     al,ah           ;vypocet B (I-R-G)
        sub     al,ch
        xlatb
        xchg    ch,al
        xlatb
        xchg    al,ah
        xlatb
        xchg    al,ah
        shl     al,3            ;al<<3 000RRRRRGGGGG000  eax
        shl     eax,2           ;ax<<2 0RRRRRGGGGG00000  eax
        or      al,ch           ;or ch 0RRRRRGGGGGBBBBB  eax
        stosw                   ;zapis dva body
        stosw
        inc     esi             ;zvys indexy do bufferu
        inc     edx
        inc     ebp
        dec     cl              ;sniz citac
        test    cl,3            ;test na dolni 2 bity, pri 0 jdeme na dalsi radek
        jnz     kiyglp1
        mov     eax,_line_skip  ;delka obrazovky
        sub     eax,16          ;odecti celkem 4*2*2=16 byte
        add     edi,eax         ;pricti k ukazateli na obrazovku
        or      cl,cl           ;test na cl==0
        jnz     kiyglp1         ;kdyz je pozitivni tak konec
        pop     ebp
        ret
_TEXT   ends
end
