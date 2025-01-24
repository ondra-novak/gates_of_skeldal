.model small
.386

DGROUP group _DATA

extrn _start_poss:word
extrn _end_poss:word
extrn _scr_max_x:dword
extrn _scr_next_line:dword

_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

public draw_flat_triangle_
draw_flat_triangle_:
dft_lp2:mov  ebx,edi
        movsx  edx,_start_poss[esi*2]
        lea  edi,[ebx+edx*2]
        cmp  dx,_end_poss[esi*2]
        jz   dft_skpa
        jl   dft_dirr
        std
        cmp  edx,0
        js   dft_skpa
        cmp  edx,_scr_max_x
        jbe  dft_drr1
        mov  edi,_scr_max_x
        lea  edi,[ebx+edi*2]
        jmp  dft_drr1
dft_dirr:cld
        cmp  edx,_scr_max_x
        jg   dft_skpa
        cmp  edx,0
        jns  dft_drr1
        mov  edi,ebx
dft_drr1:movsx edx,_end_poss[esi*2]
        cmp  edx,0
        jns  dft_dc1
        xor  edx,edx
        jmp  dft_dc2
dft_dc1:cmp  edx,_scr_max_x
        jna   dft_dc2
        mov  edx,_scr_max_x
dft_dc2:shl  edx,1
        add  edx,ebx
dft_lp1: stosw
        cmp  edi,edx
        jnz  dft_lp1
dft_skpa:
        stosw
        mov  edi,_scr_next_line
        lea  edi,[ebx+edi]
        inc  esi
        dec  ecx
        jnz  dft_lp2
        cld
        ret
_TEXT   ends
end




