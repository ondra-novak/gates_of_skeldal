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
extrn _gran_mask:dword
extrn _lastbank:dword

_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

extrn mapvesaadr_:proc
extrn switchmap_:proc
extrn switchvesabank_:proc

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
                        ;eax - pozadi
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
        push    ebp
        mov     ebp,eax
        mov     ebx,_zoom[offset tzoom.ytable]
z32d:   push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
z32c:   mov     al,[edx]
        inc     edx
        or      al,al
        jz      z32a
        lodsw
        test    ax,8000h
        jnz      z32e
        stosw
        stosw
        jmp     z32b
z32f:   test    eax,8000h
        jz      z32g
        test    eax,80000000h
        jz      z32h
z32e:   mov     eax,[ds:ebp]
        stosd
        jmp     z32b
z32g:   rol     eax,16
        mov     ax,[ds:ebp+2]
        rol     eax,16
        stosd
        jmp     z32b
z32h:   mov     ax,[ds:ebp]
        stosd
        jmp     z32b
z32a:   lodsd
        test    eax,80008000h
        jnz      z32f
        stosd
z32b:   add     ebp,4
        dec     cx
        jnz     z32c
        pop     esi
        mov     eax,[ebx]
        and     eax,0ffffh
        add     esi,eax
        add     ebx,2
        add     edi,_zoom[offset tzoom.textline]
        xor     cx,cx
        sub     ecx,10000h
        pop     cx
        jnz     z32d
        pop     ebp
        ret

        public zooming32b_
zooming32b_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;zoom.xtable - tabulka pro x
                        ;zoom.ytable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;zoom.ycount - interni (ulozeni maskovane adr stranky)
                        ;zoom.xmax - interni (pocet dwordu do prepnuti)
                        ;zoom.linelen - interni (velikost stranky v dwordech)


        call    mapvesaadr_     ;mapuj spravne edi
        push    ebp
        mov     ebp,eax
        mov     ebx,_zoom[offset tzoom.ytable]
z32bd:  push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
        push    edi
z32bc:  mov     al,[edx]
        inc     edx
        or      al,al
        jz      z32ba
        lodsw
        test    ax,8000h
        jnz      z32be
        stosw
        stosw
        jmp     z32bb
z32bf:  test    eax,8000h
        jz      z32bg
        test    eax,80000000h
        jz      z32bh
z32be:  mov     eax,[ds:ebp]
        stosd
        jmp     z32bb
z32bg:  rol     eax,16
        mov     ax,[ds:ebp+2]
        rol     eax,16
        stosd
        jmp     z32bb
z32bh:  mov     ax,[ds:ebp]
        stosd
        jmp     z32bb
z32ba:  lodsd
        test    eax,80008000h
        jnz      z32bf
        stosd
z32bb:  add     ebp,4
        movzx   eax,di                  ;prevezmi dolni cast edi do eax
        dec     eax                     ;odecti 1 (0=0xffffffff)
        cmp     eax,_gran_mask          ;je vetsi nez granualita?
        jc      z32gb                   ;ne, vse je ok
        mov     eax,_lastbank           ;ano, pak vem cislo banky
        inc     eax                     ;pricti 1
        mov     _lastbank,eax           ;zapis jako nove cislo banky
        pushad                          ;uchovej vse
        call    switchvesabank_         ;prepni banku
        popad                           ;obnov vse
        mov     edi,0a0000h             ;vynuluj dolni cast di
z32gb:  dec     cx                      ;tady pokracujem
        jnz     z32bc                   ;opakuj cxkrat
        pop     edi                     ;obnov edi
        add     di,1280                 ;pricti radek
        and     di,word ptr _gran_mask  ;maskuj granualitou
        pop     esi
        mov     eax,[ebx]
        and     eax,0ffffh
        add     esi,eax
        add     ebx,2
        xor     cx,cx
        sub     ecx,10000h
        pop     cx
        jnz     z32bd
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
                        ;eax - pozadi
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;ebx xlat
        push    ebp
        mov     ebp,eax
z256d:  push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
z256c:  xor     eax,eax
        mov     al,[edx]
        inc     edx
        or      al,al
        jz      z256a
        lodsw
        test    eax,8000h
        jnz     z256e
        mov     al,[ebx+eax*2]
        mov     ah,al
        stosw
        jmp     z256b
z256e:  mov     ax,[ds:ebp]
        and     eax,7fffh
        mov     al,[ebx+eax*2]
        stosb
        mov     ax,[ds:ebp+2]
        and     eax,7fffh
        mov     al,[ebx+eax*2]
        stosb
        jmp     z256b
z256g:  mov     ax,[ds:ebp]
        and     eax,7fffh
        mov     al,[ebx+eax*2]
        stosb
        jmp     z256h
z256i:  mov     ax,[ds:ebp+2]
        and     eax,7fffh
        mov     al,[ebx+eax*2]
        stosb
        jmp     z256b
z256a:  lodsw
        test    eax,8000h
        jnz     z256g
        mov     al,[ebx+eax*2]
        stosb
z256h:  lodsw
        test    eax,8000h
        jnz     z256i
        mov     al,[ebx+eax*2]
        stosb
z256b:  add     ebp,4
        xor     ebx,1
        dec     cx
        jnz     z256c
        xor     ebx,1
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
        pop     cx
        jnz     z256d
        pop     ebp
        ret

swap_at_end     macro   reg16,reg32
        local   sw1,sw2

sw2:    movzx   eax,reg16           ;nacti 16 bitovou cast registru
        dec     eax                 ;sniz ke kontrole o 1
        cmp     eax,_gran_mask      ;kontrola zda je vetsi nebo rovny masce
        jc      sw1                 ;kdyz ne tak preskoc na sw1
        mov     eax,_lastbank       ;vem cislo banky
        inc     eax                 ;pricti jednicku
        mov     _lastbank,eax       ;vloz zpatky do banky
        pushad
        call    switchvesabank_       ;prepni banku
        popad
        sub     reg32,_gran_mask    ;odecti delku stranky od cele adresy
        dec     reg32
sw1:
endm


        public zooming256b_
zooming256b_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;ebx xlat
        push    ebp
        mov     ebp,eax
z256bd: push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    edi
        call    mapvesaadr_
        push    esi
z256bc: xor     eax,eax
        mov     al,[edx]
        inc     edx
        or      al,al
        jz      z256ba
        lodsw
        test    eax,8000h
        jnz     z256be
        mov     al,[ebx+eax*2]
        mov     ah,al
        stosw
        jmp     z256bb
z256be: mov     ax,[ds:ebp]
        and     eax,7fffh
        mov     al,[ebx+eax*2]
        stosb
        mov     ax,[ds:ebp+2]
        and     eax,7fffh
        mov     al,[ebx+eax*2]
        stosb
        jmp     z256bb
z256bg: mov     ax,[ds:ebp]
        and     eax,7fffh
        mov     al,[ebx+eax*2]
        stosb
        jmp     z256bh
z256bi: mov     ax,[ds:ebp+2]
        and     eax,7fffh
        mov     al,[ebx+eax*2]
        stosb
        jmp     z256bb
z256ba: lodsw
        test    eax,8000h
        jnz     z256bg
        mov     al,[ebx+eax*2]
        stosb
z256bh: lodsw
        test    eax,8000h
        jnz     z256bi
        mov     al,[ebx+eax*2]
        stosb
z256bb: add     ebp,4
        swap_at_end di,edi
        xor     ebx,1
        dec     cx
        jnz     z256bc
        xor     ebx,1
        pop     esi
        pop     edi
        add     edi,640
        mov     edx,_zoom[offset tzoom.ytable]
        mov     eax,[edx]
        and     eax,0ffffh
        add     esi,eax
        add     edx,2
        mov     _zoom[offset tzoom.ytable],edx
        add     edi,_zoom[offset tzoom.textline]
        xor     cx,cx
        sub     ecx,10000h
        pop     cx
        jnz     z256bd
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

movsdx  macro           ;presun do videopameti s kontrolou na banky
        local   sw1,konec
        push    eax
        movzx   eax,di
        add     eax,ecx
        cmp     eax,_gran_mask
        ja      sw1
        shr     ecx,2
        rep     movsd
        jmp     konec
sw1:    mov     eax,_gran_mask
        sub     ax,di
        inc     eax
        sub     ecx,eax
        xchg    eax,ecx
        shr     ecx,2
        rep     movsd
        xchg    eax,ecx
        mov     eax,_lastbank
        inc     eax
        mov     _lastbank,eax
        pushad
        call    switchvesabank_
        popad
        mov     edi,0a0000h
        shr     ecx,2
        rep     movsd
konec:  pop     eax
endm
public  scroll_support_32b_
scroll_support_32b_:
                        ;edi - lbuffer + pozice na obrazovce /pokud mozno del 4
                        ;esi - oldbuffer
                        ;edx - newbuffer
                        ;ebx - xlat
                        ;ecx - size pokud mozno / 4;
        call    mapvesaadr_     ;mapuj spravne edi
        push    ebp             ;uchovej ebp
        mov     ebp,360         ;ebp pro tuto chvili predstavuje citac
        and     ecx,not 1
        shl     ecx,1           ;korekce ecx na delitele 4x
        mov     eax,ecx         ;uchovej ecx jeste v eax - zachova citac sloupcu
scrl1b: push    esi             ;uchovej esi
        movsdx                  ;presun ecx bajtu (jen ctverice)
        mov     ecx,1280         ;dopocitej ecx do 640
        sub     ecx,eax
        pop     esi             ;obnov esi
        xchg    esi,edx         ;esi je nyni novy blok
        push    esi             ;uchovek esi
        movsdx
        pop     esi             ;obnov esi
        xchg    esi,edx         ;vrat edx a esi do puvodniho stavu
        mov     ecx,eax         ;obnov zase ecx z eax
        add     esi,1280        ;dalsi radek
        add     edx,1280
        dec     ebp             ; dokud nejsme na konci
        jnz     scrl1b          ;opakuj
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
scrl2a: lodsw
        and     eax,7fffh
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
scrl3a: lodsw
        and     eax,7fffh
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

public  scroll_support_256b_
scroll_support_256b_:
                        ;edi - lbuffer + pozice na obrazovce
                        ;esi - oldbuffer
                        ;edx - newbuffer
                        ;ebx - xlat
                        ;ecx - size;
        call    mapvesaadr_
        push    ebp             ;uchovej ebp
        mov     eax,360         ;eax pro tuto chvili predstavuje citac
        mov     ebp,edx
scrl1ba:push    eax
        push    ecx             ;uchovej ecx jeste v eac - zachova citac sloupcu
        push    esi             ;uchovej esi
        xor     eax,eax
        shr     ecx,1
scrl2ba:lodsw
        and     eax,7fffh
        mov     dl,[ebx+2*eax]
        xor     ebx,1
        lodsw
        and     eax,7fffh
        mov     dh,[ebx+2*eax]
        xor     ebx,1
        mov     [edi],dx
        add     edi,2
        swap_at_end di,edi
        dec     ecx
        jnz     scrl2ba
        pop     esi
        xchg    esi,ebp         ;esi je nyni novy blok
        pop     eax
        mov     ecx,640         ;dopocitej ecx do 640
        sub     ecx,eax
        push    eax
        push    esi             ;uchovek esi
        xor     eax,eax
        shr     ecx,1
scrl3ba:lodsw
        and     eax,7fffh
        mov     dl,[ebx+2*eax]
        xor     ebx,1
        lodsw
        and     eax,7fffh
        mov     dh,[ebx+2*eax]
        xor     ebx,1
        mov     [edi],dx
        add     edi,2
        swap_at_end di,edi
        dec     ecx
        jnz     scrl3ba
        pop     esi             ;obnov esi
        xchg    esi,ebp         ;vrat ebp a esi do puvodniho stavu
        pop     ecx             ;obnov zase ecx z eax
        pop     eax
        add     esi,1280        ;dalsi radek
        add     ebp,1280
        xor     ebx,1
        dec     eax             ; dokud nejsme na konci
        jnz     scrl1ba          ;opakuj
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

public  put_image_      ;vlozi obrazek v hicolor
put_image_:             ;ESI - obrazek
                        ;EDI - obrazovka
                        ;EAX - startline
                        ;EBX - velikostx
                        ;EDX - velikosty
        shl     eax,1
        xor     ecx,ecx
        mov     cx,[esi]
        imul    eax,ecx
        add     esi,eax
        add     esi,6
        mov     eax,ecx
puti_lp:mov     ecx,ebx
        shr     ecx,1
        rep     movsd
        rcl     ecx,1
        rep     movsw
        mov     ecx,eax
        sub     ecx,ebx
        add     esi,ecx
        sub     edi,ebx
        sub     edi,ebx
        add     edi,1280
        dec     edx
        jnz     puti_lp
        ret

public  put_8bit_clipped_ ;vlozi 8bit obrazek omezen velikosti
put_8bit_clipped_:       ;ESI - obrazek
                         ;EDI - obrazovka
                         ;EAX - startline
                         ;EBX - velikostx
                         ;EDX - velikosty

        push    ebp
        mov     ebp,ebx
        xor     ecx,ecx
        mov     cx,[esi]
        imul    eax,ecx
        lea     ebx,[esi+6];
        add     esi,6+512
        add     esi,eax
        mov     eax,ecx
put8_lp:mov     ecx,ebp
        push    eax
put8lp2:xor     eax,eax
        lodsb
        or      eax,eax
        jz      put8_trns
        mov     eax,[ebx+eax*2]
        stosw
put8nxt:dec     ecx
        jnz     put8lp2
        pop     eax
        mov     ecx,eax
        sub     ecx,ebp
        add     esi,ecx
        sub     edi,ebp
        sub     edi,ebp
        add     edi,1280
        dec     edx
        jnz     put8_lp
        pop     ebp
        ret
put8_trns:
        add     edi,2
        jmp     put8nxt

public textmode_effekt_

textmode_effekt_:  ;genetuje effekt v textovem rezimu
        mov     edx,3c4h
        mov     eax,0402h
        out     dx,ax
        mov     eax,0704h
        out     dx,ax
        Mov     edx,3ceh
        mov     eax,0204h
        out     dx,ax
        mov     eax,0005h
        out     dx,ax
        mov     eax,0406h
        out     dx,ax
        mov     bl,8
        xor     bh,bh
tde2:   mov     edi,0xA0000h
        mov     ecx,32*256
tde1:   not     bh
        or      bh,bh
        jz      tde3
        shr     byte ptr [edi],1
        jmp     tde4
tde3:   shl     byte ptr [edi],1
tde4:   inc     edi
        dec     ecx
        jnz     tde1
        mov     edx,0x3da
        mov     bh,3
tdew2:  in      al,dx
        and     al,8
        jz     tdew2
tdew1:  in      al,dx
        and     al,8
        jnz     tdew1
        dec     bh
        jnz     tdew2
        dec     bl
        jnz     tde2
        mov     edx,3c4h
        mov     eax,0302h
	out	dx,ax
        mov     eax,0304h
	out	dx,ax
        Mov     edx,3ceh
        mov     eax,0004h
	out	dx,ax
        mov     eax,1005h
	out	dx,ax
        mov     eax,0E06h
	out	dx,ax
        ret

public  put_textured_bar__
                        ;zobrazi texturovany obdelnik tvoreny cyklickou texturou
                        ;vstup
                        ;    EDI - pozice na obrazovce
                        ;    EBX - textura
                        ;    ECX - velikosty
                        ;    EDX - velikostx
                        ;    EAX - yofs
                        ;    ESI - xofs

put_textured_bar__:
        push    ebp             ;uchovej EBP - bude pouzit jako univerzalni registr
        mov     ebp,eax         ;zacneme o EAX radku v texture niz
        shl     edx,16          ;nachvili uchovej dx v horni pulce
        mul     word ptr [ebx]  ;zjisti pocatecni adresu v texture
        shl     eax,16
        shrd    eax,edx,16      ;EAX=AX*word ptr[ebx]
        shr     edx,16
        xchg    esi,eax         ;napln esi pocatkem
        lea     esi,[esi+512]   ;vypocti zacatek v texture+xlat
        lea     ebx,[ebx+6]     ;postav ebx na zacatek xlat
        add     esi,ebx         ;pricti zacatek textury
        add     esi,eax         ;pricti k ukazateli ofset v x
ptb_l2: push    eax             ;uchovej xofset
        push    edx             ;uchovej velikost v x
        push    esi             ;uchovej zacatek radky v texture
        shl     ebp,16          ;odsun citac offset do horni pulky ebp
        add     bp,ax           ;pricti k bp offset v x
ptb_l1: xor     eax,eax         ;pred kazdym bodem vynuluj eax
        lodsb                   ;nacti barvu
        or      al,al
        jz      ptb_s1
        mov     ax,[ebx+eax*2];vyhledej barvu v palete
        mov     [edi],ax        ;zapis hicolor barvu
ptb_s1: add     edi,2
        inc     bp              ;zvys counter v x
        cmp     bp,[ebx-6]      ;byl dosazen pravy roh textury?
        jc      ptb_skip1       ;ne pokracuj
        mov     ax,bp           ;nacti eax counterem
        sub     esi,eax         ;odexti to cele od esi - tj spatky doleva
        xor     bp,bp
ptb_skip1:
        dec     edx
        jnz     ptb_l1
        shr     ebp,16          ;vrat counter pro y
        pop     esi             ;obnov pocatek v texture
        mov     ax,[ebx-6]      ;nalouduj do eax delku x (horni pulka je zarucene prazdna)
        add     esi,eax         ;pricti k esi
        inc     ebp             ;zvys y counter
        cmp     bp,[ebx-4]      ;test zda jsme dosahli dolniho rohu textury
        jc      ptb_skip2       ;ne pokracuj
        mov     eax,ebp         ;ano nalouduj eax
        mul     word ptr [ebx-6];vynasob ho delkou v x
        shl     eax,16
        shrd    eax,edx,16      ;eax=ax*word ptr [ebx-4]
        sub     esi,eax         ;jsme na zacatku
        xor     ebp,ebp
ptb_skip2:
        pop     edx             ;obnov zbyvajici registry
        pop     eax
        lea     edi,[edi+1280] ;presun se na dalsi radek na obrazovky
        sub     edi,edx
        sub     edi,edx
        dec     ecx             ;odecti 1 od globalniho citace radek
        jnz     ptb_l2          ;konec velke smycky
        POP     EBP
        ret

public  trans_bar_              ;kresli transparetni pravouhelnik ucite barvy
                                ;EDI - X
                                ;ESI - Y
                                ;ECX - YS
                                ;EDX - XS bere se nejmensi sude cislo
                                ;EBX - BARVA
trans_bar_:
        mov     eax,1280
        imul    eax,esi
        shl     edi,1
        add     edi,eax
        add     edi,_screen
        mov     eax,ebx
        shl     eax,16
        shld    ebx,eax,16
        and     ebx,7bde7bdeh
trs_l2: mov     esi,edi
        push    edx
        shr     edx,1
trs_l1: lodsd
        and     eax,7bde7bdeh
        add     eax,ebx
        shr     eax,1
        stosd
        dec     edx
        jnz     trs_l1
        pop     edx
        add     edi,1280
        sub     edi,edx
        sub     edi,edx
        dec     ecx
        jnz     trs_l2
        ret

public trans_line_x_
                        ;EDI - X
                        ;ESI - Y
                        ;ECX - XS
                        ;EDX - COLOR
trans_line_x_:
        mov     eax,1280
        imul    eax,esi
        shl     edi,1
        add     edi,eax
        add     edi,_screen
        mov     esi,edi
        mov     eax,edx
        shl     eax,16
        shld    edx,eax,16
        and     edx,7bdf7bdfh
        ror     ecx,1
trlx_l: lodsd
        and     eax,7bdf7bdfh
        add     eax,edx
        shr     eax,1
        stosd
        dec     cx
        jnz     trlx_l
        shl     ecx,1
        jnc     trlx_s
        lodsw
        and     eax,7bdf7bdfh
        add     eax,edx
        shr     eax,1
        stosw
trlx_s: ret

public trans_line_y_
                        ;EDI - X
                        ;ESI - Y
                        ;ECX - YS
                        ;EDX - COLOR
trans_line_y_:
        mov     eax,1280
        imul    eax,esi
        shl     edi,1
        add     edi,eax
        add     edi,_screen
        mov     esi,edi
        and     edx,7bdfh
trly_l: mov     ax,[esi]
        and     eax,7bdfh
        add     eax,edx
        shr     eax,1
        mov     [esi],ax
        add     esi,1280
        dec     ecx
        jnz     trly_l
        ret


pic_start equ 2+2+2+512*5+512*5
ed_stack equ 640*4+480*4
ed_stk1  equ 480*4

public enemy_draw_
enemy_draw_:               ;ESI - Source_picture
                           ;EDI - Target
                           ;EBX - Offset of palette
                           ;EDX - scale (1-320)
                           ;EAX - max_space (kolik ma nahore mista)
                           ;ECX - Hi_omezeni z prava, Lo_omezeni zleva

        cmp     eax,470
        jc      ed_ok1
        ret
ed_ok1: push    ebp
        sub     esp,ed_stack ;vyhrad zasobnik
        mov     ebp,esp
        push    ecx          ;uchovej ecx               +1
        push    edi          ;uchovej edi               +2
        push    ebx          ;uchovej ebx               +3
        mov     ebx,eax      ;volne misto je ted v ebx
        mov     edi,ebp      ;nastav edi na zacatek tabulek
        push    ebp          ;                          +4
        movzx   ecx,word ptr[esi]     ;precti xs
        movzx   eax,word ptr[esi+2]   ;precti ys
        imul    eax,ecx      ;xs*ys
        sub     eax,ecx      ;minus 1 radek
        xor     ebp,ebp
ed_lp1b:stosd                ;zapis hodnotu do tabulky
        add     ebp,320      ;zmensovani
ed_lp1a:cmp     ebp,edx
        jc      ed_nxt1      ;pri ebp<edx pokracuj dal
        sub     ebp,edx
        sub     eax,ecx      ;odecti radku
        jbe     ed_end1      ;pokud je to posledni tak konec1
        jmp     ed_lp1a
ed_nxt1:dec     ebx
        jnz     ed_lp1b
ed_end1:xor     eax,eax
        dec     eax
        stosd                ;tabulka pro y je hotova
        pop     ebp          ;                          +3
        lea     edi,[ebp+ed_stk1] ;nastav se na druhou tabulku
        xor     eax,eax      ;vynuluj vypoctove registry
        xor     ebx,ebx
        movzx   ecx,word ptr[esi]  ;ecx je velikost obrazku v x
ed_nxt2:stosd                ;zapis x souradnici
        add     ebx,320      ;pocitej zmenseni
ed_lp2a:cmp     ebx,edx
        jc      ed_nxt2
        sub     ebx,edx
        inc     eax
        cmp     eax,ecx      ;pokud je x>=velikosti obrazku tak konec
        jc     ed_lp2a
        xor     eax,eax
        dec     eax          ;zapis -1 na konec tabulky
        pop     ebx          ;                          +2
        stosd
        mov     eax,edi      ;konecnou pozici pro tabulku x zapis do eax
        pop     edi          ;obnov registry edi exc ebx+1
        add     ebx,esi      ;najdi adresu prekladove tabulky barev
        add     ebx,2        ;preskoc transparentni barvu
        add     esi,pic_start;presun se na zacatek obrazku
        pop     ecx          ;                          +0
        movzx   edx,cx       ;vem levy okraj
        lea     edx,[ebp+edx*4]
        lea     edx,[edx+ed_stk1];edx ukazuje na sloupce
        cmp     eax,edx      ;je-li levy okraj za platnou tabulkou tak konec
        jbe     ed_err
        shr     ecx,16       ;ecx ted obsahuje pravy okraj
ed_lp4: push    esi          ;uchovej esi               +1
        push    edi          ;uchovej edi               +2
        add     esi,[ebp]    ;spocitej spravnou hodnotu esi
        push    ecx          ;uchovej pravy okraj       +3
        push    edx          ;uchovej ukazatel na tabulku+4
ed_lp3: mov     eax,[edx]
        cmp     eax,-1       ;testuj konec tabulky
        jz      ed_end3      ;pri pozitivnim vysledku => konec radky
        movzx   eax,byte ptr[esi+eax];vem barvu
        or      al,al
        jz      ed_skp1      ;preskoc transparentni barvu
        dec     al
        jz      ed_shd       ;1=shadow
        mov     eax,[ebx+eax*2];vyzvedni hicolor
        stosw                ;zapis barvu
        jmp     ed_skp2
ed_shd: movzx   eax,word ptr[edi];vem barvu
        and     eax,7bdfh   ;stmav
        shr     eax,1
        stosw               ;zapis
        jmp     ed_skp2     ;skok na konec
ed_skp1:add     edi,2       ;preskoc bod
ed_skp2:add     edx,4       ;dalsi pozice
        dec     ecx         ;dokud nedosahneme praveho okraje obrazku
        jnz     ed_lp3
ed_end3:pop     edx         ;obnov vse ulozene
        pop     ecx
        pop     edi
        pop     esi         ;                           +0
        add     ebp,4       ;dalsi y souradnice
        sub     edi,1280    ;dalsi radka
        cmp     dword ptr [ebp],-1 ;test na konec tabulky
        jnz     ed_lp4      ;jinak pokracuj
ed_err: add     esp,ed_stack;vymaz tabulku
        pop     ebp         ;obnov ebp
        ret                 ;konec

public enemy_draw_mirror_     ;kresli zrcadlove otocenou potvoru
enemy_draw_mirror_:               ;ESI - Source_picture
                           ;EDI - Target
                           ;EBX - Offset of palette
                           ;EDX - scale (1-320)
                           ;EAX - max_space (kolik ma nahore mista)
                           ;ECX - Hi_omezeni z prava, Lo_omezeni zleva

        push    ebp
        sub     esp,ed_stack ;vyhrad zasobnik
        mov     ebp,esp
        push    ecx          ;uchovej ecx               +1
        push    edi          ;uchovej edi               +2
        push    ebx          ;uchovej ebx               +3
        mov     ebx,eax      ;volne misto je ted v ebx
        mov     edi,ebp      ;nastav edi na zacatek tabulek
        push    ebp          ;                          +4
        movzx   ecx,word ptr[esi]     ;precti xs
        movzx   eax,word ptr[esi+2]   ;precti ys
        imul    eax,ecx      ;xs*ys
        sub     eax,ecx      ;minus 1 radek
        xor     ebp,ebp
edmlp1b:stosd                ;zapis hodnotu do tabulky
        add     ebp,320      ;zmensovani
edmlp1a:cmp     ebp,edx
        jc      edmnxt1      ;pri ebp<edx pokracuj dal
        sub     ebp,edx
        sub     eax,ecx      ;odecti radku
        jbe     edmend1      ;pokud je to posledni tak konec1
        jmp     edmlp1a
edmnxt1:dec     ebx
        jnz     edmlp1b
edmend1:xor     eax,eax
        dec     eax
        stosd                ;tabulka pro y je hotova
        pop     ebp          ;                          +3
        lea     edi,[ebp+ed_stk1] ;nastav se na druhou tabulku
        xor     ebx,ebx
        movzx   ecx,word ptr[esi]  ;ecx je velikost obrazku v x
        mov     eax,ecx      ;eax je velikost obrazku v x
        dec     eax
edmnxt2:stosd                ;zapis x souradnici
        add     ebx,320      ;pocitej zmenseni
edmlp2a:cmp     ebx,edx
        jc      edmnxt2
        sub     ebx,edx
        sub     eax,1
        jnc     edmlp2a
        xor     eax,eax
        dec     eax          ;zapis -1 na konec tabulky
        pop     ebx          ;                          +2
        stosd
        mov     eax,edi      ;konecnou pozici pro tabulku x zapis do eax
        pop     edi          ;obnov registry edi exc ebx+1
        add     ebx,esi      ;najdi adresu prekladove tabulky barev
        add     ebx,2        ;preskoc transparentni barvu
        add     esi,pic_start;presun se na zacatek obrazku
        pop     ecx          ;                          +0
        movzx   edx,cx       ;vem levy okraj
        lea     edx,[ebp+edx*4]
        lea     edx,[edx+ed_stk1];edx ukazuje na sloupce
        cmp     eax,edx      ;je-li levy okraj za platnou tabulkou tak konec
        jbe     edmerr
        shr     ecx,16       ;ecx ted obsahuje pravy okraj
edmlp4: push    esi          ;uchovej esi               +1
        push    edi          ;uchovej edi               +2
        add     esi,[ebp]    ;spocitej spravnou hodnotu esi
        push    ecx          ;uchovej pravy okraj       +3
        push    edx          ;uchovej ukazatel na tabulku+4
edmlp3: mov     eax,[edx]
        cmp     eax,-1       ;testuj konec tabulky
        jz      edmend3      ;pri pozitivnim vysledku => konec radky
        movzx   eax,byte ptr[esi+eax];vem barvu
        or      al,al
        jz      edmskp1      ;preskoc transparentni barvu
        dec     al
        jz      edmshd       ;1=shadow
        mov     eax,[ebx+eax*2];vyzvedni hicolor
        stosw                ;zapis barvu
        jmp     edmskp2
edmshd: movzx   eax,word ptr[edi];vem barvu
        and     eax,7bdfh   ;stmav
        shr     eax,1
        stosw               ;zapis
        jmp     edmskp2     ;skok na konec
edmskp1:add     edi,2       ;preskoc bod
edmskp2:add     edx,4       ;dalsi pozice
        dec     ecx         ;dokud nedosahneme praveho okraje obrazku
        jnz     edmlp3
edmend3:pop     edx         ;obnov vse ulozene
        pop     ecx
        pop     edi
        pop     esi         ;                           +0
        add     ebp,4       ;dalsi y souradnice
        sub     edi,1280    ;dalsi radka
        cmp     dword ptr [ebp],-1 ;test na konec tabulky
        jnz     edmlp4      ;jinak pokracuj
edmerr: add     esp,ed_stack;vymaz tabulku
        pop     ebp         ;obnov ebp
        ret                 ;konec


pic_start_15 equ 2+2+2+512



public  put_picture2picture_ ;vlozi obrazek do obrazku (256c->256)
put_picture2picture_:
                             ;ESI - obrazek 256c
                             ;EDI - obrazek 256
                             ;EAX - Xpos
                             ;EDX - Ypos
        movzx   ecx,word ptr [edi]              ;vyzvedni sirku obrazku
        imul    edx,ecx                         ;nova adresa=edi+(Xpos+sirka*Ypos)
        add     eax,edx
        mov     edx,ecx
        lea     edi,[edi+eax+pic_start]         ;edi obsahuje novou adresu
        mov     ecx,[esi]                       ;ecx obsahuje Xsize a Ysize obrazku
        sub     ecx,10000h                      ;Ysize-1
        lea     esi,[esi+pic_start]             ;nastav esi na zacatek bitmapy
ppp_lp2:push    ecx                             ;uchovej velikosti obrazku v zasobniku
ppp_lp1:lodsb                                   ;nacti bod ze zdroje
        or      al,al                           ;zkontroluj zda to neni transparentni barva
        jz      ppp_trn                         ;pokud je tak ji prezskoc
        mov     [edi],al                        ;zapis barvu
ppp_trn:inc     edi                             ;dalsi bod
        dec     cx                              ;odecitej x sirku
        jnz     ppp_lp1                         ;dokud to neni vse
        pop     ecx                             ;obnov ecx
        movzx   eax,cx                          ;vem jeste jednou sirku
        sub     eax,edx                         ;spocitej kolik bajtu je nutne
        sub     edi,eax                         ;prezkocit k dosazeni dalsi radky
        sub     ecx,10000h                      ;opakuj pro y radku
        jnc     ppp_lp2
        ret

public  trans_bar25_            ;ztmaveni o 25%
                                ;EDI - X
                                ;ESI - Y
                                ;ECX - YS
                                ;EDX - XS bere se nejmensi sude cislo
trans_bar25_:
        mov     eax,1280
        imul    eax,esi
        shl     edi,1
        add     edi,eax
        add     edi,_screen
trs2_l2:mov     esi,edi
        push    edx
        shr     edx,1
trs2_l1:lodsd
        mov     ebx,eax
        and     ebx,739c739ch
        shr     ebx,2
        sub     eax,ebx
        stosd
        dec     edx
        jnz     trs2_l1
        pop     edx
        add     edi,1280
        sub     edi,edx
        sub     edi,edx
        dec     ecx
        jnz     trs2_l2
        ret



;public double_xicht_            ;zobrazi zvetseny xicht
;                                ;ESI - source
;                                ;EDI - TARGET
;double_xicht_:
;        push    ebp
;        mov     ecx,[esi+2]
;        lea     ebx,[esi+6]
;        mov     edx,[ebx-6]
;        movzx   eax,byte ptr [esi]
;        mov     ebp,[eax*2+ebx]




public enemy_draw_transp_
enemy_draw_transp_:        ;ESI - Source_picture
                           ;EDI - Target
                           ;EBX - Offset of palette
                           ;EDX - scale (1-320)
                           ;EAX - max_space (kolik ma nahore mista)
                           ;ECX - Hi_omezeni z prava, Lo_omezeni zleva

        cmp     eax,470
        jc      et_ok1
        ret
et_ok1: push    ebp
        sub     esp,ed_stack ;vyhrad zasobnik
        mov     ebp,esp
        push    ecx          ;uchovej ecx               +1
        push    edi          ;uchovej edi               +2
        push    ebx          ;uchovej ebx               +3
        mov     ebx,eax      ;volne misto je ted v ebx
        mov     edi,ebp      ;nastav edi na zacatek tabulek
        push    ebp          ;                          +4
        movzx   ecx,word ptr[esi]     ;precti xs
        movzx   eax,word ptr[esi+2]   ;precti ys
        imul    eax,ecx      ;xs*ys
        sub     eax,ecx      ;minus 1 radek
        xor     ebp,ebp
et_lp1b:stosd                ;zapis hodnotu do tabulky
        add     ebp,320      ;zmensovani
et_lp1a:cmp     ebp,edx
        jc      et_nxt1      ;pri ebp<edx pokracuj dal
        sub     ebp,edx
        sub     eax,ecx      ;odecti radku
        jbe     et_end1      ;pokud je to posledni tak konec1
        jmp     et_lp1a
et_nxt1:dec     ebx
        jnz     et_lp1b
et_end1:xor     eax,eax
        dec     eax
        stosd                ;tabulka pro y je hotova
        pop     ebp          ;                          +3
        lea     edi,[ebp+ed_stk1] ;nastav se na druhou tabulku
        xor     eax,eax      ;vynuluj vypoctove registry
        xor     ebx,ebx
        movzx   ecx,word ptr[esi]  ;ecx je velikost obrazku v x
et_nxt2:stosd                ;zapis x souradnici
        add     ebx,320      ;pocitej zmenseni
et_lp2a:cmp     ebx,edx
        jc      et_nxt2
        sub     ebx,edx
        inc     eax
        cmp     eax,ecx      ;pokud je x>=velikosti obrazku tak konec
        jc     et_lp2a
        xor     eax,eax
        dec     eax          ;zapis -1 na konec tabulky
        pop     ebx          ;                          +2
        stosd
        mov     eax,edi      ;konecnou pozici pro tabulku x zapis do eax
        pop     edi          ;obnov registry edi exc ebx+1
        cmp     byte ptr [esi+5],2      ;obrazek bez palety?
        jz      et_pl1
        add     esi,pic_start;presun se na zacatek obrazku (za paletu)
        jmp     et_pl2
et_pl1: add     esi,6        ;(obrazek bez palety, presun se za hlavicku)
et_pl2: pop     ecx          ;                          +0
        movzx   edx,cx       ;vem levy okraj
        lea     edx,[ebp+edx*4]
        lea     edx,[edx+ed_stk1];edx ukazuje na sloupce
        cmp     eax,edx      ;je-li levy okraj za platnou tabulkou tak konec
        jbe     et_err
        shr     ecx,16       ;ecx ted obsahuje pravy okraj
et_lp4: push    esi          ;uchovej esi               +1
        push    edi          ;uchovej edi               +2
        add     esi,[ebp]    ;spocitej spravnou hodnotu esi
        push    ecx          ;uchovej pravy okraj       +3
        push    edx          ;uchovej ukazatel na tabulku+4
et_lp3: mov     eax,[edx]
        cmp     eax,-1       ;testuj konec tabulky
        jz      et_end3      ;pri pozitivnim vysledku => konec radky
        movzx   eax,byte ptr[esi+eax];vem barvu
        test    al,80h
        jnz     et_shd
        or      al,al
        jz      et_skp1      ;preskoc transparentni barvu
        mov     eax,[ebx+eax*2];vyzvedni hicolor
        stosw                ;zapis barvu
        jmp     et_skp2
et_shd: and     word ptr[edi],07bdfh
        mov     eax,[ebx+eax*2];vyzvedni hicolor
        and     eax,7bdfh   ;stmav
        add     ax,word ptr[edi]
        shr     eax,1
        stosw               ;zapis
        jmp     et_skp2     ;skok na konec
et_skp1:add     edi,2       ;preskoc bod
et_skp2:add     edx,4       ;dalsi pozice
        dec     ecx         ;dokud nedosahneme praveho okraje obrazku
        jnz     et_lp3
et_end3:pop     edx         ;obnov vse ulozene
        pop     ecx
        pop     edi
        pop     esi         ;                           +0
        add     ebp,4       ;dalsi y souradnice
        sub     edi,1280    ;dalsi radka
        cmp     dword ptr [ebp],-1 ;test na konec tabulky
        jnz     et_lp4      ;jinak pokracuj
et_err: add     esp,ed_stack;vymaz tabulku
        pop     ebp         ;obnov ebp
        ret                 ;konec


public enemy_draw_mirror_transp_     ;kresli zrcadlove otocenou potvoru pruhledne
enemy_draw_mirror_transp_:               ;ESI - Source_picture
                           ;EDI - Target
                           ;EBX - Offset of palette
                           ;EDX - scale (1-320)
                           ;EAX - max_space (kolik ma nahore mista)
                           ;ECX - Hi_omezeni z prava, Lo_omezeni zleva

        push    ebp
        sub     esp,ed_stack ;vyhrad zasobnik
        mov     ebp,esp
        push    ecx          ;uchovej ecx               +1
        push    edi          ;uchovej edi               +2
        push    ebx          ;uchovej ebx               +3
        mov     ebx,eax      ;volne misto je ted v ebx
        mov     edi,ebp      ;nastav edi na zacatek tabulek
        push    ebp          ;                          +4
        movzx   ecx,word ptr[esi]     ;precti xs
        movzx   eax,word ptr[esi+2]   ;precti ys
        imul    eax,ecx      ;xs*ys
        sub     eax,ecx      ;minus 1 radek
        xor     ebp,ebp
etmlp1b:stosd                ;zapis hodnotu do tabulky
        add     ebp,320      ;zmensovani
etmlp1a:cmp     ebp,edx
        jc      etmnxt1      ;pri ebp<edx pokracuj dal
        sub     ebp,edx
        sub     eax,ecx      ;odecti radku
        jbe     etmend1      ;pokud je to posledni tak konec1
        jmp     etmlp1a
etmnxt1:dec     ebx
        jnz     etmlp1b
etmend1:xor     eax,eax
        dec     eax
        stosd                ;tabulka pro y je hotova
        pop     ebp          ;                          +3
        lea     edi,[ebp+ed_stk1] ;nastav se na druhou tabulku
        xor     ebx,ebx
        movzx   ecx,word ptr[esi]  ;ecx je velikost obrazku v x
        mov     eax,ecx      ;eax je velikost obrazku v x
        dec     eax
etmnxt2:stosd                ;zapis x souradnici
        add     ebx,320      ;pocitej zmenseni
etmlp2a:cmp     ebx,edx
        jc      etmnxt2
        sub     ebx,edx
        sub     eax,1
        jnc     etmlp2a
        xor     eax,eax
        dec     eax          ;zapis -1 na konec tabulky
        pop     ebx          ;                          +2
        stosd
        mov     eax,edi      ;konecnou pozici pro tabulku x zapis do eax
        pop     edi          ;obnov registry edi exc ebx+1
        cmp     byte ptr [esi+5],2      ;obrazek bez palety?
        jz      etmpl1
        add     esi,pic_start;presun se na zacatek obrazku (za paletu)
        jmp     etmpl2
etmpl1: add     esi,6        ;(obrazek bez palety, presun se za hlavicku)
etmpl2: pop     ecx          ;                          +0
        movzx   edx,cx       ;vem levy okraj
        lea     edx,[ebp+edx*4]
        lea     edx,[edx+ed_stk1];edx ukazuje na sloupce
        cmp     eax,edx      ;je-li levy okraj za platnou tabulkou tak konec
        jbe     etmerr
        shr     ecx,16       ;ecx ted obsahuje pravy okraj
etmlp4: push    esi          ;uchovej esi               +1
        push    edi          ;uchovej edi               +2
        add     esi,[ebp]    ;spocitej spravnou hodnotu esi
        push    ecx          ;uchovej pravy okraj       +3
        push    edx          ;uchovej ukazatel na tabulku+4
etmlp3: mov     eax,[edx]
        cmp     eax,-1       ;testuj konec tabulky
        jz      etmend3      ;pri pozitivnim vysledku => konec radky
        movzx   eax,byte ptr[esi+eax];vem barvu
        test    al,80h
        jnz     etmshd
        or      al,al
        jz      etmskp1      ;preskoc transparentni barvu
        mov     eax,[ebx+eax*2];vyzvedni hicolor
        stosw                ;zapis barvu
        jmp     etmskp2
etmshd: and     word ptr[edi],07bdfh
        mov     eax,[ebx+eax*2];vyzvedni hicolor
        and     eax,7bdfh   ;stmav
        add     ax,word ptr[edi]
        shr     eax,1
        stosw               ;zapis
        jmp     etmskp2     ;skok na konec
etmskp1:add     edi,2       ;preskoc bod
etmskp2:add     edx,4       ;dalsi pozice
        dec     ecx         ;dokud nedosahneme praveho okraje obrazku
        jnz     etmlp3
etmend3:pop     edx         ;obnov vse ulozene
        pop     ecx
        pop     edi
        pop     esi         ;                           +0
        add     ebp,4       ;dalsi y souradnice
        sub     edi,1280    ;dalsi radka
        cmp     dword ptr [ebp],-1 ;test na konec tabulky
        jnz     etmlp4      ;jinak pokracuj
etmerr: add     esp,ed_stack;vymaz tabulku
        pop     ebp         ;obnov ebp
        ret                 ;konec




zobraz_2 macro
        lodsw
        movzx   ebp,ah
        movzx   ebp,ds:[ebx+ebp*2]
        shl     ebp,16
        movzx   eax,al
        movzx   eax,[ebx+eax*2]
        or      eax,ebp
        stosd
endm

zobraz_1 macro
        lodsb
        movzx   eax,al
        movzx   eax,word ptr [eax*2+ebx]
        stosw
endm


public show_anm_buff_      ;zobrazi frame animace (normal);

show_anm_buff_:            ;ESI - SOURCE
                           ;EDI - TARGER
                           ;EBX - PALETA

        push    ebp
        mov     ecx,180*320
shwab4: zobraz_2
        dec     ecx
        jnz     shwab4
        ret

public show_anm_delta_      ;zobrazi frame animace (normal);


show_anm_delta_:           ;ESI - SOURCE
                           ;EDI - TARGER
                           ;EBX - PALETA

        push    ebp             ;uchovej EBP pro dalsi mozne pouziti
        mov     ch,180          ;180 radek
        mov     eax,[esi]       ;vem offset na delta data
        lea     ebp,[esi+eax+4]     ;ebp obsahuje tento offset
        lea     esi,[esi+4]     ;esi obsahuje offset na delta control
shwad5: push    edi             ;uchovej zacatek radky
        push    edx             ;i pro transp data
shwad3: lodsb                   ;vem skip hodnotu
        mov     ah,al           ;uchovej ji ah - kvuli destruktivnimu testu
        and     al,not 3fh      ;na posledni 2 bity
        cmp     al,0c0h         ;C0 a vyssi znamenaji ze se preskakuje radek
        jz      shwad1          ;pri pozitivnim testu je preskoc
        movzx   eax,ah          ;vem skip hodnotu do eax
        shl     eax,2           ;na obrazovce *2 bodu *2 za bod (x8)
        add     edi,eax         ;pricti k edi
        lodsb                   ;vem copy hodnotu
        mov     cl,al           ;zaved counter
        xchg    esi,ebp         ;prohod ukazatele control a data
        push    ebp             ;uchovej control
shwad2: zobraz_2                ;zobraz bod
        dec     cl              ;opakuj tolikrat kolik je copy hodnota
        jnz     shwad2
        pop     ebp             ;obnov control
        xchg    esi,ebp         ;prohod ukazatele control a data
        jmp     shwad3          ;a precti dalsi skip hodnotu
shwad1: pop     edx             ;obnov registry ukazujici na zacatek radky
        pop     edi
        and     ah,03fh         ;maskuj spodnich 6 bitu
        inc     ah              ;+1
shwad4: add     edi,640
        dec     ch              ;odecti counter radek
        dec     ah              ;odecti counter
        jnz     shwad4          ;a opakuj ah krat
        or      ch,ch           ;je li counter nulovy tak padame
        jnz     shwad5
        pop     ebp             ;obnov ebp
        ret                     ;konec
        ret

public klicovani_anm_

klicovani_anm_:                 ;esi source edi target al mirror
        or      al,al
        jnz     klicovani_anm_back_
        mov     ecx,180
ka_lp2: mov     ebx,320
ka_lp1: lodsw
        movzx   eax,ax
        test    eax,0x8000
        jnz     ka_skip
        mov     edx,eax
        shl     edx,16
        or      eax,edx
        mov     [edi+1280],eax
        mov     [edi],eax
ka_skip:add     edi,4
        dec     ebx
        jnz     ka_lp1
        add     edi,1280
        dec     ecx
        jnz     ka_lp2
        ret

public klicovani_anm_back_

klicovani_anm_back_:                 ;esi source edi target
        mov     ecx,180
        add     edi,1280
kba_lp2: mov     ebx,320
kba_lp1: lodsw
        sub     edi,4
        movzx   eax,ax
        test    eax,0x8000
        jnz     kba_skip
        mov     edx,eax
        shl     edx,16
        or      eax,edx
        mov     [edi+1280],eax
        mov     [edi],eax
kba_skip:dec     ebx
        jnz     kba_lp1
        add     edi,1280*3
        dec     ecx
        jnz     kba_lp2
        ret




public small_anm_buff_         ;zobrazi frame v mensim ramecku

small_anm_buff_:           ;ESI - SOURCE
                           ;EDI - TARGER
                           ;EBX - PALETA

        mov     ecx,179*10000h
shmab4: mov     cx,320
shmab3: zobraz_1
        dec     cx
        jnz     shmab3
        add     edi,640
        sub     ecx,010000h
        jnc     shmab4
        ret

public small_anm_delta_      ;zobrazi frame animace (normal);


small_anm_delta_:           ;ESI - SOURCE
                           ;EDI - TARGER
                           ;EBX - PALETA

        mov     ch,180          ;180 radek
        mov     eax,[esi]       ;vem offset na delta data
        lea     edx,[esi+eax+4]     ;ebp obsahuje tento offset
        lea     esi,[esi+4]     ;esi obsahuje offset na delta control
shmad5: push    edi             ;uchovej zacatek radky
shmad3: lodsb                   ;vem skip hodnotu
        mov     ah,al           ;uchovej ji ah - kvuli destruktivnimu testu
        and     al,not 3fh      ;na posledni 2 bity
        cmp     al,0c0h         ;C0 a vyssi znamenaji ze se preskakuje radek
        jz      shmad1          ;pri pozitivnim testu je preskoc
        movzx   eax,ah          ;vem skip hodnotu do eax
        shl     eax,2           ;na obrazovce *2 bodu *1 zoom *2 za bod (x4)
        add     edi,eax         ;pricti k edi
        lodsb                   ;vem copy hodnotu
        mov     cl,al           ;zaved counter
        xchg    esi,edx         ;prohod ukazatele control a data
shmad2: zobraz_1                ;zobraz bod
        zobraz_1                ;zobraz bod
        dec     cl              ;opakuj tolikrat kolik je copy hodnota
        jnz     shmad2
        xchg    esi,edx         ;prohod ukazatele control a data
        jmp     shmad3          ;a precti dalsi skip hodnotu
shmad1: pop     edi
        and     ah,03fh         ;maskuj spodnich 6 bitu
        inc     ah              ;+1
shmad4: add     edi,1280
        dec     ch              ;odecti counter radek
        dec     ah              ;odecti counter
        jnz     shmad4          ;a opakuj ah krat
        or      ch,ch           ;je li counter nulovy tak padame
        jnz     shmad5
        ret                     ;konec
        ret

public scroll_and_copy_
scroll_and_copy_:                ;odscroluje a kopiruje obrazovku do screenu
                                 ;pouzitelne pro titulky
                                 ;0x8000 je transparentni
                                 ;edi screen
                                 ;ebx buffer
                                 ;esi back_picture
                                 ;ecx velikost bufferu (pocet_radku)
                                 ;edx pocet bajtu o kolik je nutne posouvat (*1280)
                                 ;eax - ukazatel na tittle lines
        push    ebp
        mov     ebp,eax         ;uloz ukazatel na titlelines
        mov     eax,edx
        imul    eax,1280
sac_lp1:push    ecx
        push    eax
        mov     eax,[ebp]       ;nacti zac
        mov     ecx,[ebp+edx*8]
        cmp     eax,ecx
        jc      sac_sk1
        mov     eax,ecx
sac_sk1:shl     eax,1           ;adresa
        add     esi,eax         ;pricti k pointrum
        add     edi,eax
        add     ebx,eax
        add     ebp,4           ;presun se na kon
        shr     eax,1           ;eax je zacatek v bodech
        mov     ecx,[ebp+edx*8] ;vem konec
        cmp     ecx,[ebp]
        jnc     sac_sk2
        mov     ecx,[ebp]
sac_sk2:push    ecx
        sub     ecx,eax         ;kon-zac+1=celk pocet
        xchg    edx,[esp+4]
sac_lp2:mov     eax,[ebx+edx]   ;Vem data na dalsim radku
        mov     [ebx],eax       ;Zapis je sem
        add     ebx,4           ;dalsi pozice
        test    eax,7fff7fffh   ;je to transparentni?
        jz      sac_all         ;pokud ano, zobraz_obrazek
        bt      eax,15          ;test bitu 15
        jnc     sac_1           ;Neni nastaven - zobraz primo barvu
        mov     ax,[esi]        ;jinak vem barvu z obrazku
sac_1:  stosw                   ;zapis barvu
        add     esi,2           ;dalsi pozice
        rol     eax,16          ;presun eax na dolni slovo
        bt      eax,15          ;test bitu 15
        jnc     sac_2           ;neni nastaven - zobraz primo barvu
        mov     ax,[esi]        ;jinak vem barvu z obrazku
sac_2:  stosw                   ;zapis barvu
        add     esi,2           ;dalsi pozice
        jmp     sac_end
sac_all:movsd                   ;presun celeho slova
sac_end:sub     ecx,2           ;odecti counter
        ja      sac_lp2         ;je li nad opakuj
        add     ecx,640         ;pricti sirku, - kolik jsme prejeli
        pop     eax
        sub     ecx,eax       ;odecti konec -> kolik preskocit
        add     ebp,4           ;aktualizuj ukazatele
        shl     ecx,1           ;v adresach
        add     esi,ecx
        add     edi,ecx
        add     ebx,ecx
        pop     eax
        xchg    edx,eax
        pop     ecx             ;obnov ecx
        dec     ecx             ;sniz citac radku
        jnz     sac_lp1         ;dokud neni nula
        pop     ebp             ;obnov ebp
        ret                     ;konec


        public lodka32_
lodka32_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;eax - lodka
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
        push    ebp
        mov     ebp,eax
        mov     ebx,_zoom[offset tzoom.ytable]
l32d:   push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
l32c:   mov     eax,dword ptr [ebp]
        or      eax,eax
        jnz     l32e
        mov     al,[edx]
        inc     edx
        or      al,al
        jz      l32a
        lodsw
        stosw
        stosw
        jmp     l32b
l32e:   stosd
        movzx   eax,byte ptr[edx]       ;0 - o 4 1 - o 2
        xor     eax,1                   ;0->1  1->0
        inc     eax                     ;1+1=2 0+1=1
        shl     eax,1                   ;2*2=4 1*2=2
        add     esi,eax
        inc     edx
        jmp     l32b
l32a:   lodsd
        stosd
l32b:   add     ebp,4
        dec     cx
        jnz     l32c
        pop     esi
        mov     eax,[ebx]
        and     eax,0ffffh
        add     esi,eax
        add     ebx,2
        add     edi,_zoom[offset tzoom.textline]
        xor     cx,cx
        cmp     ecx,60*65536
        cmc
        sbb     eax,eax
        and     eax,1280
        sub     ebp,eax
        sub     ecx,10000h
        pop     cx
        jnz     l32d
        pop     ebp
        ret

        public lodka32b_
lodka32b_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;zoom.xtable - tabulka pro x
                        ;zoom.ytable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;zoom.ycount - interni (ulozeni maskovane adr stranky)
                        ;zoom.xmax - interni (pocet dwordu do prepnuti)
                        ;zoom.linelen - interni (velikost stranky v dwordech)


        call    mapvesaadr_     ;mapuj spravne edi
        push    ebp
        mov     ebp,eax
        mov     ebx,_zoom[offset tzoom.ytable]
l32bd:  push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
        push    edi
l32bc:  mov     eax,dword ptr [ebp]
        or      eax,eax
        jnz     l32be
        mov     al,[edx]
        inc     edx
        or      al,al
        jz      l32ba
        lodsw
        stosw
        stosw
        jmp     l32bb
l32be:  stosd
        movzx   eax,byte ptr[edx]       ;0 - o 4 1 - o 2
        xor     eax,1                   ;0->1  1->0
        inc     eax                     ;1+1=2 0+1=1
        shl     eax,1                   ;2*2=4 1*2=2
        add     esi,eax
        inc     edx
        jmp     l32bb
l32ba:  lodsd
        stosd
l32bb:  add     ebp,4
        movzx   eax,di                  ;prevezmi dolni cast edi do eax
        dec     eax                     ;odecti 1 (0=0xffffffff)
        cmp     eax,_gran_mask          ;je vetsi nez granualita?
        jc      l32gb                   ;ne, vse je ok
        mov     eax,_lastbank           ;ano, pak vem cislo banky
        inc     eax                     ;pricti 1
        mov     _lastbank,eax           ;zapis jako nove cislo banky
        pushad                          ;uchovej vse
        call    switchvesabank_         ;prepni banku
        popad                           ;obnov vse
        mov     edi,0a0000h             ;vynuluj dolni cast di
l32gb:  dec     cx                      ;tady pokracujem
        jnz     l32bc                   ;opakuj cxkrat
        pop     edi                     ;obnov edi
        add     di,1280                 ;pricti radek
        and     di,word ptr _gran_mask  ;maskuj granualitou
        pop     esi
        mov     eax,[ebx]
        and     eax,0ffffh
        add     esi,eax
        add     ebx,2
        xor     cx,cx
        cmp     ecx,60*65536
        cmc
        sbb     eax,eax
        and     eax,1280
        sub     ebp,eax
        sub     ecx,10000h
        pop     cx
        jnz     l32bd
        pop     ebp
        ret


        public lodka256_
lodka256_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;eax - lodka
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;ebx xlat
        push    ebp
        mov     ebp,eax
l256d:  push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
l256c:  mov     eax,[ebp]
        or      eax,eax
        jnz     l256e
        movzx   eax,byte ptr [edx]
        inc     edx
        or      al,al
        jz      l256a
        lodsw
        mov     al,[ebx+eax*2]
        mov     ah,al
        stosw
        jmp     l256b
l256e:  movzx   eax,ax
        mov     al,[ebx+eax*2]
        mov     ah,al
        stosw
        movzx   eax,byte ptr[edx]       ;0 - o 4 1 - o 2
        xor     eax,1                   ;0->1  1->0
        inc     eax                     ;1+1=2 0+1=1
        shl     eax,1                   ;2*2=4 1*2=2
        add     esi,eax
        inc     edx
        jmp     l256b
l256a:  lodsw
        mov     al,[ebx+eax*2]
        stosb
        lodsw
        mov     al,[ebx+eax*2]
        stosb
l256b:  add     ebp,4
        xor     ebx,1
        dec     cx
        jnz     l256c
        xor     ebx,1
        pop     esi
        mov     edx,_zoom[offset tzoom.ytable]
        mov     eax,[edx]
        and     eax,0ffffh
        add     esi,eax
        add     edx,2
        mov     _zoom[offset tzoom.ytable],edx
        add     edi,_zoom[offset tzoom.textline]
        xor     cx,cx
        cmp     ecx,60*65536
        cmc
        sbb     eax,eax
        and     eax,1280
        sub     ebp,eax
        sub     ecx,10000h
        pop     cx
        jnz     l256d
        pop     ebp
        ret


        public lodka256b_
lodka256b_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;zoom.xtable - tabulka pro x
                        ;zoom.xtable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;ebx xlat
        push    ebp
        mov     ebp,eax
l256bd: push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    edi
        call    mapvesaadr_
        push    esi
l256bc: mov     eax,[ebp]
        or      eax,eax
        jnz     l256be
        movzx   eax,byte ptr[edx]
        inc     edx
        or      al,al
        jz      l256ba
        lodsw
        mov     al,[ebx+eax*2]
        mov     ah,al
        stosw
        jmp     l256bb
l256be: movzx   eax,ax
        mov     al,[ebx+eax*2]
        mov     ah,al
        stosw
        movzx   eax,byte ptr[edx]       ;0 - o 4 1 - o 2
        xor     eax,1                   ;0->1  1->0
        inc     eax                     ;1+1=2 0+1=1
        shl     eax,1                   ;2*2=4 1*2=2
        add     esi,eax
        inc     edx
        jmp     l256bb
l256ba: lodsw
        mov     al,[ebx+eax*2]
        stosb
l256bh: lodsw
        mov     al,[ebx+eax*2]
        stosb
l256bb: add     ebp,4
        swap_at_end di,edi
        xor     ebx,1
        dec     cx
        jnz     l256bc
        xor     ebx,1
        pop     esi
        pop     edi
        add     edi,640
        mov     edx,_zoom[offset tzoom.ytable]
        mov     eax,[edx]
        and     eax,0ffffh
        add     esi,eax
        add     edx,2
        mov     _zoom[offset tzoom.ytable],edx
        add     edi,_zoom[offset tzoom.textline]
        xor     cx,cx
        cmp     ecx,60*65536
        cmc
        sbb     eax,eax
        and     eax,1280
        sub     ebp,eax
        sub     ecx,10000h
        pop     cx
        jnz     l256bd
        pop     ebp
        ret



_TEXT  ends


END






if 0
;POZOR DUMP!


        public zooming32b_
zooming32b_:
                        ;esi - zdrojovy blok
                        ;edi - cil
                        ;zoom.xtable - tabulka pro x
                        ;zoom.ytable - tabulka pro y
                        ;zoom.textline - rozdil mezi pravym okrajem a levym okrajem
                        ;               pri prechodu na novou radku
                        ;ecx ysize:xsize
                        ;zoom.ycount - interni (ulozeni maskovane adr stranky)
                        ;zoom.xmax - interni (pocet dwordu do prepnuti)

        push    ebp
        mov     ebp,eax
        mov     ebx,_zoom[offset tzoom.ytable]
z32bd:  push    cx
        mov     edx,_zoom[offset tzoom.xtable]
        push    esi
        push    edi
        call    mapvesaadr_
z32bc:  mov     al,[edx]
        inc     edx
        or      al,al
        jz      z32ba
        lodsw
        test    ax,8000h
        jnz      z32be
        stosw
        stosw
        jmp     z32bb
z32bf:  test    eax,8000h
        jz      z32bg
        test    eax,80000000h
        jz      z32bh
z32be:  mov     eax,[ds:ebp]
        stosd
        jmp     z32bb
z32bg:  rol     eax,16
        mov     ax,[ds:ebp+2]
        rol     eax,16
        stosd
        jmp     z32bb
z32bh:  mov     ax,[ds:ebp]
        stosd
        jmp     z32bb
z32ba:  lodsd
        test    eax,80008000h
        jnz      z32bf
        stosd
z32bb:  add     ebp,4
        dec     cx
        jnz     z32bc
        pop     edi
        add     edi,2048
        pop     esi
        mov     eax,[ebx]
        and     eax,0ffffh
        add     esi,eax
        add     ebx,2
        xor     cx,cx
        sub     ecx,10000h
        pop     cx
        jnz     z32bd
        pop     ebp
        ret

scroll_support_32b_:
                        ;edi - lbuffer + pozice na obrazovce
                        ;esi - oldbuffer
                        ;edx - newbuffer
                        ;ebx - xlat
                        ;ecx - size;
        push    ebp             ;uchovej ebp
        mov     ebp,360         ;ebp pro tuto chvili predstavuje citac
        mov     eax,ecx         ;uchovej ecx jeste v eac - zachova citac sloupcu
scrl1b: push    edi
        push    esi             ;uchovej esi
        call    mapvesaadr_
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
        pop     edi
        add     edi,2048
        dec     ebp             ; dokud nejsme na konci
        jnz     scrl1b          ;opakuj
        pop     ebp
        ret

endif
