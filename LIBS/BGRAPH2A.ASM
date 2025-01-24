.model small
.386


DGROUP group _DATA
extrn _linelen:dword
extrn _screen:dword
extrn _curcolor:word
extrn _charcolors:word[7]
extrn _pictlen:dword
extrn _screen_buffer_size:dword
extrn _lastbank:dword
extrn _granuality:dword
extrn _gran_mask:dword

SEGA000 equ     0a0000h

_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

extrn switchvesabank_:proc


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
        mov     ax,_curcolor    ;nacti aktualni barvu
        or      ebx,ebx ;kontrola zda je ebx>=0
        js      lineup  ;neni pak bude se kreslit nahoru
        mov     esi,edx ;vynuluj esi - pocitadlo mezikroku
        shr     esi,1
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
        mov     esi,edx ;vynuluj esi - pocitadlo mezikroku
        shr     esi,1
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
        cmp     edx,480
        jc      ppok2
        mov     eax,480
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

xvga_data:
        DB      62h,4fh,50h,85h,56h,80h,0bh,3eh
        DB      00h,40h,00h,00h,00h,00h,00h,28h
        Db     0EAh,0ch,0DFh,28h,00h,0e7h,04h,0c3h,0ffh



        public  init_lo_
init_lo_:
        mov eax,13h
        int 10h       ;{Inicializace modu 320x200x256}

        mov edx,3c4h   ;{Zpristupneni Memory Mode reg.}
        mov al,04h    ;{04h -> 3c4h}
        out dx,al
                      ;{Cte port 3c5h a modifikuje ho:}
        mov bl,0f7h   ;{Spristupni video ram pomoci}
        inc edx       ;{bitovych map - Map Mask Reg.}
        in  al,dx     ;{rozdeleni na 4 bitove mapy}
        and al,bl     ;{jedna adresa reprezentuje 4 pixely}
        out dx,al     ;{(kazdy je v jedne ze 4 bitovych map)}

        mov ecx,0ffffh ;{Vynulovani segmentu  0A000h}
        mov edi,0a0000h
        mov al,00h

int_lo1:rep stosb

        cli

        mov edx,3c4h   ;{Port 3c4h -> Index Reset Reg.}
        mov al,00h    ;{Spristupni Reset Register}
        out dx,al

        inc edx        ;{Synchronizujici reset}
        inc al
        out dx,al     ;{Port 3c2h ->}
        mov edx,3c2h  ;{VGA Miscellaneous Output Reg.}
        mov al,0c7h   ;{Nastavi 480 radku  -bit 7,8=1,1}
        out dx,al     ;{        28.322 MHz -bit 3,2=0,1}

        mov edx,3c5h   ;{A opet reset}
        mov al,03h
        out dx,al

        sti
                     ;{Spristupni Color/VGA Data Reg.}
        mov edx,3d4h   ;{Port 3d4h}
        mov al,11h
        out dx,al

        inc edx      ;  {Bit 7 - Protect Bit}
        in  al,dx
        and al,7fh
        out dx,al


        mov esi,offset xvga_data
        mov ecx,19h
        mov bl,00h
int_lo2:
        mov edx,3d4h
        mov al,bl
        out dx,al
        inc edx
        mov al,byte ptr [cs:esi]
        out dx,al

        inc si
        inc bl
        dec ecx

        jne int_lo2

        ret


        public  Redraw_lo_
Redraw_lo_:             ;esi 614Kb obrazovka
                        ;edi lbuffer
                        ;ebx xlattable

        mov     eax,1102h
rdrwlo2:mov     edx,3c4h
        out     dx,ax
        push    eax
        push    esi
        xor     eax,eax
        mov     edi,0xa0000h
        mov     ecx,40*480
rdrwlo1:lodsd
        add     esi,12
        shld     edx,eax,16
        and     eax,7bdeh            ;odmaskuj pro soften
        and     edx,7bdeh
        add     eax,edx              ;soften
        shr     eax,1
        mov     al,[ebx+eax*2]       ;vyber barvu z palety
        mov     dl,al
        lodsd
        add     esi,12
        shld    edx,eax,16
        and     eax,7bdeh            ;odmaskuj pro soften
        and     dx,7bdeh
        add     ax,dx              ;soften
        shr     eax,1
        mov     al,[ebx+eax*2]       ;vyber barvu z palety
        shr     edx,8
        mov     ah,dh
        xchg    ah,al
        stosw
        dec     ecx
        jnz     rdrwlo1
        pop     esi
        pop     eax
        add     esi,4
        rol     ah,1
        jnc     rdrwlo2
        ret

        public  Redraw256_
Redraw256_:              ;esi - source
                        ;edi - target
                        ;ebx - xlat

        test     ebx,1
        jz      rdrwlo7
        inc     ebx
rdrwlo7:xor     eax,eax
rdrwlo4:mov     ecx,480*65536
rdrwlo6:mov     cx,320
rdrwlo5:mov     edx,[esi]
        and     edx,7fff7fffh
        add     esi,4
        mov     ax,dx
        mov     al,[ebx+eax*2]
        xor     ebx,1
        shr     edx,16
        mov     ah,[ebx+edx*2]
        xor     ebx,1
        stosw
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
        mov     ecx,(640*480*2) shr 2
        rep     movsd
        ret


        public  redrawbox_lo_
redrawbox_lo_:          ;esi source
                        ;edi target
                        ;ebx xlat
                        ;ecx xs
                        ;edx,ys
        shr     ecx,1
        mov     eax,1102h
        xchg    ecx,edi
        shr     ecx,1
        rol     ah,cl
        shr     ecx,2
        add     ecx,0a0000h
        xchg    ecx,edi
        shl     ecx,16
        and     edx,0ffffh
        or      ecx,edx
        push    ebp
        mov     edx,3c4h             ;edx adresa sequenceru
        mov     ebp,ecx              ;ebp velikost ramce
        sub     ebp,10000h
rboxlo2:out     dx,ax                ;nastav bit roviny
        push    esi
        push    edi
        push    eax
        push    ebp               ;bp pocet radku
rboxlo1:mov     ecx,[esi]            ;nacti dve barvy
        shld    eax,ecx,16           ;presun horni pulku do ecx
        and     eax,7bdfh            ;odmaskuj pro soften
        and     ecx,7bdfh
        add     eax,ecx              ;soften
        shr     eax,1                ;/2
        mov     al,[ebx+eax*2]       ;vyber barvu z palety
        mov     [edi],al             ;zapis
        add     esi,_linelen         ;dalsi radek
        add     edi,80
        dec     bp                   ;dokud to neni vsechno
        jnz     rboxlo1
        pop     ebp
        pop     eax
        pop     edi
        pop     esi
        add     esi,4
        rol     ah,1
        adc     edi,0
        sub     ebp,10000h
        jnc      rboxlo2
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
        and     edx,07fff7fffh
        add     esi,4
        mov     ax,dx
        mov     al,[ebx+eax*2]
        xor     ebx,1
        shr     edx,16
        mov     ah,[ebx+edx*2]
        xor     ebx,1
        stosw
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
        cmp     edx,480
        jc      gpok2
        mov     eax,480
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

public  switchmap_      ;eax - bank
switchmap_:
        cmp     eax,_lastbank
        jz      swbnk1
        pushad
        mov     _lastbank,eax
        call    switchvesabank_
        popad
swbnk1: ret

public  mapvesaadr_    ;edi - adr
mapvesaadr_:
        push    eax
        push    ecx
        mov     eax,edi
        mov     ecx,_granuality
        shr     eax,cl
        call    switchmap_
        pop     ecx
        pop     eax
        and     edi,_gran_mask
        add     edi,0xa0000
        ret

mapvesaadr macro
        push    eax
        push    ecx
        mov     eax,edi
        mov     ecx,_granuality
        shr     eax,cl
        call    switchmap_
        pop     ecx
        pop     eax
        and     edi,_gran_mask
        add     edi,0xa0000
endm


        public  redraw32b_
redraw32b_:              ;esi source
                        ;edi target
                        ;ebx notused (xlat)
        xor     eax,eax
rdrw32b:mov     ebx,480
rdrw32b1:mov     edi,eax
        call    mapvesaadr_
        mov     ecx,320
        rep     movsd
        add     eax,2048
        dec     ebx
        jnz     rdrw32b1
        ret


        public  redrawbox32b_
redrawbox32b_:           ;esi source
                        ;edi target
                        ;ebx xs
                        ;edx ys
        mov     eax,edi
rbox32b:mov     edi,eax
        call    mapvesaadr_
        mov     ecx,ebx
        shr     ecx,1
        rep     movsd
        rcl     ecx,1
        rep     movsw
        mov     ecx,ebx
        shl     ecx,1
        sub     esi,ecx
        add     eax,2048
        add     esi,_linelen
        dec     edx
        jnz     rbox32b
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
        public  Redraw256b_
Redraw256b_:              ;esi - source
                        ;edi - target
                        ;ebx - xlat
        xor     edi,edi
        call    mapvesaadr_
        xor     eax,eax
rdrwlb4:mov     ecx,480*65536
rdrwlb6:mov     cx,320
rdrwlb5:mov     edx,[esi]
        and     edx,7fff7fffh
        add     esi,4
        mov     ax,dx
        mov     al,[ebx+eax*2]
        shr     edx,16
        xor     ebx,1
        mov     ah,[ebx+edx*2]
        stosw
        swap_at_end di,edi
        xor     ebx,1
        dec     cx
        jnz     rdrwlb5
        xor     ebx,1
        sub     ecx,65536
        jnz     rdrwlb6
        ret

        public  Redrawbox256b_
Redrawbox256b_:          ;esi - source
                        ;edi - target
                        ;ebx - xlat
                        ;ecx - ys
                        ;edx - xs

        shl     ecx,16
        shr     edx,1
        mov     cx,dx
        test    ebx,1
        jz      rboxlb7
        inc     ebx
rboxlb7:xor     eax,eax
rboxlb6:push    ecx
        push    esi
        push    edi
        call    mapvesaadr_
rboxlb5:mov     edx,[esi]
        and     edx,7fff7fffh
        add     esi,4
        mov     ax,dx
        mov     al,[ebx+eax*2]
        shr     edx,16
        xor     ebx,1
        mov     ah,[ebx+edx*2]
        stosw
        swap_at_end di,edi
        xor     ebx,1
        dec     cx
        jnz     rboxlb5
        pop     edi
        pop     esi
        pop     ecx
        add     esi,_linelen
        add     edi,640
        xor     ebx,1
        sub     ecx,65536
        test    ecx,0ffff0000h
        jnz     rboxlb6
        ret

        public  Redraw16_plane
Redraw16_plane:         ;esi - source
                        ;edi - target (A0000h)
                        ;ebx - xlat
                        ;ebp - planenum (jednicka v prislusnem bitu)
        mov     ecx,640*480
        mov     edi,SEGA000
rdrw16_l1:
        shl     eax,1
        movzx   edx,word ptr[esi]
        test    [ebx+edx*2],ebp
        jz      rdrw16_s
        or      eax,1
rdrw16_s:
        add     esi,2
        dec     ecx
        test    ecx,15
        jnz     rdrw16_l1
        xchg    al,ah
        test    ecx,31
        jnz     rdrw16_l1
        rol     eax,16
        stosd
        or      ecx,ecx
        jnz     rdrw16_l1
        ret

        public  Redraw16_
Redraw16_:

        push    ebp
        push    esi
        mov     edx,3c4h
        mov     eax,0802h
        mov     ebp,8h
        out     dx,ax
        call    Redraw16_plane
        pop     esi
        push    esi
        mov     edx,3c4h
        mov     eax,0402h
        mov     ebp,4h
        out     dx,ax
        call    Redraw16_plane
        pop     esi
        push    esi
        mov     edx,3c4h
        mov     eax,0202h
        mov     ebp,2h
        out     dx,ax
        call    Redraw16_plane
        pop     esi
        mov     edx,3c4h
        mov     eax,0102h
        mov     ebp,1h
        out     dx,ax
        call    Redraw16_plane
        pop     ebp
        ret

;Redraw16_:
        push    ebp
        mov     edx,3c4h
        mov     eax,0f02h
        out     dx,ax
        mov     edx,3ceh
        mov     eax,0205h
        out     dx,ax
        mov     eax,8008h
        mov     edi,SEGA000
        mov     ebp,640*480
rdw16:  movzx   ecx,word ptr[esi]
        mov     cl,byte ptr [ebx+ecx*2]
        out     dx,ax
        mov     [edi],cl
        add     esi,2
        ror     ah,1
        adc     edi,0
        dec     ebp
        jnz     rdw16
        pop     ebp
        mov     eax,0005h
        out     dx,ax
        mov     eax,0ff08h
        out     dx,ax
        ret

        public  Redraw16box_plane
                        ;esi - pozice
                        ;edi - output
                        ;ecx - ch = xs, hodni ecx=ys
                        ;esi - source
                        ;ebx - xlat
                        ;ebp - planenum (jednicka v prislusnem bitu)
redraw16box_plane:
        sub     ecx,10000h
rbx16_l1:
        mov     cl,ch
        push    esi
        push    edi
rbx16_l2:
        mov     ah,8
rbx16_l3:
        shl     al,1
        movzx   edx,word ptr[esi]
        test    [ebx+edx*2],ebp
        jz      rbx16_s
        or      al,1
rbx16_s:
        add     esi,2
        dec     ah
        jnz     rbx16_l3
        stosb
        dec     cl
        jnz     rbx16_l2
        pop     edi
        pop     esi
        add     esi,1280
        add     edi,80
        sub     ecx,10000h
        jnc     rbx16_l1
        ret

        public  Redrawbox16_
                        ;esi - pozice
                        ;edi - output
                        ;ecx - xs
                        ;edx - ys
                        ;ebx - xlat
Redrawbox16_:
        push    ebp
        mov     eax,esp
        push    esi
        push    edi
        shl     ecx,16
        and     edx,0ffh
        shl     edx,8
        or      ecx,edx
        push    ecx
        push    eax
        mov     edx,3c4h
        mov     eax,0802h
        out     dx,ax
        mov     ebp,8
        call    Redraw16box_plane
        mov     ebp,esp
        mov     ecx,[ebp+4]
        mov     edi,[ebp+8]
        mov     esi,[ebp+12]
        mov     edx,3c4h
        mov     eax,0402h
        out     dx,ax
        mov     ebp,4
        call    Redraw16box_plane
        mov     ebp,esp
        mov     ecx,[ebp+4]
        mov     edi,[ebp+8]
        mov     esi,[ebp+12]
        mov     edx,3c4h
        mov     eax,0202h
        out     dx,ax
        mov     ebp,2
        call    Redraw16box_plane
        mov     ebp,esp
        mov     ecx,[ebp+4]
        mov     edi,[ebp+8]
        mov     esi,[ebp+12]
        mov     edx,3c4h
        mov     eax,0102h
        out     dx,ax
        mov     ebp,1
        call    Redraw16box_plane
        pop     esp
        pop     ebp
        ret



public  redraw32bb_
Redraw32bb_:
                        ;esi source
                        ;edi target

        mov     ebx,(640*480)*2 ;ebx je velikost obrazovky
        mov     edx,_gran_mask
        inc     edx
rd32bb2:mov     ecx,ebx         ;ecx je velikost prenaseneho bloku
        cmp     ecx,edx         ;kontrola zda li se prenasi prez stranku
        jc      rd32bb1         ;pokud ne pa ecx==ebx
        mov     ecx,edx         ;jinak prenos stranky
rd32bb1:mov     eax,edi         ;uchovej edi
        sub     ebx,ecx         ;ve tvaru o ecx vetsi (po prenosu
        add     eax,ecx         ;a ebx sniz o prenaseny pocet
        shr     ecx,2           ;prenos probiha ve dword rezimu
        mapvesaadr              ;mapuj stranku edi=>edi
        rep     movsd           ;prenos pameti
        mov     edi,eax         ;obnov edi
        or      ebx,ebx         ;test ebx na nulu
        jnz     rd32bb2         ;konec
        ret


public  redrawbox32bb_
redrawbox32bb_:
                        ;esi source
                        ;edi target
                        ;ebx xs
                        ;edx ys

         shl    ebx,1           ;delka je *2
rdb32bb3:mov    eax,edi         ;precti adresu
         and    eax,_gran_mask  ;maskuj stranku
         sub    eax,_gran_mask  ;eax je pocet bajtu do nasledujiciho prenuti stranky
         neg    eax             ;+1
         inc    eax
         push   ebx             ;uchovej delku ctverce v zasobniku
rdb32bb2:mov    ecx,ebx         ;vem minimum z cisel ebx a eax a dej to ecx
         cmp    ecx,eax
         jc     rdb32bb1
         mov    ecx,eax
rdb32bb1:sub    ebx,ecx         ;odecti delku prenasenych bajtu
         add    edi,ecx         ;pricti delku k adrese
         push   edi             ;uloz to do sp
         sub    edi,ecx         ;sniz adresu na puvodni hodnotu
         shr    ecx,2           ;del ecx 4 (presun dvojslov)
         mapvesaadr             ;namapuj adresu a banku
         rep    movsd           ;PRESUN
         pop    edi             ;obnov adresu
         or     ebx,ebx         ;kontrola zda je prenesen radek
         jnz    rdb32bb2        ;ne, opakuj od urceni minima
         pop    ebx             ;ano, obnov delku ctverce ze zasobniku
         mov    ecx,1280        ;od 1280 (640*2) odecti
         sub    ecx,ebx         ;delku radky
         add    edi,ecx         ;pricti to jak k cili
         add    esi,ecx         ;          tak ke zdroji
         dec    edx             ;sniz citac radku
         jnz    rdb32bb3        ;dokud neni 0
         ret
_TEXT  ends
END
