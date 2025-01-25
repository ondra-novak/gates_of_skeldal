.model small
.386

DGROUP group _DATA


extrn _tasklist_sp:dword[]
extrn _tasklist_low:dword[]
extrn _tasklist_top:dword[]
extrn _task_info:dword[]
extrn _curtask:dword
extrn _nexttask:dword
extrn _taskcount:dword
extrn __STACKLOW:dword
extrn __STACKTOP:dword
extrn _taskparam:dword
extrn free_:proc

_TEXT   segment byte public 'CODE' use32
        assume CS:_TEXT
        assume DS:DGROUP



        public task_sleep_
task_sleep_:
        mov     _taskparam,eax
        pushfd
        pushad
        mov     eax,_curtask
        mov     esi,[_tasklist_sp]
        mov     edi,[_tasklist_low]
        mov     edx,[_tasklist_top]
        mov     ecx,[_task_info]
        mov     [esi+eax*4],esp
        mov     ebx,_nexttask
term_1: cmp     eax,ebx
        mov     eax,ebx
        jnz     twake
        xor     eax,eax
taskloop:
        inc     ebx
        cmp     ebx,_taskcount
        jc      tasktest
        xor     ebx,ebx
tasktest:
        cmp     dword ptr [esi+ebx*4],0
        jz      taskloop
        test    byte ptr [ecx+ebx],2 ;test zda je task waiting
        jnz     taskloop                ;ano - pak ho preskoc (task 0 nesmi byt waiting)
twake:  mov     _curtask,eax
        mov     _nexttask,ebx
        mov     esp,[esi+eax*4]
        mov     ebx,[edi+eax*4]
        mov     __STACKLOW,ebx
        mov     ebx,[edx+eax*4]
        mov     __STACKTOP,ebx
        popad
        popfd
        mov     eax,_taskparam
        ret

        public  task_terminating_
task_terminating_:
        ;tato funkce je volana po kazdem ukonceni ulohy
        mov     eax,[_tasklist_sp]
        mov     esp,[eax]
        mov     ebx,[_tasklist_low]
        mov     ebx,[ebx]
        mov     __STACKLOW,ebx
        mov     ebx,[_tasklist_top]
        mov     ebx,[ebx]
        mov     __STACKTOP,ebx
        mov     eax,_curtask
        mov     esi,[_tasklist_low]
        mov     eax,[esi+4*eax]
        call    free_
        mov     eax,_curtask
        xor     ebx,ebx
        mov     esi,[_tasklist_sp]
        mov     [esi+4*eax],ebx
        mov     esi,[_tasklist_sp]
        mov     edi,[_tasklist_low]
        mov     edx,[_tasklist_top]
        mov     eax,_curtask
        mov     ebx,_nexttask
        jmp     term_1

_TEXT ends
end
