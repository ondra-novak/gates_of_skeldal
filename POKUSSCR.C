/*
 * Nahodi SVGA
 */
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_AREA 0xa000
#define SCREEN_LIN_ADDR ((SCREEN_AREA) << 4)
#define SCREEN_SIZE 65537

    unsigned char	pomoc[256];
void main()
  {
    unsigned short       *pscreen;
    unsigned int 	sizegran;
    int         	i,err;
    int         	col;
    int         	g;
    union REGPACK  	regs;
//-------------------------------------------------------------
    //pomoc = (unsigned char *)malloc(512);
    memset( &regs, 0, sizeof(union REGPACK) );
    regs.w.ax = 0x4f02;
    regs.w.bx = 0x0111;
    intr( 0x10, &regs);//nastaveni videomodu
    //regs.w.ax = 0x4f01;
    //regs.w.cx = 0x0101;
    //regs.w.es = FP_SEG (pomoc);
    //regs.w.di = FP_OFF (pomoc);
    //intr( 0x10, &regs);//zjisteni granularity v KB
    //err = regs.w.ax;
    //sizegran = pomoc[4];
    //sizegran = sizegran*1024;
   //if (sizegran<=65536)
//----------------------------
    for( col = 0; col < 100; col++ ) {
	trace:if (!(inp( 0x03da )&8 )) goto trace;// cekani na raytrace
        for( g = 0; g < 10; g++ ) {
        	regs.w.ax = 0x4f05;
        	regs.w.bx = 0;
        	regs.w.dx = g;
        	intr( 0x10, &regs);//prepnuti videostranky
           pscreen = (unsigned short *)SCREEN_LIN_ADDR;
           for( i = 0; i < SCREEN_SIZE; i+=sizeof(*pscreen)  ) {
                    *pscreen = i+col*g;
                    pscreen+=1;
    			}
    		}
	}
regs.w.ax = 0x0003;
intr (0x10,&regs);
//for( g = 0; g < 15; g++ ) printf("%X...%X\n",g,pomoc[g]*1024);
//free (pomoc);
//printf("err %X\n",err);
}
