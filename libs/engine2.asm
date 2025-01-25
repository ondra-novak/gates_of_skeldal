.model small
.386

DGROUP group _DATA

tzoom struc
        startptr DD ? ;0
        texture  DD ? ;4
        textline DD ? ;8
        linelen  DD ? ;12
        xtable   DD ? ;16
        ytable   DD ? ;20
        palette  DD ? ;24
        ycount   DW ? ;28
        xmax     DW ?
tzoom  ends

extrn _zoom:dword [8]
extrn _lbuffer:dword
extrn _screen:dword


_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

        public sikma_zleva_
sikma_zleva_:
        mov     edi,_zoom       ;nacti ukazatel do obrazovky
        mov     ebx,_zoom[offset tzoom.palette] ;ukazatel na paletu
        mov     cx,word ptr _zoom[offset tzoom.ycount] ;velikost textury na y
        shl     ecx,16  ;vloz do horni pulky ecx
        mov     esi,_zoom[offset tzoom.texture] ;nacti ukazatel na texturu
skzl3:  mov     edx,_zoom[offset tzoom.xtable] ;nacti ukazetel na zvetsovaci tabulku x
        push    esi ;uchovej esi
        push    edi ;uchovej edi
        mov     cx,_zoom[offset tzoom.xmax]
skzl1:  xor     eax,eax ;vynuluj eax pro spravny vypocet
        lodsb       ;nacti bod
        add     esi,[edx] ;posun se od nekolik pozic v texture podle hodnoty v tabulce x
        add     edx,4 ;posun se v tabulce x o dalsi polozku
        or      al,al ;test bodu na nulu
        jz      skz1  ;preskoc transparetni barvu
        cmp     al,1  ;test bodu na jedna
        jz      skz2  ;ukonci kresleni linky pokud narazi na 1
        mov     ax,[eax*2+ebx] ;konverze barvy podle palety
        mov     [edi],ax ;nakresli bod na obrazovce
skz1:   add     edi,2 ;dalsi pozice
        dec     cx
        jnz     skzl1 ;opakuj dokola
skz2:   pop     edi ;obnov edi
        pop     esi ;obnov esi
        mov     edx,_zoom[offset tzoom.ytable] ;vyzvedni ukazatel na ytable
        mov     cx,[edx] ;cx - o kolik pozic se mam v texture posunout dolu
        or      cx,cx
        jz      skzskp
skzl2:  add     esi,_zoom[offset tzoom.textline] ;posun o jednu pozici
        dec     cx ;sniz citac
        jnz     skzl2 ;dokud neni nula
skzskp:add     edx,2 ;dalsi hodnota v tabulce
        mov     _zoom[offset tzoom.ytable],edx ;uloaz na puvodni misto
        sub     edi,_zoom[offset tzoom.linelen] ;odecti tolik, kolik odpovida lince na obrazovce
        sub     ecx,10000h ;sniz horni pulku ecx o jedna
        jnz     skzl3 ;opakuj dokud neni nula
        ret

        public sikma_zprava_
sikma_zprava_:
        mov     edi,_zoom       ;nacti ukazatel do obrazovky
        mov     ebx,_zoom[offset tzoom.palette] ;ukazatel na paletu
        mov     cx,word ptr _zoom[offset tzoom.ycount] ;velikost textury na y
        shl     ecx,16  ;vloz do horni pulky ecx
        mov     esi,_zoom[offset tzoom.texture] ;nacti ukazatel na texturu
skzp3:  mov     edx,_zoom[offset tzoom.xtable] ;nacti ukazetel na zvetsovaci tabulku x
        push    esi ;uchovej esi
        push    edi ;uchovej edi
        mov     cx,_zoom[offset tzoom.xmax]
skzp1:  xor     eax,eax ;vynuluj eax pro spravny vypocet
        lodsb       ;nacti bod
        add     esi,[edx] ;posun se od nekolik pozic v texture podle hodnoty v tabulce x
        add     edx,4 ;posun se v tabulce x o dalsi polozku
        or      al,al ;test bodu na nulu
        jz      skz3  ;preskoc transparetni barvu
        cmp     al,1  ;test bodu na jedna
        jz      skz4  ;ukonci kresleni linky pokud narazi na 1
        mov     ax,[eax*2+ebx] ;konverze barvy podle palety
        mov     [edi],ax ;nakresli bod na obrazovce
skz3:   sub     edi,2 ;dalsi pozice
        dec     cx
        jnz     skzp1 ;opakuj dokola
skz4:   pop     edi ;obnov edi
        pop     esi ;obnov esi
        mov     edx,_zoom[offset tzoom.ytable] ;vyzvedni ukazatel na ytable
        mov     cx,[edx] ;cx - o kolik pozic se mam v texture posunout dolu
        or      cx,cx
        jz      skpskp
skzp2:  add     esi,_zoom[offset tzoom.textline] ;posun o jednu pozici
        dec     cx ;sniz citac
        jnz     skzp2 ;dokud neni nula
skpskp: add     edx,2 ;dalsi hodnota v tabulce
        mov     _zoom[offset tzoom.ytable],edx ;uloaz na puvodni misto
        sub     edi,_zoom[offset tzoom.linelen] ;odecti tolik, kolik odpovida lince na obrazovce
        sub     ecx,10000h ;sniz horni pulku ecx o jedna
        jnz     skzp3 ;opakuj dokud neni nula
        ret


        public zooming32_
zooming32_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
        push    ebp
        mov     bp,cx
        mov     ebx,_zoom[offset tzoom.ytable]
z32d:   mov     cx,bp
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
z32c:   mov     al,[edx]
        inc     edx
        or      al,al
        jz      z32a
        lodsw
        stosw
        stosw
        jmp     z32b
z32a:   movsd
z32b:   dec     cx
        jnz     z32c
        pop     esi
        mov     eax,[ebx]
        and     eax,0ffffh
        add     esi,eax
        add     ebx,2
        add     edi,_zoom[offset tzoom.textline]
        xor     cx,cx
        sub     ecx,10000h
        jnz     z32d
        pop     ebp
        ret

        public zooming_lo_
zooming_lo_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;ebx xlat
        push    ebp
        mov     bp,cx
zlod:   mov     cx,bp
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
zloc:   xor     eax,eax
        mov     al,[edx]
        inc     edx
        or      al,al
        jz      zloa
        lodsw
        mov     eax,[ebx+eax*2]
        stosb
        jmp     zlob
zloa:   lodsw
        mov     eax,[ebx+eax*2]
        stosb
        add     esi,2
zlob:   dec     cx
        jnz     zloc
        pop     esi
        mov     edx,_zoom[offset tzoom.ytable]
        mov     eax,[edx]
        shl     eax,1
        and     eax,0ffffh
        add     esi,eax
        add     edx,2
        mov     _zoom[offset tzoom.ytable],edx
        add     edi,_zoom[offset tzoom.textline]
        xor     cx,cx
        sub     ecx,20000h
        jnz     zlod
        pop     ebp
        ret

        public zooming256_
zooming256_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;ebx xlat
        push    ebp
        mov     bp,cx
z256d:  mov     cx,bp
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
z256c:  xor     eax,eax
        mov     al,[edx]
        inc     edx
        or      al,al
        jz      z256a
        lodsw
        mov     eax,[ebx+eax*2]
        stosb
        stosb
        jmp     z256b
z256a:  lodsw
        mov     eax,[ebx+eax*2]
        stosb
        xor     eax,eax
        lodsw
        mov     eax,[ebx+eax*2]
        stosb
z256b:  dec     cx
        jnz     z256c
        pop     esi
        mov     edx,_zoom[offset tzoom.ytable]
        mov     eax,[edx]
        and     eax,0ffffh
        add     esi,eax
        add     edx,2
        mov     _zoom[offset tzoom.ytable],edx
        add     edi,_zoom[offset tzoom.textline]
        xor     cx,cx
        sub     ecx,10000h
        jnz     z256d
        pop     ebp
        ret

;        public  scroll_left_
;scroll_left_:           ;edi - kam
;                        ;lbuffer - obrazovka
;                        ;zoom.startptr - novy obsah obrazovky
;                        ;ebx - xlat
;                        ;eax - o_kolik
;        mov     bx,ax           ;horni pulka ebx vi o kolik se posunuje vlevo
;        rol     ebx,16
;        mov     edx,640         ;dolni pulka ebx vi, jak velky blok je presouvan
;        sub     edx,eax
;        sub     edx,2
 ;       mov     bx,dx
 ;       mov     esi,edi         ;vypocet esi
 ;       shl     eax,1           ;esi = edi + 2 * o_kolik;
 ;       add     esi,eax
 ;       mov     edx,360         ;napln citac edx cislem udavajici pocet radku
 ;       add     edi,_lbuffer    ;k edi na zacatku pricti hodnotu _lbuffer
 ;       mov     eax,esi         ;uchovej esi v eax, behem prenosu bude modifikovan
;scrl1:  add     esi,_lbuffer    ;pricti k esi zacatek obrazovky
;        xor     ecx,ecx         ;vynuluj ecx
;        mov     cx,bx           ;do ecx naladuj delku bloku
;        shr     ecx,1           ;presun 32-bit
;        rep movsd
;        adc     ecx,1
;;        rep movsw
;        mov     esi,_zoom       ;vem ukazatel na novy obsah
;        mov     ecx,ebx         ;vezmi horni pulku EBx
;        shr     ecx,16          ;to je hodnota, kolik se ma v prava doplnit
;        shr     ecx,1           ;presun 32-bit
;        rep movsd
 ;       adc     ecx,1
 ;       rep movsw
 ;       add     _zoom,640*2      ;dalsi radka
 ;;       add     eax,640*2        ;dalsi radka
;        mov     esi,eax
;        dec     edx             ;dokud neni konec
 ;       jnz     scrl1
 ;       ret
 ;

public  scroll_support_32_
scroll_support_32_:
                        ;edi - lbuffer + pozice na obrazovce
                        ;esi - oldbuffer
                        ;edx - newbuffer
                        ;ebx - xlat
                        ;ecx - size;
        push    ebp             ;uchovej ebp
        mov     ebp,360         ;ebp pro tuto chvili predstavuje citac
        mov     eax,ecx         ;uchovej ecx jeste v eac - zachova citac sloupcu
scrl1:  push    esi             ;uchovej esi
        shr     ecx,1           ;presun ecx bloku
        rep     movsd
        rcl     ecx,1
        rep     movsw
        mov     ecx,640         ;dopocitej ecx do 640
        sub     ecx,eax
        pop     esi             ;obnov esi
        xchg    esi,edx         ;esi je nyni novy blok
        push    esi             ;uchovek esi
        shr     ecx,1           ;presun
        rep     movsd
        rcl     ecx,1
        rep     movsw
        pop     esi             ;obnov esi
        xchg    esi,edx         ;vrat edx a esi do puvodniho stavu
        mov     ecx,eax         ;obnov zase ecx z eax
        add     esi,1280        ;dalsi radek
        add     edx,1280
        dec     ebp             ; dokud nejsme na konci
        jnz     scrl1           ;opakuj
        pop     ebp
        ret

public  scroll_support_256_
scroll_support_256_:
                        ;edi - lbuffer + pozice na obrazovce
                        ;esi - oldbuffer
                        ;edx - newbuffer
                        ;ebx - xlat
                        ;ecx - size;
        push    ebp             ;uchovej ebp
        mov     ebp,360         ;ebp pro tuto chvili predstavuje citac
scrl1a: push    ecx         ;uchovej ecx jeste v eac - zachova citac sloupcu
        push    esi             ;uchovej esi
        xor     eax,eax
scrl2a: lodsw
        mov     al,[ebx+2*eax]
        xor     ebx,1
        stosb
        dec     ecx
        jnz     scrl2a
        pop     esi
        xchg    esi,edx         ;esi je nyni novy blok
        pop     eax
        mov     ecx,640         ;dopocitej ecx do 640
        sub     ecx,eax
        push    eax
        push    esi             ;uchovek esi
        xor     eax,eax
scrl3a: lodsw
        mov     al,[ebx+2*eax]
        xor     ebx,1
        stosb
        dec     ecx
        jnz     scrl3a
        pop     esi             ;obnov esi
        xchg    esi,edx         ;vrat edx a esi do puvodniho stavu
        pop     ecx             ;obnov zase ecx z eax
        add     esi,1280        ;dalsi radek
        add     edx,1280
        xor     ebx,1
        dec     ebp             ; dokud nejsme na konci
        jnz     scrl1a          ;opakuj
        pop     ebp
        ret


public fcdraw_                  ;Kresli strop nebo podlahu podle draw_table
                                ;EDX - sourceTxt
                                ;EBX - TargerTxt - LineOfset
                                ;  (Lineofs je pocet bajtu odpovidajici
                                ;  souradnicim [0,184] pro podlahu nebo [0,0]
                                ; pro strop)
                                ;EAX - draw_table
fcdraw_:mov     esi,[eax]
        mov     edi,esi
        add     edi,ebx
        add     esi,edx
        mov     ecx,[eax+4]
        shr     ecx,1
        rep     movsd
        rcl     ecx,1
        rep     movsw
        mov     ecx,[eax+8]
        add     eax,12
        or      ecx,ecx
        jnz     fcdraw_
        ret

_TEXT  ends
END
