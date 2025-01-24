.model small
.386

_DATA   segment byte public 'DATA' use32
public _call_proc
_call_proc      dd      ?
old_stack       dd      ?
old_stack_seg   dw      ?
new_stack_pos   dd      ?
new_stack_seg   dw      ?

_DATA   ends

DGROUP group _DATA
_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP


extern _dos_setvect_:proc
extern __GETDS:proc

public dos_int24_

dos_int24_:cld
           push ds
           call __GETDS
           mov  old_stack,esp
           mov  old_stack_seg,ss
           lss  esp,new_stack_pos
           push es
           push fs
           push gs
           push ebx
           push ecx
           push edx
           push edi
           push esi
           push ebp
           mov  ax,ds
           mov  es,ax
           mov  fs,ax
           mov  gs,ax
           mov  dl,ah
           mov  bl,al
           mov  eax,edi
           call _call_proc
           pop  ebp
           pop  esi
           pop  edi
           pop  edx
           pop  ecx
           pop  ebx
           pop  gs
           pop  fs
           pop  es
           lss  esp,old_stack
           pop  ds
           iretd


public install_dos_error_

;eax - procedura
;edx - stack
install_dos_error_:
        mov     _call_proc,eax
        mov     new_stack_pos,edx
        mov     new_stack_seg,ss
        mov     ebx,offset dos_int24_
        mov     cx,cs
        mov     eax,24h
        call    _dos_setvect_
        ret
_TEXT ends
End
