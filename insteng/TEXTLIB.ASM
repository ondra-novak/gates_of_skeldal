.model small
.386

SEGB800 equ     0b8000h

_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

public load_font_
load_font_:            ;esi font
        mov     esi,offset fonttable
        mov     edx,3c4h
        mov     eax,0402h
	out	dx,ax
        mov     eax,0704h
        out     dx,ax
        Mov     edx,3ceh
        mov     eax,0204h
	out	dx,ax
        mov     eax,0005h
	out	dx,ax
        mov     eax,0406h
	out	dx,ax
        mov     edi,0xA0000h
        mov     edx,256
lfOpk:  mov     ecx,16
	rep	movsb
        add     edi,16
        dec     edx
        jne     lfopk
        mov     edx,3c4h
        mov     eax,0302h
	out	dx,ax
        mov     eax,0304h
        out     dx,ax
        Mov     edx,3ceh
        mov     eax,0004h
	out	dx,ax
        mov     eax,1005h
	out	dx,ax
        mov     eax,0E06h
	out	dx,ax
        ret

public set_font_8x8_
set_font_8x8_:
        cli
        mov   edx,3d4h
        mov   eax,0100h
        out   dx,ax
        mov   edx,3c4h
        mov   al,1
        out   dx,al
        inc   edx
        in al,dx
        or al,1
        out   dx,al
        mov  edx,03dah
        in   al,dx
        mov  edx,03c0h
        mov  al,13h
        out  dx,al
        mov  al,0
        out   dx,ax
        mov  al,32
        out  dx,al
        mov   edx,3d4h
        mov   eax,0300h
        out   dx,ax
        sti
        ret

public  turn_flashing_          ;ebx - ON/OFF
turn_flashing_:
        mov     eax,1003h
        int     10h
        ret


public  get_window_size_        ;eax,edx-velikost
get_window_size_:
        imul    eax,edx
        shl     eax,1
        add     eax,4
        ret

public  save_window_            ;eax,edx-pozice
                                ;ecx,ebx-velikost
                                ;edi - buffer
save_window_
        stosb
        imul    eax,160
        lea     eax,[eax+edx*2]
        lea     esi,[eax+SEGB800]
        mov     al,dl
        stosb
        mov     al,bl
        mov     ah,cl
        stosw
        mov     edx,ebx
