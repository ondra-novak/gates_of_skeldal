#include <stdio.h>
#include <stdlib.h>
#include <bgraph.h>
#include <pcx.h>
#include <math.h>
#include <conio.h>
#include <mem.h>


#define PIC_XS 320
#define PIC_YS 184
#define HICOLOR 32768
#define PICTURE "TEST1.PCX"
#define PI 3.141592654
#define C(x) ((x)==0?0.5:1.0)
#define QUALITY 4

typedef signed char t_y_buffer[PIC_YS*PIC_XS];
typedef signed char t_cb_buffer[PIC_YS*PIC_XS/2];
typedef signed char t_cr_buffer[PIC_YS*PIC_XS/2];

typedef signed char T_COLORT[HICOLOR][3];
typedef short T_COLORIT[64][64][64];

t_y_buffer y_buff,y_buff_new;
t_cb_buffer cb_buff,cb_buff_new;
t_cr_buffer cr_buff,cr_buff_new;
T_COLORT colort;
T_COLORIT colorit;

/*char y_iquant[64]=
  {
  16,11,10,16,24,40,51,61,
  12,12,14,19,26,58,60,55,
  14,13,16,24,40,57,69,56,
  14,17,22,29,51,87,80,61,
  18,22,37,56,68,109,103,77,
  24,35,55,64,81,104,113,92,
  49,64,78,87,103,121,120,101,
  72,92,95,98,112,100,103,99
  };
*/
signed char y_iquant[64]=
  {
  4, 8, 9, 10,15,20,22,30,
  8,8,9,11,16,21,24,30,
  9,9,10,12,20,25,30,36,
  9,10,11,13,25,30,35,38,
  10,12,15,20,25,35,40,45,
  11,14,20,27,41,50,55,60,
  20,30,40,45,50,61,60,50,
  32,48,49,51,50,51,50,49,
  };


char test[]=
  {
  139,144,149,153,155,155,155,155,
  144,151,153,156,159,156,156,156,
  150,155,160,163,158,156,156,156,
  159,161,162,160,160,159,159,159,
  159,160,161,162,162,155,155,155,
  161,161,161,161,160,157,157,157,
  162,162,161,163,162,157,157,157,
  162,162,161,161,163,158,158,158
  };

signed char c_iquant[64]=
  {
  2,4,5,5,6,10,11,15,
  4,4,5,5,8,10,12,15,
  4,4,6,6,10,12,15,18,
  4,5,5,6,12,15,17,18,
  5,6,7,10,12,16,20,22,
  6,7,10,13,20,25,26,30,
  10,15,20,22,25,30,30,25,
  16,24,25,26,25,26,25,25,
  };



signed char koef_table[PIC_YS*PIC_XS];

short *picture;

char code_tab[][2]=
  {
  {0,0},{1,0},{0,1},{0,2},{1,1},{2,0},{3,0},{2,1},{1,2},{0,3},{0,4},{1,3},{2,2},
  {3,1},{4,0},{5,0},{4,1},{3,2},{2,3},{1,4},{0,5},{0,6},{1,5},{2,4},{3,3},{4,2},
  {5,1},{6,0},{7,0},{6,1},{5,2},{4,3},{3,4},{2,5},{1,6},{0,7},{1,7},{2,6},{3,5},
  {4,4},{5,3},{6,2},{7,1},{7,2},{6,3},{5,4},{4,5},{3,6},{2,7},{3,7},{4,6},{5,5},
  {6,4},{7,3},{7,4},{6,5},{5,6},{4,7},{5,7},{6,6},{7,5},{7,6},{6,7},{7,7}
  };

signed char dct[64][64]; //v poradi [koef;x,y] - koef dle tabulky
signed char ict[64][64]; //v poradi [x,y;koef];koef dle tabulky

signed short mlt[256][256];   //[i,j]=i*j
signed char mlt2[256][256];  //[i,j]=i*j (s oriznutim)


void create_color_table(void)
  {
  int r,g,b;
  int y,cb,cr,i;


  puts("Creating SECAM CT table");
  for(i=0;i<HICOLOR;i++)
     {
     r=i>>10;
     g=(i>>5) & 0x1f;
     b=i & 0x1f;
     r<<=3;
     g<<=3;
     b<<=3;
     y=(r*299+g*587+b*114)/1000;
     cb=564*(b-y)/1000;
     cr=713*(r-y)/1000;
     colort[i][0]=y-128;
     colort[i][1]=cb;
     colort[i][2]=cr;
     }
  for(y=0;y<64;y++)
     for(cb=-32;cb<31;cb++)
        for(cr=-32;cr<31;cr++)
           {
           int y2=y<<2,cr2=cr<<3,cb2=cb<<3;
           r=y2+cr2*1402/1000;
           g=y2-(cb2*344+cr2*714)/1000;
           b=y2+cb2*1772/1000;
           r>>=3;
           g>>=3;
           b>>=3;
           if (r>31) r=31;
           if (g>31) g=31;
           if (b>31) b=31;
           if (r<0) r=0;
           if (g<0) g=0;
           if (b<0) b=0;
           i=(r<<10)+(g<<5)+b;
           colorit[(y-32)&63][cb&63][cr&63]=i;
           }
  }

#define NORMALIZE(i) if (i>127) i=127;else if (i<-128) i=-128;else;

void rozklad_obrazku(void)
  {
  int i;

  for(i=0;i<PIC_XS*PIC_YS;i++)
     {
     int j=i>>1,k;
     k=colort[picture[i+3]][0]-y_buff_new[i];
     NORMALIZE(k);
     y_buff[i]=k;
     if (i & 1)
        {
        cb_buff[j]+=colort[picture[i+3]][1]>>2;
        cr_buff[j]+=colort[picture[i+3]][2]>>2;
        k=cb_buff[j]-cb_buff_new[j];NORMALIZE(k);
        cb_buff[j]=k;
        k=cr_buff[j]-cr_buff_new[j];NORMALIZE(k);
        cr_buff[j]=k;
        }
     else
        {
        cb_buff[j]=colort[picture[i+3]][1]>>2;
        cr_buff[j]=colort[picture[i+3]][2]>>2;
        }
     }
  }

void slozeni_obrazku(void)
  {
  int i,x,y;
  int yy,cb,cr,w;
  unsigned short *ww;

  i=0;
  for(y=0;y<184;y++)
     for(x=0;x<320;x++)
        {
        yy=y_buff_new[i]>>2;
        cb=(cb_buff_new[i>>1]>>2);
        cr=(cr_buff_new[i>>1]>>2);
        w=colorit[yy&63][cb&63][cr&63];w=w+(w<<16);
        ww=screen+x*2+y*1280;
        ww[0]=w;ww[1]=w;
        ww[640]=w;ww[641]=w;
        //screen[x+y*640]=colorit[y_buff[i]>>2][16][16];
        i++;
        }
  showview(0,0,0,0);
  }

void create_dct(void)
  {
  int j,k,u,v,x,y;

  puts("Creating DCT table:");
  for(k=0;k<64;k++)
     for(j=0;j<64;j++)
        {
          float res;

           u=code_tab[j][0];
           v=code_tab[j][1];
           x=k & 7;y=k>>3;
           res=127*cos((2*x+1)*u*PI/16)*cos((2*y+1)*v*PI/16);
           dct[j][k]=(signed char)res;
        }

  }

void create_ict(void)
  {
  int j,k,u,v,x,y;


  puts("Creating ICT table:");
  for(j=0;j<64;j++)
        for(k=0;k<64;k++)
           {
           float res;

           u=code_tab[j][0];
           v=code_tab[j][1];
           x=k & 7;y=k>>3;
           res=127*cos((2*x+1)*u*PI/16)*cos((2*y+1)*v*PI/16);
           ict[k][j]=(signed char)res;
           }
  }

void create_mult()
  {
  int i,j,k;

  puts("Creating MULT table:");
  for(i=-128;i<128;i++)
     for(j=-128;j<128;j++)
      {
      mlt[i & 0xff][j & 0xff]=i*j;
      k=i*j;NORMALIZE(k);
      mlt2[i & 0xff][j & 0xff]=k;
      }
  }

#define GET_DCT(i,k,p) (mlt[i][dct[k][p]])
#define GET_ICT(i,k,p) (mlt[i][ict[k][p]])

void dct_buffer(void *buffer,int box_x,int box_y,int linelen,char *quant)
  {
  int nextline=7*linelen;
  int row,col,kn,pp;
  char *c,*p;
  signed char *ktab;
  quant;

  c=buffer;
  ktab=koef_table;

  for(row=0;row<box_y;row++)
     {
     for (col=0;col<box_x;col++)
        {
        char *kk=(char *)dct;
        for(kn=0;kn<64;kn++)
           {
           int suma=0;
           int u,v;
           p=c;
           u=code_tab[kn][0];
           v=code_tab[kn][1];
           for(pp=0;pp<64;)
              {
              suma+=mlt[(*p++)][(*kk++)];
              pp++;if (!(pp & 7)) p+=linelen-8;
              }
           suma/=128;
           suma>>=2;
           suma*=C(u)*C(v);NORMALIZE(suma);
           //suma/=quant[kn];
           *ktab++=(signed char)(suma);
           }
        c+=8;
        }
     c+=nextline;
     }
  }

void ict_buffer(void *buffer,int box_x,int box_y,int linelen,char *quant)
  {
  int nextline=7*linelen;
  int row,col,kn,pp;
  signed char *c,*p;
  signed char *ktab;
  char kofs[64],ko=0;
  quant;

  c=buffer;
  ktab=koef_table;

  for(row=0;row<box_y;row++)
     {
     for (col=0;col<box_x;col++)
        {
        p=c;
        for(pp=0;pp<64;)
           {
           int suma=0;
           signed char *ictr=&ict[pp];
           for(kn=0,ko=0;kn<64;kn++) if (ktab[kn]) {kofs[ko++]=kn;}
           for(kn=0;kn<ko;kn++)
              {
              unsigned char kc=kofs[kn];
              unsigned char cc=ktab[kc];
              //int mult=mlt2[cc][quant[kn]];
              //suma+=mlt[mult & 0xff][ictr[kn] & 0xff];
              //int mult=(signed char)cc*(quant[kc]);
              suma+=(signed char)cc*ictr[kc];
              }
           suma>>=7;
           suma>>=2;
           suma+=p[0];
           NORMALIZE(suma);
           p[0]=suma;p++;
           pp++;if (!(pp & 7)) p+=linelen-8;
           }
        ktab+=64;
        c+=8;
        }
     c+=nextline;
     }
  }


main()
  {
  char pcn[100];
  int i;
  create_color_table();
  create_dct();
  create_ict();
  create_mult();
  memset(y_buff_new,0,sizeof(y_buff_new));
  memset(cr_buff_new,0,sizeof(cr_buff_new));
  memset(cb_buff_new,0,sizeof(cb_buff_new));
  //dct_buffer(test,1,1,8,y_iquant);
  //ict_buffer(y_buff_new,40,23,320,y_iquant);

  initmode32();

  for(i=0;i<100;i++)
     {
  sprintf(pcn,"TEST%02d.PCX",i);
  open_pcx(pcn,A_15BIT,(void **)&picture);
  rozklad_obrazku();
  free(picture);
  dct_buffer(y_buff,40,23,320,y_iquant);
  ict_buffer(y_buff_new,40,23,320,y_iquant);
  dct_buffer(cr_buff,20,23,160,c_iquant);
  ict_buffer(cr_buff_new,20,23,160,c_iquant);
  dct_buffer(cb_buff,20,23,160,c_iquant);
  ict_buffer(cb_buff_new,20,23,160,c_iquant);


  //memcpy(y_buff_new,y_buff,sizeof(y_buff));
//  memcpy(cr_buff_new,cr_buff,sizeof(cr_buff));
//  memcpy(cb_buff_new,cb_buff,sizeof(cb_buff));

  slozeni_obrazku();
     }
  getchar();
  }
