#include <types.h>
#include <stdio.h>
#include <math.h>
#include <bgraph.h>
#include <mem.h>
#include <conio.h>
#include "flc.h"

typedef short C_MATICE[8][8];
typedef float R_MATICE[8][8];

C_MATICE quant=
  {
  {16,11,10,16,24,40,51,61},
  {12,12,14,19,26,58,60,55},
  {14,13,16,24,40,57,69,56},
  {14,17,22,29,51,87,80,61},
  {18,22,37,56,68,109,103,77},
  {24,35,55,64,81,104,113,92},
  {49,64,78,87,103,121,120,101},
  {72,92,95,98,112,100,103,99}
  };

/*C_MATICE test=
  {
  {139,144,149,153,155,155,155,155},
  {144,151,153,156,159,156,156,156},
  {150,155,160,163,158,156,156,156},
  {159,161,162,160,160,159,159,159},
  {159,160,161,162,162,155,155,155},
  {161,161,161,161,160,157,157,157},
  {162,162,161,163,162,157,157,157},
  {162,162,161,161,163,158,158,158}
  };

*/
char code_tab[][2]=
  {
  {0,0},{1,0},{0,1},{0,2},{1,1},{2,0},{3,0},{2,1},{1,2},{0,3},{0,4},{1,3},{2,2},
  {3,1},{4,0},{5,0},{4,1},{3,2},{2,3},{1,4},{0,5},{0,6},{1,5},{2,4},{3,3},{4,2},
  {5,1},{6,0},{7,0},{6,1},{5,2},{4,3},{3,4},{2,5},{1,6},{0,7},{1,7},{2,6},{3,5},
  {4,4},{5,3},{6,2},{7,1},{7,2},{6,3},{5,4},{4,5},{3,6},{2,7},{3,7},{4,6},{5,5},
  {6,4},{7,3},{7,4},{6,5},{5,6},{4,7},{5,7},{6,6},{7,5},{7,6},{6,7},{7,7}
  };


short cos_tab[64][64][256];
short cuv_tab[64];

#define CUV (1/1.414)
#define PI 3.14159265
#define C(u) (u==0?CUV:1)
#define FIXED 128
#define SCAN_LINE 640

char vystup[120000];
char frame[120000];
char *ip;

void create_cos_tab()
  {
  char u,v,i,k,x,y;
  int j;

  for(j=0;j<256;j++)
     for(i=0;i<64;i++)
       for(k=0;k<64;k++)
        {
         u=code_tab[i][0];
         v=code_tab[i][1];
         x=k & 0x7;
         y=k>>3;
        cos_tab[k][i][j]=(short)(quant[u][v]/2*(j-128)*cos((2*x+1)*u*PI/16)*cos((2*y+1)*v*PI/16)*FIXED/4);
        }
  for(i=0;i<64;i++)
     {
     u=code_tab[i][0];
     v=code_tab[i][1];
     cuv_tab[i]=(u==0)+(v==0);
     }
  }

void Dopredna_transformace(C_MATICE data,R_MATICE koef)
  {
  char u,v,x,y;
  float msum;

  for(u=0;u<8;u++)
     for(v=0;v<8;v++)
        {
        msum=0;
        for(x=0;x<8;x++)
           for(y=0;y<8;y++)
              msum+=data[x][y]*cos((2*x+1)*u*PI/16)*cos((2*y+1)*v*PI/16);
        msum*=0.25*C(u)*C(v);
        koef[u][v]=msum;
        }
  }

short zaokrouhlit(float f)
  {
  if (f>0) return (short)(f+0.5);
  if (f<0) return (short)(f-0.5);
  return 0;
  }

void Kvantifikace(R_MATICE data,C_MATICE out)
  {
  char u,v;

  for(u=0;u<8;u++)
     for(v=0;v<8;v++)
        out[u][v]=zaokrouhlit(data[u][v]*2/quant[u][v]);
  }

void Kodovani(C_MATICE data,char *out)
  {
  int i;
  for(i=0;i<64;i++)
     {
     *out++=(char)(data[code_tab[i][0]][code_tab[i][1]]);
     }
  }

void Zpetna_rychla_transformace(char *data,int delka,char *vystup)
  {
  char *hodn;
  char *p;
  char i,j;
  int s;

  p=vystup;
  for(j=0;j<64;j++)
     {
     s=0;hodn=data;
     for(i=0;i<delka;i++)
       {
       switch (cuv_tab[i])
           {
           case 0:s=s+cos_tab[j][i][*hodn++^0x80];break;
           case 1:s=s+cos_tab[j][i][*hodn++^0x80]*(short)(FIXED*C(0))/FIXED;break;
           case 2:s=s+cos_tab[j][i][*hodn++^0x80]>>1;break;
           }
       }
     *p++=(char)(s/FIXED);
     }
  }

void read_bar_8x8(char *data,C_MATICE vystup)
  {
  int i,j;
  for(i=0;i<8;i++)
     {
     for(j=0;j<8;j++)
        vystup[j][i]=*data++;
     data+=SCAN_LINE-j;
     }
  }

void write_jpg_info(char *block,char *vystup,int *size)
  {
  for(*size=64;*size>0;(*size)--)
      if (block[*size-1]!=0) break;
  memcpy(vystup,size,1);
  memcpy(vystup+1,block,*size);
  }



void konvert_color_Y()
  {
  int i;
  char *c,*d;
  c=frame_buffer;
  d=frame;
  for(i=0;i<sizeof(frame);i++,c++) *d++=flc_paleta[*c][0]+flc_paleta[*c][1]+flc_paleta[*c][2]>>2;
  }

void compress_layer(char *from,char *to)
  {
  R_MATICE r;
  C_MATICE c;
  char out[64];
  int s,x,y;

  for(y=0;y<24;y++)
     for(x=0;x<80;x++)
        {
        read_bar_8x8(&from[(y*SCAN_LINE+x)*8],c);
        Dopredna_transformace(c,r);
        Kvantifikace(r,c);
        Kodovani(c,out);
        write_jpg_info(out,to,&s);
        to+=s+1;
        }
  }

void decompress_layer(char *from,char *to)
  {
  char size;
  int x,y,z;
  char out[64];
  char *a;

  for(y=0;y<24;y++)
     for(x=0;x<80;x++)
       {
       a=to+((SCAN_LINE*y+x)*8);
       size=*from++;
       Zpetna_rychla_transformace(from,size,out);
       from+=size;
       for(z=0;z<8;z++) memcpy(a+=640,out+z*8,8);
       }
  }

void display()
  {
  word *c;
  char *z;
  char d;
  int i;

  c=lbuffer;
  z=frame;
  for(i=0;i<640*180;i++)
     {
     d=*z++;d>>=3;
     *c++=d+(d<<5)+(d<<10);
     }
  }

void main()
  {

  printf("creating %d Kb table\n",sizeof(cos_tab)/1024);
  create_cos_tab();
  Open_FLC("trava.flc");
  Get_first_frame();
  Decompress_frame();
  Close_FLC();
  konvert_color_Y();
  puts("compress");
  compress_layer(frame,vystup);
  puts("decompress");
  decompress_layer(vystup,frame);
  initmode32(NULL);
  display();
  getche();
  closemode();
  }


