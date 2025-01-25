.model small
.386P

DGROUP group _DATA


tchannel struct
        PLAYPOS         DD ?
        STARTLOOP       DD ?
        ENDLOOP         DD ?
        SPEEDMAJ        DD ?
        SPEEDMIN        DW ?
        MINORPOS        DW ?
        SMPTYPE         DW ?;0 - DISABLED, 1 - 8 BIT, 2 - 16 BIT
        VOLUMELEFT      DW ?
        VOLUMERIGHT     DW ?
tchannel ends

rm_playdata struct
        XLAT_TAB        DB 256 DUP(?)
        SCANOFS         DW ?
        SCANSEG         DW ?
        PORT            DW ?
rm_playdata ends

extrn  _chaninfo:dword
extrn  _mixbuffer:dword
extrn  _backsnd:dword    ;ukazatel na buffer s hudbou na pozadi (64Kb)
extrn  _backstep:dword   ;krok o kolik se meni _backsnd
extrn  _backfine:dword   ;citac udavajici kdy se zmeni _backsnd
extrn  _backsndbuff:dword ;buffer hudby na pozadi
extrn  _call_back_data:dword ;data_call_back_procedury
extrn  _call_back_sel:word ;data_call_back_procedury

extrn    _jumptable:dword[3] ;[0] - skip, [1] - mix8, [2] - mix16
extrn    _getdma:dword
extrn    _ido:dword

extrn    _predstih:word
extrn    _lastdma:dword
extrn    _lastmix:dword
extrn    _surpos:dword
extrn    _mixpos:dword
extrn    _mixsize:dword
extrn   _samplerate:dword

extrn   _dmaposadr:dword
extrn   _dmasizadr:dword
extrn   _dmanum:byte
extrn   _dpmiselector:word

extrn   _idt_map:dword

extrn   _test_counter:dword

extrn   _mixer_zavora:byte
extrn   _timer_test_port:word
extrn   _oldvect:dword

extrn   _devport:dword
extrn   _devirq:dword

extrn   _gus_dma_type:byte
extrn   _gus_last_buff:byte

_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

gus_out         equ     0
gus_irqs        equ     6
gus_timer_c     equ     8
gus_timer_d     equ     9
gus_irq_dma     equ     0bh
gus_voice       equ     102h
gus_reg         equ     103h
gus_reg_data    equ     104h
gus_pio         equ     107h

gus_block       equ     2000h



        PUBLIC  MIXER_
MIXER_: ;THIS IS MAIN MIXING PROCEDURE.
        ;BEFORE USING, YOU MUST SET UP ALL VARIBLES TO THE CORRECT VALUES.
        ;_MIXBUFFER MUST START ON PAGE OF MEMORY (EX. 0x10000 0x20000 0x30000)
        ;PROCEDURE MUST BE CALLED EACH 55ms OR LITTLE.
        ;IF YOU USING INTERRUPT, REMEBER, THAT YOU MUST STORE ALL REGISTERS
        ;BEFORE USE.

        cmp     _mixer_zavora,0
        jnz     mixend
        inc     _mixer_zavora
        CALL    calcsize       ;VYPOCET DELKY MIXOVANI
        mov     edi,_mixpos     ;Nejprve se vymaze mixovaci pamet
        mov     ecx,_mixsize
        shr     ecx,2
        jz      MIXALL
MIXOK:  xor     eax,eax
MIXCLR: mov     [edi],ax
        add     di,2
        mov     [edi],ax
        add     di,2
        dec     ecx
        jnz     MIXCLR
        lea     ebx,_chaninfo   ;TED SE ZACNE MIXOVAT
MIXING: xor     eax,eax
        mov     ax,smptype[ebx]
        call    _jumptable[eax*4]
        add     ebx,sizeof(tchannel)
        cmp     ebx,(32*sizeof(tchannel))+offset _chaninfo
        jnz     MIXING
        call    _ido
        mov     ecx,_mixsize
        add     word ptr _mixpos,cx
MIXALL: mov     _mixer_zavora,0
MIXEND: ret


CALCSIZE:call   _getdma         ;eax - pozice v mixovaci pameti
        mov     ecx,eax         ;ecx - pozice dma
        sub     cx,word ptr _mixpos     ;odecti mixpos (dma-mix_pos<0)
        js      CALCOK
        shr     cx,2
        ;add     _predstih,cx
        ;add     _predstih,64
CALCOK: mov     ebx,_lastdma
        mov     _lastdma,eax
        sub     ax,bx
        add     ax,_predstih
        add     ax,word ptr _lastdma
        mov     bx,word ptr _mixpos
        sub     ax,bx
        js      CALCEND
        and     ax,not 3
        add     ax,4
        and     eax,0ffffh
        mov     _mixsize,eax
        ret
CALCEND:mov     _mixsize,0
        ret

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



MIXS_   :;primixuje sampl do bufferu  (8-bit stereo)
         ;musi platit vsechny promenne a ebx je adresa kanalu
        mov     edi,_mixpos
        mov     ecx,_mixsize
        mov     esi,[ebx]
        push    ebp
        mov     bp,minorpos[ebx]
mixslp: mov     dl,[esi]
        mov     dh,dl
        mov     al,byte ptr volumeleft[ebx+1]
        imul    dl
        shl     eax,1
        mov     dl,ah
        mov     al,byte ptr volumeright[ebx+1]
        imul    dh
        shl     eax,1
        mov     dh,ah
        mov     ax,[edi]
        addc    al,dl,7fh
        addc    ah,dh,7fh
        mov     [edi],ax        ;je smixovano
        add     di,2            ;dalsi pozice
        add     bp,speedmin[ebx]
        adc     esi,speedmaj[ebx]
        cmp     esi,endloop[ebx]
        jc      mixsskp
        mov     esi,startloop[ebx]
        cmp     esi,endloop[ebx]
        jnz     mixsskp
        mov     smptype[ebx],0
        jmp     mixsend
mixsskp:dec     ecx
        dec     ecx
        jnz     mixslp
mixsend:mov     minorpos[ebx],bp
        mov     playpos[ebx],esi
        pop     ebp
        ret

mixskip:ret
mixs2 :;primixuje sampl do bufferu  (16-bit stereo)
        ;musi platit vsechny promenne a ebx je adresa kanalu
        mov     edi,_mixpos        ;vem mixovaci pozici
        mov     ecx,_mixsize       ;vem pocet potrebnych bajtu
        mov     esi,[ebx]          ;vyzvedni ukazatel na sample
        push    ebp                ;zachovej bp
        mov     bp,minorpos[ebx]   ;bp pro tuto chvili predstavuje minorpozici
mix2slp:mov     dl,[esi+1]         ;vem sample, ale jen vyssi bajt
        mov     dh,dl              ;zkopiruj levy kanal do praveho
        mov     al,byte ptr volumeleft[ebx+1];vyzvedni hlasitost leveho kanalu
        imul    dl                 ;nasob vzorek hlasitosti
        shl     eax,1              ;vysledek je v ah vydelen 256x
        mov     dl,ah              ;schovej vysledek do dl
        mov     al,byte ptr volumeright[ebx+1];to same pro pravy kanal
        imul    dh
        shl     eax,1
        mov     dh,ah
        mov     ax,[edi]        ;ted precti aktualni stav samplu v bufferu
        addc    al,dl,7fh       ;pricti levy kanal s clippingem
        addc    ah,dh,7fh       ;pricti pravy kanal s clippingem
        mov     [edi],ax        ;je smixovano
        add     di,2            ;dalsi pozice
        add     bp,speedmin[ebx]  ;skok po minor hodnotach
        lahf                      ;uchovej priznaky v ah (hlavne cf)
        adc     esi,speedmaj[ebx] ;skok po major hodnotach
        sahf                      ;obnov  cf
        adc     esi,speedmaj[ebx] ;a jeste jednou celkem o 2xvic nez je rychlost
        cmp     esi,endloop[ebx]  ;test na konec samplu
        jc      mix2sskp           ;pokud jsme prekrocili konec mohou nastat dva
        mov     esi,startloop[ebx];pripady:
        cmp     esi,endloop[ebx]  ;  bud ma sampl opakovani pak zacina na adrese
        jnz     mix2sskp           ;  startloop nebo nema
        mov     smptype[ebx],0    ;  pak je vypnut
        jmp     mix2send           ;skoc na konec
mix2sskp:dec     ecx               ;pokracujeme v prehravani
        dec     ecx               ;byly naplneny 2 bajty, citac o 2 dolu
        jnz     mix2slp            ;opakuj cele mixovani dokud neni citac nule
mix2send:mov     minorpos[ebx],bp  ;uloz minorpozici
        mov     playpos[ebx],esi  ;uloz majorpozici
        pop     ebp               ;obnov BP
        ret                       ;konec

m8sido: mov     esi,_mixpos             ;vem mixovaci pozici
        sub     si,2                    ;o jeden sampl dozadu
        and     esi,not 1
        mov     edi,_backfine           ;finepozice
        mov     ebx,_backsnd            ;pozice v bufferu
        mov     ecx,_mixsize            ;pocet bajtu
        sub     ecx,2
        shl     ecx,16                  ;uloz do horni poloviny ecx
        mov     ax,[esi]                ;vezmi sample
        xor     eax,8080h               ;neznaminkova korekce
m8sido1:add     si,2                    ;dalsi sample
        mov     dx,[esi]                ;nacti do dx sample
        xor     edx,8080h               ;neznaminkova korekce
        add     al,dl                   ;secti leve kanaly
        rcr     al,1                    ;nasleduje deleni 2x prumer
        add     ah,dh                   ;totez pro prave kanaly
        rcr     ah,1
        xor     eax,8080h               ;neznaminkova korekce
        mov     cl,byte ptr _backsndbuff[1+ebx*4]  ;nacti back sound pro levej kanal
        mov     ch,byte ptr _backsndbuff[3+ebx*4];nactu back sound pro pravej kanal
        addc    al,cl,7fh               ;secti s clippingem levy kanal
        addc    ah,ch,7fh               ;secti s clippingem pravy kanal
        add     di,word ptr _backstep   ;pricti fine pozici
        adc     bx,word ptr [_backstep+2];preteceni jde do bx plus step
;        and     ebx,03ffffh              ;celkem 16384*4 samplu
        xor     eax,8080h               ;neznaminkova korekce
        sub     si,2                    ;uloz na zdrojovou pozici
        mov     [esi],ax
        add     si,2
        mov     eax,edx                 ;pouzity sampl pro prumerovani se
        sub     ecx,20000h              ;do ax a odecti dva takty pro citac
        jnc     m8sido1                 ;a opakuj dokud neni konec
        mov     _backsnd,ebx
        mov     _backfine,edi
        ret                             ;navrat


sbdma:  mov     edx,_dmaposadr
        out     0ch,al
        out     0d8h,al
        xor     eax,eax
        in      al,dx
        xchg    al,ah
        in      al,dx
        xchg    al,ah
        cmp     _dmanum,4
        jc      sbdma1
        shl     eax,1
sbdma1: add     eax,_mixbuffer
        ret


PUBLIC setsbpro_
setsbpro_:
        lea     edi,_jumptable
        mov     eax,offset mixskip
        stosd
        mov     eax,offset mixs_
        stosd
        mov     eax,offset mixs2
        stosd
        mov     _getdma,offset sbdma
        mov     _ido,offset m8sido
        ret


MIXM_   :;primixuje sampl do bufferu  (8-bit mono)
         ;musi platit vsechny promenne a ebx je adresa kanalu
        mov     edi,_mixpos    ;vezmi mixovaci pozici
        mov     ecx,_mixsize   ;vezmi delku mixovani
        mov     esi,[ebx]      ;nacti ukazatel na aktualni pozici v samplu
        push    ebp            ;uchovej docasne BP
        mov     bp,minorpos[ebx] ;bp ma novou ulohu, drzi minor pozici
        mov     al,byte ptr volumeleft[ebx+1] ;vypocti hlasitost
        mov     ah,byte ptr volumeright[ebx+1]
        add     al,ah
        rcr     al,1           ;jak prumer leve a prave hlasitosti
        mov     dh,al
mixmlp: mov     al,[esi]       ;vezmi sample
        imul    dh             ;vynasob s hlasitosti
        shl     eax,1          ;vydel /256x (ah je vysledek)
        mov     al,[edi]       ;vezmi momentalni obsah bufferu
        addc    al,ah,7fh      ;secti s clippingem
        mov     [edi],al        ;je smixovano
        inc     di              ;dalsi pozice
        add     bp,speedmin[ebx]
        adc     esi,speedmaj[ebx]
        cmp     esi,endloop[ebx]
        jc      mixmskp
        mov     esi,startloop[ebx]
        cmp     esi,endloop[ebx]
        jnz     mixmskp
        mov     smptype[ebx],0
        jmp     mixmend
mixmskp:dec     ecx
        jnz     mixmlp
mixmend:mov     minorpos[ebx],bp
        mov     playpos[ebx],esi
        pop     ebp
        ret

MIXM2 :;primixuje sampl 16 do bufferu  (8-bit mono)
        ;musi platit vsechny promenne a ebx je adresa kanalu
        mov     edi,_mixpos    ;vezmi mixovaci pozici
        mov     ecx,_mixsize   ;vezmi delku mixovani
        mov     esi,[ebx]      ;nacti ukazatel na aktualni pozici v samplu
        push    ebp            ;uchovej docasne BP
        mov     bp,minorpos[ebx] ;bp ma novou ulohu, drzi minor pozici
        mov     al,byte ptr volumeleft[ebx+1] ;vypocti hlasitost
        mov     ah,byte ptr volumeright[ebx+1]
        add     al,ah
        rcr     al,1           ;jak prumer leve a prave hlasitosti
        mov     dh,al
mixmlp2:mov     al,[esi+1]       ;vezmi sample
        imul    dh             ;vynasob s hlasitosti
        shl     eax,1          ;vydel /256x (ah je vysledek)
        mov     al,[edi]       ;vezmi momentalni obsah bufferu
        addc    al,ah,7fh      ;secti s clippingem
        mov     [edi],al        ;je smixovano
        inc     di              ;dalsi pozice
        add     bp,speedmin[ebx]
        lahf                      ;uchovej priznaky v ah (hlavne cf)
        adc     esi,speedmaj[ebx] ;skok po major hodnotach
        sahf                      ;obnov  cf
        adc     esi,speedmaj[ebx] ;a jeste jednou celkem o 2xvic nez je rychlost
        cmp     esi,endloop[ebx]
        jc      mixmskp2
        mov     esi,startloop[ebx]
        cmp     esi,endloop[ebx]
        jnz     mixmskp2
        mov     smptype[ebx],0
        jmp     mixmend2
mixmskp2:dec     ecx
        jnz     mixmlp2
mixmend2:mov     minorpos[ebx],bp
        mov     playpos[ebx],esi
        pop     ebp
        ret

m8mido: mov     esi,_mixpos             ;vem mixovaci pozici
        dec     si                      ;o jeden sampl dozadu
        lea     edi,_backfine        ;ukazatel na back sound buffer
        mov     ebx,_backsnd            ;pozice v bufferu
        mov     ecx,_mixsize            ;pocet bajtu
        dec     ecx
        shl     ecx,16                  ;uloz do horni poloviny ecx
        mov     al,[esi]                ;vezmi sample
        xor     eax,80h                 ;neznaminkova korekce
m8mido1:inc     si                      ;dalsi sample
        mov     dl,[esi]                ;nacti do dx
        xor     edx,80h                 ;neznaminkova korekce
        add     al,dl                   ;secti kanal
        rcr     al,1                    ;nasleduje deleni 2x prumer
        xor     eax,80h                 ;vysledek oznamenkuj - bude se scitat
        mov     cl,byte ptr _backsndbuff[1+ebx*4];nacti back sound pro levej kanal
        mov     ch,byte ptr _backsndbuff[3+ebx*4];nactu back sound pro pravej kanal
        add     di,word ptr _backstep   ;pricti fine pozici
        adc     bx,word ptr [_backstep+2];preteceni jde do bx plus step
;       and     ebx,03ffffh             ;celkem 16384*4 samplu
        sar     cl,1                    ;del levy kanal 2ma
        sar     ch,1                    ;del pravy kanal 2ma
        add     cl,ch                   ;secti oba kanaly
        addc    al,cl,7fh               ;secti s clippingem kanal
        xor     eax,80h                 ;neznaminkova korekce pro SB Pro
        dec     si                      ;uloz na zdrojovou pozici
        mov     [esi],al
        inc     si
        mov     eax,edx                 ;pouzity sampl pro prumerovani se
        sub     ecx,10000h              ;do ax a odecti jeden takt pro citac
        jnc     m8mido1                 ;a opakuj dokud neni konec
        mov     _backsnd,ebx
        ret                             ;navrat

m8midos:mov     esi,_mixpos             ;vem mixovaci pozici
        dec     si                      ;o jeden sampl dozadu
        lea     edi,_backfine        ;ukazatel na back sound buffer
        mov     ebx,_backsnd            ;pozice v bufferu
        mov     ecx,_mixsize            ;pocet bajtu
        dec     ecx
        shl     ecx,16                  ;uloz do horni poloviny ecx
        mov     al,[esi]                ;vezmi sample
        xor     eax,80h                 ;neznaminkova korekce
m8mido2:inc     si                      ;dalsi sample
        mov     dl,[esi]                ;nacti do dx
        xor     edx,80h                 ;neznaminkova korekce
        add     al,dl                   ;secti kanal
        rcr     al,1                    ;nasleduje deleni 2x prumer
        xor     eax,80h                 ;vysledek oznamenkuj - bude se scitat
        mov     cl,byte ptr _backsndbuff[1+ebx*4];nacti back sound pro levej kanal
        mov     ch,byte ptr _backsndbuff[3+ebx*4];nactu back sound pro pravej kanal
        add     di,word ptr _backstep   ;pricti fine pozici
        adc     bx,word ptr [_backstep+2];preteceni jde do bx plus step
;       and     ebx,03ffffh             ;celkem 16384*4 samplu
        sar     cl,1                    ;del levy kanal 2ma
        sar     ch,1                    ;del pravy kanal 2ma
        add     cl,ch                   ;secti oba kanaly
        addc    al,cl,7fh               ;secti s clippingem kanal
        dec     si                      ;uloz na zdrojovou pozici
        mov     [esi],al
        inc     si
        mov     eax,edx                 ;pouzity sampl pro prumerovani se
        sub     ecx,10000h              ;do ax a odecti jeden takt pro citac
        jnc     m8mido2                 ;a opakuj dokud neni konec
        mov     _backsnd,ebx
        ret                             ;navrat




PUBLIC setsb2_
setsb2_:
        lea     edi,_jumptable
        mov     eax,offset mixskip
        stosd
        mov     eax,offset mixm_
        stosd
        mov     eax,offset mixm2
        stosd
        mov     _getdma,offset sbdma
        mov     _ido,offset m8mido
        ret

PUBLIC setsb2_s_
setsb2_s_:
        lea     edi,_jumptable
        mov     eax,offset mixskip
        stosd
        mov     eax,offset mixm_
        stosd
        mov     eax,offset mixm2
        stosd
        mov     _getdma,offset sbdma
        mov     _ido,offset m8midos
        ret


MIX16_ :;primixuje sampl do bufferu  (16-bit stereo)
        ;musi platit vsechny promenne a ebx je adresa kanalu
        mov     edi,_mixpos             ;vem mixovaci pozici
        mov     ecx,_mixsize            ;vem mixovaci delku
        mov     esi,[ebx]               ;vyzvedni ukazatel na sample
        push    ebp                     ;uchovej BP
        mov     bp,minorpos[ebx]        ;bp bude drzet minor pozice
mix1lp: mov     al,[esi]                ;nacti sample
        mov     dl,al                   ;al a dl obsahuji levej a pravej kanal
        mov     ah,byte ptr volumeright[ebx+1] ;nacti pravou hlastitot
        imul    ah                      ;vynasob hlasitosti
        shl     eax,1                   ;vse je vyreseno, deleni neni potreba
        xchg    edx,eax                 ;ted pracuj s druhym kanalem
        mov     ah,byte ptr volumeleft[ebx+1] ;nactu hlasitost
        imul    ah                      ;vynasob
        shl     eax,1
        addc    [edi],dx,7fffh          ;k aktualnimu vzorku pricti hodnotu
        add     di,2                    ;dalsi pozice
        addc    [edi],ax,7fffh          ;je smixovano
        add     di,2                    ;dalsi pozice
        add     bp,speedmin[ebx]
        adc     esi,speedmaj[ebx]
        cmp     esi,endloop[ebx]
        jc      mix1skp
        mov     esi,startloop[ebx]
        cmp     esi,endloop[ebx]
        jnz     mix1skp
        mov     smptype[ebx],0
        jmp     mix1end
mix1skp:sub     ecx,4
        jnz     mix1lp
mix1end:mov     minorpos[ebx],bp
        mov     playpos[ebx],esi
        pop     ebp
        ret

mix162: ;primixuje sampl do bufferu  (16-bit stereo)
        ;musi platit vsechny promenne a ebx je adresa kanalu
        mov     edi,_mixpos             ;vem mixovaci pozici
        mov     ecx,_mixsize            ;vem mixovaci delku
        mov     esi,[ebx]               ;vyzvedni ukazatel na sample
        push    ebp                     ;uchovej BP
        mov     bp,minorpos[ebx]        ;bp bude drzet minor pozice
mix12lp:movsx   eax,word ptr [esi]      ;nacti sample
        imul    word ptr volumeright[ebx] ;nacti pravou hlastitot a vynasob
        shl     edx,1                   ;vse je vyreseno, deleni neni potreba
        addc    [edi],dx,7fffh          ;k aktualnimu vzorku pricti hodnotu
        add     di,2                    ;dalsi pozice
        movsx   eax,word ptr [esi]      ;nacti sample
        imul    word ptr volumeleft[ebx] ;nacti pravou hlastitot a vynasob
        shl     edx,1                   ;vse je vyreseno, deleni neni potreba
        addc    [edi],dx,7fffh          ;k aktualnimu vzorku pricti hodnotu
        add     di,2                    ;dalsi pozice
        add     bp,speedmin[ebx]
        lahf                      ;uchovej priznaky v ah (hlavne cf)
        adc     esi,speedmaj[ebx] ;skok po major hodnotach
        sahf                      ;obnov  cf
        adc     esi,speedmaj[ebx]
        cmp     esi,endloop[ebx]
        jc      mix12skp
        mov     esi,startloop[ebx]
        cmp     esi,endloop[ebx]
        jnz     mix12skp
        mov     smptype[ebx],0
        jmp     mix12end
mix12skp:sub     ecx,4
        jnz     mix12lp
mix12end:mov     minorpos[ebx],bp
        mov     playpos[ebx],esi
        pop     ebp
        ret


m16ido: push    ebp
        mov     esi,_mixpos             ;vem mixovaci pozici
        sub     si,4                    ;o jeden sampl dozadu
        mov     edi,_backfine           ;finepozice
        mov     ebx,_backsnd            ;pozice v bufferu
        mov     ecx,_mixsize            ;pocet bajtu
        sub     ecx,4
        shl     ecx,16                  ;uloz do horni poloviny ecx
        mov     eax,[esi]               ;vezmi sample
        xor     eax,80008000h           ;neznaminkova korekce
m16ido1:add     si,4                    ;dalsi sample
        mov     ebp,eax
        and     ebp,0ffff0000h
        mov     edx,[esi]               ;nacti do dx sample
        xor     edx,80008000h           ;neznaminkova korekce
        add     ax,dx                   ;secti leve kanaly
        rcr     ax,1                    ;nasleduje deleni 2x prumer
        add     ebp,edx                 ;totez pro prave kanaly
        rcr     ebp,1
        shld    eax,ebp,16
        xor     eax,80008000h           ;neznaminkova korekce
        mov     cx,word ptr _backsndbuff[ebx*4]  ;nacti back sound pro levej kanal
        addc    ax,cx,7fffh             ;secti s clippingem levy kanal
        mov     cx,word ptr _backsndbuff[2+ebx*4];nacti back sound pro pravej kanal
        rol     eax,16
        addc    ax,cx,7fffh             ;secti s clippingem pravy kanal
        add     di,word ptr _backstep   ;pricti fine pozici
        adc     bx,word ptr [_backstep+2];preteceni jde do bx plus step
;       and     ebx,03ffffh              ;celkem 16384*4 samplu
        sub     si,4                    ;uloz na zdrojovou pozici
        mov     [esi],eax
        add     si,4
        mov     eax,edx                 ;pouzity sampl pro prumerovani se
        sub     ecx,40000h              ;do ax a odecti ctyri takty pro citac
        jnc     m16ido1                 ;a opakuj dokud neni konec
        pop     ebp
        mov     _backsnd,ebx
        mov     _backfine,edi
        ret                             ;navrat



PUBLIC setsb16_
setsb16_:
        lea     edi,_jumptable
        mov     eax,offset mixskip
        stosd
        mov     eax,offset mix16_
        stosd
        mov     eax,offset mix162
        stosd
        mov     _getdma,offset sbdma
        mov     _ido,offset m16ido
        ret


dpmibufalloc:
        mov     ax,0100h
        int     31h
        ret

public buff_dealloc_
buff_dealloc_:
        mov     dx,_dpmiselector
        mov     ax,0101h
        int     31h
        ret

public buff_alloc_

buff_alloc_:
        mov     ebx,4200
        call dpmibufalloc
        jc      allcerror
        test    eax,0fffh
        jz      allc_ok
        push    eax
        mov     eax,0101h
        int     31h
        pop     eax
        mov     ebx,eax
        and     ebx,0f000h
        add     ebx,1000h
        sub     ebx,eax
        dec     ebx
        call    dpmibufalloc
        jc      allcerror
        push    edx
        call    buff_alloc_
        mov    ecx,edx
        pop     edx
        push    eax
        mov     eax,0101h
        int     31h
        pop     eax
        ret
allc_ok:mov     _dpmiselector,dx
        shl     eax,4
        ret
allcerror:
        mov     word ptr _dpmiselector,0
        xor     eax,eax
        ret

public int_relocation_
int_relocation_:
        mov     eax,0de0ah
        int     67h
        shl     ebx,16
        mov     bx,cx
        rol     ebx,16
        mov     eax,ebx
        ret

public int_mixer_alloc_
int_mixer_alloc_:
        lea     eax,_idt_map
        sidt    [eax]
;        db      0fh
;        db      01
;        db      08h
        add     eax,2
        mov     eax,[eax]
        shl     ebx,3
        add     eax,ebx
        cli
        mov     edi,eax
        push    ds
        pop     es
        mov     ecx,offset int_normal_
        mov     eax,ecx
        stosw
        mov     ax,cs
        stosw
        mov     eax,8e00h
        stosw
        mov     eax,ecx
        shr     eax,16
        stosw
        sti
        mov     ebx,1000h
        call    dpmibufalloc
        mov     esi,4*8
        mov     edi,offset rm_old
        mov     ecx,4
        rep     movsb
        mov     _call_back_sel,dx
        mov     esi,offset rm_data_
        mov     edi,eax
        shl     edi,4
        mov     _call_back_data,edi
        mov     ecx,1000
        rep     movsb
        mov     edi,4*8
        mov     word ptr [edi],8
        mov     [edi+2],ax
        ret

public int_high_alloc_
int_high_alloc_:
        lea     eax,_idt_map
        sidt    [eax]
        add     eax,2
        mov     eax,[eax]
        shl     ebx,3
        add     eax,ebx
        cli
        mov     edi,eax
        push    ds
        pop     es
        mov     ecx,offset int_hspd_
        mov     eax,ecx
        stosw
        mov     ax,cs
        stosw
        mov     eax,8e00h
        stosw
        mov     eax,ecx
        shr     eax,16
        stosw
        sti
        mov     ebx,1000h
        call    dpmibufalloc
        mov     esi,4*8
        mov     edi,offset rm_old
        mov     ecx,4
        rep     movsb
        mov     _call_back_sel,dx
        mov     esi,offset rm_data_
        mov     edi,eax
        shl     edi,4
        mov     _call_back_data,edi
        mov     ecx,1000
        rep     movsb
        mov     edi,4*8
;        mov     word ptr [edi],(high_speed - rm_data_)
;        mov     [edi+2],ax
        ret

public high_speed_parm_
                        ;eax dos delay
                        ;edx port
                        ;ebx mixbuffer
                        ;esi xlattab
high_speed_parm_:
        mov     edi,offset w_max
        mov     [edi],ax
        mov     edi,offset rm_hspd
        mov     ecx,64
        rep     movsd
        xor     eax,eax
        stosw
        mov     eax,ebx
        shr     eax,4
        stosw
        mov     eax,edx
        stosw
        ret
public int_dealloc_
int_dealloc_:
        mov     edi,4*8
        mov     esi,offset rm_old
        mov     ecx,4
        rep     movsb
        mov     dx,_call_back_sel
        mov     ax,101h
        int     31h
        ret

public write_adlib_
write_adlib_:
        mov     dx,_timer_test_port
        out     dx,al
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        mov     al,ah
        inc     edx
        out     dx,al
        mov     ah,36
        dec     edx
wwait:  in      al,dx
        dec     ah
        jnz     wwait
        ret
public write_adlib_nodelay_
write_adlib_nodelay_:
        mov     dx,_timer_test_port
        out     dx,al
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        mov     al,ah
        inc     edx
        out     dx,al
        ret


;public int_normal_:
int_normal_:
        cli
        pushad
        push    ds
        push    es
        mov     ax,seg _DATA
        mov     ds,ax
        mov     es,ax
        mov     al,20h
        out     20h,al
        mov     dx,_timer_test_port
        or      dx,dx
        jz      intn_ok
        in      al,dx
        test    al,80h
        jz      intn_call_dos;
        mov     ax,8004h
        call    write_adlib_
intn_ok:inc     _test_counter
        sti
        call    mixer_
        cli
intn_sk:pop     es
        pop     ds
        popad
        iretd

intn_call_dos:call _oldvect
        jmp     intn_sk

public  int_hspd_: ;high speed mode interrupt - pro D/A a PC Speaker
int_hspd_:
        push    ds
        push    esi
        push    ebx
        push    edx
        push    eax
        mov     al,20h
        out     20h,al
        mov     ax,seg _DATA
        mov     ds,ax
        mov     ebx,_call_back_data
        add     ebx,(offset rm_hspd-offset rm_data_)
        xor     eax,eax
        xor     esi,esi
        mov     ax,[ebx+scanofs]
        mov     si,[ebx+scanseg]
        shl     esi,4
        add     esi,eax
        mov     dx,[ebx+port]
        lodsb
        xlatb
        out     dx,al
        inc     word ptr [ebx+scanofs]
        mov     ebx,_call_back_data
        dec     word ptr [ebx]
        jnz     hs_end
        mov     ax,[ebx+2]
        mov     [ebx],ax
        sti
        call    mixer_
        cli
hs_end:
        pop     eax
        pop     ebx
        pop     edx
        pop     esi
        pop     ds
        iretd

;------------------------------- ULTRASND LIB -----------------
;Special thanx for Dalibor Straka

gus_sv  macro   voice
        mov     edx,_devport
        add     edx,gus_voice
        mov     al,voice
        out     dx,al
        ENDM

gus_w   macro   reg,data
        mov     edx,_devport
        mov     al,reg
        add     edx,gus_reg
        out     dx,al
        mov     eax,data
        add     edx,gus_reg_data-gus_reg
        out     dx,ax
        in      ax,dx
        in      ax,dx
        in      ax,dx
        in      ax,dx
        in      ax,dx
        in      ax,dx
        mov     eax,data
        out     dx,ax
        ENDM

gus_wb  macro   reg,data
        mov     edx,_devport
        mov     al,reg
        add     edx,gus_reg
        out     dx,al
        mov     al,data
        add     edx,gus_reg_data-gus_reg+1
        out     dx,al
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        in      al,dx
        mov     al,data
        out     dx,al
        ENDM

gus_wbd macro   reg,data
        mov     edx,_devport
        mov     al,reg
        add     edx,gus_reg
        out     dx,al
        mov     al,data
        add     edx,gus_reg_data-gus_reg+1
        out     dx,al
        endm

gus_r   macro   reg             ;vysledek v eax
        mov     edx,_devport
        mov     al,reg
        add     edx,gus_reg
        out     dx,al
        add     edx,gus_reg_data-gus_reg
        in      ax,dx
        movzx   eax,ax
        ENDM


;organizace GUS RAM
;<0 - gus_block>        prehravaci blok
;<32768 - +gus_block>   mirror buffer (prazdny)

extrn  prepare_dma_:proc             ;priprava DMA

public  stop_gus_       ;deinicializuje GUS
stop_gus_:
        mov     cl,31           ;ted se vyresetuji vsechny kanaly
igus_2: gus_sv  cl              ;vyber hlas
        gus_wb  0,2             ;vypni hlas
        gus_wb  1,0             ;nastav krok na 0
        gus_w   2,0003h         ;nastav hlas na konec 64KB bloku
        gus_w   3,0ff00h
        gus_w   4,0003h
        gus_w   5,0ff00h
        gus_w   0Ah,0003h
        gus_w   0Bh,0ff00h
        gus_wb  0Dh,3fh           ;vypni volume ramp
        gus_wb  07h,0fh           ;hlasitost 0
        gus_wb  08h,10h           ;hlasitost 0
        gus_w   09h,0           ;hlasitost 0
        sub     cl,1
        jnc     igus_2
        ret

gus_dma db      0,11h,11h,22h,22h,33h,44h,55h

public  init_gus_       ;inicializuje GUS do rezimu prehravani samplu

init_gus_:
        mov     edx,_devport
        mov     al,19h          ;zapni out
        out     dx,al
        mov     edx,_devport
        lea     edx,[edx+gus_irq_dma]
        movzx   eax,_dmanum
        mov     al,cs:gus_dma[eax]
        out     dx,al           ;nastav DMA
        gus_wbd 4ch,0           ;reset gus
        mov     edx,_devport
        add     edx,6
        mov     ecx,1000
igus_3: in      al,dx
        loop    igus_3          ;cekani
        gus_wbd 4ch,1           ;reset gus
        add     edx,6
        mov     ecx,1000
igus_4: in      al,dx
        loop    igus_4
        gus_wbd 4ch,3           ;povol dac
        mov     edx,_devport
        mov     edi,_mixbuffer
        mov     ecx, 65536 shr 2
        xor     eax,eax
        rep     stosd   ;vynuluj dma_buffer
        mov     edx,_devport
        add     edx,gus_irqs
        in      al,dx
        gus_r   41h
        gus_wb  41h,0
        mov     edx,_devport
        add     edx,gus_irqs
        in      al,dx
        gus_r   41h
        gus_r   41h
        gus_w   42h,0
        mov     eax,_mixbuffer
        mov     edx,65536
        call    prepare_dma_
        movzx   ecx,_gus_dma_type
        gus_wb  41h,cl        ;vynuluj prvnich 64KB GUSRAM
        xor     edx,edx
        mov     eax,617400
        mov     ecx,_samplerate
        div     ecx
        add     dx,8000h
        adc     al,0
        cmp     al,33
        jc      igus_1
        mov     al,32
igus_1: dec     al
        or      al,0c0h
        mov     cl,al
        gus_wb  0eh,cl
        call    stop_gus_
        gus_sv  0
        mov     ecx,0           ;nastaveni hlasu cislo 0
        gus_w   2,ecx           ;zacni na 0
        gus_w   3,ecx
        gus_w   0Ah,ecx
        gus_w   0Bh,ecx
        mov     ecx,(gus_block-4) shl 8  ;konec az za gus_blockem
        gus_w   5,ecx
        shr     ecx,16
        gus_w   4,ecx
        mov     ecx,(1 shl 0bh)
        gus_w   1,ecx           ;krok 2
        gus_w   9,0ffffh          ;hlasitost max
        gus_wb  0ch,0           ;panning vlevo
        gus_wb  00,0ch         ;spust hlas
        gus_sv  1               ;nastaveni hlasu cislo 1
        mov     ecx,2 shl 8
        gus_w   3,ecx           ;zacni na 2
        gus_w   0Bh,ecx
        shr     ecx,16
        gus_w   2,ecx
        gus_w   0Ah,ecx
        mov     ecx,(gus_block-2) shl 8  ;konec az za gus_blockem
        gus_w   5,ecx
        shr     ecx,16
        gus_w   4,ecx
        mov     ecx,(1 shl 0bh)
        gus_w   1,ecx         ;krok 2
        gus_w   9,0ffffh          ;hlasitost max
        gus_wb  0ch,0fh        ;panning vpravo
        gus_wb  00,0ch         ;spust hlas
        ret

gus_move_buff:                  ;presun bufferu do GUS
        push    eax
        mov     edx,_devport
        add     edx,gus_irqs
        in      al,dx
        gus_r   41h
        gus_wbd 41h,0
        mov     edx,_devport
        add     edx,gus_irqs
        in      al,dx
        gus_r   41h
        gus_r   41h
        mov     edx,_devport
        add     edx,gus_irqs
        in      al,dx
        pop     eax
        xor     esi,esi
        and     ah,1fh
        or      ah,ah
        jz     gus_mb1
        mov     esi,(gus_block shr 1)
gus_mb1:mov     cl,4
        test    _gus_dma_type,4
        jz      gus_mb2
        inc     cl
gus_mb2:shr     esi,cl
        push    esi
        mov     eax,_mixbuffer
        mov     edx,ebx
        call    prepare_dma_
        pop     esi
        gus_w   42h,esi
        movzx   ecx,_gus_dma_type
        gus_wbd 41h,cl
        ret

gus_skip:mov    eax,_mixbuffer
        mov     _mixpos,eax
        xor     ecx,ecx
        mov     _mixsize,ecx
        mov     cl,_gus_last_buff
        gus_sv  0
        gus_r   8Ah
        shl     eax,8
        cmp     eax,gus_block shr 1
        rcl     ch,1
        xor     cl,ch
        jz      gus_ie
        mov     ah,ch
        mov     _gus_last_buff,ch
        mov     ecx,(gus_block shr 1)
        mov     _mixsize,ecx
gus_ie: pop     eax
        ret



m16idog:push    ebp
        mov     esi,_mixpos             ;vem mixovaci pozici
        mov     edi,_backfine           ;finepozice
        mov     ebx,_backsnd            ;pozice v bufferu
        mov     ecx,_mixsize            ;pocet bajtu
        shr     ecx,2
        dec     ecx
        shl     ecx,16
m16ido2:mov     eax,[esi]
        mov     cx,word ptr _backsndbuff[ebx*4]  ;nacti back sound pro levej kanal
        addc    ax,cx,7fffh             ;secti s clippingem levy kanal
        mov     cx,word ptr _backsndbuff[2+ebx*4];nacti back sound pro pravej kanal
        rol     eax,16
        addc    ax,cx,7fffh             ;secti s clippingem pravy kanal
        add     di,word ptr _backstep   ;pricti fine pozici
        adc     bx,word ptr [_backstep+2];preteceni jde do bx plus step
        and     ebx,03ffffh              ;celkem 16384*4 samplu
        mov     [esi],eax
        add     si,4
        sub     ecx,10000h              ;do ax a odecti ctyri takty pro citac
        jnc     m16ido2                 ;a opakuj dokud neni konec
        pop     ebp
        mov     _backsnd,ebx
        mov     _backfine,edi
        mov     ah,_gus_last_buff
        mov     ebx,_mixsize            ;pocet bajtu
        call    gus_move_buff
        ret                             ;navrat





PUBLIC setGUS_
setGUS_:
        lea     edi,_jumptable
        mov     eax,offset mixskip
        stosd
        mov     eax,offset mix16_
        stosd
        mov     eax,offset mix162
        stosd
        mov     _getdma,offset GUS_SKIP
        mov     _ido,offset m16idog
        ret

public  gus_setchan_vol_
gus_setchan_vol_:               ;ecx,eax
        gus_sv  al
        gus_w   9,ecx
        ret

public  gus_setchan_pan_
gus_setchan_pan_:               ;ecx,eax
        gus_sv  al
        gus_wb   0ch,cl
        ret


_TEXT16 SEGMENT BYTE PUBLIC USE16 'CODE'
    ASSUME  cs:_TEXT16
public rm_proc_:
rm_data_:
w_count:dw      2       ;citac preruseni
w_max   dw      2       ;maximalni hodnota citace
rm_old  dw      ?       ;stary vektor preruseni
        dw      ?
rm_proc_:
        cli
        int     1ch
        cli
        push    si
        xor     si,si
        dec     word ptr cs:[si]
        jnz     w_skip
        push    ax
        mov     ax,cs:[si+2]
        mov     cs:[si],ax
        pop     ax
        pushf
        call    dword ptr cs:[si+4]
w_skip: pop     si
        iret

rm_hspd db      300 dup(?)
high_speed:
        push    ds
        push    si
        push    bx
        push    dx
        push    ax
        mov     bx,offset rm_hspd-offset rm_data_
        lds     si,cs:[bx+scanofs]
        mov     dx,cs:[bx+port]
        lodsb
        xlat    cs:
        out     dx,al
        mov     cs:[bx+scanofs],si
        mov     al,20h
        out     20h,al
        dec     word ptr cs:[0]
        jnz     hspd_end
        mov     ax,cs:[2]
        mov     cs:[0],ax
        pushf
        call    dword ptr cs:[4]
        int     1ch
hspd_end:
        pop     ax
        pop     dx
        pop     bx
        pop     si
        pop     ds
        iret

_TEXT16 ends


_TEXT   ends

END

