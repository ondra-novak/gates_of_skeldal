#include <libs/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <mem.h>
#include <libs/memman.h>
#include <libs/bgraph.h>
#include "flc.h"
#include "anipack.h"
#include <conio.h>
#include "cinema.h"

#define BOX_X 8
#define BOX_Y 8

typedef int T_BOX[BOX_X*BOX_Y];

typedef char PALETA[256][3];

PALETA cur_pal;
int box_end;
T_BOX cur_box;
T_BOX sort_box;
char box_data[BOX_X*BOX_Y],conv_data[BOX_X*BOX_Y];
char last_boxes[80][23][BOX_X*BOX_Y];
char preview=1;
char pack_repeat=0;

char *flc_name;

char vystup[120000];
char differs[120000];
char *ip;
char bit_mode;
char same;
FILE *anim;
char *anim_name;

word new_paleta[256],old_paleta[256];
word play_screen[240000];

int per_frame=60000;
int old_per_box;
char rastry=1;
int rastry_rozliseni=100;
int per_box;
int now_calc_size;
int hranice_velikosti[][2]={
                 {2,0},
                 {11,1},
                 {20,2},
                 {21,2},
                 {30,3},
                 {31,3},
                 {32,3},
                 {33,3},
                 {42,4},
                 {43,4},
                 {44,4},
                 {45,4},
                 {46,4},
                 {47,4},
                 {48,4},
                 {49,4},
                 };


char compress_table[]={0,0,1,2,2,3,3,3,3,4,4,4,4,4,4,4};
//char compress_table[]={0,0,1,2,2,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,};
//char compress_table[]={0,0,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,};
int hranice=0;


int get_distance(int col1,int col2)
  {
  return abs(cur_pal[col1][0]-cur_pal[col2][0])+
         abs(cur_pal[col1][1]-cur_pal[col2][1])+
         abs(cur_pal[col1][2]-cur_pal[col2][2]);
  }



void create_differs()
  {
  word *c;
  char *d,*e;
  int i;
  int j,k,x;

  j=0;k=0x7fffffff;
  for(i=1;i<256;i++)
     if ((x=get_distance(0,i))<=
     k)
     {
     k=x;j=i;
     }
  c=play_screen;
  d=frame_buffer;
  e=differs;
  for(i=0;i<120000;i++,c++,d++,e++)
     {
     if (!*d) *d=j;
     if (*c!=new_paleta[*d])
       *e=*d;
     else
        *e=0;
     }
  }

void box_read(int xbox,int ybox)
  {
  int x,y,k;
  char *a,*bd;

  same=1;
  box_end=0;
  bd=box_data;
  a=differs;
  memset(cur_box,0xff,sizeof(cur_box));
  xbox=BOX_X*xbox;
  ybox=BOX_Y*ybox;
  a+=640*ybox+xbox;
  for(y=ybox;y<ybox+BOX_Y;y++,a+=640-BOX_X)
     for(x=xbox;x<xbox+BOX_X;x++,a++)
       {
       for(k=0;k<box_end && cur_box[k]!=*a;k++);
       if (k==box_end) cur_box[box_end++]=*a;
       *bd++=*a;
       }
  }

void conv_pal_hicolor()
  {
  int i,r,g,b;
  memcpy(old_paleta,new_paleta,sizeof(new_paleta));
  for(i=0;i<256;i++)
     {
     r=(flc_paleta[i][0]>>3);
     g=(flc_paleta[i][1]>>3);
     b=(flc_paleta[i][2]>>3);
     new_paleta[i]=(r<<10)+(g<<5)+b;
     }
  memcpy(cur_pal,flc_paleta,sizeof(flc_paleta));
  new_paleta[0]=0x8000;
  }

/*
void set_vga_palette()
  {
  int x;
  char *c;

  c=cur_pal;
  outp (0x3c8
     , 0);
  for (x=0; x<768; x++,c++)
     {
     while (inp(0x3da) & 1);
     while (!(inp(0x3da) & 1));
     outp (0x3c9,*c);
     }
  }


void get_vga_palette()
  {
  int x;
  char *c;

  c=cur_pal;
  outp (0x3c7, 0);
  for (x=0; x<768; x++,c++)
     {
     while (inp(0x3da) & 1);
     while (!(inp(0x3da) & 1));
     *c=inp (0x3c9);
     }
  cur_pal[0][0]=0;
  cur_pal[0][1]=0;
  cur_pal[0][2]=0;}
 */




/*void box_analize()
  {
  int i,c;
  int r,g,b;

  for(i=0;i<box_end;i++)
     {
     c=cur_box[i].color;
     r=cur_pal[c][0];
     g=cur_pal[c][1];
     b=cur_pal[c][2];
     cur_box[i].distance=r+g+b;
     }
  }
 */



void box_sort()
  {
  int c;
  int i,max,j,ds,d,a,z;

  d=0;
  a=0;z=2;
  c=cur_box[a];
  sort_box[0]=c;
  do
  {
  max=0;j=-1;
  for(i=0;i<box_end;i++)
     {
  if (cur_box[i]==0)
     {
     j=i;
     break;
     }
  if (cur_box[i]>=0 && (ds=get_distance(c,cur_box[i]))>max)
     {
     j=i;max=ds;
     }
     }
  if (j>=0)
     {
     c=cur_box[j];
     sort_box[d++]=c;
     cur_box[j]=-1;
     a=j;
     }
  if (max>hranice)
     {
     z=d;
     }
  }
  while (j!=-1);
  if (box_end>1) box_end=z;
  }

int zvol_metodu()
  {
  int i=0;

  for(;hranice_velikosti[i][0]<per_box;i++);
  return i;
  }


void store_palette()
  {
  int bits,palsize,i;

  bits=compress_table[box_end];
  palsize=1<<bits;
  box_end=palsize;
  bit_mode=bits;
  if (box_end<palsize && box_end) palsize=box_end;
  if (palsize==1 && sort_box[0]==0)
     {
     ip-=2;
     if (pack_repeat && ip[1]<255 && ip[0]<2)
        {
        ip[0]=0;
        ip[1]++;
        ip+=2;
        }
     else
        {
        ip+=2;
        *ip++=0;
        *ip++=0;
        pack_repeat=1;
        }
     return;
     }
  pack_repeat=0;
  *ip++=palsize;
  for(i=0;i<palsize;i++) *ip++=sort_box[i];
  }
/*
void box_test_same(int xbox,int ybox)
  {
  int x,y;
  char *c,*d;

  same=0;
  c=play_screen;
  d=conv_data;
  xbox=BOX_X*xbox;
  ybox=BOX_Y*ybox;
  c+=640*ybox+xbox;
  for(y=ybox;y<ybox+BOX_Y;y++,c+=640-BOX_X)
     for(x=xbox;x<xbox+BOX_X;x++,c++)
         if (*c!=*d++) return;
  same=1;
  }

*/
void conv_box(int x,int y)
  {
  int i,j,max,indx,indx2,ds;
  char c;
  int tab[256];
  int subst[256];
  int dst[256];

  x,y,dst;
  memset(tab,0xff,sizeof(tab));
  memset(subst,0xff,sizeof(subst));
  tab[0]=0;
  subst[0]=0;
  for(i=0;i<BOX_X*BOX_Y;i++)
     {
     c=box_data[i];
     if (tab[c]==-1)
        {
        max=0x7fffffff;indx=0;indx2=-1;
        for(j=(sort_box[0]==0);j<box_end;j++)
           {
           if (c==sort_box[j] && c)
              {
              indx=j;
              indx2=j;
              max=0;
              break;
              }
           if (sort_box[j] && (ds=get_distance(c,sort_box[j]))<max)
                                  {
                                  if (indx) indx2=indx;
                                  max=ds;
                                  indx=j;
                                  }
           }
        if (indx2==-1) indx2=indx;
        tab[c]=indx;
        subst[c]=indx2;
        dst[c]=max;
        }
     else
        {
        indx=tab[c];
        indx2=subst[c];
        }
     if (sort_box[0]==0 && indx2==0 && indx!=0)
         exit(0);
     else
     if (sort_box[0]==0 && indx2!=0 && indx==0)
        exit(0);
     if (rastry && indx!=indx2 && get_distance(sort_box[indx],sort_box[indx2])<rastry_rozliseni && (((i>>1) & 1) ^ ((i>>3) & 1)) && indx2!=-1) indx=subst[c];
     if (sort_box[0]==0 && c && last_boxes[x][y][i]!=0 && get_distance(c,last_boxes[x][y][i])<dst[c])
        {
        indx=0;
        }
     else
      if (box_data[i]) last_boxes[x][y][i]=sort_box[indx];
     box_data[i]=indx;
     conv_data[i]=sort_box[indx];
     }
  }

void save_box(int x,int y)
  {
  char bits;
  if (box_end>=sizeof(compress_table))
     {
     int i;

     *ip++=64;
     memcpy(ip,box_data,sizeof(box_data));
     memcpy(conv_data,box_data,sizeof(box_data));
     for(i=0;i<sizeof(box_data);i++)
        if (box_data[i])last_boxes[x][y][i]=box_data[i];
     ip+=sizeof(box_data);
     pack_repeat=0;
     }
  else
     {
     store_palette();
     conv_box(x,y);
     bits=bit_mode;
     switch (bits)
        {
        case 1:ip=save_1bit(&box_data,ip);break;
        case 2:ip=save_2bit(&box_data,ip);break;
        case 3:ip=save_3bit(&box_data,ip);break;
        case 4:ip=save_4bit(&box_data,ip);break;
        }
     }
  }
void conv_complette(int x,int y)
  {
  int i;
  box_read(x,y);
  box_sort();
  if (box_end>(i=zvol_metodu())) box_end=i;
  save_box(x,y);
  }

void conv_pict()
  {
  int i,j,count=0;

  pack_repeat=0;
  now_calc_size=0;
  conv_pal_hicolor();
  create_differs();
  ip=vystup;
  old_per_box=-1;
  for(i=0;i<23;i++)
     for(j=0;j<80;j++)
       {
       per_box=(per_frame-(ip-vystup))/(80*23-count);
       if (old_per_box==-1) old_per_box=per_box;else if (per_box>old_per_box)per_box+=(per_box-old_per_box);
       count++;
       conv_complette(j,i);
       }
  }


/*
void decompr_box(word *adr)
  {
  char mode;
  word palxlat[16];
  int i;

  mode=*ip++;
  if (mode==255) return;
  if (mode<64)
     {
     for(i=0;i<mode;i++) palxlat[i]=new_paleta[*ip++];
     mode=hranice_velikosti[mode-1][1];
     }
  else mode=8;
  switch (mode)
     {
     case 0:load_0bit(adr,palxlat);break;
     case 1:ip=load_1bit(ip,adr,palxlat);break;
     case 2:ip=load_2bit(ip,adr,palxlat);break;
     case 3:ip=load_3bit(ip,adr,palxlat);break;
     case 4:ip=load_4bit(ip,adr,palxlat);break;
     case 8:ip=load_8bit(ip,adr,new_paleta);break;
     }
  }


void decompr_pict()
  {
  int x;
  int y;

  ip=vystup;
  for (y=0;y<115200;y+=640*8)
     for (x=0;x<640;x+=8)
       decompr_box(play_screen+x+y);
  }
*/
void adjust_pict()
  {
  int i,j,k,l;

  for(i=0;i<24;i++)
     for(j=0;j<80;j++)
       for(k=0;k<8;k++)
         for(l=0;l<8;l++)
           play_screen[(i*8+k)*640+(j*8+l)]=new_paleta[last_boxes[j][i][k*8+l]];
  }


void show_play_screen()
  {
  int i,j;
  word *c,*d;

  c=lbuffer;j=0;
  c+=(60)*640;
  d=play_screen;
  for(i=0;i<180;i++)
     {
     memcpy(c,d,1280);
     c+=640*2;
     d+=640;
     }
  d=play_screen;
  c=lbuffer+61*640;
delay(30);
  for(i=0;i<180;i++)
   {
     for(j=0;j<320;j++)
     {
     *(unsigned long *)c=((*(unsigned long *)d & 0x7BDE7BDE)+(*(unsigned long *)(d+640) & 0x7BDE7BDE))>>1;
     c+=2;
     d+=2;
     }
   c+=640;
   }

  }

void save_palette()
  {
  int i,il,ip,ps;
  for(i=0;i<256 && old_paleta[i]==new_paleta[i];i++);
  il=i;
  if (il==256) ip=il;
  else
     {
     for(i=255;i>0 && old_paleta[i]==new_paleta[i];i--);
     ip=i+1;
     }
  ps=ip-il;
  if (ps)
     {
     i=TRACK_PALETTE;
     fwrite(&i,1,1,anim);
     fwrite(&il,1,2,anim);
     fwrite(&ps,1,2,anim);
     fwrite(&new_paleta[il],sizeof(word),ps,anim);
     }
  }

void save_frame(int size)
  {
  int i;

  i=TRACK_VIDEO;
  fwrite(&i,1,1,anim);
  fwrite(&size,1,sizeof(size),anim);
  fwrite(vystup,1,size,anim);
  }

void save_mark(int i)
  {
  fwrite(&i,1,1,anim);
  }

void int10_3();
#pragma aux int10_3=\
  "mov  eax,3"\
  "int  10h"\
 modify [eax];

void int10_13();
#pragma aux int10_13=\
  "mov  eax,13h"\
  "int  10h"\
 modify [eax];

void pack_anim()
  {
  int x;int size;

  Open_FLC(flc_name);
  Get_first_frame();
  Decompress_frame();
  conv_pict();
  adjust_pict();
  if (preview) show_play_screen();
  size=ip-vystup;printf("Frame 1: %d \n",size);
  save_palette();
  save_frame(size);
  for (x=2; x<=h_flc.frames+10; x++)
	{
     if (x<=h_flc.frames)
        {
        Get_next_frame ();
        Decompress_frame ();
        if (preview) show_play_screen();
        }
     conv_pict();
     adjust_pict();
     size=ip-vystup;printf("Frame %d: %d \n",x,size);
     save_palette();
     save_frame(size);
     save_mark(TRACK_DELAY);
  }
  save_mark(TRACK_END);
  }
  /*
  decompr_pict();
  show_play_screen();
  getch();
     decompr_pict();
     show_play_screen();
  */

void load_frame()
  {
  int size;
  fread(&size,1,sizeof(size),anim);
  fread(vystup,1,size,anim);
  }

void load_palette()
  {
  word i;
  word size;
  fread(&i,1,2,anim);
  fread(&size,1,2,anim);
  fread(&new_paleta[i],2,size,anim);
  }
/*
char play_track(FILE *anim)
  {
  char i;

  fread(&i,1,1,anim);
  switch (i)
     {
     case TRACK_VIDEO:
           load_frame();
           decompr_pict();
           break;
     case TRACK_PALETTE:
           load_palette();
           break;
     case TRACK_MUSIC:break;
     case TRACK_END:return 0;
     case TRACK_DELAY:
           show_play_screen();
  //         delay(7);
           break;
     }
  return 1;
  }


void play_anim(FILE *anim)
  {
  while(play_track(anim));
  }
*/
main(int argc,char *argv[])
  {
  if (argc<3)
     {
     puts("");
     puts("Poziti: cnmbuild filename.flc filename.cnm [perframe] [rastry]");
     puts("");
     puts("perframe - omezeni velikosti na jeden frame");
     puts("rastry - cislo udavajici hranici pro rastrovani (0 - bez rastru)");
     return 1;
     }
  memset(play_screen,0xff,sizeof(play_screen));
  memset(frame_buffer,0xff,sizeof(frame_buffer));
  memset(new_paleta,0xff,sizeof(new_paleta));
  memset(last_boxes,0,sizeof(last_boxes));

  flc_name=argv[1];
  anim_name=argv[2];
  if (argc>=4) sscanf(argv[3],"%d",&per_frame);
  if (argc>=5) sscanf(argv[4],"%d",&rastry_rozliseni);
  if (initmode32(NULL)) preview=0;
  if (banking)
     {
     closemode();preview=0;
     }
  anim=fopen(anim_name,"wb");
  pack_anim();
  fclose(anim);
  closemode();
  return 0;
  }
