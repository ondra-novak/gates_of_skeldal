.model small
.386P

DGROUP group _DATA

;real procedura pro prehravani na PCSPEAKERu.
;Tato procedura musi obsahovat vsechny deklarace, ktere vyuziva Protect Mode
;Tyto deklarace jsou pristupne i s PM. Opacne to vsak nelze

rl_pm_bufseg    equ     (offset pm_bufseg-rm_proc)
rl_bufseg       equ     (offset bufseg-rm_proc)
rl_bufpos       equ     (offset bufpos-rm_proc)
rl_port         equ     (offset port-rm_proc)
rl_xlattab      equ     (offset xlattab-rm_proc)


_TEXT16 segment byte public 'CODE' use16
        assume cs:_TEXT16
        assume ds:_TEXT16
rm_proc:                        ;Zde zacina obsluha INT 8 z realu
        cli
        push    ds              ;Program nevstupuje do dalsich int kvuly zdrzovani
        push    ax              ;Nejprve se ulozi vsechny ohrozene registry
        push    bx
        push    dx
        push    si
        mov     ax,cs:[rl_bufseg]
        mov     ds,ax           ;ds segment ukazuje na buffer
        mov     dx,cs:[rl_port] ;nacti adresu portu
        lea     bx,cs:[rl_xlattab];nacti zacatek
        mov     si,cs:[rl_bufpos];nacti si - adresu prehravaneho bytu
        lods    ds:[si]         ;precti byte
        mov     cs:[rl_bufpos],si;uloz si
        add     bl,al
        adc     bh,0
        mov     al,cs:[bx]
        out     dx,al           ;posli ho na port
        mov     al,20h
        out     20h,al          ;posli end of interrupt
        pop     si              ;obnov se ulozene.
        pop     dx
        pop     bx
        pop     ax
        pop     ds
        iret

bufseg  dw      0x1234  ;segment kde je ulozen playing buffer
                        ;predpoklada se ze je 64Kb dlouhy.
bufpos  dw      ?       ;ukazatel na aktualni prehravana data
                        ;data jsou v bufferu ulozena unsigned
port    dw      ?       ;cislo portu, kam je nutne data posilat
pm_bufseg dw      ?     ;buffer z protectu
xlattab db      256 dup (?) ;prekladova tabulka pro PC_SPEAK.
                        ;pro DAC vyplnte cisli 0 - 255

rm_konec:               ;ukazatel na konec rm_proc
_TEXT16 ends


.data

old8rmofs       dw      ?
old8rmseg       dw      ?
old8pmofs       dd      ?
old8pmseg       dw      ?

rmproc_rmseg    dw      ?  ;segment rm_proc v realnem rezimu
rmproc_pmseg    dw      ?  ;segment rm_proc v chranenem rezim

timer           dw      ?  ;hodnota casovace pro simulaci puvodni INT 8
timer_max       dw      ?  ;max hodnota casovace

zavora          db      0  ;zavora do pm_proc

SPKTAB  Db      20h, 1fh, 1eh, 1dh, 1ch, 1bh, 1ah, 19h, 18h, 17h, 16h, 15h, 14h, 13h
	Db      12h, 11h, 11h, 10h, 10h, 0fh, 0fh, 0eh, 0eh, 0dh, 0dh, 0dh, 0ch, 0ch, 0ch, 0ch
	Db      0bh, 0bh, 0bh, 0bh, 0Ah, 0ah, 0ah, 0ah, 0ah, 09h, 09h, 09h, 09h, 09h, 09h, 09h
	Db      09h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 08h, 07h, 07h
	Db      07h, 07h, 07h, 07h, 07h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h, 06h
	Db      05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 05h, 04h, 04h, 04h, 04h, 04h, 04h
	Db      04h, 04h, 04h, 04h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 03h, 02h, 02h
	Db      02h, 02h, 02h, 02h, 02h, 02h, 02h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h, 01h
        Db      01h, 01h        ;prekladova tabulka pro speakra.
        db      40h, 40h, 40h, 40h, 40h, 40h, 40h, 40h, 40h, 40h, 3fh, 3fh, 3fh, 3fh
	Db      3fh, 3fh, 3fh, 3fh, 3fh, 3fh, 3fh, 3eh, 3eh, 3eh, 3eh, 3eh, 3eh, 3eh, 3eh, 3eh
	Db      3eh, 3eh, 3dh, 3dh, 3dh, 3dh, 3dh, 3dh, 3dh, 3dh, 3dh, 3ch, 3ch, 3ch, 3ch, 3ch
	Db      3ch, 3ch, 3ch, 3ch, 3ch, 3bh, 3bh, 3bh, 3bh, 3bh, 3bh, 3bh, 3bh, 3bh, 3bh, 3ah
	Db      3ah, 3ah, 3ah, 3ah, 3ah, 3ah, 3ah, 3ah, 3ah, 39h, 39h, 39h, 39h, 39h, 39h, 39h
	Db      39h, 39h, 39h, 38h, 38h, 38h, 38h, 38h, 38h, 38h, 38h, 37h, 37h, 37h, 37h, 37h
	Db      36h, 36h, 36h, 36h, 35h, 35h, 35h, 35h, 34h, 34h, 34h, 33h, 33h, 32h, 32h, 31h
	Db      31h, 30h, 30h, 2fh, 2eh, 2dh, 2ch, 2bh, 2ah, 29h, 28h, 27h, 26h, 25h, 24h, 23h
	Db      22h, 21h


_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP

public  load_rm_proc_    ;funkce nacte rm_proc do realne pameti
load_rm_proc_ :
        mov     ebx,(rm_konec-rm_proc+16) shr 4 ;velikost v paragrafech
        mov     eax,100h
        int     31h             ;alokuj DOS pamet
        mov     rmproc_rmseg,ax ;segment
        mov     rmproc_pmseg,dx ;selektor
        sbb     al,al           ;al=0xff pri chybe
        jc      lrmp_e          ;skok pri chybe
        push    es              ;uchovej segmentove registry
        push    ds
        mov     es,dx
        xor     edi,edi
        mov     dx,ds
        mov     ds,dx
        mov     esi,offset rm_proc
        mov     ecx,(rm_konec-rm_proc)
        rep     movsb           ;presun rm_proc z protect mem do real mem
        pop     ds
        pop     es
lrmp_e: ret

public  purge_rm_proc_   ;funkce uvolni rm_proc z pameti
purge_rm_proc_:
        mov     dx,rmproc_pmseg
        mov     eax,101h
        int     31h             ;dealokuj
        sbb     al,al           ;al obsahuje chybu
        ret
        ;tutu proceduru je nutne volat jako posledni!

public  rm_proc_set_    ;funkce nastavuje promenne v rm_proc
                        ;ax - bufseg - rm adresa bufferu
                        ;dx - bufseg - pm adresa bufferu
                        ;cx - adresa portu
                        ;bx - mod XLAT - 0 normal mode
                        ;                1 pcspeak mode
        assume es:_TEXT16
rm_proc_set_:
        push    es              ;uloz es
        mov     si,ds ;seg bufseg
        mov     es,si
        mov     es:bufseg,ax       ;zapis rm bufseg
        mov     es:pm_bufseg,dx    ;zapis pm bufseg
        mov     es:port,cx         ;zapis cislo portu
        or      bx,bx           ;test bx
        jz      rps_nrm         ;je li nulovy - normal mode
        lea     esi,SPKTAB      ;vem spktab
        lea     edi,es:xlattab
        mov     ecx,256
        rep     movsb           ;prenes ji do xlattab
        jmp     rps_pcs
rps_nrm:xor     ah,ah           ;al =0
        mov     al,80h
        lea     edi,es:xlattab
rps_nr1:stosb                   ;zapis al do xlattab
        inc     al              ;al=al+1
        inc     ah
        jnz     rps_nr1         ;dokud to nebude cela tabulka
rps_pcs:pop     es              ;obnov es
        ret
;POZNAMKA: tato procedura modifikuje data v originalnim vzoru. Proto ji je
;nutne volat pred vlastnim load_rm_proc

pc_speak_protect:               ;funkce pro obsluhu pcspeak v protect mode
        push    ds
        push    es              ;uchovej ohrozene registry
        push    eax
        push    ebx
        push    edx
        push    esi
        mov     ax,seg _DATA
        mov     ds,ax
        mov     ds,ds:rmproc_pmseg ;nacti ukazatel na datovou oblast v rmm
      assume ds:_TEXT16
        mov     ebx,rl_xlattab ;nacti ukazatel na xlattab
        movzx   edx,word ptr ds:[rl_port]   ;nacti cislo portu
        movzx   esi,word ptr ds:[rl_bufpos]  ;nacti ukazatel na vzorek
        mov     es,word ptr ds:[rl_pm_bufseg]    ;nacti pm_selector na buffer
        lods    byte ptr [es:esi];precti sample
        mov     word ptr ds:[rl_bufpos],si;uloz ukazatel na dalsi vzorek
        xlatb                   ;preloz podle xlattab
        out     dx,al           ;posli ho na port
        mov     ax,seg _DATA    ;vem selector na globalni data
        mov     ds,ax           ;napln jim ds
      assume ds:DGROUP
        mov     al,20h          ;posli eoi
        out     20h,al
        dec     timer           ;sniz citac
        jnz     pcs_p1          ;pokud neni na nule, skoc na konec
        mov     ax,timer_max   ;napln citac maximalni hodnotou
        mov     timer,ax
        cmp     zavora,0        ;odtestuj zavoru
        jnz     pcs_p1          ;zavrena zavora by zpusobila preplneni INT8
        mov     zavora,1        ;zavri zavoru
        pushfd                  ;uchovej flagy - simuluj INT
        sti                     ;povol preruseni (vsechna preruseni na INT8 projdou zavorou)
        call    far dword ptr[old8pmofs] ;skok do puvodniho osetreni INT8
        mov     zavora,0        ;zakazovat int neni treba, jeho stav je ulozen ve flags
pcs_p1: pop     esi             ;otevri zavoru - obnov registry
        pop     edx
        pop     ebx
        pop     eax
        pop     es
        pop     ds
        iretd                   ;konec obsluhy preruseni.

public  pc_speak_run_           ;funkce instaluje bimodalni ovladac PCSPEAK
pc_speak_run_:
                                ;eax - sample freq
                                ;edx - frekvence simulace int 8 (18.2 HZ)
        cli                     ;zakaz preruseni
        push    eax             ;tyto parametry zatim nejsou potreba
        push    edx
        mov     eax,200h        ;nejprve zjisti puvodni obsluhu RM
        mov     bl,8            ;INT 8
        int     31h
        mov     old8rmofs,dx    ;uloz hodnoty
        mov     old8rmseg,cx
        mov     eax,204h        ;pak precti starou obsluhu PM
        int     31h
        mov     old8pmofs,edx   ;uloz hodnoty
        mov     old8pmseg,cx
        mov     eax,205h         ;nastav novou obsluju PM
        mov     cx,cs
        mov     edx,offset pc_speak_protect
        int     31h
        mov     eax,201h        ;nastav novou obsluhu RM
        mov     cx,rmproc_rmseg
        mov     dx,0
        int     31h
        pop     ebx             ;obnov sample freq
        pop     eax             ;obnov simint freq -> do ebx!
        mov     ecx,eax         ;sample freq uchovej jeste v ecx
        xor     edx,edx         ;vynuluj edx, horni polovina edx&eax
        div     ebx             ;delenim ziskej timer (samplef/simintf)
        mov     timer,ax       ;uloz timer -> deleni neni presne ale v ramci
        mov     timer_max,ax   ;presnosti vyhovuje
        mov     eax,1234dch     ;ziskej nastaveni 8253 delenim zakladni freqvence
        xor     edx,edx
        div     ecx             ;to je ted v eax
        mov     ecx,eax         ;uchovej ho v ecx
        mov     al,34h          ;citac 1 word rezim 2 - 1:m
        out     43h,al          ;zapis nastaveni
        mov     al,cl           ;dolni hodnotu citace
        out     40h,al
        mov     al,ch           ;horni hodnotu citace
        out     40h,al
        sti                     ;povol preruseni
        ret


public pc_speak_stop_           ;tato funkce zasatvi prehravani PCSPEAK a vrati puvodni vektory
pc_speak_stop_:
        cli
        mov     al,36h          ;nejprve zpomal casovace
        out     43h,al
        mov     al,0
        out     40h,al
        out     40h,al
        mov     bl,8
        mov     eax,201h         ;vrat puvodni rm vektor
        mov     cx,old8rmseg
        mov     dx,old8rmofs
        int     31h
        mov     eax,205h         ;vrat puvodni pm vektor
        mov     cx,old8pmseg
        mov     edx,old8pmofs
        int     31h
        sti
        ret

public pc_speak_enable_         ;povoluje prehravani na PCSpeaker(TM)
pc_speak_enable_:               ;nastavuje specialni rezim prehravani.
        in      al,61H
	or      al,3
	out     61H,al
	mov     AL,90H
	out     43H,al
        ret

public pc_speak_disable_         ;zakazuje prehravani na PCSpeaker(TM)
pc_speak_disable_:               ;funkce defacto vraci puvodni nastaveni.
SPKOFF: in      al,061H
	and     al,0FCH
	out     61H,al
        ret

public pc_speak_position_       ;ziska aktualni pozici
pc_speak_position_:
        push    es
        mov     es,rmproc_pmseg
        movzx   eax,word ptr es:[rl_bufpos]
        movzx   ebx,word ptr es:[rl_bufseg]
        shl     ebx,4
        add     eax,ebx
        pop     es
        ret

public pc_speak_set_proc_       ;zapise na [edi] adresi na pc_speak_position_
pc_speak_set_proc_:
        mov     [edi],offset pc_speak_position_
        ret

_TEXT ends
end
