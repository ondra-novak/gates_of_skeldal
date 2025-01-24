#include <types.h>
#include <stdio.h>
#include <pcx.h>
#include <bgraph.h>
#include <memman.h>
#include <mem.h>
#include "bitline.h"

int komprimuj_rovinu(void *vstup, int velikost,int rovina,void *buffer,int *vysledek)
  {
  int i;
  int m[4]={0,0,0,0},min,j;
  char c[4][1930];
  char *d;

  buffer;vysledek;
  rovina=1<<rovina;
  for(i=0;i<4;i++)
     {
     switch (i)
        {
        case 0:d=(char *)cmode0(vstup,c[i],velikost,rovina);break;
        case 1:d=(char *)cmode1(vstup,c[i],velikost,rovina);break;
        case 2:d=(char *)cmode2(vstup,c[i],velikost,rovina);break;
        case 3:d=(char *)cmode3(vstup,c[i],velikost,rovina);break;
        }
     m[i]=d-c[i];
     }
  if (m[0]<=m[1] && m[0]<=m[2]) j=0;
  else if (m[0]>=m[1] && m[1]<=m[2]) j=1;
  else j=2;
  if (m[j]>=m[3]) j=3;
  min=m[j];
  printf("%5d %5d %5d %5d -   %5d\n",m[0],m[1],m[2],m[3],min);
  *vysledek=min;
  memcpy(buffer,c[j],min);
  return j;
  }


void *decomprimuj_rovinu(void *vstup,void *vystup,int velikost,int rovina,int mode)
  {
  char *d;
  mode>>=rovina*2;
  mode&=0x3;
  rovina=1<<rovina;
  switch (mode)
     {
     case 0:d=cread0(vstup,vystup,velikost,rovina);break;
     case 1:d=cread1(vstup,vystup,velikost,rovina);break;
     case 2:d=cread2(vstup,vystup,velikost,rovina);break;
     case 3:d=cread3(vstup,vystup,velikost,rovina);break;
     }
  return d;
  }

void test()
  {
  char *pcx;
  char *c;
  void *d,*compr;
  unsigned short *blckdata;
  int vel,suma=0;
  int i,j,m;

  compr=getmem(640*480*3);d=compr;
  open_pcx("d:\\!!!\\SKYNET\\TOMB\\DATA\\cred3.pcx",A_15BIT,&pcx);
  c=pcx+2+2+2;
  for (i=0;i<180*4;i++)
    {
    blckdata=d;blckdata[0]=i;blckdata[1]=0;blckdata[2]=0;
    d=(char *)d+6;
    for (j=0;j<15;j++)
     {
     m=komprimuj_rovinu(c,160,j,d,&vel);
     blckdata[1+(j>>3)]|=m<<((j & 0x7)*2);
     suma+=vel;
     d=(char *)d+vel;
     }
    suma+=6;
    c+=160*2;
    }
  printf("%d / %d / %d \n",180*20*15*4, suma, suma*100/(180*20*15*4));
  c=pcx+2+2+2;
  memset(c,0,640*480*2);
  d=compr;
  for (i=0;i<180*4;i++)
    {
    blckdata=d;
    d=(char *)d+6;
    for(j=0;j<15;j++)
       d=decomprimuj_rovinu(d,&c[160*2*blckdata[0]],160,j,(int)blckdata[1]);
    }
  }

main()
  {
  test();
  }



