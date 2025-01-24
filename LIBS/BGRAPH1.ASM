.model small
.386


DGROUP group _DATA
extrn _linelen:dword
extrn _screen:dword
extrn _curcolor:word
extrn _charcolors:word[7]
extrn _pictlen:dword
extrn _screen_buffer_size:dword

SEGA000 equ     0a0000h

_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

        public  getadr32_
getadr32_:
getadr_:mul     _linelen ;EAX - y; ESI - X;
        shl     esi,1
        add     eax,esi
        add     eax,_screen
        ret


        public  point32_ ;EAX - Y; ESI - X; ECX - COLOR
point32_: call    getadr_
        mov     [eax],cx
        ret



        public bar32_ ;AX - X1, BX - Y1, CX - X2, DX - Y2
bar32_: cmp     cx,ax   ;x1<x2
        jnc     bar2
        xchg    ax,cx   ;neni-li, prohod je
bar2:   cmp     dx,bx   ;y1<y2
        jnc     bar3
        xchg    bx,dx   ;neni-li, prohod je
bar3:   push    es      ;uchovej es
        push    edx     ;uchovej edx
        mov     dx,ds   ;prekopiruj ds do es
        mov     es,dx
        and     eax,0ffffh ;vymuluj hornich 16-bitu z eax
        mov     esi,eax
        mov     ax,bx
        call    getadr_ ;Zjisti adresu (ESI - X, EAX - Y)
        xchg    esi,eax ;ESI adresa, v eax je X
        shr     eax,1
        pop     edx     ;obnov edx
        sub     dx,bx   ;dx=y2-y1+1;
        inc     dx
        sub     cx,ax   ;cx=x2-x1+1
        inc     cx
        and     ecx,0ffffh ;vynuluj hodnich 16-biy ecx
        mov     ebx,ecx ;uchovej ecx v ebx
        mov     ax,_curcolor ;nacti aktualni barvu
        shl     eax,16  ;hodnotu odroluj do horni poloviny
        mov     ax,_curcolor ;nacti aktualni barvu jeste jednou
bar1:   mov     edi,esi ;edi je pozice leveho sloupce obdelniku
        shr     ecx,1   ;ecx/2 po 32-bitech
        rep     stosd   ;zapis linky
        rcl     ecx,1   ;pokud byla sirka licha je v ecx cislo 1
        rep     stosw   ;zapis pripadneho jednoho bodu
        mov     ecx,ebx ;obnov ecx
        add     esi,_linelen ;jdi na dalsi radek
        dec     dx      ;sniz dx - citac
        jnz     Bar1    ;dokud neni vsechno
        pop     es      ;obnoc es
        ret

        public  hor_line32_
Hor_line32_: ;EAX - Y ESI - X1 ECX - X2
        call    getadr_ ;eax obsahuje adresu bodu
        shr     esi,1
        push    es      ;uchovej es
        cmp     ecx,esi ;x2>x1
        jnc     horlin1
        xchg    ecx,esi ;jestli ne tak je prohod
horlin1:sub     ecx,esi ;xs=x2-x1+1
        inc     ecx     ;
        mov     si,ds   ;ds -> es
        mov     es,si
        mov     edi,eax ;esi je adresa
        mov     ax,_curcolor     ;nacti barvu
        shl     eax,16
        mov     ax,_curcolor
        shr     ecx,1
        rep     stosd   ;nakresli caru
        rcl     ecx,1
        rep     stosw   ;popripade jeste jeden bod
        pop     es      ;obnov es
        ret

public  ver_line32_
ver_line32_: ;EAX - Y1 ESI - X ECX - Y2
        cmp     ecx,eax ;y2>y1
        jnc     verlin1
        xchg    ecx,eax ;jestli ne tak je prohod
verlin1:sub     ecx,eax
        inc     ecx
        call    getadr_ ;eax obsahuje adresu bodu
        mov     esi,eax
        mov     ax,_curcolor     ;nacti barvu
verlin2:mov     [esi],ax ;kresli caru po bodech
        add     esi,_linelen
        dec     ecx
        jnz     verlin2
        ret

        public  line_32_
line_32_:       ;eax - Y, esi - X, ecx - xs, ebx - ys  (xs,ys muze byt zaporne)
        or      ecx,ecx
        jns     line1   ;kdyz je ecx (xs) zaporny je nutne otocit smer a presunout se na druhy konec linky
        add     esi,ecx
        add     eax,ebx
        neg     ebx
        neg     ecx
line1:  push    es      ;uchovej es
        or      ecx,ecx ;kdyz je ecx=0 pak je to specialni pripad
        inc     ecx
        jz     lnext
        call    getadr_ ;zjisti adresu prvnihi bodu
        mov     edi,eax ;vloz ji do di
        mov     ax,ds   ;ds -> es
        mov     es,ax
        mov     ax,_curcolor    ;nacti aktualni barvu
        or      ebx,ebx ;kontrola zda je ebx>=0
        js      lineup  ;neni pak bude se kreslit nahoru
        xor     esi,esi ;vynuluj esi - pocitadlo mezikroku
        mov     edx,ecx ;delku cary na ose x do porovnavaciho registru
        inc     ebx
lined3: add     esi,ebx ;do mezikroku pricti ys
        cmp     esi,edx ;kdyz to prekrocilo poronavaci registr
        jc      lined1  ;zacnes kreslit dolu, jinak v pravo
lined2: mov     [edi],ax;zapis bod
        add     edi,_linelen ;a posun dolu
        sub     esi,edx ;odect od mezikroku hodnotu v porovnavacim registru
        cmp     esi,edx ;je stale preteceni?
        jnc     lined2  ;pokud ano opakuj zapsani bodu
        add     edi,2   ;dalsi xs
        dec     ecx     ;sniz citac Xs
        jnz     lined3  ;pokracuj pro dalsi Xs
        jmp     linee   ;pokud to byl posledni, pak konec
lined1: stosw           ;zapis bod a posun v pravo
        dec     ecx     ;pokracuj pro dalsi ys
        jnz     lined3  ;dokud neni konec
        jmp     linee   ;pak jdi na lineEnd
lineup: neg     ebx     ;neguj ebx
        xor     esi,esi ;vynuluj esi - pocitadlo mezikroku
        mov     edx,ecx ;delku cary na ose x do porovnavaciho registru
        inc     ebx
lineu3: add     esi,ebx ;do mezikroku pricti ys
        cmp     esi,edx ;kdyz to prekrocilo poronavaci registr
        jc      lineu1  ;zacnes kreslit nahoru, jinak v pravo
lineu2: mov     [edi],ax;zapis bod
        sub     edi,_linelen ;a posun nahoru
        sub     esi,edx ;odect od mezikroku hodnotu v porovnavacim registru
        cmp     esi,edx ;je stale preteceni?
        jnc     lineu2  ;pokud ano opakuj zapsani bodu
        add     edi,2   ;dalsi xs
        dec     ecx     ;sniz citac Xs
        jnz     lineu3  ;pokracuj pro dalsi Xs
        jmp     linee   ;pokud to byl posledni, pak konec
lineu1: stosw           ;zapis bod a posun v pravo
        dec     ecx     ;pokracuj pro dalsi ys
        jnz     lineu3  ;dokud neni konec
        jmp     linee
lnext:  mov     ecx,ebx
        add     ecx,eax
        call    ver_line32_
linee:  pop     es
        ret

        public  char_32_
char_32_:               ;edi - pozice na obrazovce
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
        mov     ax,_charcolors[EAX*2] ;vyjmi barvu
        cmp     ax,0ffffh;0xffff je barva ktera se nekresli;
        jz      chr4    ;
        mov     [ebx],ax;zobraz ji na obrazovce
        jmp     chr4    ;a skoc na konec smycky
chr3:   cmp     al,0ffh ;pokud je al=255 pak jsme narazily na terminator.
        jz      chrend  ;V tom pripade KONEC
        sub     al,6    ;odecti do al 6. Ziskas pocet transparetnich pozic
        mov     dl,al   ;uloz je do citace
chr1:   dec     dl      ;pro kazdou pozici to dl odecti dokud neni 0
chr2:
chr4:   add     ebx,_linelen;dalsi radka
        dec     dh      ;odecti citac radek
        jnz     chr5    ;dokud neni nula
        add     edi,2   ;dalsi sloupec
        dec     cl      ;odecti citac sloupcu
        jnz     chr6    ;dokud neni nula
chrend: ret             ;konec

        public  char2_32_
char2_32_:              ;edi - pozice na obrazovce
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
        mov     ax,_charcolors[EAX*2] ;vyjmi barvu
        push    ebx
        mov     [ebx],ax;zobraz ji na obrazovce
        mov     [ebx+2],ax;zobraz ji na obrazovce
        add     ebx,_linelen
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
chr24:  add     ebx,_linelen;dalsi radka
        add     ebx,_linelen
        dec     dh      ;odecti citac radek
        jnz     chr25    ;dokud neni nula
        add     edi,4   ;dalsi sloupec
        dec     cl      ;odecti citac sloupcu
        jnz     chr26    ;dokud neni nula
chr2end: ret             ;konec

        public  charsize_
charsize_:              ;esi - ukazatel na font
                        ;al - znak
        and     eax,0ffh
        mov     ax,[esi][eax*2]
        or      ax,ax
        jz      chsend
        add     esi,eax
        lodsw
chsend: and     eax,0ffffh
        ret

        public  put_picture_
put_picture_:           ;esi - X
                        ;eax - Y
                        ;edi - obrazek
        mov     ecx,esi ;uchovej x v ecx
        mov     ebx,eax ;uchovej y v ebx
        call    getadr_ ;zjisti adresu bodu
        mov     esi,eax ;vloz ji do esi
        xchg    esi,edi ;prohod esi a edi aby obsahovali zpravne informace
        xor     eax,eax ;vynuluj pripadne bajty v horni polovine eax
        lodsw           ;nacti xs
        mov     _pictlen,eax
        mov     edx,eax ;z kopiruj jeste do pracovniho dx
        add     edx,ecx ;pricti k nemu souradnici
        cmp     edx,640 ;je-li vetsi nez max velikost obrazovky x
        jc      ppok1   ;
        mov     eax,640 ;pak za eax dosad plnou velikost
        sub     eax,ecx ;odecti souradnici
ppok1:  mov     ecx,eax ;vloz do ecx
        lodsw           ;totez pro y
        mov     edx,eax
        add     edx,ebx
        cmp     edx,400
        jc      ppok2
        mov     eax,400
        sub     eax,ebx
ppok2:  mov     edx,eax
        lodsw
        mov     ebx,edi
        cmp     al,15
        jnz     pp_next
        jmp     pp15bit
pp_next:cmp     al,8
        jnz     pp_nxt2
        jmp     pp8bit
pp_nxt2:ret

pp15bit:mov     eax,ecx
pp15bi1:shr     ecx,1
        rep     movsd
        rcl     ecx,1
        rep     movsw
        mov     ecx,_pictlen
        sub     ecx,eax
        shl     ecx,1
        add     esi,ecx
        mov     ecx,eax
        add     ebx,_linelen
        mov     edi,ebx
        dec     edx
        jnz     pp15bi1
        ret

pp8bit: push    ebp
        mov     ebp,ecx
        mov     ebx,esi
        add     esi,512
pp8bit1:xor     eax,eax
        lodsb
        or      al,al
        jz      pp8bit2
        mov     eax,[ebx+eax*2]
        stosw
pp8bit4:dec     ecx
        jnz     pp8bit1
        mov     ecx,ebp
        sub     edi,ecx
        sub     edi,ecx
        add     edi,_linelen
        sub     esi,ebp
        add     esi,_pictlen
        dec     edx
        jnz     pp8bit1
        pop     ebp
        ret
pp8bit2:add     edi,2
        jmp     pp8bit4

        public  setPal_
setPal_:mov     edx,3c6h
        mov     al,255
        out     dx,al
        xor     ah,ah
setpal1:mov     edx,3c8h
        mov     al,ah
        out     dx,al
        mov     edx,3c9h
        lodsb
        out     dx,al
        lodsb
        out     dx,al
        lodsb
        out     dx,al
        inc     ah
        jne     setPal1
        ret


        public  Redraw_lo_
Redraw_lo_:             ;esi 512Kb obrazovka
                        ;edi lbuffer
                        ;ebx xlattable
        test     ebx,1
        jz      rdrwlo3
        inc     ebx
rdrwlo3:push    ebp
        mov     ebp,200 shl 16
        xor     eax,eax
        xor     ecx,ecx
        xor     edx,edx
rdrwlo2:mov     bp,160
rdrwlo1:mov     ecx,[esi]
        mov     edx,[esi+1280]
        and     ecx,7bdf7bdfh
        and     edx,7bdf7bdfh
        add     ecx,edx
        shr     ecx,1
        shrd    ecx,edx,16
        and     edx,7bdfh
        and     ecx,7bdfh
        add     edx,ecx
        shr     edx,1
        mov     al,[ebx+edx*2]
        xor     ebx,1
        add     esi,4
        mov     ecx,[esi]
        mov     edx,[esi+1280]
        and     ecx,7bdf7bdfh
        and     edx,7bdf7bdfh
        add     ecx,edx
        shr     ecx,1
        shrd    ecx,edx,16
        and     edx,7bdfh
        and     ecx,7bdfh
        add     edx,ecx
        shr     edx,1
        mov     ah,[ebx+edx*2]
        stosw
        xor     ebx,1
        add     esi,4
        dec     bp
        jnz     rdrwlo1
        xor     ebx,1
        add     esi,1280
        sub     ebp,65536
        jnz     rdrwlo2
        pop     ebp
        ret

        public  Redraw256_
Redraw256_:              ;esi - source
                        ;edi - target
                        ;ebx - xlat

        test     ebx,1
        jz      rdrwlo7
        inc     ebx
rdrwlo7:xor     eax,eax
rdrwlo4:mov     ecx,400*65536
rdrwlo6:mov     cx,320
rdrwlo5:mov     edx,[esi]
        add     esi,4
        mov     ax,dx
        mov     al,[ebx+eax*2]
        shr     edx,16
        xor     ebx,1
        mov     ah,[ebx+edx*2]
        stosw
        xor     ebx,1
        dec     cx
        jnz     rdrwlo5
        xor     ebx,1
        sub     ecx,65536
        jnz     rdrwlo6
        ret

        public  redraw32_
redraw32_:              ;esi source
                        ;edi target
                        ;ebx notused (xlat)
        mov     ecx,128000
        rep     movsd
        ret


        public  redrawbox_lo_
redrawbox_lo_:          ;esi source
                        ;edi target
                        ;ebx xlat
                        ;ecx xs
                        ;edx,ys
        shr     ecx,1
        shr     edx,1
        dec     edx
        test    ebx,1
        jz      rboxlo3
        inc     ebx
rboxlo3:push    ebp
        mov     ebp,edx
        shl     ebp,16
        mov     bp,cx
        xor     eax,eax
        xor     ecx,ecx
        xor     edx,edx
rboxlo2:push    ebp
        push    esi
        push    edi
rboxlo1:mov     ecx,[esi]
        mov     edx,[esi+1280]
        and     ecx,7bdf7bdfh
        and     edx,7bdf7bdfh
        add     ecx,edx
        shr     ecx,1
        shrd    ecx,edx,16
        and     edx,7bdfh
        and     ecx,7bdfh
        add     edx,ecx
        shr     edx,1
        mov     al,[ebx+edx*2]
        stosb
        xor     ebx,1
        add     esi,4
        dec     bp
        jnz     rboxlo1
        pop     edi
        pop     esi
        pop     ebp
        xor     ebx,1
        add     esi,_linelen
        add     esi,_linelen
        add     edi,320
        sub     ebp,65536
        jnc     rboxlo2
        pop     ebp
        ret

        public  Redrawbox256_
Redrawbox256_:          ;esi - source
                        ;edi - target
                        ;ebx - xlat
                        ;ecx - ys
                        ;edx - xs

        shl     ecx,16
        shr     edx,1
        mov     cx,dx
        test    ebx,1
        jz      rboxlo7
        inc     ebx
rboxlo7:xor     eax,eax
rboxlo6:push    ecx
        push    esi
        push    edi
rboxlo5:mov     edx,[esi]
        add     esi,4
        mov     ax,dx
        mov     al,[ebx+eax*2]
        shr     edx,16
        xor     ebx,1
        mov     ah,[ebx+edx*2]
        stosw
        xor     ebx,1
        dec     cx
        jnz     rboxlo5
        pop     edi
        pop     esi
        pop     ecx
        add     esi,_linelen
        add     edi,640
        xor     ebx,1
        sub     ecx,65536
        test    ecx,0ffff0000h
        jnz     rboxlo6
        ret

        public  redrawbox32_
redrawbox32_:           ;esi source
                        ;edi target
                        ;ebx xs
                        ;edx ys
        shr     ebx,1
rbox32: mov     ecx,ebx
        rep     movsd
        mov     ecx,ebx
        shl     ecx,2
        sub     esi,ecx
        sub     edi,ecx
        add     esi,_linelen
        add     edi,_linelen
        dec     edx
        jnz     rbox32
        ret

        public  get_picture_
get_picture_:           ;esi - X
                        ;eax - Y
                        ;ebx - xs
                        ;ecx - ys
                        ;edi - obrazek
        mov     [edi],ebx ;zapis velikost obrazku
        mov     [edi+2],ecx
        mov     ecx,esi ;uchovej x v ecx
        mov     ebx,eax ;uchovej y v ebx
        call    getadr_ ;zjisti adresu bodu
        mov     esi,eax ;vloz ji do esi
        xor     eax,eax ;vynuluj pripadne bajty v horni polovine eax
        mov     ax,[edi];vem velikost x
        add     edi,2
        mov     _pictlen,eax
        mov     edx,eax ;z kopiruj jeste do pracovniho dx
        add     edx,ecx ;pricti k nemu souradnici
        cmp     edx,640 ;je-li vetsi nez max velikost obrazovky x
        jc      gpok1   ;
        mov     eax,640 ;pak za eax dosad plnou velikost
        sub     eax,ecx ;odecti souradnici
gpok1:  mov     ecx,eax ;vloz do ecx
        mov     ax,[edi];vem velikost y
        add     edi,2
        mov     edx,eax
        add     edx,ebx
        cmp     edx,400
        jc      gpok2
        mov     eax,400
        sub     eax,ebx
gpok2:  mov     edx,eax
        mov     ax,15   ;nastav typ 15
        stosw           ;zapis
        mov     ebx,esi ;uloz esi jeste do ebx
gp15bit:mov     eax,ecx ;uloz ecx jeste do eax
gp15bi1:shr     ecx,1   ;pocet dvojic bodu
        rep     movsd   ;presun do pameti
        rcl     ecx,1   ;pokud zbyl jeste jeden
        rep     movsw   ;tak ted
        mov     ecx,_pictlen
        sub     ecx,eax
        shl     ecx,1
        add     edi,ecx ;jdi na dalsi radku v obrazku
        mov     ecx,eax ;obnov counter
        add     ebx,_linelen ;jdi na dalsi radku na obrazovce
        mov     esi,ebx ;obnov esi
        dec     edx     ;dokud neni konec
        jnz     gp15bi1
        ret

        public  hor_line_xor_
Hor_line_xor_: ;EAX - Y ESI - X1 ECX - X2
        call    getadr_ ;eax obsahuje adresu bodu
        shr     esi,1
        cmp     ecx,esi ;x2>x1
        jnc     xhorlin1
        xchg    ecx,esi ;jestli ne tak je prohod
xhorlin1:sub     ecx,esi ;xs=x2-x1+1
        inc     ecx     ;
        mov     edi,eax ;esi je adresa
        mov     ax,_curcolor     ;nacti barvu
xhorlin2:xor     [edi],ax
        add     edi,2
        dec     ecx
        jnz     xhorlin2
        ret

public  ver_line_xor_
ver_line_xor_: ;EAX - Y1 ESI - X ECX - Y2
        cmp     ecx,eax ;y2>y1
        jnc     xverlin1
        xchg    ecx,eax ;jestli ne tak je prohod
xverlin1:sub     ecx,eax
        inc     ecx
        call    getadr_ ;eax obsahuje adresu bodu
        mov     esi,eax
        mov     ax,_curcolor     ;nacti barvu
xverlin2:xor     [esi],ax ;kresli caru po bodech
        add     esi,_linelen
        dec     ecx
        jnz     xverlin2
        ret


_TEXT  ends

END
