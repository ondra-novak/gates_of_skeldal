#include <skeldal_win.h>
#include <libs/types.h>
#include <mem.h>
#include <malloc.h>
#include <stdio.h>
#include <libs/bgraph.h>
#include <libs/event.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/gui.h>
#include <libs/strlite.h>
#include <strlists.h>
#include "mapy.h"
#include <libs/basicobj.h>
#include "steny.h"
#include "globals.h"
#include "edit_map.h"
#include "wiz_tool.h"
#include "mob_edit.h"

TMAP mapa;
TMAP_EDIT minfo;
TVYKLENEK vyklenky[256];
long map_win=-1;
long tool_bar=-1;
char tool_sel=30;
long xmap_offs=0,ymap_offs=0,cur_layer=0;
int maplen=1;
int m_zoom=3;
static char nocenter=0;

#define ZELENA RGB555(0,31,0)
#define CERVENA RGB555(31,0,0)
#define MODRA RGB555(0,0,31)

char _actions []=
  "<��dn�>\0"
  "Otev�i dve�e\0"
  "Zav�i dve�e\0"
  "Otev�i nebo zav�i\0"
  "Spus� animaci prim\0"
  "Uka� prim. st�nu\0"
  "Schovej prim.st�nu\0"
  "Uk/schov.pri.st�nu\0"
  "Spus� animaci sek.\0"
  "Uka� sek. st�nu\0"
  "Schovej sek. st�nu\0"
  "Uk/schov.sek.st�nu\0"
  "Schovej prim.&sek.\0"
  "Zobraz text\0"
  "Kod.z�mek (star�-Nepou��vat!)\0"
  "Otev�i teleport\0"
  "Uzav�i teleport\0"
  "Kodov� z�mek (log)\0"
  "Konec hry\0"
  "\\\0";

char chka[]={0,1,1,1,1,1,1,1,1,1,1,1,0};
char _steny2 []="Severn�\0V�chodn�\0Ji�n�\0Z�padn�\0\\\0";
char _zivly []="Ohe�\0Voda\0Zem�\0Vzduch\0Mysl\0\\\0";

char _sector_types[]=
  "!Voln�!\0""Normaln�\0""Schody\0""Lo�ka na vod�\0""L�va\0"
  "Sm�r sever\0""Sm�r v�chod\0""Sm�r jih\0""Sm�r z�pad\0""Voda\0""Sloup\0""Dira\0""Teleport\0"
  "Tla��tko(norm)\0""Tla��tko(zma�k)\0"
  "Fl�tna (Sever)\0""Fl�tna (V�chod)\0""Fl�tna (Jih)\0""Fl�tna (Z�pad)\0"
  "Opu�t�n� mapy\0""V�r\0""Sloup&Smrt\0""Ub�ra �ivoty\0"
  "\\\0";

char _type_multi_actions[]=
  "Nop\0""Sound effekt\0""Text global\0""Text local\0""Send action\0""Fireball\0""Destroy Item\0"
  "Load Level\0""Drop Item\0""Start dialog\0""Start Shop\0""Code Lock\0""Cancel Action\0"
  "Lock\0""Swap Sectors\0""HIT Player\0""If Jump\0""Call SpecProc\0"
  "If have item\0""Story (local)\0""If test action\0""Experience\0""Teleport_group\0""Play Anim\0"
  "Create Item\0""If flag Jump\0""Change Flag\0""Drop Unique Item\0"
  "Drop Money\0""Give Unique Item\0"
  "If Item Holded\0""Write Book\0"
  "Random Jump\0""The End\0""Send Monter To\0""Share Multiaction\0""Change Music\0""Global Event\0"
  "If Sector Num\0""If Sector Type\0"
  "\\\0";

char _typy_zbrani[]=
  "Me� (t��k� ostr� rovn�)\0"
  "Sekera (t��k� ostr� ost.)\0"
  "Kladivo (tup�)\0"
  "H�l\0"
  "D�ka (Lehk� ostr�)\0"
  "��p (st�eln�)\0"
  "Ostatn� / Spec\0"
  "\\\0";

char _typy_veci[]=
  "Nespecifikov�no\0"
  "Zbra� tv��� v tv��\0"
  "Vrhac� zbra�\0"
  "St�eln� zbra�\0"
  "Zbroj\0"
  "Svitek / H�lka\0"
  "Lektvar\0"
  "Voda\0"
  "J�dlo\0"
  "Speci�ln�\0"
  "Runa\0"
  "Pen�ze\0"
  "Svitek s textem\0"
  "Prach\0"
  "Ostatn�\0"
  "\\\0";

char _umisteni_veci[]=
  "Nikam\0"
  "Zavazadlo\0"
  "Na t�lo (naho�e)\0"
  "Na t�lo (dole)\0"
  "Na hlavu\0"
  "Na nohy\0"
  "Kutna (na t�lo/naho�e/dole/helma)\0"
  "Na krk\0"
  "Do ruky\0"
  "Obouru�\0"
  "Prsten\0"
  "��p\0"
  "\\0";

char _side_flgs[]=
  "Automap\0"
  "Nepr�choz�(hr�c)\0"
  "Nepr�choz�(nestv�ra)\0"
  "Nepr�choz�(v�c)\0"
  "Nepr�choz�(zvuk)\0"
  "Poplach\0"
  "Pr�choz� akce\0"
  "Pr�chledn� st�na\0"
  "Prim:Animace\0"
  "Prim:Viditeln�\0"
  "Prim:Tam a zp�t\0"
  "Prim:Sm�r\0"
  "Sek:Animace\0"
  "Sek:Viditeln�\0"
  "Sek:Tam a zp�t\0"
  "Sek:Sm�r\0"
  "Lev� oblouk\0"
  "Prav� oblouk\0"
  "Dva druhy st�n\0"
  "Spec. mapovat\0"
  "Kop�ruj p��choz� ud�lost\0"
  "Po�li jinou ud�lost\0"
  "Aplikuj i z druh� strany\0"
  "Autoanimace p�ep�na��\0"
  "Zm�na automapingu\0"
  "Zm�na pr�chodnosti(hr��)\0"
  "Zm�na pr�chodnosti(nestv�ra)\0"
  "Zm�na pr�chodnosti(v�c)\0"
  "Zm�na pr�chodnosti(zvuk)\0"
  "Tajn� st�na\0"
  "TRUESEE (iluze)\0"
  "Neviditeln� na map�\0"
  "Potvora je ve h�e\0"
  "Potvora je v oblasti\0"
  "\\\0";

char **side_flgs;
char **actions;
char **steny2;
char **zivly;
char **sector_types;
char **act_types;
char **typy_zbrani;
char **typy_veci;
char **umisteni_veci;
char **mob_procs=NULL;
char **wall_procs=NULL;

char side_chscr[]=
  {8,8,4,1,2,1,4,1,1,1,1,16,8,8,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
   4,4,4,4,8,8,0};

word fgc[32]=
  {0,RGB555(0,27,31),RGB555(4,27,16),RGB555(0,15,31),RGB555(16,16,16),RGB555(0,31,31),RGB555(4,23,15),RGB555(0,31,31),
  RGB555(20,0,0),RGB555(20,31,31),RGB555(24,15,15),RGB555(20,15,18),RGB555(28,0,0),RGB555(28,31,31),RGB555(28,23,0),0
  ,WINCOLOR,RGB555(0,27,31),RGB555(4,27,16),RGB555(0,15,31),RGB555(16,16,16),RGB555(0,31,31),RGB555(4,23,15),RGB555(0,31,31),
  RGB555(20,0,0),RGB555(20,31,31),RGB555(24,15,15),RGB555(20,15,18),RGB555(28,0,0),RGB555(28,31,31),RGB555(28,23,0),0};

void init_maps(void)
  {
  memset(&mapa,0,sizeof(mapa));
  }

int find_free_sector(void)
  {
  int i=1;

  while (i<maplen+1 && mapa.sectordef[i].sector_type) i++;
  if (mapa.sectordef[i].sector_type) return -1;
  if (i>=MAPSIZE) return -1;
  return i;
  }

int find_sector(int x,int y,int layer)
  {
  int i=0;
  char p;

  do
     {
     i++;
     while (i<maplen && (!(p=mapa.sectordef[i].sector_type) ||
      minfo[i].x!=x || minfo[i].y!=y))
      i++;
     if (i==maplen) return -1;
     }
  while (minfo[i].layer!=layer);
  return i;
  }

void wire_2_sectors(int sector1,int sector2,int smer)
  {
  TSTENA *st;

  st=&mapa.sidedef[sector1][smer];
  st->flags|=0x80;
  st->flags&=~0x021e;
  mapa.sectordef[sector1].step_next[smer]=sector2;
  smer=(smer+2)& 3;
  st=&mapa.sidedef[sector2][smer];
  st->flags|=0x80;
  st->flags&=~0x021e;
  mapa.sectordef[sector2].step_next[smer]=sector1;
  }


void wire_sector(int sector)
  {
  int x,y,l,s;

  x=minfo[sector].x;
  y=minfo[sector].y;
  l=minfo[sector].layer;
  s=find_sector(x,y-1,l);
  if (s+1) wire_2_sectors(sector,s,0);
  s=find_sector(x+1,y,l);
  if (s+1) wire_2_sectors(sector,s,1);
  s=find_sector(x,y+1,l);
  if (s+1) wire_2_sectors(sector,s,2);
  s=find_sector(x-1,y,l);
  if (s+1) wire_2_sectors(sector,s,3);
  }

int add_sector(int x,int y,int layer,int source_num)
  {
  int w;

  w=find_sector(x,y,layer);
  if (!(w+1))
     {
     TSTENA *st,*sc;
     int i;

     w=find_free_sector();
     if (w>=maplen) maplen=w+1;
     if (w==-1) return -1;
     for (i=0;i<4;i++)
        {
        st=&mapa.sidedef[w][i];
        sc=&mapa.sidedef[source_num][i];
        memcpy(st,sc,sizeof(TSTENA));
        st->flags&=~0xfe;
        st->flags|=0x021e;
        }
     memcpy(&mapa.sectordef[w],&mapa.sectordef[source_num],sizeof(TSECTOR));
     memset(&mapa.sectordef[w].step_next,0,sizeof(mapa.sectordef[w].step_next));
     if (!mapa.sectordef[w].sector_type) mapa.sectordef[w].sector_type=1;
     minfo[w].x=x;
     minfo[w].y=y;
     minfo[w].layer=layer;
     minfo[w].flags=0;
     }
  wire_sector(w);
  return w;
  }

void unwire_2_sectors(int sector1,int sector2,int smer)
  {
  TSTENA *st;

  st=&mapa.sidedef[sector1][smer];
  mapa.sectordef[sector1].step_next[smer]=0;
  st->flags&=~0xfe;
  st->flags|=0x021e;
  smer=(smer+2)& 3;
  st=&mapa.sidedef[sector2][smer];
  st->flags&=~0xfe;
  st->flags|=0x021e;
  mapa.sectordef[sector2].step_next[smer]=0;
  }

void flags_2_sectors(int sector1,int sector2,int smer,long flags)
  {
  TSTENA *st;

  st=&mapa.sidedef[sector1][smer];
  st->flags|=flags;
  smer=(smer+2)& 3;
  st=&mapa.sidedef[sector2][smer];
  st->flags|=flags;
  }


void tenka_stena(int x,int y,int layer,int smer,int flags)
  {
  int s1,s2;

  s1=find_sector(x,y,layer);
  if (s1+1)
     {
     if (mapa.sectordef[s1].step_next[smer])
        {
        s2=mapa.sectordef[s1].step_next[smer];
        if (s2>=0 && s2<MAPSIZE)
           if (!flags) unwire_2_sectors(s1,s2,smer);
          else flags_2_sectors(s1,s2,smer,flags);
        }
     }
  }

void delete_sector(int x,int y, int l)
  {
  int s1,i,j;

  s1=find_sector(x,y,l);
  if (s1+1)
     {
     mapa.sectordef[s1].sector_type=0;
     remove_items_from_sector(s1);
     remove_mobs_from_sector(s1);
     for(i=0;i<MAPSIZE;i++)
       if (mapa.sectordef[i].sector_type)
        for(j=0;j<4;j++)
         if (mapa.sectordef[i].step_next[j]==s1)
           {
           mapa.sidedef[i][j].flags&=~0xfe;
           mapa.sidedef[i][j].flags|=0x221e;
           mapa.sectordef[i].step_next[j]=0;
           }
     }
  while (maplen-1 && !mapa.sectordef[maplen-1].sector_type) maplen--;
  }

void sipka_nahoru(int x,int y)
  {
  line(x+(M_ZOOM>>1),y,x,y+(M_ZOOM>>1));
  line(x+(M_ZOOM>>1),y,x+M_ZOOM,y+(M_ZOOM>>1));
  line(x+(M_ZOOM>>1),y,x+(M_ZOOM>>1),y+M_ZOOM);
  }

void sipka_dolu(int x,int y)
  {
line(x+(M_ZOOM>>1),y+M_ZOOM,x,y+(M_ZOOM>>1));
line(x+(M_ZOOM>>1),y+M_ZOOM,x+M_ZOOM,y+(M_ZOOM>>1));
line(x+(M_ZOOM>>1),y,x+(M_ZOOM>>1),y+M_ZOOM);
  }

void sipka(int x,int y,int q,int color)
  {
  char c[2]=" \0";

  if (m_zoom<3) return;
  c[0]=q;
  charcolors[0]=RGB555(16,9,0);
  charcolors[1]=color;
  curfont=icones;
  if (m_zoom>3) fontdsize=1;
  set_aligned_position(x+(M_ZOOM>>1),y+(M_ZOOM>>1),1,1,c);
  outtext(c);
  fontdsize=0;
  }

void prepinac(int x,int y,int q,TSTENA *p,char tma)
  {
  curcolor=0;
  if (p->sector_tag) curcolor=RGB555(31,16,0);
  if (tma) curcolor|=RGB555(0,0,31);
  if (p->oblouk & 0x10) curcolor|=RGB555(0,31,0);
  if (curcolor)
     {
     if (curcolor==RGB555(31,31,31)) curcolor=0;
     switch (q)
       {
       case 0: bar(x+(M_ZOOM>>1)-1,y,x+(M_ZOOM>>1)+1,y+2);break;
       case 2: bar(x+(M_ZOOM>>1)-1,y+M_ZOOM,x+(M_ZOOM>>1)+1,y+M_ZOOM-2);break;
       case 1: bar(x+M_ZOOM,y+(M_ZOOM>>1)-1,x+M_ZOOM-2,y+(M_ZOOM>>1)+1);break;
       case 3: bar(x,y+(M_ZOOM>>1)-1,x+2,y+(M_ZOOM>>1)+1);break;
       }
     }
  }

void norm_rectangle(int x1,int y1,int x2,int y2)
  {
  ver_line(x1,y1,y2);
  ver_line(x2,y1,y2);
  hor_line(x1,y1,x2);
  hor_line(x1,y2,x2);
}

void autocenter_map()
  {
  int i;
  int xmax;
  int xmin;
  int ymax;
  int ymin;
  char sett=0;
  char chnged=0;

  if (nocenter)
     {
     nocenter=0;
     return;
     }
  for(i=1;i<maplen;i++)
     if (mapa.sectordef[i].sector_type && minfo[i].layer==cur_layer)
        {
        int x=minfo[i].x,y=minfo[i].y;
        chnged=1;
        if (sett)
           {
           if (x<xmin) xmin=x;
           if (x>xmax) xmax=x;
           if (y<ymin) ymin=y;
           if (y>ymax) ymax=y;
           }
        else
           {
           xmin=xmax=x;ymin=ymax=y;sett=1;
           }
        }
  if (chnged)
     {
     xmap_offs=(xmax+xmin)>>1;
     ymap_offs=(ymax+ymin)>>1;
     c_set_value(map_win,20,xmap_offs);
     c_set_value(map_win,30,ymap_offs);
     }
  }

void workspace_draw(int x1,int y1,int x2, int y2,OBJREC *o)
  {
  int x,y,i,j,xc,yc;
  int mapx,mapy;
  int mmz=M_ZOOM;
  int mmz1=mmz/4;
  int mmz3=mmz*3/4;
  int yoff,xoff;
  char q;

  mmz=M_ZOOM;
  xc=(o->xs % M_ZOOM)>>1;
  yc=(o->ys % M_ZOOM)>>1;
  *(long *)o->data=1;
  bar(x1,y1,x2,y2);
  if (m_zoom>1)
  for (y=y1+yc;y<y2;y+=M_ZOOM)
     for (x=x1+xc;x<x2;x+=M_ZOOM)
       point(x,y,0x0);
  mapx=(x2-x1)>>m_zoom;
  mapy=(y2-y1)>>m_zoom;
  xoff=xmap_offs-mapx/2;
  yoff=ymap_offs-mapy/2;
  for (j=0;j<2;j++)
  for (i=1;i<maplen;i++)
     if ((q=mapa.sectordef[i].sector_type)!=0)
      if (minfo[i].layer==cur_layer)
     {
     int lay;
     x=minfo[i].x-xoff;
     y=minfo[i].y-yoff;
     lay=minfo[i].layer;
     if (x>=0 && x<mapx && y>=0 && y<mapy)
        {
        x=(x<<m_zoom)+o->locx+xc;
        y=(y<<m_zoom)+o->locy+yc;

        switch (q)
           {
           case 1:curcolor=RGB555(31,31,31);break;
           case 4:curcolor=RGB555(31,15,0);break;
           case 3:
           case 20:
           case 9:curcolor=RGB555(0,15,31);break;
           case 22:curcolor=RGB555(0,31,0);break;
           default: curcolor=RGB555(31,31,31);
           }

        if (minfo[i].flags & 1) curcolor=RGB555(31,31,0);
        if (!j) bar(x,y,x+M_ZOOM-1,y+M_ZOOM-1);
        else
           {
           TSTENA *p;
           long flg;
           char sd;

          //if (tool_sel==50)
           {
           int p;
           p=i*4;
           ikris(p,x,y,x+(M_ZOOM>>1),y+(M_ZOOM>>1));
           ikris(p+1,x+(M_ZOOM>>1),y,x+M_ZOOM,y+(M_ZOOM>>1));
           ikris(p+2,x+(M_ZOOM>>1),y+(M_ZOOM>>1),x+M_ZOOM,y+M_ZOOM);
           ikris(p+3,x,y+(M_ZOOM>>1),x+(M_ZOOM>>1),y+M_ZOOM);
           }
           p=&(mapa.sidedef[i][0]);
           curcolor=RGB555(31,31,31);
           flg=p->flags;
           sd=p->side_tag;
           if (flg & 0x21e)
              {curcolor=fgc[((flg>>1)&0xf) | ((flg>>5)&0x10)];hor_line(x,y,x+mmz);}
           if (flg & 0x10000)  {curcolor=0x1f;hor_line(x,y,x+mmz1);}
           if (flg & 0x20000)  {curcolor=0x1f;hor_line(x+mmz3,y,x+mmz);}
           if (sd & 0x80)      {curcolor=RGB555(19,0,0);hor_line(x,y+mmz-1,x+mmz);}
           p++;flg=p->flags;
           sd=p->side_tag;
          if (flg & 0x21e)
              {curcolor=fgc[((flg>>1)&0xf) | ((flg>>5)&0x10)];ver_line(x+M_ZOOM,y,y+M_ZOOM);}
          if (flg & 0x10000)  {curcolor=0x1f;ver_line(x+mmz,y,y+mmz1);}
          if (flg & 0x20000)  {curcolor=0x1f;ver_line(x+mmz,y+mmz3,y+mmz);}
          if (sd & 0x80)      {curcolor=RGB555(19,0,0);ver_line(x+1,y,y+mmz);}

           p++;flg=p->flags;
           sd=p->side_tag;
          if (flg & 0x21e)
              {curcolor=fgc[((flg>>1)&0xf) | ((flg>>5)&0x10)];hor_line(x,y+M_ZOOM,x+M_ZOOM);}
          if (flg & 0x10000)  {curcolor=0x1f;hor_line(x+mmz3,y+mmz,x+mmz);}
          if (flg & 0x20000)  {curcolor=0x1f;hor_line(x,y+mmz,x+mmz1);}
          if (sd & 0x80)      {curcolor=RGB555(19,0,0);hor_line(x,y+1,x+mmz);}
          p++;flg=p->flags;
           sd=p->side_tag;
          if (flg & 0x21e)
              {curcolor=fgc[((flg>>1)&0xf) | ((flg>>3)&0x10)];ver_line(x,y,y+M_ZOOM);}
          if (flg & 0x10000)  {curcolor=0x1f;ver_line(x,y+mmz3,y+mmz);}
          if (flg & 0x20000)  {curcolor=0x1f;ver_line(x,y,y+mmz1);}
          if (sd & 0x80)      {curcolor=RGB555(19,0,0);ver_line(x+mmz-1,y,y+mmz);}
          curcolor=0;
          switch (q)
           {
           case 1:break;
           case 2:sipka(x,y,'s',ZELENA);break;
           case 3:sipka(x,y,'l',ZELENA);break;
           //case 4:curcolor=0x0;norm_rectangle(x+(M_ZOOM>>2),y+(M_ZOOM>>2),x+M_ZOOM-(M_ZOOM>>2)-1,y+M_ZOOM-(M_ZOOM>>2)-1);break;
           case 5:
           case 6:
           case 7:
           case 8:sipka(x,y,q-1,ZELENA);break;
           case 10:if (M_ZOOM>=4) curcolor=0x1f;bar(x+(M_ZOOM>>1)-2,y+(M_ZOOM>>1)-2,x+(M_ZOOM>>1)+1,y+(M_ZOOM>>1)+1);break;
           case 11:curcolor=0x0;bar(x+(M_ZOOM>>2),y+(M_ZOOM>>2),x+M_ZOOM-(M_ZOOM>>2)-1,y+M_ZOOM-(M_ZOOM>>2)-1);break;
           case 12:sipka(x,y,'T',ZELENA);break;
           case 13:curcolor=0x0;norm_rectangle(x+(M_ZOOM>>2),y+(M_ZOOM>>2),x+M_ZOOM-(M_ZOOM>>2)-1,y+M_ZOOM-(M_ZOOM>>2)-1);break;
           case 14:curcolor=RGB555(31,0,0);bar(x+(M_ZOOM>>2),y+(M_ZOOM>>2),x+M_ZOOM-(M_ZOOM>>2)-1,y+M_ZOOM-(M_ZOOM>>2)-1);
           case 15:
           case 16:
           case 17:
           case 18:sipka(x,y,q-15+4,MODRA);break;
           case 19:sipka(x,y,'.',RGB555(31,31,0));break;
           case 20:sipka(x,y,'!',ZELENA);break;
           case 21:sipka(x,y,'M',0x8000);break;
           }
          if (mob_map[i]!=-1) sipka(x,y,((mob_map[i]>>14) & 3)+4,CERVENA);
          for (q=0;q<4;q++) prepinac(x,y,q,&(mapa.sidedef[i][q]),(multi_actions[(i<<2)+q]!=NULL));
           }
        }
     }
  }

void swaps(int *x1,int *y1,int *x2,int *y2)
  {
  if (*x1>*x2)
     {
     *x1+=*x2;
     *x2=*x1-*x2;
     *x1=*x1-*x2;
     }
  if (*y1>*y2)
     {
     *y1+=*y2;
     *y2=*y1-*y2;
     *y1=*y1-*y2;
     }
  }


void draw_rectangle(int x1,int y1,int x2,int y2)
  {
    swaps(&x1,&y1,&x2,&y2);
    xor_rectangle(x1,y1,x2-x1,y2-y1);
  }

void *working(EVENT_MSG *msg)
  {
  static work_counter=0;
  int i;
  char *c;

  if (msg->msg==E_INIT) return &working;
  if (msg->msg==E_DONE) return NULL;
  c=(char *)msg->data;
  strcpy(c,"  Pracuji ");
  c=strchr(c,'\0');
  for (i=0;i<work_counter;i++) *c++='.';
  *c='\0';
  msg->data=(void *)c;
  work_counter=(work_counter+1) & 7;
  return NULL;
  }

void *print_layer(EVENT_MSG *msg)
  {
   char *c;

  if (msg->msg==E_INIT) return &print_layer;
  if (msg->msg==E_DONE) return NULL;
  c=(char *)msg->data;
  sprintf(c,"%d. patro",cur_layer);
  c=strchr(c,'\0');
  msg->data=(void *)c;
  msg->msg=-1;
  return NULL;
  }


void add_map_bar(va_list args)
  {
  int x,y;
  long f;
  int x1=va_arg(args,int);
  int y1=va_arg(args,int);
  int x2=va_arg(args,int);
  int y2=va_arg(args,int);

  f=get_draw_flags();
  swaps(&x1,&y1,&x2,&y2);
     if (y1==y2)
       for(x=x1;x<x2;x++) tenka_stena(x,y1,cur_layer,0,f);
    else
    if (x1==x2)
       for(y=y1;y<y2;y++) tenka_stena(x1,y,cur_layer,3,f);
    else
    for (y=y1;y<y2;y++)
        {
       for (x=x1;x<x2;x++)
         add_sector(x,y,cur_layer,0);
         task_sleep(NULL);
     }
  redraw_desktop();
  }

void del_map_bar(va_list args)
  {
  int x,y;
  int x1=va_arg(args,int);
  int y1=va_arg(args,int);
  int x2=va_arg(args,int);
  int y2=va_arg(args,int);


  swaps(&x1,&y1,&x2,&y2);
    for (y=y1;y<y2;y++)
     {
     for (x=x1;x<x2;x++)
       delete_sector(x,y,cur_layer);
     task_sleep(NULL);
     }
  redraw_desktop();
  }

void unselect_map(void)
  {
  int i;

  for(i=0;i<MAPSIZE;i++) minfo[i].flags&=~1;
  }

void edit_map_bar(int x1,int y1,int x2,int y2)
  {
  int x,y,i;
  char c=0;
  char *shift;


  shift=(char *)0x417;
  if (~GetKeyState(VK_CONTROL) & 0x80)unselect_map();
  swaps(&x1,&y1,&x2,&y2);
  for(y=y1;y<y2;y++)
     for(x=x1;x<x2;x++)
       {
         i=find_sector(x,y,cur_layer);
         if (i+1)
           {
           c=1;
           if (tool_sel==40)
              {
              if (find_window(multiaction_win)!=NULL)
                 {
                 minfo[i].flags|=1;
                 update_multiactions();
                 }
              else open_wiz_tool();
              }
           else
              if (find_window(enemy_win)==NULL) info_sector(i);
              else select_enemy(i);
           goto skok;
           }
       }
  skok:
     for (y=1;y<maplen;y++)
        {
        TMAP_EDIT_INFO *mf;

        mf=minfo+y;
        mf->flags|=(mf->x>=x1 && mf->x<x2 && mf->y>=y1 && mf->y<y2 && mf->layer==cur_layer && mapa.sectordef[y].sector_type!=0);
        }
  if (tool_sel==40 && c==0) close_wiz_tool();
  }


void workspace_event(EVENT_MSG *msg,OBJREC *o)
  {
  static char draw=0;
  static int xf,yf,xo,yo;
  int xn,yn,xc,yc;
  int mapx,mapy,xoff,yoff;
  int msx,msy;

  xc=(o->xs % M_ZOOM)>>1;
  yc=(o->ys % M_ZOOM)>>1;
  xn=o->locx+xc;
  yn=o->locy+yc;
  mapx=(o->xs)>>m_zoom;
  mapy=(o->ys)>>m_zoom;
  xoff=xmap_offs-mapx/2;
  yoff=ymap_offs-mapy/2;
  switch (msg->msg)
     {
     case E_MOUSE:
        {
        MS_EVENT *ms;

        ms=get_mouse(msg);
        msx=ms->x-xn;
        msy=ms->y-yn;
        if (tool_sel==50)
           {
           if (ms->event_type & 0xA)
              {
              int i,j;
              xo=(msx>>m_zoom)+xoff;
              yo=(msy>>m_zoom)+yoff;
              for(i=0;i<maplen && (minfo[i].x!=xo || minfo[i].y!=yo || minfo[i].layer!=cur_layer);i++);
              if (i==maplen) return;
              xo=(msx) & (M_ZOOM-1);
              yo=(msy) & (M_ZOOM-1);
              j=0;
              if (xo>(M_ZOOM>>1)) j++;
              if (yo>(M_ZOOM>>1)) j=3-j;
              select_item(i*4+j);
              return;
              }
           return;
           }
        if (ms->event_type & 0xA)
           {
           xf=(msx+(M_ZOOM>>1)) & ~(M_ZOOM-1);
           yf=(msy+(M_ZOOM>>1)) & ~(M_ZOOM-1);
           xo=xf;yo=yf;
           draw=ms->event_type & 0xA;
           return;
           }
        if (draw && (ms->tl1 || ms->tl2))
           {
           draw_rectangle(xn+xf,yn+yf,xn+xo,yn+yo);
           xo=(msx+(M_ZOOM>>1)) & ~(M_ZOOM-1);
           yo=(msy+(M_ZOOM>>1)) & ~(M_ZOOM-1);
           draw_rectangle(xn+xf,yn+yf,xn+xo,yn+yo);
           return;
           }
        if (ms->event_type & 0x14 && draw)
           {
            int i;

            i=tool_sel;
             if (tool_sel==10 && !(draw & 2)) i=20;
             if (tool_sel==20 && !(draw & 2)) i=10;
            draw=0;
            xf=(xf>>m_zoom)+xoff;
            xo=(xo>>m_zoom)+xoff;
            yf=(yf>>m_zoom)+yoff;
            yo=(yo>>m_zoom)+yoff;
            switch (i)
              {
              case 10:add_task(8196,add_map_bar,xf,yf,xo,yo);break;
              case 20:add_task(8196,del_map_bar,xf,yf,xo,yo);break;
              case 30:
              case 40:
                if (xf==xo) xo=1+(xf=msx/M_ZOOM+xoff);
                if (yf==yo) yo=1+(yf=msy/M_ZOOM+yoff);
                 edit_map_bar(xf,yf,xo,yo);
                 redraw_object(o);
                break;
              }

           }
        }
        break;
     case E_GET_FOCUS: break;
     case E_LOST_FOCUS: if (draw) redraw_object(o);draw=0;break;
     }
  }
void workspace(OBJREC *o)
  {
//o->call_init=workspace_init;
  o->call_draw=workspace_draw;
  o->call_event=workspace_event;
  o->datasize=4;
  }

void close_with_tool(void)
  {
  WINDOW *w;
  close_window(waktual);
  if ((w=find_window(tool_bar))!=NULL) close_window(w);
  if ((w=find_window(multiaction_win))!=NULL) close_window(w);
  if ((w=find_window(enemy_win))!=NULL) close_window(w);
  if ((w=find_window(wiz_tool_numb))!=NULL) close_window(w);
  if ((w=find_window(sektor_win))!=NULL) close_window(w);
  if ((w=find_window(item_win))!=NULL) close_window(w);
  if ((w=find_window(vzor_win))!=NULL) close_window(w);
  }

void change_tools()
  {
  int i;
  WINDOW *w;

  if (find_window(tool_bar)==NULL) return;
  for (i=10;i<50;i+=10)c_set_value(tool_bar,i,i==tool_sel);
  if (tool_sel!=30) if (find_window(sektor_win)!=NULL)
     {
     close_window(find_window(sektor_win));
     if (tool_sel!=40) unselect_map(); else open_wiz_tool();
     send_message(E_DONE,E_KEYBOARD,chozeni2);
     }
  if (tool_sel!=40)
     {
     if ((w=find_window(wiz_tool_numb))!=NULL) close_window(w);
     if ((w=find_window(multiaction_win))!=NULL) close_window(w);
     if (tool_sel!=30) unselect_map();
     else
        for(i=0;i<maplen;i++) if (minfo[i].flags & 1)
           {
           info_sector(i);
           break;
           }
     redraw_desktop();
     }
  if (tool_sel==10) open_draw_win(); else
     if (find_window(draw_win)!=NULL) close_window(find_window(draw_win));
  if (tool_sel==40 && find_window(multiaction_win)==NULL) open_wiz_tool();
}

void tool_select()
  {
  tool_sel=o_aktual->id;
  change_tools();
  }


void scroll_workspace()
  {
  ymap_offs=f_get_value(0,20);
  xmap_offs=f_get_value(0,30);
  c_set_value(0,10,0);
  }


void layer_show()
  {
  send_message(E_STATUS_LINE,E_ADD,E_IDLE,print_layer);
  }
void layer_hide()
  {
  send_message(E_STATUS_LINE,E_DONE,E_IDLE,print_layer);
  }

void layer_plus()
  {
  cur_layer++;
  if (cur_layer>MAX_DEPTH) cur_layer=MAX_DEPTH;
  autocenter_map();
  c_set_value(0,10,0);
  }

void layer_minus()
  {
  cur_layer--;
  if (cur_layer<MIN_DEPTH) cur_layer=MIN_DEPTH;
  autocenter_map();
  c_set_value(0,10,0);
  }


void create_map_win(int xp,...)
  {
  if (find_window(map_win)==NULL)
     {
     CTL3D *ctl;
     FC_TABLE cl;
     int *xpp;

     autocenter_map();
     cl[0]=0;cl[1]=0x610;
     map_win=def_window(500,451,"Kreslen� mapy");
     waktual->x=7;waktual->y=2;
     on_control_change(close_with_tool);
     define(10,2,20,473,408,0,workspace);
     property(def_border(1,0),NULL,NULL,WINCOLOR);
     o_end->autoresizex=1;o_end->autoresizey=1;
     ctl=def_border(3,WINCOLOR);
     waktual->minsizx=140;
     waktual->minsizy=90;
     define(20,3,42,17,365,1,scroll_bar_v,-100,100,(59*8)/M_ZOOM,SCROLLBARCOL);
     property(ctl,NULL,NULL,WINCOLOR);c_default(ymap_offs);on_control_change(scroll_workspace);
     o_end->autoresizey=1;
     define(21,1,20,21,17,1,scroll_button,-2,-100,"\x1e");
     property(NULL,icones,&cl,WINCOLOR);on_control_change(scroll_support);
     define(22,1,22,21,17,2,scroll_button,2,100,"\x1f");
     property(NULL,icones,&cl,WINCOLOR);on_control_change(scroll_support);
     define(30,26,3,352,15,3,scroll_bar_h,-100,100,(59*8)/M_ZOOM,SCROLLBARCOL);
     property(ctl,NULL,NULL,WINCOLOR);c_default(xmap_offs);on_control_change(scroll_workspace);
     o_end->autoresizex=1;
     define(31,96,1,21,19,2,scroll_button,2,100,"\x1c");
     property(NULL,icones,&cl,WINCOLOR);on_control_change(scroll_support);
     define(32,1,1,21,19,3,scroll_button,-2,-100,"\x1d");
     property(NULL,icones,&cl,WINCOLOR);on_control_change(scroll_support);
     define(40,60,1,30,19,2,button,"/");property(NULL,icones,&cl,WINCOLOR);
     on_control_enter(layer_show);on_control_exit(layer_hide);on_control_change(layer_plus);
     define(50,28,1,30,19,2,button,"\\");property(NULL,icones,&cl,WINCOLOR);
     on_control_enter(layer_show);on_control_exit(layer_hide);on_control_change(layer_minus);
     define(60,1,1,19,19,2,resizer);
     xpp=&xp;
     xpp++;
     if (xp>=0) movesize_win(waktual,xp,*xpp,*(xpp+1),*(xpp+2));
     else  movesize_win(waktual,3,3,SCR_WIDTH_X-130,SCR_WIDTH_Y-10);
     }
  else
     {
     select_window(map_win);
     }
    if (find_window(tool_bar)==NULL)
     {
     redraw_window();
     tool_bar=def_window(90,90,"Tools");
     waktual->x=SCR_WIDTH_X-1-94;waktual->y=desktop_y_size-90-3;
     define(20,1,55,40,34,0,toggle_button,"\xC");property(NULL,icones,&icone_color,WINCOLOR);
      on_control_change(tool_select);c_default(tool_sel==20);
     define(30,42,20,40,34,0,toggle_button,"\xD");property(NULL,icones,&icone_color,WINCOLOR);
      on_control_change(tool_select);c_default(tool_sel==30);
     define(40,42,55,40,34,0,toggle_button,"\xE");property(NULL,icones,&icone_color,WINCOLOR);
      on_control_change(tool_select);c_default(tool_sel==40);
     define(10,1,20,40,34,0,toggle_button,"\xB");property(NULL,icones,&icone_color,WINCOLOR);
      on_control_change(tool_select);c_default(tool_sel==10);
     }
if (tool_sel==10)open_draw_win();
if (tool_sel==40)open_wiz_tool();
redraw_desktop();
  }

void zoomin()
  {
  WINDOW *w,s;
  if (m_zoom<5)
     {
     m_zoom++;
     w=find_window(map_win);
     if (w!=NULL)
        {
        s=*w;
        nocenter=1;
        close_window(w);
        create_map_win(s.x,s.y,s.xs,s.ys);
        }

     }
  }
void zoomout()
 {
  WINDOW *w,s;
  if (m_zoom>0)
     {
     m_zoom--;
     w=find_window(map_win);
     if (w!=NULL)
        {
        s=*w;
        nocenter=1;
        close_window(w);
        create_map_win(s.x,s.y,s.xs,s.ys);
        }
     }
 }
void set_defaults(void)
  {
  mapa.sidedef[0][0].flags=0x1fc42201;
  mapa.sidedef[0][0].xsec=250/2;
  mapa.sidedef[0][0].ysec=160/2;
  mapa.sectordef[0].flags=0x11;
  }

void mapy_init(void)
  {
  actions=(char **)build_static_list(_actions);
  steny2=(char **)build_static_list(_steny2);
  zivly=(char **)build_static_list(_zivly);
  sector_types=(char **)build_static_list(_sector_types);
  act_types=(char **)build_static_list(_type_multi_actions);
  typy_veci=(char **)build_static_list(_typy_veci);
  typy_zbrani=(char **)build_static_list(_typy_zbrani);
  umisteni_veci=(char **)build_static_list(_umisteni_veci);
  side_flgs=(char **)build_static_list(_side_flgs);
  }

static __inline long output_code(void *target,long bitepos,int bitsize,int data)
  {
  *((long *)target+bitepos/8)|=data<<(bitepos & 0x7);
  return bitepos+bitsize;
  }

static __inline long input_code(void *source,long *bitepos,int bitsize,int mask)
  {
  long res=*((long *)source+(*bitepos>>8));
  res>>=(*bitepos & 0x7);
  bitepos[0]+=bitsize;
  return res;
  }

/*long output_code(void *target,long bitepos,int bitsize,int data);
#pragma aux output_code parm [edi][edx][ebx][eax]=\
     "mov     ecx,edx"\
     "shr     ecx,3"\
     "add     edi,ecx"\
     "mov     ecx,edx"\
     "and     cl,7h"\
     "shl     eax,cl"\
     "or      [edi],eax"\
     "add     edx,ebx"\
    value[edx] modify [ecx];


int input_code(void *source,long *bitepos,int bitsize,int mask);
#pragma aux input_code parm [esi][edi][ebx][edx]=\
     "mov     ecx,[edi]"\
     "mov     eax,ecx"\
     "shr     eax,3"\
     "mov     eax,[esi+eax]"\
     "and     cl,7"\
     "shr     eax,cl"\
     "and     eax,edx"\
     "add     [edi],ebx"\
    value[eax] modify [ecx];
*/



void calc_changes_mem(void *orgn,void *new, void *maskreg, char *scr)
  {
  long bitepos1=0;
  long bitepos2=0;
  long bitepos3=0;
  int mask;
  int v1,v2;

  while (*scr)
     {
     mask=(1<<scr[0])-1;
     v1=input_code(orgn,&bitepos1,*scr,mask);
     v2=input_code(new,&bitepos2,*scr,mask);
     if (v1!=v2) bitepos3=output_code(maskreg,bitepos3,*scr,mask);
     else bitepos3+=*scr;
     scr++;
     }
  }


void move_changes(void *source,void *target, void *chmem, long size)
  {
  char *s,*t,*c;

  s=source;
  t=target;
  c=chmem;
  while (size--)
     {
     char a=*s;
     t[0]&=~c[0];
     a&=c[0];
     t[0]|=a;
     s++;c++;t++;
     }
  }
