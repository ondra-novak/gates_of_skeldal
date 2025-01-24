.model small
.386
;16 bit line (de)comprimator
;
_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP



cwrite    macro REG,BIT        ;zapise al do [di] pouze BIT bitu ze zadu
          local cwrit2
          shl   edx,BIT        ;posun EDX o pripraveny pocet  bitu
          and   REG,0ffh shr (8-BIT) ;odmaskuj pripadne bity v REG
          or    dl,REG         ;pridej REG do dl
          add   ch,BIT         ;pripocti pridany pocet bitu
          cmp   ch,8           ;jestli to preteklo
          jc    cwrit2         ;ne, vse je OK
          sub   ch,8           ;ano pak odecti 8 bitu
          mov   cl,8           ;cl=8-ch;
          sub   cl,ch          ;
          shl   edx,cl         ;tj kolik zbejva posunout.
          mov   [edi],dh       ;zapis osmice
          shr   dl,cl          ;vrat dl do puvodniho stavu
          inc   edi            ;dalsi adresa
cwrit2:
endm

cwrit8    macro REG            ;zapise 8 bitu do [EDI]
          shl   edx,8          ;posun 8 bitu
          or    dl,REG         ;sfuzuj
          mov   cl,8           ;kolik urcite preteklo
          sub   cl,ch          ;a o kolik teda posunout
          shl   edx,cl         ;posun na dorovnani na bajt
          mov   [edi],dh       ;zapis bajt
          shr   dl,cl          ;spodek vrat do puvodniho stavu.
          inc   edi            ;dalsi adresa
endm

write_ax: ;zapisuje AX do [edi] v komprimovanem tvaru
          ;hodnoty
          ; 0 ...  000
          ; 1 ...  001
          ; 2 ...  010
          ; 3 ...  011
          ; 4 ...  100
          ; 5 ...  101
          ; 6 ...  110
          ; 7 ...  11100000
          ; 8 ...  11100001
          ; 9 ...  11100010
          ;10 ...
          ;...
          ;37 ...  11111110
          ;38 ...  1111111100000000
          ;39 ...  1111111100000001
          ;
          ;291 ..  1111111111111101
          ;292 ..  1111111111111110
          ;>293 .. 1111111111111111 Lo Hi

          cmp   eax,293        ;eax>293 (7+31+255)
          jnc   wr_16          ;zapis 16 bitove hodnoty
          cmp   al,38          ;eax>38 (7+31)
          jnc   wr_8           ;zapis 8 bitove hodnoty
          cmp   al,7           ;eax>7
          jnc   wr_5           ;zapis 5 bitove hodnoty
          cwrite al,3             ;pro cisla eax<7 (0..6) staci 3 bity
          ret                  ;konec
wr_16:    sub   eax,293        ;odecti to co je zname
          mov   cl,0ffh        ;zapis dvou hodnot 255 jako znacka ze nasleduje
          cwrit8 cl            ;16 bitova hodnota
          mov   cl,0ffh
          cwrit8 cl
          cwrit8 al            ;zapis nizsi
          mov   al,ah
          cwrit8 ah            ;zapis vyssi
          ret
wr_5:     sub   al,7           ;odecti co zname
          or    al,0E0h        ;tri bity na 1 znaci 5 bitovou hodnotu
          cwrit8 al            ;zapis to
          ret
wr_8:     sub   al,38          ;odecti to co je uz zname.
          mov    cl,0ffh       ;zapis jedne hodnoty 255 znaci 8 bitovou hodnotu
          cwrit8 cl            ;zapis ji
          cwrit8 al            ;zapis al
          ret


;vsechny mody -
;  vstup: ESI - data
;         EDI - buffer
;         ECX - delka dat (max 65535)
;         EBX - bitova rovina (bit na pozici roviny = 1)

flush   macro
        local fjmp
        shr     ecx,16
        or      ch,ch
        jz      fjmp
        mov     eax,7
        call    write_ax
fjmp:
endm

public  cmode0_
cmode0_:  ;komprimuje pro mod 11110000 (uvazuji se retezce 00001 nebo 11110 pricemz
          ; 0000111110 se pocita vcetne rozdilu (
          ; \---/\---/
          ;   4    4   tj 100100


          push  ebp
          xor   edx,edx
          xor   ebp,ebp
          xor   eax,eax
cmode0b:  lodsw
          and   ax,bx
          cmp   ax,bp
          jnz   cmode0a
          add   eax,10000h
          dec   cx
          jnz   cmode0b
          shr   eax,16
          shr   ecx,16
          call  write_ax
          flush
          pop   ebp
          ret
cmode0a:  xor   ebp,ebx
          shr   eax,16
          rol   ecx,16
          call  write_ax
          rol   ecx,16
          dec   cx
          jnz   cmode0b
          flush
          pop   ebp
          ret

public  cmode1_

cmode1_:  ;komprimuje pro mod 00010001
          ;komprimuje se vcetne jednicky tj
          ;
          ;000100001
          ;\--/\---/
          ; 3    4   tj 011100
          xor   edx,edx
          xor   eax,eax
cmode1b:  lodsw
          and   ax,bx
          jnz   cmode1a
          add   eax,10000h
          dec   cx
          jnz   cmode1b
          shr   eax,16
          shr   ecx,16
          call  write_ax
          flush
          ret
cmode1a:  shr   eax,16
          rol   ecx,16
          call  write_ax
          rol   ecx,16
          dec   cx
          jnz   cmode1b
          flush
          ret

public  cmode2_

cmode2_:  ;komprimuje pro mod 11101110
          ;komprimuje se vcetne 0 tj
          ;
          ;111101011110111110
          ;\---/\/\---/\----/
          ;  4   1  4    4    tj 100001100100
          xor   edx,edx
          xor   eax,eax
cmode2b:  lodsw
          and   ax,bx
          jz    cmode2a
          add   eax,10000h
          dec   cx
          jnz   cmode2b
          shr   eax,16
          shr   ecx,16
          call  write_ax
          flush
          ret
cmode2a:  shr   eax,16
          rol   ecx,16
          call  write_ax
          rol   ecx,16
          dec   cx
          jnz   cmode2b
          flush
          ret

public cmode3_
cmode3_:   ;kopiruje rovinu
          mov   edx,8           ;8 bitu
cmode3b:  lodsw                 ;cti slovo
          and   ax,bx           ;kontroluj rovinu
          cmp   ax,bx           ;nastav carry podle vysledku
          cmc                   ;ale spravne
          rcl   dh,1            ;soupni ho do dh
          dec   dl              ;dalsi pozice
          jnz   cmode3a         ;kdyz to neni vse pokracuj
          mov   [edi],dl        ;je to vse
          inc   edi             ;dalsi bajt
          mov   edx,8           ;dalsich 8 bitu
cmode3a:  dec   ecx             ;dalsi slovo
          jnz   cmode3b         ;dokud to neni vse.
          cmp   dl,8
          jz    cmode3c
          mov   cl,dl
          shl   dh,cl
          mov   [edi],dh
          inc   edi
cmode3c:  ret


; DEKOMPRIMACE

cread     macro REG,BIT
          local crd1
          mov   cl,BIT
          cmp   ch,cl                  ;zjisti zda pocet zbylych bitu
          jnc   crd1                  ;neni nahodou min nez potrebnych
          mov   cl,ch                   ;ano, tj posun ty zbyle do dh
          shl   edx,cl
          mov   dl,[esi]                ;do dl vloz dalsi bajt
          inc   esi                     ;a zvys adresu
          sub   cl,BIT                  ;odecti pocet pozadovanych bitu
          mov   ch,8                    ;to je aktualizovana hodnota noveho ch
          neg   cl                      ;v cl je nyni mnozstvi chybejicich bitu
crd1:     shl   edx,cl                  ;prisun je do dh
          sub   ch,cl
          mov   REG,dh                  ;vem je do REG
          and   REG,0xff shr (8-BIT)    ;odmaskuj horejsek
endm

;vsechny mody -
;  vstup: ESI - data
;         EDI - vysledek
;         ECX - delka dat (max 65535)
;         EBX - bitova rovina (bit na pozici roviny = 1)

cread_ax macro
         local crd_ok
         rol   ecx,16          ;nejdriv schovej cx v horni pulce
         xor   eax,eax         ;vynuluj eax
         cread al,3            ;nacti 3 bity
         cmp   al,7            ;pokud to da hodnotu 7 pak pokracuj
         jnz   crd_ok          ;jinak jsem hotov
         cread al,5            ;nacti dalsi 5 bitu
         add   al,7            ;pricti ztracenych 7
         cmp   al,38           ;pokracuj kdyz to da dohromady 38
         jnz   crd_ok
         cread al,8            ;nacti 8 bitu
         add   eax,38           ;a pricti 38
         cmp   eax,293          ;pokracuj kdyz to da 293
         jnz   crd_ok
         cread al,8            ;precti dalsi 8 bitu
         cread ah,8            ;Nejdriv dolni a pak horni
         add   eax,293
crd_ok:  rol   ecx,16          ;obnov cx

endm

public cread0_
cread0_: and   ecx,0ffffh     ;vynuluj horni pulku ecx
         xor   edx,edx        ;vynuluj pamet bitu
         cread_ax             ;precti prvni zaznam
         or    eax,eax        ;je li 0 zaciname jednickami
         jnz   cread0a
cread0c: cread_ax             ;precti zaznam
         inc   eax            ;+1
cread0b: xor   [edi],bx       ;invertuj rovinu
         add   edi,2
         dec   ecx            ;odecitej ecx
         dec   eax            ;odecti citac
         jnz   cread0b        ;dokud neni ax=0
         or    cx,cx          ;kontrola zda nejsme na konci
         jz    cread0e        ;kdyz ne
         cread_ax             ;precti dalsi zaznam
         inc   eax            ;+1
cread0a: sub   cx,ax          ;ted se jenom preskoci
         jz    cread0e        ;ale kdyz jsme na konci tak nic nedelej
         shl   eax,1          ;preskoci 2*eax bajtu
         add   edi,eax        ;skok!
         jmp   cread0c        ;a jedem znova.
cread0e: ret

public cread1_
cread1_: and   ecx,0ffffh
         xor   edx,edx
cread1a: cread_ax             ;cti zaznam
         add   edi,eax        ;preskoc EAX bajtu a zapis 1
         xor   [edi],ebx
         add   edi,2
         dec   ecx            ;jedna za jeden bod
         sub   cx,ax          ;a jeste ax bajtu
         jnz   cread1a        ;jeste to neni vse?
         ret

public cread2_
cread2_: and   ecx,0ffffh
         xor   edx,edx
cread2a: cread_ax             ;cti zaznam
         add   edi,eax        ;zapis EAX bajtu a preskoc 1
cread2b: xor   [edi],ebx
         add   edi,2
         dec   ecx            ;jedna za jeden bod
         dec   eax
         jnz   cread2b
         add   edi,2
         dec   ecx
         jnz   cread2a
         ret

public cread3_
cread3_: mov    dh,8            ;dh - citac bitu
         mov    dl,[esi]        ;vezmi prvni osmici
         inc    esi             ;zvys adresu
cread3b: shl    dl,1            ;nacti bit do cf
         sbb    eax,eax         ;je-li nastaven je vysledek 0xffffffff
         and    eax,ebx         ;nastav prislusnou rovinu
         xor    [edi],ax        ;proved XOR
         add    edi,2           ;dalsi bod
         dec    ecx             ;odecti si ho
         jz     cread3a         ;kdyz uz jsme na konci skok na cread3a
         dec    dh              ;odecti citac bitu
         jz     cread3_         ;jakmile klesne na nulu zacni od zacatku
         jmp    cread3b         ;jinak opakuj.
cread3a: ret

_TEXT  ends

END
