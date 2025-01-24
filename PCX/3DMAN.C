#include "..\types.h"
#include "..\bgraph.h";
#include "pcx.h"
#include <mem.h>
#include <malloc.h>
#include <stdio.h>
typedef char bitmap[640*480+512+6+16];

#define DATASTART (512+6)
#define DATASIZE (640*480)

bitmap out;
bitmap in1;

void clear_bm(bitmap *c)
{
memset(((char *)c+DATASTART),0,DATASIZE);
}

void adjust_point(int *x,int *y)
  {
  while (*x>=640) (*x)-=640;
  while (*x<0) (*x)+=640;
  while (*y>=480) (*y)-=480;
  while (*y<0) (*y)+=480;
  }

void set_point(int x,int y,int c)
  {
  adjust_point(&x,&y);
  out[DATASTART+x+y*640]=c;
  }

int get_point(int x,int y)
  {
  x=x*640/500;y=y*480/2000;
  adjust_point(&x,&y);
  return in1[DATASTART+x+y*640];
  }

#define TXT_Y 480
#define TXT_X 640
#define OKO_Y 103
#define OKO_X 250
#define OKO_Z (-1100)
#define OSA_Y 1000
#define OSA_X 500

int vypocet_z(int y)
  {
  return(OSA_Y-y)*(-OKO_Z)/(y-OKO_Y);
  }

int vypocet_x(int x,int z)
  {
  return(x-OKO_X)*z/(-OKO_Z)+x;
  }

void smycka()
  {
  int x1,y1,z1;
  int x2;

  for(y1=351;y1>178;y1--)
     {
     z1=vypocet_z(y1);
     for(x1=0;x1<639;x1++)
        {
        x2=vypocet_x(x1-70,z1);
        set_point(x1,y1+9+16,get_point(x2,z1+350));
        }
  /*if (!(y1 & 7))*/{put_picture(0,0,out);showview(0,0,0,0);}

     }
  }

initmode32b();


void init()
  {
  initmode32b();
  memcpy(&out,&in1,sizeof(in1));
  clear_bm(out);
  }

main()
  {
  void *buffer;

  open_pcx("TEXTURA.PCX",A_8BIT,&buffer);
  memcpy(&in1,buffer,sizeof(in1));
  free(buffer);
  open_pcx("sit.PCX",A_8BIT,&buffer);
  init();
  memcpy(&out,buffer,sizeof(out));
  put_picture(0,0,&out);
  showview(0,0,0,0);
  smycka();
  getchar();
  return 0;
  }
