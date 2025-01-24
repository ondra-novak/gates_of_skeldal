/********************************/
/* Prace se SVGA VESA adapterem */
/********************************/

#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "vesa.h"
#include "dosmem.h"

  extern void wm_ChangeBank_(int bank);
  #pragma aux (ASM) wm_ChangeBank_;
 

void Set_VESA_mode (int mode)
{
	union REGS r;

    r.w.ax=0x4f02;
	r.w.bx=mode;
    int386 (0x10, &r, &r);
}


void Show_screen (char *display)
{
	wm_ChangeBank_ (0);
	memmove (0xa0000, display, 65536);
	wm_ChangeBank_ (1);
	memmove (0xa0000, (display+65536), 65536);
	wm_ChangeBank_ (2);
	memmove (0xa0000, (display+131072), 65536);
	wm_ChangeBank_ (3);
	memmove (0xa0000, (display+196608), 65536);
	wm_ChangeBank_ (4);
	memmove (0xa0000, (display+262144), 45056);
}

void Put_image (int x, int y, int xlen, int ylen, char *image)
{
	int a,b,c,d;
	long int posunuti=0;
	int banka;

	if (y<103) {wm_ChangeBank_ (0); banka=0;}
	if (y>103 && y<205) {wm_ChangeBank_ (1); banka=1;}
	if (y>205 && y<308) {wm_ChangeBank_ (2); banka=2;}
	if (y>308 && y<410) {wm_ChangeBank_ (3); banka=3;}
	if (y>410) {wm_ChangeBank_ (4); banka=4;}

	for (a=0; a<=ylen; a++)
		{

		d=y+a;
		switch (d)
		{
			case 102: 
					  if ((x+xlen)<256){
						posunuti=(y+a)*640+x;
						memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);
						wm_ChangeBank_ (1); banka=1;}
					  if (x>=256){ 
					    wm_ChangeBank_ (1); banka=1;
						posunuti=(((y+a)*640+x)-65536);
						memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);
						}
					  if (x<256 && (x+xlen)>256){
						posunuti=(y+a)*640+x; b=x; c=0;
					  	while (b!=256)
							{memmove ((0xa0000+posunuti), (image+a*(xlen+1)+c), 1);
							 posunuti++; b++; c++;}
						wm_ChangeBank_ (1); banka=1;
						posunuti=(((y+a)*640+x)-65536);
						memmove ((0xa0000+posunuti+c), (image+a*(xlen+1)+c), (xlen-c));}
					  break;
			case 204: 
					  if ((x+xlen)<512){
						posunuti=(((y+a)*640+x)-65536);
						memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);
						wm_ChangeBank_ (2); banka=2;}
					  if (x>=512){ 
					    wm_ChangeBank_ (2); banka=2;
						posunuti=(((y+a)*640+x)-131072);
						memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);}
					  if (x<512 && (x+xlen)>512){
						posunuti=(((y+a)*640+x)-65536); b=x; c=0;
					  	while (b!=512)
							{memmove ((0xa0000+posunuti), (image+a*(xlen+1)+c), 1);
							 posunuti++; b++; c++;}
						wm_ChangeBank_ (2); banka=2;
						posunuti=(((y+a)*640+x)-131072);
						memmove ((0xa0000+posunuti+c), (image+a*(xlen+1)+c), (xlen-c));}
					  break;
	
			case 307:
					  if ((x+xlen)<128){
						posunuti=(((y+a)*640+x)-131072);
						memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);
						wm_ChangeBank_ (3); banka=3;}
					  if (x>=128){ 
					    wm_ChangeBank_ (3); banka=3;
						posunuti=(((y+a)*640+x)-196608);
						memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);}
					  if (x<128 && (x+xlen)>128){
						posunuti=(((y+a)*640+x)-131072); b=x; c=0;
					  	while (b!=128)
							{memmove ((0xa0000+posunuti), (image+a*(xlen+1)+c), 1);
							 posunuti++; b++; c++;}
						wm_ChangeBank_ (3); banka=3;
						posunuti=(((y+a)*640+x)-196608);
						memmove ((0xa0000+posunuti+c), (image+a*(xlen+1)+c), (xlen-c));}
					  break;

			case 409:
					  if ((x+xlen)<384){
						posunuti=(((y+a)*640+x)-196608);
						memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);
						wm_ChangeBank_ (4); banka=4;}
					  if (x>=384){ 
					    wm_ChangeBank_ (4); banka=4;
						posunuti=(((y+a)*640+x)-262144);
						memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);}
					  if (x<384 && (x+xlen)>384){
						posunuti=(((y+a)*640+x)-196608); b=x; c=0;
					  	while (b!=384)
							{memmove ((0xa0000+posunuti), (image+a*(xlen+1)+c), 1);
							 posunuti++; b++; c++;}
						wm_ChangeBank_ (4); banka=4;
						posunuti=(((y+a)*640+x)-262144);
						memmove ((0xa0000+posunuti+c), (image+a*(xlen+1)+c), (xlen-c));}
					  break;
	
			default:
					posunuti=(y+a)*640+x-banka*65536;
					memmove ((0xa0000+posunuti), (image+a*(xlen+1)), xlen);
					break;
		  };
		  }
}

void Get_image (int x, int y, int xlen, int ylen, char *image)
{
	int a,b,c,d;
	long int posunuti=0;
	int banka;

	if (y<103) {wm_ChangeBank_ (0); banka=0;}
	if (y>103 && y<205) {wm_ChangeBank_ (1); banka=1;}
	if (y>205 && y<308) {wm_ChangeBank_ (2); banka=2;}
	if (y>308 && y<410) {wm_ChangeBank_ (3); banka=3;}
	if (y>410) {wm_ChangeBank_ (4); banka=4;}

	for (a=0; a<=ylen; a++)
		{

		d=y+a;
		switch (d)
		{
			case 102: 
					  if ((x+xlen)<256){
						posunuti=(y+a)*640+x;
						memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);
						wm_ChangeBank_ (1); banka=1;}
					  if (x>=256){ 
					    wm_ChangeBank_ (1); banka=1;
						posunuti=(((y+a)*640+x)-65536);
						memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);
						}
					  if (x<256 && (x+xlen)>256){
						posunuti=(y+a)*640+x; b=x; c=0;
					  	while (b!=256)
							{memmove ((image+a*(xlen+1)+c), (0xa0000+posunuti), 1);
							 posunuti++; b++; c++;}
						wm_ChangeBank_ (1); banka=1;
						posunuti=(((y+a)*640+x)-65536);
						memmove ((image+a*(xlen+1)+c), (0xa0000+posunuti+c), (xlen-c));}
					  break;
			case 204: 
					  if ((x+xlen)<512){
						posunuti=(((y+a)*640+x)-65536);
						memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);
						wm_ChangeBank_ (2); banka=2;}
					  if (x>=512){ 
					    wm_ChangeBank_ (2); banka=2;
						posunuti=(((y+a)*640+x)-131072);
						memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);}
					  if (x<512 && (x+xlen)>512){
						posunuti=(((y+a)*640+x)-65536); b=x; c=0;
					  	while (b!=512)
							{memmove ((image+a*(xlen+1)+c), (0xa0000+posunuti), 1);
							 posunuti++; b++; c++;}
						wm_ChangeBank_ (2); banka=2;
						posunuti=(((y+a)*640+x)-131072);
						memmove ((image+a*(xlen+1)+c), (0xa0000+posunuti+c), (xlen-c));}
					  break;
	
			case 307:
					  if ((x+xlen)<128){
						posunuti=(((y+a)*640+x)-131072);
						memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);
						wm_ChangeBank_ (3); banka=3;}
					  if (x>=128){ 
					    wm_ChangeBank_ (3); banka=3;
						posunuti=(((y+a)*640+x)-196608);
						memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);}
					  if (x<128 && (x+xlen)>128){
						posunuti=(((y+a)*640+x)-131072); b=x; c=0;
					  	while (b!=128)
							{memmove ((image+a*(xlen+1)+c), (0xa0000+posunuti), 1);
							 posunuti++; b++; c++;}
						wm_ChangeBank_ (3); banka=3;
						posunuti=(((y+a)*640+x)-196608);
						memmove ((image+a*(xlen+1)+c), (0xa0000+posunuti+c), (xlen-c));}
					  break;

			case 409:
					  if ((x+xlen)<384){
						posunuti=(((y+a)*640+x)-196608);
						memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);
						wm_ChangeBank_ (4); banka=4;}
					  if (x>=384){ 
					    wm_ChangeBank_ (4); banka=4;
						posunuti=(((y+a)*640+x)-262144);
						memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);}
					  if (x<384 && (x+xlen)>384){
						posunuti=(((y+a)*640+x)-196608); b=x; c=0;
					  	while (b!=384)
							{memmove ((image+a*(xlen+1)+c), (0xa0000+posunuti), 1);
							 posunuti++; b++; c++;}
						wm_ChangeBank_ (4); banka=4;
						posunuti=(((y+a)*640+x)-262144);
						memmove ((image+a*(xlen+1)+c), (0xa0000+posunuti+c), (xlen-c));}
					  break;
	
			default:
					posunuti=(y+a)*640+x-banka*65536;
					memmove ((image+a*(xlen+1)), (0xa0000+posunuti), xlen);
					break;
		  };
		  }

}


void Get_VESA_info (void)
{
	char far *str;
	VESA_INFO_BLOCK *p;

	p=(VESA_INFO_BLOCK *)mem_alloc(sizeof(VESA_INFO_BLOCK));

	memset(&dpmiregs,0,sizeof(DPMIREGS));
	dpmiregs.EAX=0x00004f00; 
	dpmiregs.DS=D32RealSeg(p);	
	dpmiregs.ES=D32RealSeg(p);	
	dpmiregs.EDI=D32RealOff(p);		

	WtNs386(0x10,&dpmiregs);

	str=(char far *)MK_FP(Selector,0);
	_fmemcpy(&VesaInfoBlock,str,sizeof(VESA_INFO_BLOCK));

	mem_free(p);
	if(dpmiregs.EAX!=0x4f)
		{ 
		printf ("VESA Bios extension not found!!!!");
    	exit (1);
		}
}


void Get_mode_info (int mode)
{ 
	char far *str;
	VESA_MODE_INFO_BLOCK *p;  

	p=(VESA_MODE_INFO_BLOCK *)mem_alloc(sizeof(VESA_MODE_INFO_BLOCK));

	memset(&dpmiregs,0,sizeof(DPMIREGS)); //nuluje registry
	dpmiregs.EAX=0x00004f01; 
	dpmiregs.DS=D32RealSeg(p);	
	dpmiregs.ES=D32RealSeg(p);	
	dpmiregs.EDI=D32RealOff(p);		
	dpmiregs.ECX=mode;		

	WtNs386(0x10,&dpmiregs);

	str=(char far *)MK_FP(Selector,0);
	_fmemcpy(&VesaModeInfoBlock,str,sizeof(VESA_INFO_BLOCK));

	_VGAGran=VesaModeInfoBlock.WinGranularity;

	mem_free(p);
}

