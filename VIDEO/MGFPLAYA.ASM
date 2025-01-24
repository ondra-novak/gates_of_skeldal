.model small
.386
;16 bit line (de)comprimator
;
DGROUP group _DATA

extrn _lbuffer:dword
extrn _backsndbuff:word
extrn _vals_save:dword
extrn _backsnd:dword
extrn _lastbank:dword
extrn _gr_page_end:word
extrn _gr_end_screen:dword


_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

extrn mapvesaadr_:proc
extrn switchmap_:proc
extrn switchvesabank_:proc



public show_full_interl_lfb_

show_full_interl_lfb_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp
        mov     dl,180
shfif2: mov     ecx,320
shfif1: lodsb
        movzx   eax,al
        movzx   eax,word ptr [eax*2+ebx]
        mov     ebp,eax
        shl     eax,16
        or      eax,ebp
        stosd
        dec     ecx
        jnz     shfif1
        add     edi,1280
        dec     dl
        jnz     shfif2
        pop     ebp
        ret


public show_delta_interl_lfb_
show_delta_interl_lfb_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp             ;uchovej ebp
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
        movzx   eax,word ptr[eax*2+ebx] ;expanduj hicolor barvu
        mov     ebp,eax         ;rozdvoj barvy
        shl     ebp,16
        or      eax,ebp
        stosd                   ;zapis dva body
        lodsb                   ;opakuj pro dalsi bod jeste jednou
        movzx   eax,al
        movzx   eax,word ptr[eax*2+ebx]
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
        add     edi,1280*2      ;preskoc radek
        dec     cl              ;odecti citac radku
        jnz     shdif6          ;skok pokud neni konec
        pop     ebp
        ret                     ;navrat
shdif4: inc     ch              ;pocet radek je ch+1
        sub     cl,ch           ;odecti ch od zbyvajicich radek
        jz      shdif5          ;je-li nula tak konec
shdif7: add     edi,1280*2      ;preskoc radek
        dec     ch              ;odecti ch
        jnz     shdif7          ;preskakuj dokud neni 0
        jmp     shdif6          ;cti dalsi _skip_
shdif5: pop     ebp
        ret                     ;konec


public show_full_interl_lfb130_

show_full_interl_lfb130_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp
        mov     dl,180
shfil2: mov     ecx,160
shfil1: lodsw
        movzx   ebp,al
        movzx   ebp,word ptr ds:[ebp*2+ebx]
        movzx   eax,ah
        movzx   eax,word ptr ds:[eax*2+ebx]
        shl     eax,16
        or      eax,ebp
        stosd
        dec     ecx
        jnz     shfil1
        add     edi,640
        dec     dl
        jnz     shfil2
        pop     ebp
        ret


public show_delta_interl_lfb130_
show_delta_interl_lfb130_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp             ;uchovej ebp
        mov     cl,180          ;cl pocet zbyvajicich radek
        add     esi,4           ;preskoc ukazatel
        mov     edx,esi         ;edx - zacatek delta mapy
        add     esi,[esi-4]     ;esi - zacatek dat
shdil6: push    edi             ;uloz adresu radku
shdil2: mov     ch,[edx]        ;cti _skip_ hodnotu
        mov     al,ch
        inc     edx
        or      al,03fh         ;test zda jsou 2 nejvyssi bity nastaveny
        inc     al
        jz      shdil3          ;ano - preskakovani radku
        movzx   eax,ch          ;expanduj _skip_ hodnotu do eax
        lea     edi,[eax*4+edi] ;vypocti novou pozici na obrazovce
        mov     ch,[edx]        ;cti _copy_ hodnotu
        inc     edx
shdil1: lodsw
        movzx   ebp,al
        movzx   ebp,word ptr ds:[ebp*2+ebx]
        movzx   eax,ah
        movzx   eax,word ptr ds:[eax*2+ebx]
        shl     eax,16
        or      eax,ebp
        stosd
        dec     ch              ;odecti _copy_ hodnotu
        jnz     shdil1          ;dokud neni 0
        jmp     shdil2          ;pokracuj _skip_ hodnotou
shdil3: and     ch,3fh          ;odmaskuj hodni 2 bity
        pop     edi             ;obnov edi
        jnz     shdil4          ;pokud je ch=0 preskoc jen jeden radek;
        add     edi,1280        ;preskoc radek
        dec     cl              ;odecti citac radku
        jnz     shdil6          ;skok pokud neni konec
        pop     ebp
        ret                     ;navrat
shdil4: inc     ch              ;pocet radek je ch+1
        sub     cl,ch           ;odecti ch od zbyvajicich radek
        jz      shdil5          ;je-li nula tak konec
shdil7: add     edi,1280        ;preskoc radek
        dec     ch              ;odecti ch
        jnz     shdil7          ;preskakuj dokud neni 0
        jmp     shdil6          ;cti dalsi _skip_
shdil5: pop     ebp
        ret                     ;konec


public show_full_lfb12e_

show_full_lfb12e_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp
        mov     dl,180
shfl2: mov     ecx,160
shfl1: lodsw
        movzx   ebp,al
        movzx   ebp,word ptr ds:[ebp*2+ebx]
        movzx   eax,ah
        movzx   eax,word ptr ds:[eax*2+ebx]
        shl     eax,16
        or      eax,ebp
        stosd
        dec     ecx
        jnz     shfl1
        dec     dl
        jnz     shfl2
        pop     ebp
        ret


public show_delta_lfb12e_
show_delta_lfb12e_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
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
        movzx   ebp,word ptr ds:[ebp*2+ebx]
        movzx   eax,ah
        movzx   eax,word ptr ds:[eax*2+ebx]
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
        ret                     ;navrat
shdl4: inc     ch              ;pocet radek je ch+1
        sub     cl,ch           ;odecti ch od zbyvajicich radek
        jz      shdl5          ;je-li nula tak konec
shdl7: add     edi,640        ;preskoc radek
        dec     ch              ;odecti ch
        jnz     shdl7          ;preskakuj dokud neni 0
        jmp     shdl6          ;cti dalsi _skip_
shdl5: pop     ebp
        ret                     ;konec



nextbank macro
        mov     eax,_lastbank           ;ano, pak vem cislo banky
        inc     eax                     ;pricti 1
        mov     _lastbank,eax           ;zapis jako nove cislo banky
        pushad                          ;uchovej vse
        call    switchvesabank_         ;prepni banku
        popad                           ;obnov vse
        movzx   edi,di
        add     edi,0a0000h             ;vynuluj dolni cast di
endm

skipm   macro   reg
        local   skp1

        mov     ax,_gr_page_end
        movzx   edi,di
        dec     ax
        add     edi,reg
        inc     eax
        cmp     edi,eax
        jc      skp1
        mov     eax,_lastbank           ;ano, pak vem cislo banky
        inc     eax                     ;pricti 1
        mov     _lastbank,eax           ;zapis jako nove cislo banky
        pushad                          ;uchovej vse
        call    switchvesabank_         ;prepni banku
        popad                           ;obnov vse
        movzx   edi,di
skp1:   or      edi,0a0000h
endm



public show_full_interl_bank_
show_full_interl_bank_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        call    mapvesaadr_
        mov     dl,180
shfib2: mov     ecx,320
        push    ebp
shfib1: cmp     edi,_gr_end_screen
        jc      shfib3
        nextbank
shfib3: lodsb
        movzx   eax,al
        movzx   eax,word ptr [eax*2+ebx]
        mov     ebp,eax
        shl     eax,16
        or      eax,ebp
        stosd
        dec     ecx
        jnz     shfib1
        pop     ebp
        add     edi,1280
        dec     dl
        jnz     shfib2
        ret


public show_delta_interl_bank_
show_delta_interl_bank_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        mov     cl,180          ;cl pocet zbyvajicich radek
        add     esi,4           ;preskoc ukazatel
        mov     edx,esi         ;edx - zacatek delta mapy
        add     esi,[esi-4]     ;esi - zacatek dat
shdib6: push    ebp             ;uchovej ebp
        push    edi
        call    mapvesaadr_
shdib2: mov     ch,[edx]        ;cti _skip_ hodnotu
        mov     al,ch
        inc     edx
        or      al,03fh         ;test zda jsou 2 nejvyssi bity nastaveny
        inc     al
        jz      shdib3          ;ano - preskakovani radku
        movzx   eax,ch          ;expanduj _skip_ hodnotu do eax
        lea     edi,[eax*8+edi] ;vypocti novou pozici na obrazovce
        mov     ch,[edx]        ;cti _copy_ hodnotu
        inc     edx
        cmp     edi,_gr_end_screen
        jc      shdib1
        nextbank
shdib1: lodsb                   ;vem bajt z datove oblasti
        movzx   eax,al          ;expanduj do eax
        movzx   eax,word ptr[eax*2+ebx] ;expanduj hicolor barvu
        mov     ebp,eax         ;rozdvoj barvy
        shl     ebp,16
        or      eax,ebp
        stosd                   ;zapis dva body
        cmp     edi,_gr_end_screen
        jc      shdib8
        nextbank
shdib8: lodsb                   ;opakuj pro dalsi bod jeste jednou
        movzx   eax,al
        movzx   eax,word ptr[eax*2+ebx]
        mov     ebp,eax
        shl     ebp,16
        or      eax,ebp
        stosd
        dec     ch              ;odecti _copy_ hodnotu
        jnz     shdib1          ;dokud neni 0
        jmp     shdib2          ;pokracuj _skip_ hodnotou
shdib3: pop     edi
        pop     ebp
        and     ch,3fh          ;odmaskuj hodni 2 bity
        jnz     shdib4          ;pokud je ch=0 preskoc jen jeden radek;
        add     edi,2*1280
        dec     cl              ;odecti citac radku
        jnz     shdib6          ;skok pokud neni konec
        ret                     ;navrat
shdib4: inc     ch              ;pocet radek je ch+1
        sub     cl,ch           ;odecti ch od zbyvajicich radek
        jz      shdib5          ;je-li nula tak konec
shdib7: add     edi,2*1280      ;preskoc radek
        dec     ch              ;odecti ch
        jnz     shdib7          ;preskakuj dokud neni 0
        jmp     shdib6          ;cti dalsi _skip_
shdib5: ret                     ;konec



public  sound_decompress_       ;dekompresuje hudbu

sound_decompress_:              ;esi - source
                                ;edi - target pos in backsndbuff
                                ;ecx - datasize
                                ;ebx - ampl_table
                                ;(vraci edi - pozici v backsndbuff)
        mov     edx,_vals_save
snddec1:lodsb
        movzx   eax,al
        movzx   eax,word ptr [eax*2+ebx]
        add     dx,ax
        mov     _backsndbuff[edi],dx
        add     edi,2
        and     edi,3ffffh
        rol     edx,16
        dec     ecx
        jnz     snddec1
        mov     _vals_save,edx
        ret

public test_next_frame_         ;testuje zda je volno pro dalsi zvukovy blok

test_next_frame_:               ;edi - pozice
                                ;ecx - delka bloku
        mov     ebx,_backsnd
        shl     ecx,1
        shl     ebx,2
        xor     ecx,3ffffh
        sub     edi,ebx
        jnc     tnf1
        add     edi,40000h
tnf1:   cmp     ecx,edi
        rcl     al,1
        and     al,1
        ret



public show_full_interl_lfb_256_

show_full_interl_lfb_256_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        mov     dl,180
s2fif2: mov     ecx,320
s2fif1: lodsb
        movzx   eax,al
        movzx   eax,word ptr [eax*2+ebx]
        stosw
        dec     ecx
        jnz     s2fif1
        add     edi,640
        dec     dl
        jnz     s2fif2
        ret


public show_delta_interl_lfb_256_
show_delta_interl_lfb_256_:
                        ;edi - target
                        ;esi - source
                        ;ebx - palette
        push    ebp             ;uchovej ebp
        mov     cl,180          ;cl pocet zbyvajicich radek
        add     esi,4           ;preskoc ukazatel
        mov     edx,esi         ;edx - zacatek delta mapy
        add     esi,[esi-4]     ;esi - zacatek dat
s2dif6: push    edi             ;uloz adresu radku
s2dif2: mov     ch,[edx]        ;cti _skip_ hodnotu
        mov     al,ch
        inc     edx
        or      al,03fh         ;test zda jsou 2 nejvyssi bity nastaveny
        inc     al
        jz      s2dif3          ;ano - preskakovani radku
        movzx   eax,ch          ;expanduj _skip_ hodnotu do eax
        lea     edi,[eax*4+edi] ;vypocti novou pozici na obrazovce
        mov     ch,[edx]        ;cti _copy_ hodnotu
        inc     edx
s2dif1: lodsb                   ;vem bajt z datove oblasti
        movzx   eax,al          ;expanduj do eax
        movzx   eax,word ptr[eax*2+ebx] ;expanduj hicolor barvu
        mov     ebp,eax         ;rozdvoj barvy
        lodsb                   ;opakuj pro dalsi bod jeste jednou
        movzx   eax,al
        movzx   eax,word ptr[eax*2+ebx]
        shl     eax,16
        or      eax,ebp
        stosd                   ;zapis 4 body
        dec     ch              ;odecti _copy_ hodnotu
        jnz     s2dif1          ;dokud neni 0
        jmp     s2dif2          ;pokracuj _skip_ hodnotou
s2dif3: and     ch,3fh          ;odmaskuj hodni 2 bity
        pop     edi             ;obnov edi
        jnz     s2dif4          ;pokud je ch=0 preskoc jen jeden radek;
        add     edi,640*2      ;preskoc radek
        dec     cl              ;odecti citac radku
        jnz     s2dif6          ;skok pokud neni konec
        pop     ebp
        ret                     ;navrat
s2dif4: inc     ch              ;pocet radek je ch+1
        sub     cl,ch           ;odecti ch od zbyvajicich radek
        jz      s2dif5          ;je-li nula tak konec
s2dif7: add     edi,640*2      ;preskoc radek
        dec     ch              ;odecti ch
        jnz     s2dif7          ;preskakuj dokud neni 0
        jmp     s2dif6          ;cti dalsi _skip_
s2dif5: pop     ebp
        ret                     ;konec

_TEXT ends

end
