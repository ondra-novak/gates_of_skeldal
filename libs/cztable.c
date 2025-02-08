#include <string.h>

typedef struct czxlat
  {
  unsigned char kamenik;
  unsigned char windows;
  } CZXLAT;


static CZXLAT czxlattab[]=
  {
	{0xA0,0xE1},
	{0x87,0xE8},
	{0x83,0xEF},
	{0x82,0xE9},
	{0x88,0xEC},
	{0xA1,0xED},
	{0x8D,0xE5},
	{0x8C,0xBE},
	{0xA4,0xF2},
	{0xA2,0xF3},
	{0xAA,0xE0},
	{0xA9,0xF8},
	{0xA8,0x9A},
	{0x9f,0x9D},
	{0xA3,0xFA},
	{0x96,0xF9},
	{0x98,0xFD},
	{0x91,0x9E},

	{0x8F,0xC1},
	{0x80,0xC8},
	{0x85,0xCF},
	{0x90,0xC9},
	{0x89,0xCC},
	{0x8B,0xCD},
	{0x8A,0xC5},
	{0x9C,0xBC},
	{0xA5,0xD2},
	{0x95,0xD3},
	{0xAB,0xC0},
	{0x9E,0xD8},
	{0x9B,0x8A},
	{0x86,0x8D},
	{0x97,0xDA},
	{0xA6,0xD9},
	{0x9D,0xDD},
	{0x92,0x8E},
  };

static char xlatkm2win[256];
static char xlatwin2km[256];
static char prepare=1;

static void PrepareTabs()
  {
  unsigned int i;
  for (i=0;i<256;i++) {xlatkm2win[i]=(char)i;xlatwin2km[i]=(char)i;}
  for (i=0;i<sizeof(czxlattab)/sizeof(czxlattab[0]);i++)
	{
	xlatkm2win[czxlattab[i].kamenik]=czxlattab[i].windows;
	xlatwin2km[czxlattab[i].windows]=czxlattab[i].kamenik;
	}
  prepare=0;
  }


void windows2kamenik(const char *src, int size, char *trg)
  {
  if (prepare) PrepareTabs();
  if (size<0) size=(int)strlen(src)+1;
  for (int i=0;i<size;i++) *trg++=xlatwin2km[(unsigned char)*src++];
  }

void kamenik2windows(const char *src, int size, char *trg)
  {
  if (prepare) PrepareTabs();
  if (size<0) size=(int)strlen(src)+1;
  for (int i=0;i<size;i++) *trg++=xlatkm2win[(unsigned char)*src++];
  }


int windows2kamenik_chr(int chr){
    if (prepare) PrepareTabs();
    return  xlatwin2km[chr & 0xFF];

}
int kamenik2windows_chr(int chr) {
    if (prepare) PrepareTabs();
    return  xlatkm2win[chr & 0xFF];
}
