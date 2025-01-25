#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <bios.h>
#include "memman.h"
#include "bgraph.h"


#define VIEW_SIZE_X 640
#define VIEW_SIZE_Y 360
#define TAB_SIZE_X 640
#define TAB_SIZE_Y 600
#define MIDDLE_X 320
#define MIDDLE_Y 112
#define TXT_SIZE_Y 320
#define TXT_SIZE_X_3D 74
#define TXT_SIZE_X 500
#define VIEW3D_X 2
#define VIEW3D_Z 4
#define START_X1 357
#define START_Y1 305
#define START_X2 357
#define START_Y2 -150
#define FACTOR_3D 3.33333
#define ZOOM_PHASES 9
#define C_YMAP_SIZE 79
#define F_YMAP_SIZE 176
#define CF_XMAP_SIZE 5

#define MISTNOSTI 40

typedef struct zoominfo
  {
     void *startptr, *texture;
     int32_t texture_line,line_len;
     int32_t *xtable;
     short *ytable;
     word *palette;
     word ycount;
     word xmax;
  }ZOOMINFO;


typedef struct t_info_y
  {
  int32_t drawline; //ukazatel na radku na ktere bude stena zacinat
  word vert_size; //konecna velikost steny, pokud ma pocatecni velikost TXT_SIZE_Y
  word vert_total; //maximalni velikost textury aby jeste nepresahla obrazovku
  short zoom_table[TAB_SIZE_Y];  //tabulka pro zoomovaci rutiny
  }T_INFO_Y;

typedef struct t_info_x_3d
  {
  char used;  // 1 pokud je tato strana videt
  word xpos;      //bod od leveho okraje
  word txtoffset; //posunuti x vuci texture
  word point_total; //rozdil mezi levym prednim a levym zadnim okrajem postranni steny (v adresach)
  int32_t zoom_table[MIDDLE_X]; //zoomovaci tabulka pro osu x pro postranni steny
  }T_INFO_X_3D;

typedef struct t_info_x
  {
  char used;  // 1 pokud je tato strana videt
  word xpos;  //bod od leveho okraje
  word xpos2; //totez ale pro pravou stranu
  word txtoffset; //posunuti x vuci texture
  word max_x; //pocet viditelnych bodu z textury
  word point_total;  //celkovy pocet adres mezi levym a pravym okrajem
  int32_t zoom_table[VIEW_SIZE_X]; //zoomovaci tabulka pro osu x pro kolme steny
  }T_INFO_X;

typedef struct t_floor_map
  {
  int32_t lineofs,linesize,counter;
  }T_FLOOR_MAP;

typedef struct all_view
  {
  T_INFO_Y y_table[VIEW3D_Z+1];
  T_INFO_X_3D z_table[VIEW3D_X][VIEW3D_Z];
  T_INFO_X x_table[VIEW3D_X][VIEW3D_Z+1];
  T_FLOOR_MAP f_table[CF_XMAP_SIZE][F_YMAP_SIZE];
  T_FLOOR_MAP c_table[CF_XMAP_SIZE][C_YMAP_SIZE];

  }ALL_VIEW;

typedef struct t_point
  {
  int x,y;
  }T_POINT;

typedef T_POINT t_points[VIEW3D_X+1][2][VIEW3D_Z+1];


typedef integer TESTMAP_IT[4];
typedef TESTMAP_IT TESTMAP[MISTNOSTI];

TESTMAP mapa;
char renderstop[MISTNOSTI];
char dirs[2];

t_points points;
struct all_view showtabs;

ZOOMINFO zoom;
extern char datapath[]="";
char zooming_xtable[ZOOM_PHASES][VIEW_SIZE_X];
short zooming_ytable[ZOOM_PHASES][VIEW_SIZE_Y];
short zooming_points[ZOOM_PHASES][4]
  ={
     {620,349,10,3},
     {600,338,20,7},
     {580,327,30,11},
     {560,316,40,14},
     {540,305,50,18},
     {520,293,60,21},
     {500,282,70,25},
     {480,271,80,28},
     {460,259,90,31}
  };
int zooming_step=1;
int rot_phases=2,rot_step=150;
int yreq;

void sikma_zleva(void);
#pragma aux sikma_zleva parm modify [EAX EBX ECX EDX ESI EDI]
void sikma_zprava(void);
#pragma aux sikma_zprava parm modify [EAX EBX ECX EDX ESI EDI]
void zooming32(void *source,void *target,void *xlat,int32_t xysize);
#pragma aux zooming32 parm [ESI][EDI][EBX][ECX] modify [EAX EDX]
void zooming_lo(void *source,void *target,void *xlat,int32_t xysize);
#pragma aux zooming_lo parm [ESI][EDI][EBX][ECX] modify [EAX EDX]
void zooming256(void *source,void *target,void *xlat,int32_t xysize);
#pragma aux zooming256 parm [ESI][EDI][EBX][ECX] modify [EAX EDX]
void scroll_support_32(void *lbuf,void *src1,void *src2,int size1);
#pragma aux scroll_support_32 parm [EDI][ESI][EDX][ECX] modify [EAX]
void scroll_support_256(void *lbuf,void *src1,void *src2,int size1,void *xlat);
#pragma aux scroll_support_256 parm [EDI][ESI][EDX][ECX][EBX] modify [EAX];
void fcdraw(void *source,void *target, void *table);
#pragma aux fcdraw parm [EDX][EBX][EAX] modify [ECX ESI EDI];



void *p,*p2,*pozadi,*podlaha,*strop,*sit;int i;
void (*zooming)(void *source,int32_t target,void *xlat,int32_t xysize);
void (*turn)(int32_t lbuf,void *src1,void *src2,int size1);
word *buffer_2nd;
char debug=0,nosides=0,nofloors=0,drwsit=0;

void zooming1(void *source,int32_t target,void *xlat,int32_t xysize)
  {
  zooming32(source,lbuffer+target,xlat,xysize);
  }

void zooming2(void *source,int32_t target,void *xlat,int32_t xysize)
  {
  zooming256(source,lbuffer+(target>>1),xlat,xysize);
  }

void zooming3(void *source,int32_t target,void *xlat,int32_t xysize)
  {
  zooming_lo(source,lbuffer+target,xlat,xysize);
  }

void turn1(int32_t lbuf,void *src1,void *src2,int size1)
  {
     scroll_support_32(lbuf+lbuffer,src1,src2,size1);
  }

void turn2(int32_t lbuf,void *src1,void *src2,int size1)
  {
     scroll_support_256((lbuf>>1)+lbuffer,src1,src2,size1,xlatmem);
  }








void calc_points(void)
  {
  int i,j,x1,y1,x2,y2;

  for (j=0;j<VIEW3D_X+1;j++)
     {
  x1=START_X1+2*START_X1*j;y1=START_Y1;
  x2=START_X2+2*START_X1*j;y2=START_Y2;
  for (i=0;i<VIEW3D_Z+1;i++)
     {
     points[j][0][i].x=x1;
     points[j][0][i].y=y1;
     points[j][1][i].x=x2;
     points[j][1][i].y=y2;
     x2=(int)(x2-x2/FACTOR_3D);
     y2=(int)(y2-y2/FACTOR_3D);
     x1=(int)(x1-x1/FACTOR_3D);
     y1=(int)(y1-y1/FACTOR_3D);
     }
     }
  }

void calc_x_buffer(int32_t *ptr,int32_t txt_size_x, int32_t len,int32_t total)
  {
  int i,j,old;

  old=-1;
  for (i=0;i<total;i++)
     {
     j=(i*txt_size_x)/len;
     *(ptr++)=(j-old-1);
     old=j;
     }
  }

void calc_y_buffer(short *ptr,int32_t txt_size_y, int32_t len,int32_t total)
  {
  int i,j,old;

  old=-1;
  for (i=0;i<total;i++)
     {
     j=(i*txt_size_y)/len;
     *(ptr++)=(j-old);
     old=j;
     }
  }


void create_tables(void)
  {
  int x,y;


  for (y=0;y<VIEW3D_Z+1;y++)
     {
     showtabs.y_table[y].vert_size=points[1][0][y].y-points[1][1][y].y;
     showtabs.y_table[y].vert_total=(points[1][0][y].y+MIDDLE_Y);
     showtabs.y_table[y].drawline=(VIEW_SIZE_X*(points[1][0][y].y+MIDDLE_Y))+SCREEN_OFFSET;
     calc_y_buffer(&showtabs.y_table[y].zoom_table,TXT_SIZE_Y,showtabs.y_table[y].vert_size,TAB_SIZE_Y);
     }

  for (y=0;y<VIEW3D_Z;y++)
     for (x=0;x<VIEW3D_X;x++)
     {
     int rozdil1,rozdil2;

     if (points[x][0][y+1].x>MIDDLE_X) showtabs.z_table[x][y].used=0;
     else
        {
        showtabs.z_table[x][y].used=1;
        rozdil1=points[x][0][y].x-points[x][0][y+1].x;
        rozdil2=rozdil1-MIDDLE_X+points[x][0][y+1].x;
        if (rozdil2<0)
           {
           showtabs.z_table[x][y].xpos=MIDDLE_X-points[x][0][y].x;
           showtabs.z_table[x][y].txtoffset=0;
           }
        else
           {
           showtabs.z_table[x][y].xpos=0;
           showtabs.z_table[x][y].txtoffset=(TXT_SIZE_X_3D*rozdil2/rozdil1);
           }
        showtabs.z_table[x][y].point_total=rozdil1;
        calc_x_buffer(&showtabs.z_table[x][y].zoom_table,TXT_SIZE_X_3D,rozdil1,MIDDLE_X);
        }

     }

  for (y=0;y<VIEW3D_Z+1;y++)
     for (x=0;x<VIEW3D_X;x++)
     {
     int rozdil1,rozdil2;

       {
        showtabs.x_table[x][y].used=1;
        rozdil1=points[1][0][y+1].x-points[0][0][y+1].x;
        rozdil2=-MIDDLE_X+points[x][0][y+1].x;
        if (rozdil2<0)
           {
           showtabs.x_table[x][y].xpos=MIDDLE_X-points[x][0][y+1].x;
           showtabs.x_table[x][y].txtoffset=0;
           showtabs.x_table[x][y].max_x=rozdil1;
           }
        else
           {
           showtabs.x_table[x][y].xpos=0;
           showtabs.x_table[x][y].txtoffset=(TXT_SIZE_X*rozdil2/rozdil1);
           showtabs.x_table[x][y].max_x=MIDDLE_X-points[x-1][0][y+1].x;
           }
        if (x!=0)showtabs.x_table[x][y].xpos2=VIEW_SIZE_X-(showtabs.x_table[x][y].xpos+showtabs.x_table[x][y].max_x);
                showtabs.x_table[x][y].point_total=rozdil1;
        calc_x_buffer(&showtabs.x_table[x][y].zoom_table,TXT_SIZE_X,rozdil1,VIEW_SIZE_X);
        }

     }

 for(x=0;x<CF_XMAP_SIZE;x++)
   for(y=0;y<F_YMAP_SIZE;y++)
      {
      int xl,xr,y1,yp,strd;

      strd=CF_XMAP_SIZE>>1;
      y1=(VIEW_SIZE_Y-y)-MIDDLE_Y;
      yp=1;while (points[0][0][yp].y>y1) yp++;
      if (x<strd)
        {
        xl=-points[strd-x][0][0].x;xr=-points[strd-x-1][0][0].x;
        }
      else if (x==strd)
        {
        xl=-points[0][0][0].x;xr=+points[0][0][0].x;
        }
      else if (x>strd)
        {
        xl=+points[x-strd-1][0][0].x;xr=+points[x-strd][0][0].x;
        }
      y1=(VIEW_SIZE_Y-y)-MIDDLE_Y;
      xl=xl*(y1+1)/points[0][0][0].y+MIDDLE_X;
      xr=xr*(y1+1)/points[0][0][0].y+MIDDLE_X;
      if (xl<0) xl=0;if (xr<0) xr=0;
      if (xl>639) xl=639;if (xr>639) xr=639;
      showtabs.f_table[x][y].lineofs=(y1+MIDDLE_Y)*1280+xl*2;
      showtabs.f_table[x][y].linesize=xr-xl;
      showtabs.f_table[x][y].counter=(y1-points[0][0][yp].y);
      }

 for(x=0;x<CF_XMAP_SIZE;x++)
   for(y=0;y<C_YMAP_SIZE;y++)
      {
      int xl,xr,y1,yp,strd;

      strd=CF_XMAP_SIZE>>1;
      y1=y-MIDDLE_Y;
      yp=1;while (points[0][1][yp].y<y1) yp++;
      if (x<strd)
        {
        xl=-points[strd-x][1][0].x;xr=-points[strd-x-1][1][0].x;
        }
      else if (x==strd)
        {
        xl=-points[0][1][0].x;xr=+points[0][1][0].x;
        }
      else if (x>strd)
        {
        xl=+points[x-strd-1][1][0].x;xr=+points[x-strd][1][0].x;
        }
      xl=xl*(y1-1)/points[0][1][0].y+MIDDLE_X;
      xr=xr*(y1-1)/points[0][1][0].y+MIDDLE_X;
      if (xl<0) xl=0;if (xr<0) xr=0;
      if (xl>639) xl=639;if (xr>639) xr=639;
      showtabs.c_table[x][y].lineofs=(y1+MIDDLE_Y)*1280+xl*2;
      showtabs.c_table[x][y].linesize=xr-xl;
      showtabs.c_table[x][y].counter=points[0][1][yp].y-y1;
      }


 }

void calc_zooming(char *buffer,int dvojice,int oldsiz)
  {
  int poz,roz,i,x;

  poz=-2;
  for(i=0;i<dvojice;i++)
     {
     x=(i*oldsiz/dvojice);
     roz=x-poz;
     if (roz>2) roz=2;
     if (roz<1) roz=1;
     if (roz==1) *buffer++=1; else *buffer++=0;
     poz+=roz;
     }
  }

void create_zooming(void)
  {
  int i,j;

  for (j=0;j<ZOOM_PHASES;j++)
     {
     calc_zooming(&zooming_xtable[j],320,zooming_points[j][0]);
     calc_y_buffer(&zooming_ytable[j],zooming_points[j][1],360,360);
     for(i=0;i<360;i++) zooming_ytable[j][i]*=1280;
     }

/*  calc_zooming(&zooming_xtable[0],320,570);
  calc_y_buffer(&zooming_ytable[0],350,400,400);
  for(i=0;i<400;i++) zooming_ytable[0][i]*=1280;
  calc_zooming(&zooming_xtable[1],320,500);
  calc_y_buffer(&zooming_ytable[1],320,400,400);
  for(i=0;i<400;i++) zooming_ytable[1][i]*=1280;
  calc_zooming(&zooming_xtable[2],320,450);
  calc_y_buffer(&zooming_ytable[2],280,400,400);
  for(i=0;i<400;i++) zooming_ytable[2][i]*=1280;
*/
  }

void zooming_forward(void)
  {
  int i;
  for (i=0;i<ZOOM_PHASES;i+=zooming_step)
     {
     zoom.xtable=(int32_t *)&zooming_xtable[i];
     zoom.ytable=(short *)&zooming_ytable[i];
     zoom.texture_line=0;
     zooming(screen+zooming_points[i][2]+zooming_points[i][3]*640+SCREEN_OFFSET,SCREEN_OFFSET,xlatmem,(360<<16)+320);
     }
  }
void zooming_backward(void)
  {
  int i;
  for (i=ZOOM_PHASES-1;i>=0;i-=zooming_step)
     {
     zoom.xtable=(int32_t *)&zooming_xtable[i];
     zoom.ytable=(short *)&zooming_ytable[i];
     zoom.texture_line=0;
     zooming(screen+zooming_points[i][2]+zooming_points[i][3]*640+SCREEN_OFFSET,SCREEN_OFFSET,xlatmem,(360<<16)+320);
     }
  }

/*  zoom.xtable=(int32_t *)&zooming_xtable[0];
  zoom.ytable=(short *)&zooming_ytable[0];
  zoom.texture_line=0;
  zooming(screen+35+25*640+SCREEN_OFFSET,lbuffer+SCREEN_OFFSET,xlatmem,(360<<16)+320);
  zoom.xtable=(int32_t *)&zooming_xtable[1];
  zoom.ytable=(short *)&zooming_ytable[1];
  zoom.texture_line=0;
  zooming(screen+70+40*640+SCREEN_OFFSET,lbuffer+SCREEN_OFFSET,xlatmem,(360<<16)+320);
  zoom.xtable=(int32_t *)&zooming_xtable[2];
  zoom.ytable=(short *)&zooming_ytable[2];
  zoom.texture_line=0;
  zooming(screen+95+60*640+SCREEN_OFFSET,lbuffer+SCREEN_OFFSET,xlatmem,(360<<16)+320);
*/


void turn_left()
  {
  word *kde1,c;
  int i;

  kde1=screen+SCREEN_OFFSET+70;
  c=640-140;
  for(i=0;i<rot_phases;i++)
     {
     kde1+=rot_step;
     c-=rot_step;
     turn(SCREEN_OFFSET,kde1,buffer_2nd+SCREEN_OFFSET+70,c);
      }
  }
void turn_right()
  {
  word *kde1,c;
  int i;

  kde1=screen+SCREEN_OFFSET+70+400;
  c=640-140-400;
  for(i=0;i<rot_phases;i++)
     {
     kde1-=rot_step;
     c+=rot_step;
     turn(SCREEN_OFFSET,kde1,buffer_2nd+SCREEN_OFFSET+70,c);
      }
  }


void show_cel_l(int celx,int cely,void *stena)
  {
  T_INFO_X_3D *x3d;
  T_INFO_Y *yd;

  if (nosides) return;
  x3d=&showtabs.z_table[celx][cely];
  yd=&showtabs.y_table[cely];
  if (x3d->used)
     {
  zoom.startptr=buffer_2nd+(yd->drawline+x3d->xpos);
  zoom.texture=(void *)((byte *)stena+256*2*5+2*2+2+x3d->txtoffset);
  zoom.texture_line=*(word *)stena;
  zoom.xtable=&x3d->zoom_table;
  zoom.ytable=&yd->zoom_table;
  zoom.palette=(word *)((byte *)stena+6+512*cely);
  zoom.ycount=yd->vert_size*(*((word *)stena+1))/TXT_SIZE_Y;
  if (zoom.ycount>yd->vert_total) zoom.ycount=yd->vert_total;
  zoom.line_len=1280;
  zoom.xmax=VIEW_SIZE_X;
  sikma_zleva();
     }
  }
void show_cel_r(int celx,int cely,void *stena)
  {
  T_INFO_X_3D *x3d;
  T_INFO_Y *yd;

  if (nosides) return;
  x3d=&showtabs.z_table[celx][cely];
  yd=&showtabs.y_table[cely];
  if (x3d->used)
     {
  zoom.startptr=buffer_2nd+(yd->drawline+639-x3d->xpos);
  zoom.texture=(void *)((byte *)stena+256*2*5+2*2+2+x3d->txtoffset);
  zoom.texture_line=*(word *)stena;
  zoom.xtable=&x3d->zoom_table;
  zoom.ytable=&yd->zoom_table;
  zoom.palette=(word *)((byte *)stena+6+512*cely);
  zoom.ycount=yd->vert_size*(*((word *)stena+1))/TXT_SIZE_Y;
  if (zoom.ycount>yd->vert_total) zoom.ycount=yd->vert_total;
  zoom.line_len=1280;
  zoom.xmax=VIEW_SIZE_X;
  sikma_zprava();
     }
  }

void show_cel2_l(int celx,int cely,void *stena)
  {
  T_INFO_X *x3d;
  T_INFO_Y *yd;

  if (nosides) return;
  x3d=&showtabs.x_table[celx][cely];
  yd=&showtabs.y_table[cely+1];
  if (x3d->used)
     {
  zoom.startptr=buffer_2nd+(yd->drawline+x3d->xpos);
  zoom.texture=(void *)((byte *)stena+256*2*5+2*2+2+x3d->txtoffset);
  zoom.texture_line=*(word *)stena;
  zoom.xtable=&x3d->zoom_table;
  zoom.ytable=&yd->zoom_table;
  zoom.palette=(word *)((byte *)stena+6+512*(/*cely*/+1));
  zoom.ycount=yd->vert_size*(*((word *)stena+1))/TXT_SIZE_Y;
  if (zoom.ycount>yd->vert_total) zoom.ycount=yd->vert_total;
  zoom.xmax=x3d->max_x;
  zoom.line_len=1280;
  sikma_zleva();
     }
  }
void show_cel2_r(int celx,int cely,void *stena)
  {
  T_INFO_X *x3d;
  T_INFO_Y *yd;

  if (nosides) return;
  x3d=&showtabs.x_table[celx][cely];
  yd=&showtabs.y_table[cely+1];
  if (x3d->used)
     {
  zoom.startptr=buffer_2nd+(yd->drawline+x3d->xpos2);
  zoom.texture=(void *)((byte *)stena+256*2*5+2*2+2);
  zoom.texture_line=*(word *)stena;
  zoom.xtable=&x3d->zoom_table;
  zoom.ytable=&yd->zoom_table;
  zoom.palette=(word *)((byte *)stena+6+512*(cely+1));
  zoom.ycount=yd->vert_size*(*((word *)stena+1))/TXT_SIZE_Y;
  if (zoom.ycount>yd->vert_total) zoom.ycount=yd->vert_total;
  zoom.line_len=1280;
  zoom.xmax=x3d->max_x;
  sikma_zleva();
     }
  }

void draw_floor_ceil(int celx,int cely,char f_c,void *txtr)
  {
  int y;

  if (nofloors) return;
  txtr=(void *)((word *)txtr+3);
  if (f_c==0) //podlaha
     {
     y=(VIEW_SIZE_Y-MIDDLE_Y)-points[0][0][cely].y+1;
     if (y<0) y=0;
     txtr=(void *)((word *)txtr-(VIEW_SIZE_Y-F_YMAP_SIZE)*640);
     fcdraw(txtr,buffer_2nd+SCREEN_OFFSET,&showtabs.f_table[celx+2][y]);
     if (debug)
        {
         memcpy(screen,buffer_2nd,512000);
         showview(0,0,0,0);
        }
     }
  else
     {
     y=points[0][1][cely].y+MIDDLE_Y+1;
     if (y<0) y=0;
     fcdraw(txtr,buffer_2nd+SCREEN_OFFSET,&showtabs.c_table[celx+2][y]);
     if (debug)
        {
         memcpy(screen,buffer_2nd,512000);
         showview(0,0,0,0);
        }
     }

  }

void build_map(void)
  {
  int i;
  memset(&mapa,0xff,sizeof(mapa));
  mapa[0][0]=1;
  mapa[0][2]=9;


  mapa[1][2]=0;
  mapa[1][0]=2;

  mapa[2][2]=1;
  mapa[2][1]=3;
  mapa[2][0]=5;

  mapa[3][3]=2;
  mapa[3][0]=4;
  mapa[3][2]=6;

  mapa[4][2]=3;
  mapa[4][3]=5;
//  mapa[4][0]=19;

  mapa[5][1]=4;
  mapa[5][2]=2;
  mapa[5][3]=11;

  mapa[6][0]=3;
  mapa[6][1]=7;

  mapa[7][3]=6;
  mapa[7][1]=8;

  mapa[8][3]=7;
  mapa[8][0]=9;

  mapa[9][2]=8;
  mapa[9][0]=0;

  mapa[11][1]=5;
  mapa[11][3]=12;

  mapa[12][1]=11;
  mapa[12][2]=13;

  mapa[13][0]=12;
  mapa[13][3]=14;
  mapa[13][2]=18;

  mapa[14][1]=13;
  mapa[14][3]=15;
  mapa[14][2]=17;

  mapa[15][1]=14;
  mapa[15][2]=16;
  mapa[15][0]=20;

  mapa[16][0]=15;
  mapa[16][1]=17;

  mapa[17][3]=16;
  mapa[17][0]=14;
  mapa[17][1]=18;

  mapa[18][3]=17;
  mapa[18][0]=13;
  mapa[18][2]=19;

  mapa[19][0]=18;
  mapa[19][2]=4;

  mapa[20][2]=15;mapa[20][0]=21;
  mapa[21][2]=20;mapa[21][0]=22;
  mapa[22][2]=21;
  for (i=22;i<40;i++)
     {
     if (i>22) mapa[i][3]=i-1;
     if (i<38) mapa[i][1]=i+1;
     if (i<34) mapa[i][0]=i+5;
     if (i>26) mapa[i][2]=i-5;
     }
  }


void build_left(integer y,integer sector)
  {
  if (renderstop[sector]) return;
  if (y>=VIEW3D_Z) return;
  renderstop[sector]=1;
  if (mapa[sector][dirs[1]]!=-1)
     build_left(y+1,mapa[sector][dirs[1]]);
  if (yreq!=y) return;
  draw_floor_ceil(-1,y,0,podlaha);
  draw_floor_ceil(-1,y,1,strop);
  if (mapa[sector][dirs[0]]==-1)
     show_cel_l(1,y,p);
  if (mapa[sector][dirs[1]]==-1)
     show_cel2_l(1,y,p2);
  }

void build_right(int y,int sector)
  {
  if (renderstop[sector]) return;
  if (y>=VIEW3D_Z) return;
  renderstop[sector]=1;
  if (mapa[sector][dirs[1]]!=-1)
    build_right(y+1,mapa[sector][dirs[1]]);
  if (yreq!=y) return;
  draw_floor_ceil(1,y,0,podlaha);
  draw_floor_ceil(1,y,1,strop);
  if (mapa[sector][dirs[2]]==-1)
     show_cel_r(1,y,p);
  if (mapa[sector][dirs[1]]==-1)
     show_cel2_r(1,y,p2);
  }

void swap_buffs(void)
  {
  word *p;

  p=screen;
  screen=buffer_2nd;
  buffer_2nd=p;
  }

void build_scene(int y,int sector)
  {
  if (renderstop[sector]) return;
  if (y>=VIEW3D_Z) return;
  renderstop[sector]=1;
  if (mapa[sector][dirs[0]]!=-1)
     build_left(y,mapa[sector][dirs[0]]);
  if (mapa[sector][dirs[2]]!=-1)
     build_right(y,mapa[sector][dirs[2]]);
  if (mapa[sector][dirs[1]]!=-1)
     build_scene(y+1,mapa[sector][dirs[1]]);
  if (yreq!=y) return;
  draw_floor_ceil(0,y,0,podlaha);
  draw_floor_ceil(0,y,1,strop);
  if (mapa[sector][dirs[0]]==-1)
     show_cel_l(0,y,p);
  if (mapa[sector][dirs[2]]==-1)
     show_cel_r(0,y,p);
  if (mapa[sector][dirs[1]]==-1)
     show_cel2_l(0,y,p2);
  }






void render_scene(int sector,int dir)
  {
  if (nofloors || nosides || drwsit) memcpy(buffer_2nd,(void *)((char *)sit+6),screen_buffer_size);
  else
  memset(buffer_2nd,0x0,screen_buffer_size);
  //memcpy(buffer_2nd+SCREEN_OFFSET,(void *)((char *)strop+6),640*79*2);
  //memcpy(buffer_2nd+SCREEN_OFFSET+640*184,(void *)((char *)podlaha+6),640*176*2);

  dirs[1]=dir;dirs[0]=(dir-1)&3;
  dirs[2]=(dir+1)&3;
  for (yreq=4;yreq>=0;yreq--)
  {
           memset(&renderstop,0,sizeof(renderstop));
           build_scene(0,sector);
  }
  }


void chozeni(void)
  {
  char c;  char dir=0;word sector=22;

  zooming_forward();
  swap_buffs();
  showview(0,0,0,0);
  do
     {
     while (_bios_keybrd(_KEYBRD_READY)) _bios_keybrd(_KEYBRD_READ);
     c=_bios_keybrd(_KEYBRD_READ) >> 8;
     switch (c)
        {
        case 'H':if (mapa[sector][dir]!=-1)
                       {
                       sector=mapa[sector][dir];
                       render_scene(sector,dir);
                       if (!debug) zooming_forward();
                       swap_buffs();
                       showview(0,0,0,0);
                       }break;
        case 'P':if (mapa[sector][(dir+2)&3]!=-1)
                    {
                    sector=mapa[sector][(dir+2)&3];
                    render_scene(sector,dir);
                    swap_buffs();
                    if (!debug) zooming_backward();
                    showview(0,0,0,0);
                    }break;
        case 'M':dir=(dir+1)&3;
                 render_scene(sector,dir);
                 if (!debug) turn_left();
                 swap_buffs();
                 showview(0,0,0,0);
                 break;
        case 'K':dir=(dir-1)&3;
                 render_scene(sector,dir);
                 swap_buffs();
                 if (!debug) turn_right();
                 showview(0,0,0,0);
                 break;
        case ';':debug=!debug;break;
        case '<':nosides=!nosides;break;
        case '=':nofloors=!nofloors;break;
        case '>':drwsit=!drwsit;break;
        }
     }
  while (c!=1);
  }

void ask_video(void)
  {
  char c,ok,er;
  printf("\nJaky videomode?:\n"
         "  1) 640x480x256 Pomale pocitace\n"
         "  2) 640x480xHiColor Pomale pocitace\n"
         "  3) 640x480x256 Rychle pocitace\n"
         "  4) 640x480xHiColor Rychle pocitace\n");
  screen_buffer_size=640*480*2;
    do
     {
     c=_bios_keybrd(_KEYBRD_READ)>>8;ok=1;er=0;
     line480=1;
     switch (c)
        {
        case 1:exit(0);
        case 4:line480=1;er=initmode256(load_file("xlat256.pal"));
               zooming=zooming2;ok=0;rot_phases=5;rot_step=70;
               turn=turn2;
               break;
        case 5:line480=1;er=initmode32();
               zooming=zooming1;ok=0;rot_phases=5;rot_step=70;
               turn=turn1;
               break;
        case 2:line480=1;er=initmode256(load_file("xlat256.pal"));
               zooming=zooming2;ok=0;zooming_step=2;
               turn=turn2;
               break;
        case 3:line480=1;er=initmode32();
               zooming=zooming1;ok=0;zooming_step=2;
               turn=turn1;
               break;
        }
     if (er)
        {
        ok=1;
        if (er==-1)
        printf("Rezim zrejme neni podporovan. Zkuste nainstalovat univbe\n");
        else
          printf("Graficka karta asi nepodporuje Linear Frame Buffer. \n"
          "Pokud tomu tak neni, zkontrolujte zda neni vypnuty.\n");
        }
     }
  while (ok);
  memset(screen,0,screen_buffer_size);
  buffer_2nd=(word *)getmem(screen_buffer_size);
  memset(buffer_2nd,0,screen_buffer_size);
  }

void main()
  {
  printf("%d\n",sizeof(showtabs));
  p=load_file("konvert\\bredy.hi");
  calc_points();
  create_tables();
  create_zooming();
  ask_video();
  put_picture(0,100,p);showview(0,0,0,0);free(p);
  p2=load_file("konvert\\stena2.hi");
  p=load_file("konvert\\stena2bl.hi");
  strop=load_file("konvert\\strop1.hi");
  podlaha=load_file("konvert\\podlaha1.hi");
  sit=load_file("konvert\\sit.hi");
  build_map();
  render_scene(22,0);showview(0,0,0,0);
  chozeni();
  closemode();
  }



