#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <mem.h>
#include <math.h>
#include <types.h>
#include <memman.h>
#include <bgraph.h>
#include "..\game\engine1.h"
#include "pcx.h"

void calc_points();
void create_tables();

char filename1[128];
char filename2[128];

char buffer[640*480+6+512];

char *obr1,*obr2;

typedef struct tbarva
  {
  char r,g,b;
  }TBARVA;

TBARVA shadow;
int shadow_max;

struct tbarva palette[256];


char datapath[]="";
extern struct all_view showtabs;
extern t_points points;

void do_events()
  {

  }

int get_shd_color(char src,int cur)
  {
  struct tbarva what;
  int c,d,m,p,i,ml;

  m=256*256*256;
  c=0;d=0;
  what=palette[src];
  cur=shadow_max-cur;
  cur=(int)(shadow_max*sqrt((float)cur/(float)shadow_max));
  what.r=(shadow.r+(what.r-shadow.r)*cur/shadow_max);
  what.g=(shadow.g+(what.g-shadow.g)*cur/shadow_max);
  what.b=(shadow.b+(what.b-shadow.b)*cur/shadow_max);
  for(i=0;i<256;i++)
     {
     p=(what.r-palette[i].r)*(what.r-palette[i].r)+
        (what.g-palette[i].g)*(what.g-palette[i].g)+
        (what.b-palette[i].b)*(what.b-palette[i].b);
     if (p<m)
        {
        d=c;
        ml=m;
        m=p;
        c=i;
        }
     }
  d=c;c=0;
  return c*256+d;
  }

int draw_one_line_f(yline,maxy,swap)
  {
  char *drawadr,*srcadr1,*srcadr2;
  int sizex;
  int i;
  word table[256];

  printf("Generuji podlahu: %d%% \r",yline*100/shadow_max);flushall();
  memset(table,0xff,sizeof(table));
  sizex=showtabs.f_table[2][yline].linesize;
  drawadr=showtabs.f_table[2][yline].lineofs/2+sizex/2+buffer+512+6;
  srcadr1=obr1+(showtabs.f_table[2][yline].counter*320/maxy)*500+6+512;
  srcadr2=obr2+(showtabs.f_table[2][yline].counter*320/maxy)*500+6+512;
  for(i=-320;i<320;i++)
     {
     char *point,*srcp;
     int xp;

     point=drawadr+i;
     xp=250+(i*500/sizex);
     while (xp>=1000) xp-=1000;
     while (xp<0) xp+=1000;
     if ((xp>=500)^(swap)) srcp=srcadr2+(xp % 500); else srcp=srcadr1+(xp%500);
     if (table[*srcp]==0xffff) table[*srcp]=get_shd_color(*srcp,yline);
     *point=table[*srcp]&0xff;
     }
  return showtabs.f_table[2][yline].counter;
  }

void draw_it_all_f()
  {
  int i,m,y,m2,s;

  y=0;s=0;
  m2=(360-MIDDLE_Y)-points[0][0][VIEW3D_Z].y;
  shadow_max=m2;
  for (i=0;i<VIEW3D_Z;i++)
     {
     m=points[0][0][i].y-points[0][0][i+1].y;
     while (draw_one_line_f(y++,m,s) && y<m2);
     s=!s;
     }
  printf("\n");
  }

int draw_one_line_c(yline,maxy,swap)
  {
  char *drawadr,*srcadr1,*srcadr2;
  int sizex;
  int i;
  word table[256];


  printf("Generuji strop: %d%% \r",yline*100/shadow_max);flushall();
  memset(table,0xff,sizeof(table));
  srcadr1=obr1+(showtabs.c_table[2][yline].counter*320/maxy)*500+6+512;
  srcadr2=obr2+(showtabs.c_table[2][yline].counter*320/maxy)*500+6+512;
  sizex=showtabs.c_table[2][yline].linesize;
  drawadr=showtabs.c_table[2][yline].lineofs/2+sizex/2+buffer+512+6;
  for(i=-320;i<320;i++)
     {
     char *point,*srcp;
     int xp;

     point=drawadr+i;
     xp=250+(i*500/sizex);
     while (xp>=1000) xp-=1000;
     while (xp<0) xp+=1000;
     if ((xp>=500)^(swap)) srcp=srcadr2+(xp % 500); else srcp=srcadr1+(xp % 500);
     if (table[*srcp]==0xffff) table[*srcp]=get_shd_color(*srcp,yline);
     *point=(table[*srcp]&0xff);
     }
  return showtabs.c_table[2][yline].counter;
  }

void draw_it_all_c()
  {
  int i,m,y,m2,s;

  y=0;s=0;
  m2=MIDDLE_Y+points[0][1][VIEW3D_Z].y;
  shadow_max=m2;
  for (i=0;i<VIEW3D_Z;i++)
     {
     m=points[0][1][i+1].y-points[0][1][i].y;
     while (draw_one_line_c(y++,m,s) && y<m2);
     s=!s;
     }
  printf("\n");
  }

void read_palette(char *filename,void *paleta)
  {
  FILE *f;
  long l;

  f=fopen(filename,"rb");
  fseek(f,0,SEEK_END);
  l=ftell(f);
  fseek(f,l-768,SEEK_SET);
  fread(paleta,1,768,f);
  fclose(f);
  }

int save_pcx(char *filename,int x1,int y1,int x2,int y2)
  {
  FILE *f;
  PCXHEADER head;
  int y;

  memset(&head,0,sizeof(head));
  head.id=0x050A;
  head.encoding=1;
  head.bitperpixel=8;
  head.xmin=0;
  head.ymin=0;
  head.xmax=x2-x1;
  head.ymax=y2-y1;
  head.bytesperline=x2-x1+1;
  head.hdpi=x2-x1+1;
  head.vdpi=y2-y1+1;
  head.mplanes=1;
  f=fopen(filename,"wb");
  if (f==NULL) return -1;
  fwrite(&head,1,sizeof(head),f);
  for(y=y1;y<=y2;y++)
     {
     char line[1400];
     int lineptr=0,counter=0,x;
     int b,last;
     char *p;

     counter=0;
     lineptr=0;
     p=&buffer[6+512+640*y];
     last=~*p;
     for(x=x1;x<=x2;x++)
        {
        b=*p++;
        if (b==last) counter++;
        if (counter==63 || b!=last)
           {
           if (counter)
              {
              line[lineptr++]=0xc0+counter;
              line[lineptr++]=last;
              }
           last=b;
           if (counter==63) counter=0; else counter=1;
           }
        }
     line[lineptr++]=0xc0+counter;
     line[lineptr++]=last;
     last=0;
     for(x=0;x<lineptr;x+=2)
        {
        if (line[x+1]<0xc0 && line[x]<0xc2)
           line[last++]=line[x+1];
        else
           {
           line[last++]=line[x];
           line[last++]=line[x+1];
           }
        }
     lineptr=last;
     fwrite(&line,1,lineptr,f);
     }
  y=0x0c;
  fwrite(&y,1,1,f);
  fwrite(&palette,1,sizeof(palette),f);
  fclose(f);
  return 0;
  }


void strass(char *trg,char *src,int trgnum)
  {
  strncpy(trg,src,trgnum);
  trg[trgnum-1]='\0';
  }

void help()
  {
  printf("ONS Podlahar (C)1997 \n"
         "\n"
         "Pou‘it¡: PODLAHY <obrazek1.pcx> <obrazek2.pcx> <rv> <gv> <bv> \n"
         "\n"
         "<obrazek1.pcx> <obrazek2.pcx>\n"
         "        soubory s texturami ve formatu pcx. Textura musi mit velikost 500x320\n\n"
         "<rv> <gv> <bv>\n"
         "       Cisla predstavujici slozky barvy, jenz se pouzije ke generovani mlhy,\n"
         "       nebo ke stmavovani. Jednotlive hodnoty jsou v rozsahu <0-255>\n"
         "\n"
         "Vysledne soubory se ulozi do aktualniho adresare pod jmeny:\n"
         "     PODLAHA.PCX - soubor s podlahou\n"
         "     STROP.PCX   - soubor se stropem\n"
         "\n"
         "Pozn.: Prestoze se jedna o korektni format PCX, doporucuji oba soubory \n"
         "  natahnout do Autodesk Animatora a znovu v tomto formatu ulozit\n"
         "  Stavalo se mi, ze se muj PCX dekomprimator s timto vygenerovanym souborem\n"
         "  hroutil.\n"
         );
  exit(0);
  }


main(int argv,char *argc[])
  {
  int r,g,b,i;
  char rv[10],gv[10],bv[10];
  if (argv!=6) help();
  strass(filename1,argc[1],128);
  strass(filename2,argc[2],128);
  strass(rv,argc[3],10);
  strass(gv,argc[4],10);
  strass(bv,argc[5],10);
  sscanf(rv,"%d",&r);
  sscanf(gv,"%d",&g);
  sscanf(bv,"%d",&b);
  if (r<0 || r>255 || g<0 || g>255  || b<0 || b>255 )
    {
    printf("Hodnoty by mely byt rozsahu <0-255> \n");
    abort();
    }
  shadow.r=r;shadow.g=g;shadow.b=b;
  calc_points();
  create_tables();
  read_palette(filename1,&palette);
  shadow_max=100;
  memset(buffer,get_shd_color(0,100),640*480+6+512);
  *(word *)buffer=640;
  *((word *)buffer+1)=480;
  *((word *)buffer+2)=8;
  if (open_pcx(filename1,A_8BIT,&obr1) )
     {
     printf("Nemohu najit %s.\n",filename1);
     abort();
     }
  if (open_pcx(filename2,A_8BIT,&obr2) )
     {
     printf("Nemohu najit %s.\n",filename2);
     abort();
     }
  memcpy(buffer+6,obr1+6,512);
  draw_it_all_f();
  draw_it_all_c();
  initmode32();
  for(i=0;i<screen_buffer_size/2;i++)
     screen[i]=((r>>3)<<10)+((g>>3)<<5)+(b>>3);
  put_picture(0,0,buffer);
  showview(0,0,0,0);
  getchar();
  closemode();
  save_pcx("STROP.PCX",0,0,639,92);
  save_pcx("PODLAHA.PCX",0,360-198,639,360);
  printf("Vytvoreny soubory STROP.PCX a PODLAHA.PCX\n");

  }

