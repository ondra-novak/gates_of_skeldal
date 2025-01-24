#include <types.h>
#include <stdio.h>
#include <math.h>
#include "jpegmgif.h"

typedef short C_MATICE[4][4];
typedef float R_MATICE[4][4];
/*
C_MATICE quant=
  {
  {16,10,24,51},
  {14,16,40,69},
  {18,37,68,103},
  {49,78,103,120},
  };
*/
C_MATICE quant=
  {
  {2,2,4,24},
  {2,4,8,26},
  {4,8,50,80},
  {8,30,80,88},
  };

char xlat_table[256];

char code_tab[][2]=
  {
  {0,0},{1,0},{0,1},{0,2},{1,1},{2,0},{3,0},{2,1},{1,2},{0,3},{1,3},{2,2},{3,1},
  {3,2},{2,3},{3,3}
  };

int cos_tab[16][16][256];
short cuv_tab[16];

TBOX *rbbox;
TBOX *gbbox;
TBOX *ibox;

#define CUV (1/1.414)
#define PI 3.14159265
#define C(u) (u==0?CUV:1)
#define FIXED 512
#define SCAN_LINE 640

int line_skip=4*640;

void unpack_block(void *source,void *target,int size);
#pragma aux unpack_block parm [esi][edi][ecx] modify [ebx edx eax]
void konv_iyg_hicolor(void *ii,void *rb,void *gb,void *screen);
#pragma aux konv_iyg_hicolor parm[esi][edx][ebx][edi] modify [eax ecx]

void create_cos_tab()
  {
  char u,v,i,k,x,y;
  int j,tst;
  signed char cc,*c;

  for(j=0;j<256;j++)
   for(i=0;i<16;i++)
    for(k=0;k<16;k++)
        {
         u=code_tab[i][0];
         v=code_tab[i][1];
         x=k & 3;
         y=k >> 2;
         cc=j & 0xff;
        tst=cos_tab[k][i][j]=(int)(quant[u][v]*cc*C(u)*C(v)*cos((2*x+1)*u*PI/8.0)*cos((2*y+1)*v*PI/8.0)*FIXED);
        }
  for(i=0;i<16;i++)
     {
     u=code_tab[i][0];
     v=code_tab[i][1];
     cuv_tab[i]=(u==0)+(v==0);
     }
  c=xlat_table;
  for(j=0;j<256;j++) if (j>127) *c++=0;else if(j>31) *c++=31;else *c++=j;
  }

short zaokrouhlit(float f)
  {
  if (f>0) return (short)(f+0.5);
  if (f<0) return (short)(f-0.5);
  return 0;
  }

void Dopredna_transformace(C_MATICE data,C_MATICE koef)
  {
  char u,v,x,y;
  float msum;

  for(u=0;u<4;u++)
     for(v=0;v<4;v++)
        {
        msum=0;
        for(x=0;x<4;x++)
           for(y=0;y<4;y++)
              msum+=data[x][y]*cos((2*x+1)*u*PI/8.0)*cos((2*y+1)*v*PI/8.0);
        msum*=C(u)*C(v)/4.0;
        koef[u][v]=zaokrouhlit(msum/quant[u][v]);
        }
  }

void Kodovani(C_MATICE data,char *out)
  {
  int i;
  for(i=0;i<16;i++)
     {
     *out++=(char)(data[code_tab[i][0]][code_tab[i][1]]);
     }
  }

int string_size(char *data)
  {
  int i;
  i=16;
  do
     i--;
  while (i && !data[i]);
  return i+1;
  }

int transformace(void *screen_data,signed char *trans_data,int nextline,int box_place)
  {
  C_MATICE icm,rbcm,gbcm;
  C_MATICE irm,rbrm,gbrm;
  short i,r,g,a=0;
  signed char *p;
  signed char *ii,*rb,*gb;
  int x,y;
  char *tr=trans_data;

  ii=ibox[box_place];
  rb=rbbox[box_place];
  gb=gbbox[box_place];

  for(y=0;y<4;y++)
     {
     p=(char *)screen_data+nextline*3*y;
     for(x=0;x<4;x++,a++)
        {
        i=(p[2]+p[1]+p[0]);
        r=(i-p[0]);
        g=(i-p[1]);
        icm[x][y]=i-ii[a];
        rbcm[x][y]=r-rb[a];
        gbcm[x][y]=g-gb[a];
        p+=3;
        }
     }
  Dopredna_transformace(icm,irm);
  Dopredna_transformace(rbcm,rbrm);
  Dopredna_transformace(gbcm,gbrm);
  Kodovani(irm,trans_data+1);trans_data+=1+(*trans_data=string_size(trans_data+1));
  Kodovani(rbrm,trans_data+1);trans_data+=1+(*trans_data=string_size(trans_data+1));
  Kodovani(gbrm,trans_data+1);trans_data+=1+(*trans_data=string_size(trans_data+1));
  return trans_data-tr;
  }

void zpetna_transformace(char **data,void *screen,int box_place)
  {
  TBOX *ii,*rb,*gb;
  char *dd=*data;
  unpack_block(dd+1,ii=ibox+box_place,*dd);dd+=dd[0]+1;
  unpack_block(dd+1,rb=gbbox+box_place,*dd);dd+=dd[0]+1;
  unpack_block(dd+1,gb=rbbox+box_place,*dd);dd+=dd[0]+1;
  *data=dd;
  konv_iyg_hicolor(ii,rb,gb,screen);
  }



