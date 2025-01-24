/*
        MOUSE.C - The following program demonstrates how
        to use the mouse interrupt (0x33) with DOS/4GW.

        Compile and link: wcl386 /l=dos4g mouse
*/
#include <stdio.h>
#include <dos.h>
#include <i86.h>

/* Data touched at mouse callback time -- they are in a structure to
        simplify calculating the size of the region to lock.
*/
struct callback_data
        {
        int mouse_event;
        unsigned short mouse_code;
        unsigned short mouse_bx;
        unsigned short mouse_cx;
        unsigned short mouse_dx;
        signed short mouse_si;
        signed short mouse_di;

        } cbd = { 0 };

int lock_region (void *address, unsigned length)
        {
        union REGS regs;
        unsigned linear;

        /* Thanks to DOS/4GW's zero-based flat memory model, converting
                a pointer of any type to a linear address is trivial.
        */
        linear = (unsigned) address;

        regs.w.ax = 0x600;                                              /* DPMI Lock Linear Region */
        regs.w.bx = (linear >> 16);                     /* Linear address in BX:CX */
        regs.w.cx = (linear & 0xFFFF);
        regs.w.si = (length >> 16);                     /* Length in SI:DI */
        regs.w.di = (length & 0xFFFF);
        int386 (0x31, &regs, &regs);
        return (! regs.w.cflag);                                /* Return 0 if can't lock */
        }

#pragma off (check_stack)
void _loadds far click_handler (int max, int mbx, int mcx, int mdx, int msi, int mdi)
        {
#pragma aux click_handler parm [EAX] [EBX] [ECX] [EDX] [ESI] [EDI]
        cbd.mouse_event = 1;
        cbd.mouse_code = (unsigned short) max;
        cbd.mouse_bx = (unsigned short) mbx;
        cbd.mouse_cx = (unsigned short) mcx;
        cbd.mouse_dx = (unsigned short) mdx;
        cbd.mouse_si = (signed short) msi;
        cbd.mouse_di = (signed short) mdi;
        if (cbd.mouse_code & 8) cbd.right_button = 1;
        }
void cbc_end (void)                                                     /* Dummy function so we can */
        {                                                                                               /* calculate size of code to lock */
        }                                                                                               /* (cbc_end - click_handler) */
#pragma on (check_stack)



// void main (void)
        {
        struct SREGS sregs;
        union REGS inregs, outregs;
        int installed = 0;
        int orig_mode = 0;
        int far *ptr;
        int (far *function_ptr)();

        segread(&sregs);

        /* get original video mode */

        inregs.w.ax = 0x0f00;
        int386 (0x10, &inregs, &outregs);

        printf ("Current Mode = %u\n", orig_mode = outregs.h.al);

        /* check for mouse driver */

        inregs.w.ax = 0;
        int386 (0x33, &inregs, &outregs);
        if (installed = (outregs.w.ax == -1))
                printf ("Mouse installed...\n");
        else
                printf ("Mouse NOT installed...\n");

        if (installed)
                {
                /* lock callback code and data (essential under VMM!)

                        note that click_handler, although it does a far return and
                        is installed using a full 48-bit pointer, is really linked
                        into the flat model code segment -- so we can use a regular
                        (near) pointer in the lock_region() call.
                */
                if ((! lock_region (&cbd, sizeof(cbd))) ||
                        (! lock_region ((void near *) click_handler,
                                (char *) cbc_end - (char near *) click_handler)))
                        {
                        printf ("locks failed\n");
                        exit (1);
                        }

                /* goto graphics mode */

                inregs.h.ah = 0x00;
                inregs.h.al = 0x12;
                int386 (0x10, &inregs, &outregs);

                /* show mouse cursor */

                inregs.w.ax = 0x1;
                int386 (0x33, &inregs, &outregs);

                /* set mouse cursor form */

                inregs.w.ax = 0x9;
                inregs.w.bx = 0x0;
                inregs.w.cx = 0x0;
                ptr = cursor;
                inregs.x.edx = FP_OFF (ptr);
                sregs.es        = FP_SEG (ptr);
                int386x (0x33, &inregs, &outregs, &sregs);

                /* install click watcher */

                inregs.w.ax = 0xC;
                inregs.w.cx = 0x0002 + 0x0008;
                function_ptr = click_handler;
                inregs.x.edx = FP_OFF (function_ptr);
                sregs.es        = FP_SEG (function_ptr);
                int386x (0x33, &inregs, &outregs, &sregs);

                while (!cbd.right_button)
                        {
                        if (cbd.mouse_event)
                                {
                                printf ("Ev %04hxh  BX %hu  CX %hu  DX %hu  SI %hd  DI %hd\n",
                                        cbd.mouse_code, cbd.mouse_bx, cbd.mouse_cx, cbd.mouse_dx,
                                        cbd.mouse_si, cbd.mouse_di);
                                cbd.mouse_event = 0;
                                }
                        }
                }

        /* check installation again (to clear watcher) */

        inregs.w.ax = 0;
        int386 (0x33, &inregs, &outregs);
        if (outregs.w.ax == -1)
                printf ("DONE : Mouse still installed...\n");
        else
                printf ("DONE : Mouse NOT installed...\n");

        inregs.h.ah = 0x00;
        inregs.h.al = orig_mode;
        int386 (0x10, &inregs, &outregs);
        }


