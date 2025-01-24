.model small
.386

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

extrn  _chaninfo:dword
extrn  _mixbuffer:dword
extrn  _backsnd:dword    ;ukazatel na buffer s hudbou na pozadi (64Kb)
extrn  _backstep:dword   ;krok o kolik se meni _backsnd
extrn  _backsndbuff:dword ;buffer hudby na pozadi


extrn    _jumptable:dword[3] ;[0] - skip, [1] - mix8, [2] - mix16
extrn    _getdma:dword
extrn    _ido:dword

extrn    _predstih:word
extrn    _lastdma:dword
extrn    _lastmix:dword
extrn    _surpos:dword
extrn    _mixpos:dword
extrn    _mixsize:dword

extrn   _dmaposadr:dword
extrn   _dpmiselector:word



_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

        PUBLIC  MIXER_
MIXER_: ;THIS IS MAIN MIXING PROCEDURE.
        ;BEFORE USING, YOU MUST SET UP ALL VARIBLES TO THE CORRECT VALUES.
        ;_MIXBUFFER MUST START ON PAGE OF MEMORY (EXP 0x10000 0x20000 0x30000)
        ;PROCEDURE MUST BE CALLED EACH 55ms OR LITTLE.
        ;IF YOU USING INTERRUPT, REMEBER, THAT YOU MUST STORE ALL REGISTERS
        ;BEFORE USE.

        CALL    calcsize       ;VYPOCET DELKY MIXOVANI
        mov     edi,_mixpos     ;Nejprve se vymaze mixovaci pamet
        mov     ecx,_mixsize
        shr     ecx,2
        jnz     MIXOK
        ret
MIXOK:  xor     eax,eax
MIXCLR: mov     [edi],eax
        add     di,4
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
        add     word ptr _surpos,cx
        ret


CALCSIZE:call   _getdma         ;eax - pozice v mixovaci pameti
        mov     ebx,_lastdma
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
mixslp: mov     al,[esi]
        mov     ah,al
        mov     edx,eax
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
mixs2:  ret

m8sido: mov     esi,_mixpos
        mov     edi,_surpos             ;surpos musi byt liche
        mov     ecx,_mixsize
        mov     edx,_backsnd            ;backsnd-index na hudbu v pozadi
m8sido1:mov     al,[edi]                ;vem surround data
        sar     al,1                    ;sniz jejich hlasitost
        mov     ah,[esi]                ;vem namixovana data
        addc    al,ah,7fh               ;secti je mezi sebou
        mov     ah,byte ptr _backsndbuff[edx]    ;ven data z hudby v pozadi
        add     dx,word ptr _backstep            ;mezitim jdi na dalsi index
        addc    al,ah,7fh               ;secti je mexi sebou
        mov     [edi],al
        inc     di                      ;dalsi pozice v mixbuff
        inc     si
        dec     ecx                     ;dokud to neni vsechno
        jnz     m8sido1                 ;opakuj
        mov     _backsnd,edx            ;zachovej index do backsoundbuff
        mov     esi,_surpos             ;nyni pracuj jiz s prehravanymi daty
        and     esi,not 1
        mov     edi,esi
        sub     si,2                    ;pozice o jeden sampl za
        mov     ecx,_mixsize            ;velikost/2
        shr     ecx,1
m8sido2:mov     ax,[esi]                ;vem cely sampl
        xor     ax,8080h
        mov     bx,[edi]
        xor     bx,8080h
        ;add     al,bl                   ;uplna prumerovaci interpolace
        ;rcr     al,1
        ;add     ah,bh
        ;rcr     ah,1
        mov     [esi],ax
        add     di,2                      ;dalsi vzorek
        add     si,2
        dec     ecx
        jnz     m8sido2                 ;dokud neni konec
        ret

sbdma:  mov     edx,_dmaposadr
        out     0ch,al
        out     0d8h,al
        xor     eax,eax
        in      al,dx
        xchg    al,ah
        in      al,dx
        xchg    al,ah
        add     eax,offset _mixbuffer
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

_TEXT   ends

end


