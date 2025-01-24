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

        PUBLIC  _MIXER
_MIXER: ;THIS IS MAIN MIXING PROCEDURE.
        ;BEFORE USING, YOU MUST SET UP ALL VARIBLES TO THE CORRECT VALUES.
        ;_MIXBUFFER MUST START ON PAGE OF MEMORY (EXP 0x10000 0x20000 0x30000)
        ;PROCEDURE MUST BE CALLED EACH 18ms OR LITTLE.
        ;IF YOU USING INTERRUPT, REMEBER, THAT YOU MUST STORE ALL REGISTERS
        ;BEFORE USE.

;        CALL    calcsize       ;VYPOCET DELKY MIXOVANI
        mov     edi,_mixpos     ;Nejprve se vymaze mixovaci pamet
        mov     ecx,_mixsize
        shr     ecx,2
        xor     eax,eax
MIXCLR: mov     [edi],eax
        add     di,4
        dec     ecx
        jnz     MIXCLR
        mov     ebx,_chaninfo   ;TED SE ZACNE MIXOVAT
MIXING: xor     eax,eax
        mov     ax,[ebx]offset tchannel.smptype
_TEXT ends
end
