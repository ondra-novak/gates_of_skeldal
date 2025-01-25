.model small
.386
;16 bit line (de)comprimator
;
DGROUP group _DATA

extern _old_value:byte
extern _compress_dic:dword


_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP



public fast_expand_code_
fast_expand_code_:
     cmp     eax,256
     jnc     expand
     mov     esi,[edi]
     inc     dword ptr [edi]
     mov     bl,al
     add     al,_old_value
     mov     [esi],al
     mov     _old_value,al
     ret
expand:
     mov     ebx,_compress_dic
     lea     ecx,[eax*8+ebx]
     movzx   eax,word ptr [ecx+4]
     add     [edi],eax
     push    eax
     mov     esi,[edi]
eloop:movzx   eax,word ptr [ecx+2]
     mov     [esi],al
     dec     esi
     movzx   eax,word ptr [ecx]
     lea     ecx,[eax*8+ebx]
     cmp     eax,256
     jnc     eloop
     mov     bl,al
     add     al,_old_value
     mov     [esi],al
     inc     dword ptr [edi]
     pop     ecx
elp2:inc     esi
     add     al,[esi]
     mov     [esi],al
     dec     ecx
     jnz     elp2
     mov     _old_value,al
     ret
_TEXT ends
end
