.model small
.386
;16 bit line (de)comprimator
;
DGROUP group _DATA

extern _anim_line_len:dword


_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP



save_line1 macro reg1,reg2
           shr  reg1,8
           rcr  reg2,1
           shr  reg1,8
           rcr  reg2,1
           shr  reg1,8
           rcr  reg2,1
           shr  reg1,8
           rcr  reg2,1
endm

save_linex macro reg1,reg2,rols
           shrd reg2,reg1,rols
           shr  reg1,8
           shrd reg2,reg1,rols
           shr  reg1,8
           shrd reg2,reg1,rols
           shr  reg1,8
           shrd reg2,reg1,rols
endm


public save_1bit_       ;ESI source
                        ;EDI target
                        ;vraci EDI ukazatel na konce;
save_1bit_:
           mov cl,8
s1b1:      lodsd
           save_linex eax,ebx,1
           dec cl
           jnz s1b1
           mov  eax,ebx
           stosd
           mov cl,8
s1b2:      lodsd
           save_linex eax,ebx,1
           dec cl
           jnz s1b2
           mov  eax,ebx
           stosd
           ret

public save_2bit_
save_2bit_:
            mov ch,4
c2b11:      mov cl,4
c2b12:      lodsd
            save_linex eax,ebx,2
            dec cl
            jnz c2b12
            mov  eax,ebx
            stosd
            dec ch
            jnz c2b11
            ret

public save_3bit_
save_3bit_:
            mov ch,8
c3b11:      mov cl,2
c3b12:      lodsd
            save_linex eax,ebx,3
            dec cl
            jnz c3b12
            mov  eax,ebx
            shr eax,8
            stosd
            dec  edi
            dec ch
            jnz c3b11
            ret

public save_4bit_
save_4bit_:
            mov ch,8
c4b11:      mov cl,2
c4b12:      lodsd
            save_linex eax,ebx,4
            dec cl
            jnz c4b12
            mov  eax,ebx
            stosd
            dec ch
            jnz c4b11
            ret


public load_1bit_               ;ESI - Source
                                ;EDI - Target
                                ;EBX - Palette
                                ;anim_line_len - delka radky
load_1bit_ :
        mov     ch,8
l1b1:   mov     dl,[esi]
        inc     esi
        mov     cl,8
l1b2:   movzx   eax,dl
        and     eax,1
        mov     eax,[ebx+eax*2]
        test    eax,8000h
        jnz     short l1b1s
        mov     [edi],ax
l1b1s:  add     edi,2
        shr     dl,1
        dec     cl
        jnz     l1b2
        mov     eax,_anim_line_len
        sub     eax,16
        add     edi,eax
        dec     ch
        jnz     l1b1
        ret

public load_2bit_               ;ESI - Source
                                ;EDI - Target
                                ;EBX - Palette
                                ;anim_line_len - delka radky
load_2bit_:
        mov     ch,8
l2b1:   mov     dx,[esi]
        add     esi,2
        mov     cl,8
l2b2:   mov     eax,edx
        and     eax,3
        mov     eax,[ebx+eax*2]
        test    eax,8000h
        jnz     short l2b1s
        mov     [edi],ax
l2b1s:  add     edi,2
        shr     edx,2
        dec     cl
        jnz     l2b2
        mov     eax,_anim_line_len
        sub     eax,16
        add     edi,eax
        dec     ch
        jnz     l2b1
        ret

public load_3bit_               ;ESI - Source
                                ;EDI - Target
                                ;EBX - Palette
                                ;anim_line_len - delka radky
load_3bit_:
        mov     ch,8
l3b1:   mov     edx,[esi]
        add     esi,3
        mov     cl,8
l3b2:   mov     eax,edx
        and     eax,7
        mov     eax,[ebx+eax*2]
        test    eax,8000h
        jnz     short l3b1s
        mov     [edi],ax
l3b1s:  add     edi,2
        shr     edx,3
        dec     cl
        jnz     l3b2
        mov     eax,_anim_line_len
        sub     eax,16
        add     edi,eax
        dec     ch
        jnz     l3b1
        ret

public load_4bit_               ;ESI - Source
                                ;EDI - Target
                                ;EBX - Palette
                                ;anim_line_len - delka radky
load_4bit_:
        mov     ch,8
l4b1:   mov     edx,[esi]
        add     esi,4
        mov     cl,8
l4b2:   mov     eax,edx
        and     eax,15
        mov     eax,[ebx+eax*2]
        test    eax,8000h
        jnz     short l4b1s
        mov     [edi],ax
l4b1s:  add     edi,2
        shr     edx,4
        dec     cl
        jnz     l4b2
        mov     eax,_anim_line_len
        sub     eax,16
        add     edi,eax
        dec     ch
        jnz     l4b1
        ret

public load_8bit_               ;ESI - Source
                                ;EDI - Target
                                ;EBX - Palette
                                ;anim_line_len - delka radky
load_8bit_:
        mov     ch,8
l8b1:   mov     cl,8
l8b2:   lodsb
        movzx     eax,al
        mov     eax,[ebx+eax*2]
        test    eax,8000h
        jnz     short l8b1s
        mov     [edi],ax
l8b1s:  add     edi,2
        dec     cl
        jnz     l8b2
        mov     eax,_anim_line_len
        sub     eax,16
        add     edi,eax
        dec     ch
        jnz     l8b1
        ret

public load_0bit_               ;EDI - Target
                                ;EBX - Palette
                                ;anim_line_len - delka radky
load_0bit_:
        movzx   eax,word ptr[ebx]
        test    eax,8000h
        jnz     l0b1s
        mov     ebx,eax
        shl     ebx,16
        or      eax,ebx
        mov     ch,8
l0b1:   mov     cl,4
l0b2:   stosd
        dec     cl
        jnz     l0b2
        mov     edx,_anim_line_len
        sub     edx,16
        add     edi,edx
        dec     ch
        jnz     l0b1
l0b1s:  ret


public display_interlaced_fused_lfb_
                                 ;ESI buffer1
                                 ;EBX buffer2
                                 ;EDI screen

;vezme buffer 1 a sfuzuje s bufferem 2, predtim ovsem prepise buffer2 bufferem 1
;pracuje s lfb
display_interlaced_fused_lfb_:
        mov     ecx,180
dif_lp1:push    ecx
        mov     ecx,320
dif_lp2:lodsd
        mov     edx,eax
        xchg    edx,[ebx]
        and     eax,7bde7bdeh
        and     edx,7bde7bdeh
        add     eax,edx
        shr     eax,1
        stosd
        add     ebx,4
        dec     ecx
        jnz     dif_lp2
        pop     ecx
        add     edi,1280
        dec     ecx
        jnz     dif_lp1
        ret

extern mapvesaadr_:near

public display_interlaced_fused_bank_
                                 ;ESI buffer1
                                 ;EBX buffer2
                                 ;EDI screen

;vezme buffer 1 a sfuzuje s bufferem 2, predtim ovsem prepise buffer2 bufferem 1
;pracuje s lfb
display_interlaced_fused_bank_:
        mov     ecx,180
difblp1:push    ecx
        push    edi
        call    mapvesaadr_
        mov     ecx,320
difblp2:lodsd
        mov     edx,eax
        xchg    edx,[ebx]
        and     eax,7bde7bdeh
        and     edx,7bde7bdeh
        add     eax,edx
        shr     eax,1
        stosd
        add     ebx,4
        dec     ecx
        jnz     difblp2
        pop     edi
        pop     ecx
        add     edi,4096
        dec     ecx
        jnz     difblp1
        ret




_TEXT ends
end
