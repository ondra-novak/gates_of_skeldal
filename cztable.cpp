#include <string.h>

struct czxlat
  {
  unsigned char kamenik;
  unsigned char windows;
  };


static czxlat czxlattab[]=
  {
	{0xA0,'·'},
	{0x87,'Ë'},
	{0x83,'Ô'},
	{0x82,'È'},
	{0x88,'Ï'},
	{0xA1,'Ì'},
	{0x8D,'Â'},
	{0x8C,'æ'},
	{0xA4,'Ú'},
	{0xA2,'Û'},
	{0xAA,'‡'},
	{0xA9,'¯'},
	{0xA8,'ö'},
	{0x9f,'ù'},
	{0xA3,'˙'},
	{0x96,'˘'},
	{0x98,'˝'},
	{0x91,'û'},

	{0x8F,'¡'},
	{0x80,'»'},
	{0x85,'œ'},
	{0x90,'…'},
	{0x89,'Ã'},
	{0x8B,'Õ'},
	{0x8A,'≈'},
	{0x9C,'º'},
	{0xA5,'“'},
	{0x95,'”'},
	{0xAB,'¿'},
	{0x9E,'ÿ'},
	{0x9B,'ä'},
	{0x86,'ç'},
	{0x97,'⁄'},
	{0xA6,'Ÿ'},
	{0x9D,'›'},
	{0x92,'é'},
  };

static char xlatkm2win[256];
static char xlatwin2km[256];
static char prepare=1;

static void PrepareTabs()
  {
  int i;
  for (i=0;i<256;i++) {xlatkm2win[i]=i;xlatwin2km[i]=i;}
  for (i=0;i<sizeof(czxlattab)/sizeof(czxlattab[0]);i++)
	{
	xlatkm2win[czxlattab[i].kamenik]=czxlattab[i].windows;
	xlatwin2km[czxlattab[i].windows]=czxlattab[i].kamenik;
	}
  prepare=0;
  }

extern "C"
  {

void windows2kamenik(const char *src, int size, char *trg)
  {
  if (prepare) PrepareTabs();
  if (size<0) size=strlen(src)+1;
  for (int i=0;i<size;i++) *trg++=xlatwin2km[(unsigned char)*src++];
  }

void kamenik2windows(const char *src, int size, char *trg)
  {
  if (prepare) PrepareTabs();
  if (size<0) size=strlen(src)+1;
  for (int i=0;i<size;i++) *trg++=xlatkm2win[(unsigned char)*src++];
  }

  }