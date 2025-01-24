;EAX EDX EBX ECX
.386p
jumps
;############################################################################
;                               Constanty
;############################################################################

dlt_x equ 640
dlt_y equ 480

;############################################################################
;                              Datovy segment
;############################################################################
_DATA   SEGMENT PARA PUBLIC USE32 'DATA'
align 4


EXTRN __VGAPage:BYTE
EXTRN __VGAGran:BYTE


_DATA   ENDS

DGROUP GROUP _DATA

;############################################################################
;                               Kodovy segment
;############################################################################
_TEXT   SEGMENT PARA PUBLIC USE32 'CODE'
    ASSUME  cs:_TEXT, ds:_DATA



public wm_ChangeBank__
wm_ChangeBank__ PROC

        push es
        pushad

        push eax
        mov edx, eax
        mov ebx, 0000h
        mov eax, 4f05h
        int 10h

        pop eax
        mov edx, eax
        mov ebx, 0001h
        mov eax, 4f05h
        int 10h

        popad
        pop es
        ret

wm_ChangeBank__ endp
;----------------------------------------------------------------------------

_TEXT ENDS

END


