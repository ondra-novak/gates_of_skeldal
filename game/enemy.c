#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>


#include <libs/types.h>
#include <libs/event.h>
#include <libs/memman.h>
#include <libs/bgraph.h>
#include <platform/sound.h>
#include "engine1.h"
#include "globals.h"
#include "specproc.h"

#include <string.h>

#include "ach_events.h"

#define MOB_ZNAKY "FLBLCH"
#define MOB_START 1


#define MOB_DIST 24
#define MBS_WALK 0
#define MBS_ATTACK 1
#define MBS_HIT 2


#define MBA_ATTACK 3 //potvora utoci
#define MBA_SPELL  1 //potvora caruje
#define MBA_FLEE   2 //potvora utika
#define MBA_NONE   0

#define PK_QUERY 0  //dotaz zda je v mrtvole predmet
#define PK_PICK 1 //prvni predmet v mrtvole

#define mob_walk_sound(p) if (p->locx!=p->headx || p->locy!=p->heady) mob_sound_event(p,MBS_WALK)

TMOB mobs[MAX_MOBS];
char *mob_map;
char hex_chars[]="0123456789ABCDEF";
int mob_dostal=0;
int mob_dostal_pocet;

char folow_mode=0;
char folow_mob;
char show_mob_info = 0;

char mob_go_x[]={128,255,128,0};
char mob_go_y[]={0,128,255,128};
char mob_batt_x[]={128,128+MOB_DIST,128,128-MOB_DIST};
char mob_batt_y[]={128-MOB_DIST,128,128+MOB_DIST,128};
short konv_x[]={1,1,-1,-1};
short konv_y[]={-1,1,1,-1};
char going[]={0,0,1,0,1,1};

static word *mob_paths[MAX_MOBS];
static word *mob_path_ptr[MAX_MOBS];
static int monster_block = 0;

void *sound_template=NULL;

short att_sect;
char battle=0;
char neco_v_pohybu=1;
char nohassle=0;


typedef struct tmobsavedata
{
  short anim_counter;        //citac animaci
  char anim_phase;            //cinnost kterou mob dela
  char dir;
}TMOBSAVEDATA;

static TMOBSAVEDATA **mobsavedata=0;

static void register_mob_path(int mob,word *path) //registruje cestu pro potvoru
  {
  mob_paths[mob]=path;
  mob_path_ptr[mob]=path;
  }

static void free_path(int mob)      //vymaze cestu potvore
  {
  free(mob_paths[mob]);
  mob_paths[mob]=NULL;
  mob_path_ptr[mob]=NULL;
  }

void send_mob_to(int m,word *path)
  {
  if (mob_paths[m]!=NULL)
    {
    free_path(m);
    }
  register_mob_path(m,path);
  mobs[m].stay_strategy |= MOB_WALK;
  }

void smeruj_moba(TMOB *m,int smer)
  {
  int val=128+MOB_DIST*smer;

  switch (m->dir)
    {
    case 0:m->headx=val;break;
    case 1:m->heady=val;break;
    case 2:m->headx=-val;break;
    case 3:m->heady=-val;break;
    }
  }

void save_enemy_paths(TMPFILE_WR *f)
  {
  short i;
  int s;
  word *w;

  for(i=0;i<MAX_MOBS;i++) if (mob_paths[i]!=NULL)
    {
    temp_storage_write(&i,2*1,f);
    w=mob_path_ptr[i];
    s=1;while(*w++) s++;
    temp_storage_write(&s,1*sizeof(s),f);
    temp_storage_write(mob_path_ptr[i],s*2,f);
    }
  i=-1;
  temp_storage_write(&i,1*2,f);
  }

int load_enemy_paths(TMPFILE_RD *f)
  {
  short i=-1;
  int s;
  word *w;

  for(i=0;i<MAX_MOBS;i++) free_path(i);
  temp_storage_read(&i,2*1,f);
  while(i!=-1)
    {
      temp_storage_read(&s,1*sizeof(s),f);
    w=NewArr(word,s);
    temp_storage_read(w,s*2,f);
    register_mob_path(i,w);
    if (temp_storage_read(&i,2*1,f)==0) return 1;
    }
  return 0;
  }

static void mob_reload(EVENT_MSG *msg,void **_){
  static int counter=0;

  if (msg->msg == E_KOUZLO_KOLO) {
     {
     if (insleep) return;
     if (counter++==10)
        {
        TMOB *m;
        int32_t vl;

        static int last;
        counter=0;
        while (last<MAX_MOBS)
           {
           vl=mobs[last].vlajky;
		   if (game_extras & EX_RESPAWN_MONSTERS) vl |= MOB_RELOAD;
           if (~vl & MOB_LIVE && vl & MOB_RELOAD) break; else last++;
           }
        if (last==MAX_MOBS)
           for(last=0;last<MAX_MOBS;last++)
             {
             vl=mobs[last].vlajky;
  		     if (game_extras & EX_RESPAWN_MONSTERS) vl |= MOB_RELOAD;
             if (~vl & MOB_LIVE && vl & MOB_RELOAD) break;
             }
        if (last==MAX_MOBS) return;
        m=mobs+last;
        last++;
        if (map_coord[m->home_pos].flags & MC_PLAYER ||
            mob_map[m->home_pos]) return;
        m->vlajky|=MOB_LIVE;
        m->vlajky&=~MOB_IN_BATTLE;
        m->lives=m->vlastnosti[VLS_MAXHIT];
        m->sector=m->home_pos;
        m->locx=m->headx=m->locy=m->heady=128;
        memset(m->inv,0,sizeof(m->inv));
        play_sample_at_sector(H_SND_TELEPOUT,viewsector,m->home_pos,0,0);
        add_spectxtr(m->home_pos,H_TELEP_PCX,14,1,0);
        refresh_mob_map();
        SEND_LOG("(RELOAD) Mob reloaded: '%s' at sector %d",m->name,m->home_pos);
        free_path(m-mobs);
        }
     }
  }
}

void init_mobs()
  {
  memset(mobs,0,sizeof(mobs));
  mob_map=getmem(mapsize);
  memset(mob_map,0,mapsize);
  memset(mob_paths,0,sizeof(mob_paths));
  memset(mob_path_ptr,0,sizeof(mob_path_ptr));
  send_message(E_DONE,E_KOUZLO_KOLO,mob_reload);
  send_message(E_ADD,E_KOUZLO_KOLO,mob_reload);
  }

static void register_mob_graphics(int num,char *name_part,const char *anims,const char *seq)
  {
  char fulname[14];
  char znaky[]=MOB_ZNAKY;
  int i,j,a;

  strcpy(fulname,name_part);
  strcat(fulname,"??.PCX");
  a=num;
  for(i=0;i<6;i++)
     {
     for(j=0;j<16;j++)
        {
        if (anims[i])
           {
           fulname[6]=znaky[i];
           if (j<=anims[i])
              {
              fulname[7]=*seq++;
              def_handle(a,fulname,pcx_8bit_nopal,SR_ENEMIES);
              }
           a++;
           }
        else
           {
           fulname[6]=znaky[0];
           if (j<=anims[0])
              {
              fulname[7]=*seq++;
              def_handle(a,fulname,pcx_8bit_nopal,SR_ENEMIES);
              }
           a++;
           }
        if (*seq=='\r')
           {
		   char buff[256];

           sprintf(buff,"Soubor sekvence %s obsahuje chybne udaje nebo je sekvence je moc kratka\n", fulname);
		   display_error(buff);
           exit(0);
           }
        }
     seq=strchr(seq,'\n')+1;
     }
  }




static void register_mob_sounds(int hlptr,word *sounds)
  {
  int i,z;

  for(i=0;i<4;i++)
     {
     z=sounds[i];
     if (z)
        {
        def_handle(hlptr,sound_table[z-1],wav_load,SR_ZVUKY);
        }
     hlptr++;
     }
  }

static char miri_middle(TMOB *p) //procedura zjisti zda li potvora miri do dveri
  {
  int ss;

  ss=(p->sector<<2)+p->dir;
  return (map_sides[ss].lclip!=0);
  }


static char pick_item_corpse(TMOB *m,char query)
  {
  short *p=NULL;
  int sector=m->sector;      //sektor kde se bude mrtvola prohledavat
  if (map_coord[sector].flags & MC_DEAD_PLR) //lezi tam vubec nejaka mrtvola?
     {
     int i;
     THUMAN *h;        //najdi mezi hraci mrtvolu ktera tam lezi
     for(i=0,h=postavy;i<POCET_POSTAV && p==NULL;i++,h++) if (!h->lives && h->used && h->inmaphash == current_map_hash &&h->sektor==sector)
        {
        int i;         //podivej se ji do inventare....
        for(i=0;i<HUMAN_RINGS && p==NULL;i++) if (h->prsteny[i]) p=&h->prsteny[i];
        //nejprve ber prsteny
        for(i=0;i<HUMAN_PLACES && p==NULL;i++) if (h->wearing[i]) p=&h->wearing[i];
        //pak seber zbrane a brneni
        for(i=0;i<h->inv_size && p==NULL;i++) if (h->inv[i]) p=&h->inv[i];
        //teprve potom se podivej co ma v inv.
        }
     }
  if (p!=NULL)  //nasel jsi neco?
     if (query==PK_QUERY) return 1;  //pokud se jednalo o dotaz, tak pouze vrat 1
     else
        {
        int i;
        for(i=0;i<MOBS_INV;i++) if (m->inv[i]==0) break; //zjisti jestli mas misto
        if (i==MOBS_INV) return 0; //nemas, mas smulu....
        m->inv[i]=*p;  //prenes predmet od postavy do sveho inventare.
        *p=0; //ale na puvodnim miste tento predmet znic.
        }
  else return 0;
  return 1;
  }

static char seber_predmet(TMOB *m)
  {
  short *p=NULL,*q;
  int i,j,c,z = 0;

  for(j=0,c=0;j<MOBS_INV;c+=(m->inv[j]==0),++j);
  i=0;
  do
     {
     for(;i<4 && p==NULL;i+=(p==NULL)) pop_item(m->sector,i,0,&p);
     if (i==4) return 1;
     z=count_items_total(p);
     if (z>c)
        {
        push_item(m->sector,i,p);
        free(p);
        p=NULL;
        }
     i++;
     }
  while (z>c);
  if (z<=c) for(q=p,j=0;j<MOBS_INV && (*q);j++) if (m->inv[j]==0) m->inv[j]=(abs(*q++));
  free(p);
  return 0;
  }

static void mob_sound_event(TMOB *m, int event) {
    if (m->sounds[event] && m->vlajky & MOB_LIVE
            && ~m->vlastnosti[VLS_KOUZLA] & SPL_STONED) {
        if (event == MBS_WALK) {
            play_sample_at_sector(
                    m->cislo_vzoru + 16 * 6 + event + monster_block, viewsector,
                    m->sector, m - mobs + 256,
                    (m->vlajky & MOB_SAMPLE_LOOP) != 0);
        } else {
            play_sample_at_sector(
                    m->cislo_vzoru + 16 * 6 + event + monster_block, viewsector,
                    m->sector, 0, 0);
        }
    }
}

void load_enemies(short *data,int size,int *grptr,const TMOB *template,int32_t tsize)
  {
  int i;
  short cisla[256];

  monster_block=*grptr;
  memset(cisla,0xff,sizeof(cisla));
  size>>=2;
  for(i=0;i<MAX_MOBS;i++) free_path(i);
  if (size>MAX_MOBS-MOB_START) size=MAX_MOBS-MOB_START;
  for(i=0;i<size;i++)
     {
     int j,c,cnt;
     const TMOB *b;

     b=template;
     cnt=tsize/sizeof(TMOB);
     c=data[1] & 0xfff;
     for(j=0;j<cnt && b[j].cislo_vzoru!=c;j++);
     if (j!=cnt && data[0]<mapsize && map_sectors[data[0]].sector_type && c<cnt)
        {
        mobs[i]=b[j];
        if (~mobs[i].vlajky & MOB_MOBILE) mob_map[data[0]]=i+MOB_START;
        if (mobs[i].palette>0)mobs[i].palette=rnd(mobs[i].palette);else mobs[i].palette=abs(mobs[i].palette);
        mobs[i].sector=data[0];
        mobs[i].dir=(data[1]>>14)&0x3;
        mobs[i].home_pos=data[0];
        mobs[i].vlajky|=MOB_LIVE;
        mobs[i].id = (uint8_t)j;
        if (mobs[i].speed<1)
          {
  	      char buff[256];

          sprintf(buff,"Nestvura cislo #%d (%s) je spatne definovana (rychlost)",i,mobs[i].name);
		  display_error(buff);
          exit(1);
          }
        cisla[i]=mobs[i].cislo_vzoru;
        for(j=0;j<i;j++) if (cisla[j]==cisla[i]) break;
        if (j!=i) mobs[i].cislo_vzoru=mobs[j].cislo_vzoru;
        else
           {
           char s[20];

           sprintf(s,"%s.SEQ",mobs[i].mobs_name);
           def_handle(grptr[0]++,s,NULL,SR_ENEMIES);
           mobs[i].cislo_vzoru=*grptr-monster_block;
           register_mob_graphics(*grptr,mobs[i].mobs_name,mobs[i].anim_counts,ablock(grptr[0]-1));
           grptr[0]+=6*16;
           register_mob_sounds(*grptr,mobs[i].sounds);
           grptr[0]+=4;
           sprintf(s,"%s.COL",mobs[i].mobs_name);
           def_handle(grptr[0],s,col_load,SR_ENEMIES);
           grptr[0]++;
           }
        }
     else
        {
        mobs[i].cislo_vzoru=-1;
        mobs[i].vlajky&=~MOB_LIVE;
        }
     for(j=0;j<6;j++) if (mobs[i].anim_counts[j]==0) mobs[i].anim_counts[j]=mobs[i].anim_counts[0];
     data+=2;
     }
  for(;i<MAX_MOBS;i++) mobs[i].vlajky&=~MOB_LIVE;
  }

int mob_pocet_vychodu(int sector,int dir)
  {

  sector<<=2;
  return ((map_sides[sector+dir].flags & SD_MONST_IMPS)==0)+
         ((map_sides[sector+((dir+1) & 3)].flags & SD_MONST_IMPS)==0)+
         ((map_sides[sector+((dir-1) & 3)].flags & SD_MONST_IMPS)==0);
  }

  //retval: 0 - sector is free
  //retval: 1 - sector is not passable
  //retval: 2 - players on sector
 char mob_check_next_sector(int sect,int dir,char alone,char passable)
  {
  int i;

  if (map_sides[(sect<<2)+dir].flags & SD_MONST_IMPS) return 1;
  sect=map_sectors[sect].step_next[dir];
  if ((map_coord[sect].flags & MC_PLAYER) && (~passable & MOB_PASSABLE)) return 2;
  i=mob_map[sect];
  if (i==0) return 0;
  if (alone & MOB_BIG) return 1;
  i-=MOB_START;
  if (mobs[i].stay_strategy & MOB_BIG) return 1;
  if (mobs[i].next==0) return 0;
  return 1;
  }


int mob_vyber_vychod(int r,int sector,int dir,char alone,char mobile)
  {
  int cnt=0,i;
  dir=(dir-1)&3;
  for(;r>0 && cnt<5;cnt++,dir=(dir+1)&3)
     if (!(i=mob_check_next_sector(sector,dir,alone,mobile)))
        {
        r--;
        if (!r) break;
        }
  if (cnt==5) dir=-1;
  return dir;
  }

char je_mozne_videt(int sector1, int sector2, int flag) {
    int x1, y1;
    int x2, y2;
    int xs, ys;
    int x, y, ly, s;

    if (map_coord[sector1].layer != map_coord[sector2].layer)
        return 0;
    x1 = map_coord[sector1].x;
    x2 = map_coord[sector2].x;
    y1 = map_coord[sector1].y;
    y2 = map_coord[sector2].y;
    xs = x1 - x2;
    ys = y1 - y2;
    if (xs == 0 && ys == 0)
        return 1;
    s = sector1;
    ly = 0;
    if (xs >= 0) {
        for (x = 0; x <= xs; x++) {
            y = (x + 1) * ys / (xs + 1);
            while (y > ly) {
                if ((map_sides[(s << 2)].flags & flag) == 0) {
                    s = map_sectors[s].step_next[0];
                    ly++;
                } else
                    return 0;
            }
            while (y < ly) {
                if ((map_sides[(s << 2) + 2].flags & flag) == 0) {
                    s = map_sectors[s].step_next[2];
                    ly--;
                } else
                    return 0;
            }
            if (x != xs) {
                if ((map_sides[(s << 2) + 3].flags & flag) == 0) {
                    s = map_sectors[s].step_next[3];
                } else {
                    return 0;
                }
            }
        }
    } else if (xs < 0) {
        for (x = 0; x >= xs; x--) {
            y = (x - 1) * ys / (xs - 1);
            while (y > ly)
                if ((map_sides[(s << 2)].flags & flag) == 0) {
                    s = map_sectors[s].step_next[0];
                    ly++;
                } else
                    return 0;
            while (y < ly)
                if ((map_sides[( s << 2) + 2].flags & flag) == 0) {
                    s = map_sectors[s].step_next[2];
                    ly--;
                } else
                    return 0;
            if (x != xs) {
                if ((map_sides[(s << 2) + 1].flags & flag) == 0) {
                    s = map_sectors[s].step_next[1];
                } else {
                    return 0;
                }
            }
        }
    }
    return s == sector2;
}

int q_vidis_postavu(int sector,int dir,TMOB *p,int *otocit_se,char ret)
  {
  int i,z=-1;
  int xs,ys,nd=0,d,dis=255;

  if (p->vlastnosti[VLS_KOUZLA] & SPL_BLIND) return -1;
  if (p->vlastnosti[VLS_KOUZLA] & SPL_FEAR) return -1;
  if (p->flee_num==100 && !insleep) return -1;
  for(i=0;i<POCET_POSTAV;i++)
     {
     char ok=0;
     THUMAN *ps=&postavy[i];
     if (ps->used && ps->inmaphash == current_map_hash) {
      xs=map_coord[sector].x-map_coord[postavy[i].sektor].x;
      ys=map_coord[sector].y-map_coord[postavy[i].sektor].y;
      d=MAX(abs(xs),abs(ys));
      if (d<=p->dohled && (!(ps->vlastnosti[VLS_KOUZLA] & SPL_INVIS)||p->vlajky & MOB_SENSE) && ps->used && ps->lives && ps->inmaphash == current_map_hash)
         switch(dir)
            {
            case 0:ok=ys>=0;break;
            case 1:ok=xs<=0;break;
            case 2:ok=ys<=0;break;
            case 3:ok=xs>=0;break;
            }
      if (ok) {
         char shoots = p->stay_strategy & MOB_ROGUE;
         int block_flag = shoots && MIN(abs(xs),abs(ys))  == 0?(SD_THING_IMPS):(SD_MONST_IMPS|SD_PLAY_IMPS);
         if (je_mozne_videt(sector,postavy[i].sektor,block_flag))
            {
               int alt = 0;
               if (ys>=abs(xs)) {nd=0;alt=xs>0?3:1;}
               else if (xs>=abs(ys)) {nd=3;alt=ys>0?0:2;}
               else if (ys<=(-abs(xs))) {nd=2;alt=xs>0?1:3;}
               else if (xs<=(-abs(ys))) {nd=1;alt=ys>0?2:0;}
               if (mob_check_next_sector(p->sector,nd,p->stay_strategy & MOB_BIG,p->vlajky & MOB_PASSABLE)==1)
               {
               nd=alt;
               if (mob_check_next_sector(p->sector,nd,p->stay_strategy & MOB_BIG,p->vlajky & MOB_PASSABLE)==1)
               {
                  nd=(alt+2)&3;
                  if (mob_check_next_sector(p->sector,nd,p->stay_strategy & MOB_BIG,p->vlajky & MOB_PASSABLE)==1)
                  {
                     nd=(alt+3)&3;
                  }
               }
               }
            }
            else d=255;
         } else d=255;
      if (d!=255)
         {
         d*=2;
         if (xs!=0 && ys!=0) d+=3;//dej prednost tem co jsou na tve souradnici.
         if (d<dis || (d==dis && rnd(10)<3))
            {
            dis=d;*otocit_se=nd;att_sect=postavy[i].sektor;
            z=i;
            }
         }
      }
   }
  if (dis==255) dis=-2;
  if (ret)return z;else return dis/2;
  }

int uhni_mobe(char on,int num,int kolik)
  {
  TMOB *p;

  p=&mobs[num];
  if (miri_middle(p)) kolik=0;
  if (on)
  {
  if (p->headx!=255 && p->headx!=0 && p->headx!=128) return 128-p->headx;
  if (p->heady!=255 && p->heady!=0 && p->heady!=128) return 128-p->heady;
  }
  if (p->headx!=255 && p->headx!=0) p->headx=128+kolik;
  if (p->heady!=255 && p->heady!=0) p->heady=128+kolik;
  return -kolik;
  }


void stop_mob(TMOB *p)
  {
  int num1;
  TMOB *q;

  p->mode=MBA_NONE;
  num1=mob_map[p->sector];
  if (num1) q=&mobs[num1-MOB_START];else q=p;
  if (p==q) {
     if (p->next) {
        q=mobs+p->next-MOB_START;
     } else {
        q=NULL;
     }
  }
  if (p->stay_strategy & MOB_BIG)
     {
     p->headx=128;
     p->heady=128;
     goto end;
     }
  if (q!=NULL && p->dir!=q->dir)
     {
     p->headx=mob_batt_x[p->dir];
     p->heady=mob_batt_y[p->dir];
     q->headx=mob_batt_x[q->dir];
     q->heady=mob_batt_y[q->dir];
     mob_walk_sound(p);
     mob_walk_sound(q);
     return;
     }
  if (q!=NULL && p->dir==q->dir)
     {
     if (miri_middle(q))
        {
        p->dir=(p->dir+1) & 3;
        stop_mob(q);
        }
     }
  else
     {
     if (p->dir & 1)
      {
      p->headx=mob_batt_x[p->dir];
      if (p->heady==255 || p->heady==0) p->heady=128;
      }
     else
      {
      p->heady=mob_batt_y[p->dir];
      if (p->headx==255 || p->headx==0) p->headx=128;
      }
     goto end;
     }
    {
     switch (p->dir)
        {
        case 0:
        case 2:if (q->headx==128) {
                    if (p->headx<128) q->headx=128+MOB_DIST;else q->headx=128-MOB_DIST;
                }
               p->headx=-q->headx;
               p->heady=mob_batt_y[p->dir];
               break;
        case 1:
        case 3:if (q->heady==128) {
                    if (p->heady<128) q->heady=128+MOB_DIST;else q->heady=128-MOB_DIST;
                }
               p->heady=-q->heady;
               p->headx=mob_batt_x[p->dir];break;
        }
     }
  end:
  mob_walk_sound(p);
  }

void stop_all_mobs()
  {
  int i;
  for(i=0;i<MAX_MOBS;i++)
     if (mobs[i].vlajky & MOB_LIVE) stop_mob(mobs+i);
  }

void stop_all_mobs_on_sector(int sector)
  {
  TMOB *m;
  int mm=mob_map[sector];

  while (mm)
    {
    m=mobs+mm-MOB_START;
    stop_mob(m);
    mm=m->next;
    }
  }

char mob_test_na_bitvu(TMOB *p)
  {
  int x,d;
  char c=0;
  char pt;

  if (nohassle) return 0;
  if (p->stay_strategy & MOB_WATCH)
   if ((d=q_vidis_postavu(p->sector,p->dir,p,&x,0))>-1)
        {
        p->stay_strategy|=MOB_WALK;
        if (mob_check_next_sector(p->sector,x,p->stay_strategy & MOB_BIG,p->vlajky & MOB_PASSABLE)!=1)
         {
         pt=(p->headx==p->locx && p->heady==p->locy);
         if (p->dir!=x || pt)
           {
           p->dir=x;
           if (!(p->stay_strategy & MOB_BIG) || pt)
              {
              p->headx=mob_go_x[x];
              p->heady=mob_go_y[x];
              }
           else
              {
              p->headx=128;
              p->heady=128;
              }
           }
         if (q_zacit_souboj(p,d,att_sect))
           {
           stop_mob(p);
           p->vlajky|=MOB_IN_BATTLE;
           if (!battle) zacni_souboj(p,d,att_sect);
           }
         c=1;
         }
        if (c) mob_sound_event(p,MBS_WALK);
        return c;
        }
  return 0;
  }

char return_home(TMOB *p,int *smer)
  {
  word *path;
  int i;

  i=p->dir;
  if (!mob_check_next_sector(p->sector,(i+1)&3,p->stay_strategy & MOB_BIG,0) || !mob_check_next_sector(p->sector,(i+3)&3,p->stay_strategy & MOB_BIG,0)) return 1;
  if (p->sector==p->home_pos) return 1;
  najdi_cestu(p->sector,p->home_pos,SD_MONST_IMPS,&path,(p->stay_strategy & MOB_BIG)?1:2);
  if (path==NULL)
     {
     return 1;
     }
  for(i=0;i<4 && map_sectors[p->sector].step_next[i]!=path[0];i++);
  free(path);
  if (i==4) return 1;
  if (mob_check_next_sector(p->sector,i,p->stay_strategy & MOB_BIG,0)) return 1;
  *smer=i;
  return 0;
  }

static int jdi_po_ceste(int old,TMOB *p)
  {
  int i,s;
  word *c;

  if (p->mode==MBA_FLEE && !p->actions--)   //v pride uteku pocitej kroky
     {
     p->mode=0;
     p->vlajky&=~MOB_IN_BATTLE;
     return old;
     }
  c=mob_path_ptr[p-mobs];       //vem cestu
  if (c==NULL) return old;      //neni -> konec
  if (*c==0)                    //na konci -> dealokace a konec
     {
     free_path(p-mobs);
     p->mode=0;
     p->vlajky&=~MOB_IN_BATTLE;
     return old;
     }
  s=p->sector;
  for(i=0;i<4;i++) if (map_sectors[s].step_next[i]==*c) break;
  if (i==4) return old;
  old=i;
  c++;
  mob_path_ptr[p-mobs]=c;       //uloz_ukazatel
  return old;
  }

void rozhodni_o_smeru(TMOB *p)
  {
  int sect,dir,r,v,c,alone,oldwalk,passable;
  int vdir=-1;

  alone=p->stay_strategy & MOB_BIG;
  passable=p->vlajky & MOB_PASSABLE;
  sect=p->sector;
  dir=p->dir;
  if (mob_paths[p-mobs]!=NULL) c=jdi_po_ceste(-1,p);else c=-1;
  if (c!=-1) {
    if (!mob_check_next_sector(sect,c,alone,passable))
     {
     p->headx=mob_go_x[c];
     p->heady=mob_go_y[c];
     p->dir=c;
     goto end1;
     }
    else
     {
     if (mob_path_ptr[p-mobs]-mob_paths[p-mobs]>1) mob_path_ptr[p-mobs]-=2;
     }
  }
  if (p->vlajky & MOB_IN_BATTLE)
     {
     stop_mob(p);
     goto end1;
     }
  if (call_mob_event(p->specproc,SMPR_WALK,p)) goto end1;
  oldwalk=p->walk_data;
  if (mob_test_na_bitvu(p)) return;
  p->vlajky&=~MOB_IN_BATTLE;
  c=map_sectors[sect].sector_type;
  c-=S_SMER;
  if (c>=0 && c<4 && !mob_check_next_sector(sect,c,alone,passable))
     {
     if (p->headx==p->locx && p->heady==p->locy) dir=c+4;else dir=c;
     }
  else
     {
     v=mob_pocet_vychodu(sect,dir);
     if (v==p->walk_data && !mob_check_next_sector(sect,dir,alone,passable))
        if (p->headx==p->locx && p->heady==p->locy) dir=p->dir+4;else dir=p->dir;
     else
        {
        r=1;
				if (v==0) v=1;
        p->walk_data=v;
        if (v==1 && p->stay_strategy & MOB_GUARD) r=return_home(p,&dir);
        if (r)
           {
           if (v<2) r=1;else r=rnd(v)+1;
           vdir=dir=mob_vyber_vychod(r,sect,dir,alone,passable);
           //if ( p->stay_strategy & MOB_WATCH && rnd(100)<20 && lv<128 && dir!=p->dir) dir=-1;
           }
        }
     }
 if (dir==-1)
     {
     dir=p->dir;p->dir=(p->dir+2)&3;
     stop_mob(p);
     p->walk_data=rnd(32)+223;
     if (vdir!=-1)p->dir=vdir;
     }
  else
     if (p->dir!=dir || miri_middle(p))
       {
        if (p->stay_strategy & MOB_BIG && (p->locx!=p->headx || p->locy!=p->heady))
        {
        stop_mob(p);
        p->walk_data=oldwalk;
        }
        else if (((p->dir-dir) &0x3)==2 && (p->headx!=p->locx || p->heady!=p->locy))
           {
           stop_mob(p);
           p->walk_data=0;
           }
        else
           {
           dir&=3;
           p->headx=mob_go_x[dir];
           p->heady=mob_go_y[dir];
           p->dir=dir;
           }
       }
  end1:
  if (p->headx!=p->locx || p->heady!=p->locy) mob_sound_event(p,MBS_WALK);
  }

void krok_moba(TMOB *p)
 {
  if (!mob_check_next_sector(p->sector,p->dir,p->stay_strategy,p->vlajky))
     {
        p->headx=mob_go_x[p->dir];
        p->heady=mob_go_y[p->dir];
     }
  }


typedef struct enemy_face_tag {
    int pos;
    int face;
    int mirror;
} TENEMY_FACE;


TENEMY_FACE get_enemy_face(TMOB *p,int dirmob,int action,int curdir)
  {

    TENEMY_FACE ret;
  int view;int pos;
  int xs,ys;

  ret.mirror = 0;
  if (action==MOB_ATTACK) pos=4;
  else if (action==MOB_TO_HIT) pos=5;
  else if (action==MOB_DEATH)
     {
      ret.pos = 0;
     ret.face = 0;
     return ret;
     }
  else
     {
     xs=p->headx-p->locx;
     ys=p->heady-p->locy;
	 if (!(game_extras & EX_WALKDIAGONAL) || p->stay_strategy & MOB_BIG)
	   if (ys!=0 && xs!=0) {
		  if (p->dir==1 || p->dir==3) xs=0;else ys=0;
	   }
     if (xs) dirmob=(xs<0)?3:1;
     if (ys) dirmob=(ys<0)?0:2;
     pos=(2+dirmob-curdir)&0x3;
	 if (game_extras & EX_WALKDIAGONAL && !( p->stay_strategy & MOB_BIG))
	   {
	   switch (curdir & 0x3)
		  {
		  case 0: if (p->locx>p->headx) pos=1;else if (p->locx<p->headx) pos=3;break;
		  case 1: if (p->locy>p->heady) pos=1;else if (p->locy<p->heady) pos=3;break;
		  case 2: if (p->locx>p->headx) pos=3;else if (p->locx<p->headx) pos=1;break;
		  case 3: if (p->locy>p->heady) pos=3;else if (p->locy<p->heady) pos=1;break;
		 }
	   }
     }
  if (pos==3) ret.mirror=pos=1;
  if (p->anim_counter==-1) view=pos*16;
  else view=pos*16+(p->anim_counter % (MAX(p->anim_counts[pos],1)))+1;
  ret.face = view;
  ret.pos = pos;
  return ret;
  }

void get_pos(int x,int y,int *xpos,int *ypos,int dir)
  {
  switch(dir)
     {
     case 0:*xpos=x;*ypos=-y;break;
     case 1:*xpos=y;*ypos=x;break;
     case 2:*xpos=-x;*ypos=y;break;
     case 3:*xpos=-y;*ypos=-x;break;
     }
  }

/*
void draw_blood(int zasah,int celx,int cely,int posx,int posy)
  {
  draw_placed_texture(ablock(H_MZASAH1+zasah-1),celx,cely,posx+64,posy+64,75,0);
  }
*/
static const void *mob_select_palette(TMOB *p)
  {
  const char *palet;

  palet=ablock(p->cislo_vzoru+6*16+4+monster_block);
  return palet+(p->palette)*PIC_FADE_PAL_SIZE;
  }

static void CheckMobStoned(int num)
{
  if (mobs[num].vlastnosti[VLS_KOUZLA] & SPL_STONED)
  {
      TMOB *p=mobs+num;
      TMOBSAVEDATA *save;
      if (!mobsavedata)
      {
        mobsavedata=(TMOBSAVEDATA **)getmem(sizeof(TMOBSAVEDATA *)*MAX_MOBS);
        memset(mobsavedata,0,sizeof(TMOBSAVEDATA)*MAX_MOBS);
      }
      save=mobsavedata[num];
      if (save==NULL)
      {
        save=mobsavedata[num]=(TMOBSAVEDATA *)getmem(sizeof(TMOBSAVEDATA));
        save->anim_counter=p->anim_counter;
        save->anim_phase=p->anim_phase;
        save->dir=p->dir;
      }
      else
      {
        p->anim_counter=save->anim_counter;
        p->anim_phase=save->anim_phase;
        p->dir=save->dir;
        p->headx=p->locx;
        p->heady=p->locy;
      }
  }
  else
  {
    if (mobsavedata && mobsavedata[num])
    {
      free(mobsavedata[num]);
      mobsavedata[num]=0;
    }
  }


}

static void fill_drw_enemy_struct(TMOB *m, DRW_ENEMY *enm, char *buff, int curdir) {
    if (!show_mob_info) {
        enm->more_info = NULL;
        return;
    }
    char flag_buff[100] = "";
    char strategy_buff[100] = "";
    const char *strategies[] = {
            "WALK","WATCH","LISTEN","BIG","GUARD","PICK","PICKING","ROGUE"
    };
    const char *flags[] = {
            "BATTLE","PASSABLE","SENS","MOB","RESPWN","CAST","SNDLP","LIVE"
    };
    const char *modes[] = {"NONE","CAST","FLEE","ATTACK"};

    const char *actions[] = {
            "STANDFWD","STANDLEFT","STANDBACK","STANDRIGHT","STANDLEFTMIRR",
            "WALKFWD","WALKLEFT","WALKBACK","WALKRIGHT","WALKLEFTMIRR",
            "ATTACK","ATTACKMIRR","INPAIN","INPAINMIRR","ERR"};

    TENEMY_FACE fc = get_enemy_face(m,m->dir,m->anim_phase,curdir);
    int action = countof(actions)-1;
    int stand = (fc.face & 0xF) == 0;
    if (fc.pos < 4) {
        action = (fc.mirror?4:fc.pos) + (stand?0:5);
    } else {
        action = ((fc.pos - 4)*2+(fc.mirror?1:0))+10;
    }

    for (int i = 0; i < 8; i++) {
        if (m->stay_strategy & (1<<i)) {
            strcat(strategy_buff,strategies[i]);strcat(strategy_buff," ");
        }
        if (m->vlajky & (1<<i)) {
            strcat(flag_buff,flags[i]);strcat(flag_buff," ");
        }
    }

    sprintf(buff,"name: %s\n"
                 "action: %s\n"
                 "walkdata: %d\n"
                 "sight: %d\n"
                 "reach: %d\n"
                 "hit: %d\n"
                 "intent: %s\n"
                 "strategy: %s\n"
                 "flags: %s\n"
                 "has_path: %s",
                m->name,
                actions[action],
                m->walk_data,
                m->dohled,
                m->dosah,
                m->dostal,
                modes[(uint8_t)m->mode],
                strategy_buff,
                flag_buff,
                mob_paths[m - mobs]!=NULL?"YES":"NO"
           );
    enm->more_info = buff;
}


void draw_mob_call(int num,int curdir,int celx,int cely,char shiftup)
  {
  TMOB *p,*q;
  TENEMY_FACE view;
  int vw;
  TENEMY_FACE view2;
  int vw2;
  DRW_ENEMY drw1,drw2;
  char buff[256], buff2[256];

  set_font(H_FLITT5,RGB555(31,31,0));
  CheckMobStoned(num-MOB_START);
  p=&mobs[num-MOB_START];
  shiftup|=(p->stay_strategy & MOB_BIG);

  get_pos(p->locx-128,p->locy-128,&drw1.posx,&drw1.posy,curdir);
  view=get_enemy_face(p,p->dir,p->anim_phase,curdir);
  vw=p->cislo_vzoru+view.face+monster_block;
    if ((p->vlastnosti[VLS_KOUZLA] & SPL_INVIS) && !true_seeing) {
        drw1.txtr = NULL;
        vw = 0;
    } else {
        drw1.txtr = ablock(vw);
    }
  drw1.celx=celx;
  drw1.cely=cely;
  drw1.mirror=view.mirror;
  drw1.adjust=p->adjusting[view.face];
  drw1.shiftup=shiftup;
  drw1.num=p->lives;
  drw1.palette=mob_select_palette(p);
  drw1.stoned= (p->vlastnosti[VLS_KOUZLA] & SPL_STONED)!=0;
  see_monster|=(~p->vlajky & MOB_PASSABLE);
  fill_drw_enemy_struct(p, &drw1, buff, curdir);
  if (p->next)
     {
     CheckMobStoned(p->next-MOB_START);
     q=&mobs[p->next-MOB_START];
     get_pos(q->locx-128,q->locy-128,&drw2.posx,&drw2.posy,curdir);
     view2=get_enemy_face(q,q->dir,q->anim_phase,curdir);
     vw2=view2.face+q->cislo_vzoru+monster_block;
     drw2.shiftup=shiftup;
     drw2.celx=celx;
     drw2.cely=cely;
     alock(vw);
     alock(vw+6*16+5);
        if ((q->vlastnosti[VLS_KOUZLA] & SPL_INVIS) && !true_seeing) {
            drw2.txtr = NULL;
            vw2 = 0;
        } else {
            drw2.txtr = ablock(vw2);
        }
     drw2.mirror=view2.mirror;
     alock(vw2);
     alock(vw2+6*16+5);
     drw2.adjust=q->adjusting[view2.face];
     drw2.num=q->lives;
     drw2.palette=mob_select_palette(q);
     drw2.stoned=(q->vlastnosti[VLS_KOUZLA] & SPL_STONED)!=0;
     see_monster|=(~q->vlajky & MOB_PASSABLE);
     fill_drw_enemy_struct(q, &drw2, buff2, curdir);
     }
  else
     {
     draw_enemy(&drw1);
     return;
     }
  if (drw1.posy>drw2.posy)
     {
     draw_enemy(&drw1);
     draw_enemy(&drw2);
     }
  else
     {
     draw_enemy(&drw2);
     draw_enemy(&drw1);
     }
  aunlock(vw);
  aunlock(vw2);
  aunlock(vw+6*16+5);
  aunlock(vw2+6*16+5);
  }

void draw_mob(int num,int curdir,int celx,int cely,char shiftup)
  {
  int ss=(num<<2);
  num=mob_map[num];
  if (!num) return;
  set_lclip_rclip(celx,cely,map_sides[ss+((curdir+3)&3)].lclip,map_sides[ss+((curdir+1)&3)].lclip);
  draw_mob_call(num,curdir,celx,cely,shiftup);
  }


void otoc_moba(TMOB *p)
  {
  p->walk_data=255;
  rozhodni_o_smeru(p);
  }


static int mob_check_teleport(int sect,int num)
  {
  int i;
  if (!ISTELEPORTSECT(sect)) return sect;
  play_sample_at_sector(H_SND_TELEPOUT,viewsector,sect,0,0);
  add_spectxtr(sect,H_TELEP_PCX,14,1,0);
  sect=map_sectors[sect].sector_tag;
  play_sample_at_sector(H_SND_TELEPOUT,viewsector,sect,0,0);
  add_spectxtr(sect,H_TELEP_PCX,14,1,0);
  if (map_coord[sect].flags & MC_PLAYER)
     {
     THUMAN *h=postavy;
     for(i=0;i<POCET_POSTAV;i++,h++) if (h->sektor==sect) player_hit(h,h->lives,0);
     }
  mobs[num].locx=128;
  mobs[num].locy=128;
  return sect;
  }

void mob_step_next(int num,int sect,int dir,char *change)
  {
  int c,numm,d = 0;
  TMOB *p;

  *change+=128;
  numm=num+MOB_START;
  if (~mobs[num].vlajky & MOB_MOBILE)
     {
     c=mob_map[sect];
     if (c!=numm)
       mobs[c-MOB_START].next=0;
      else mob_map[sect]=mobs[num].next;
     mobs[num].next=0;
     recheck_button(sect,1);
     }
  sect=map_sectors[sect].step_next[dir];
  if (~mobs[num].vlajky & MOB_MOBILE)
     {
     sect=mob_check_teleport(sect,num);
     c=mob_map[sect];
     if (c)
        {
        mobs[num].next=c;
        d=uhni_mobe(1,num,0);
        if (d) d=uhni_mobe(1,c-MOB_START,d);
        else d=uhni_mobe(1,c-MOB_START,-MOB_DIST);
        }
     mob_map[sect]=numm;
     recheck_button(sect,1);
     }
  mobs[num].sector=sect;
  p=&mobs[num];
  p->dir=dir;
  rozhodni_o_smeru(p);
  if (p->next)
     uhni_mobe(0,num,d);
  if (p->stay_strategy & MOB_PICKING)
     {
     for(c=0;c<4;c++) if (map_items[(p->sector<<2)+c]!=NULL) break;
     if (c==4 && pick_item_corpse(p,PK_QUERY)) c=0;
     if (c!=4)
        {
        stop_mob(p);
        p->stay_strategy|=MOB_PICK;
        }
     }
  }

void mob_check(int num,TMOB *p)
  {
  int sect,q,z;

  sect=p->sector;
  q=p->stay_strategy & MOB_BIG;
  z=p->vlajky & MOB_PASSABLE;
  if (p->locy<64) {
     if (mob_check_next_sector(sect,0,q,z)) otoc_moba(p);
     else mob_step_next(num,sect,0,&p->locy);
  } else if (p->locx>191) {
     if (mob_check_next_sector(sect,1,q,z)) otoc_moba(p);
     else mob_step_next(num,sect,1,&p->locx);
  } else if (p->locy>191) {
     if (mob_check_next_sector(sect,2,q,z)) otoc_moba(p);
     else mob_step_next(num,sect,2,&p->locy);
  } else if (p->locx<64) {
     if (mob_check_next_sector(sect,3,q,z)) otoc_moba(p);
     else mob_step_next(num,sect,3,&p->locx);
  }
  if (battle && p->mode!=MBA_FLEE)
     {
     mob_sound_event(p,MBS_WALK);
     stop_mob(p);
     }
  }

/*void mobs_attack(TMOB *p)
  {
  int sect,dir,asect;

  if (p->actions<=0) return;
  neco_v_pohybu=1;
  sect=p->sector;
  dir=p->dir;
  asect=map_sectors[sect].step_next[dir];
  if (!asect || map_sides[sect*4+dir].flags & SD_MONST_IMPS)
     {
     rozhodni_o_smeru(p);
     p->headx=mob_go_x[p->dir];
     p->heady=mob_go_y[p->dir];
     p->anim_phase=0;
     return;
     }
  if (!(map_coord[asect].flags & MC_PLAYER))
     {
     rozhodni_o_smeru(p);
     p->headx=mob_go_x[p->dir];
     p->heady=mob_go_y[p->dir];
     p->anim_phase=0;
     return;
     }
  p->anim_phase=MOB_ATTACK;
  p->csektor=asect;
  viewsector=asect;
  viewdir=(dir+2)&3;
  }*/

void vymaz_zasahy(THE_TIMER *q)
  {
  if (q->userdata[1]!=postavy[q->userdata[0]].dostal) return;
  postavy[q->userdata[0]].dostal=0;
  bott_draw(0);
  }

static int select_drop_inventory_place(TMOB *p) {
   int x,y,pl; 
   if (p->locx>128) x=1;else if (p->locx<128) x=-1;else x=rnd(2)*2-1;
   if (p->locy>128) y=1;else if (p->locy<128) y=-1;else y=rnd(2)*2-1;
   pl=0;if (x>0) pl++;
   if (y>0) pl=3-pl;
   return pl;
}

static int drop_inventory(TMOB *p)
  {
  int i,pl;
  short c[]={0,0};

  for(i=-1;i<MOBS_INV;i++) {
     if (p->inv[i] || (i<0 && p->money))
        {
         pl = select_drop_inventory_place(p);
        if (i<0)
           {
           int z=(int)p->money+(int)(rnd(40)-20)*(int)p->money/(int)100;
           c[0]=create_item_money(z);
           }
        else
           {
           c[0]=p->inv[i];
           p->inv[i]=0;
           }
        push_item(p->sector,pl,c);
        }
      }
   if (destroyed_items) {
      int cnt = count_items_total(destroyed_items);
      if (cnt) {
         int idx = cnt -1;
         pl = select_drop_inventory_place(p);
         push_item(p->sector,pl, destroyed_items+idx);
         destroyed_items[idx] = 0;
      }
   }
  return 0;
  }


void mob_check_death(int num,TMOB *p)
  {
  int sect;

  mob_dostal=0;
  bott_draw(0);
  if (p->lives>0) return;
  SEND_LOG("(GAME) Monster killed ... '%s'",p->name);
  ach_event_kill_monster(p->id);
  sect=p->sector;
  p->vlajky&=~MOB_IN_BATTLE & ~MOB_LIVE;
  free_path(num);
  drop_inventory(p);
  if (mob_map[sect]==num+MOB_START) mob_map[sect]=p->next;
  else
     {
     p=&mobs[mob_map[sect]-MOB_START];
     p->next=0;
     }
  pozdrz_akci();
  }
extern char att_player;

void mob_hit(TMOB *mm, int dostal) {
    int ch;
    int mob_dostal = 0, mob_dostal_pocet = 0;

    if (mm->vlajky & MOB_PASSABLE)
        return;
    if (dostal > mm->vlastnosti[VLS_MAXHIT])
        dostal = mm->vlastnosti[VLS_MAXHIT];
    mm->headx = mm->locx;
    mm->heady = mm->locy;
    mm->lives -= dostal;
    mob_dostal_pocet = dostal;
    mm->dostal += dostal;
    if (dostal > 0)
        mm->vlajky |= MOB_IN_BATTLE;
    //mm->stay_strategy|=MOB_WALK | MOB_WATCH;
    mm->dialog_flags |= 0x2;
    if (mm->lives > mm->vlastnosti[VLS_MAXHIT])
        mm->lives = mm->vlastnosti[VLS_MAXHIT];
    if (log_combat) wzprintf("%s was hit: %d, lives: %d\n", mm->name, dostal, mm->lives);
    if (dostal > 0) {
        send_experience(mm, dostal);
        att_player = select_player;
        if (dostal < mm->lives)
            ch = dostal * 3 / mm->lives;
        else
            ch = 2;
        mob_dostal = ch + 1;
        bott_draw(0);
        if (mm->lives < 1) {
            int xpos = 0;
            switch (viewdir) {
                case 0:
                    xpos = -(mm->locx - 128);
                    break;
                case 1:
                    xpos = -(mm->locy - 128);
                    break;
                case 2:
                    xpos = (mm->locx - 128);
                    break;
                case 3:
                    xpos = (mm->locy - 128);
                    break;
            }
            add_spectxtr(mm->sector, H_KILL, 10, 1, xpos * 23 / 10);
            mm->anim_phase = MOB_DEATH;
        } else
            mm->anim_phase = MOB_TO_HIT;
        mm->anim_counter = 0;
        mm->mode = MBA_NONE;
        mob_sound_event(mm, MBS_HIT);
        battle |= dostal > 0;
        if (vybrana_zbran > -1)  //utok zbrani?
                {
            int druh;
            if (vybrana_zbran != 0)  //neni to utok holyma rukama
                    {
                TITEM *it;
                it = &glob_items[vybrana_zbran - 1];
                druh = it->typ_zbrane;
            } else
                druh = TPW_OST;
            send_weapon_skill(druh);
            vybrana_zbran = -1;
        }
    }
    if (mob_dostal_pocet > 0)
        draw_blood(1, mob_dostal, mob_dostal_pocet);
}


void mob_strelba(TMOB *p)
  {
  int i;
  TITEM *t;

  for(i=0;i<item_count;i++) if (glob_items[i].umisteni==PL_SIP && glob_items[i].druh==TYP_VRHACI) break;
  if (i==item_count)
     {

     display_error("Nestvura nemuze strilet. Neni nadefinovan obekt sipu");
     exit(1);
     }
  t=glob_items+i;
  t->zmeny[VLS_MGSIL_H]=p->vlastnosti[VLS_MGSIL_H];  //adjust zmen v magickem utoku
  t->zmeny[VLS_MGSIL_L]=p->vlastnosti[VLS_MGSIL_L];
  t->zmeny[VLS_MGZIVEL]=p->vlastnosti[VLS_MGZIVEL];
  spell_throw(-((p-mobs)+1),i);
  letici_veci->flags &=~FLY_DESTROY;
  int att = p->vlastnosti[VLS_OBRAT]/5;
  int attack_roll = p->vlastnosti[VLS_UTOK_L]+rnd(p->vlastnosti[VLS_UTOK_H]-p->vlastnosti[VLS_UTOK_L]+1);
  letici_veci->hit_bonus= attack_roll + att;
  if (log_combat) {
      wzprintf("%s shoots: %d (roll %d-%d) + %d (%s/5) = %d\n",
         p->name, attack_roll, p->vlastnosti[VLS_UTOK_L],p->vlastnosti[VLS_UTOK_H],att,texty[13],letici_veci->hit_bonus);
  }
  letici_veci->damage=p->vlastnosti[VLS_DAMAGE];
  p->dostal=0;
  }

static void knock_player_back(THUMAN *p,int dir)
  {
  int sect,sid,nsect;

  sect=p->sektor;sid=sect*4+dir;
  nsect=map_sectors[sect].step_next[dir];
  if (mob_map[nsect]) return;
  destroy_player_map();
  call_macro(sid,MC_EXIT);
  if (map_sides[sid].flags & SD_PLAY_IMPS)
     {
     call_macro(sid,MC_PASSFAIL);
     build_player_map();
     return;
     }
  else call_macro(sid,MC_PASSSUC);
  p->sektor=nsect;
  viewsector=nsect;
  check_postavy_teleport();
  build_player_map();
  recheck_button(nsect,1);
  recheck_button(sect,1);
  redraw_scene();
  hold_timer(TM_BACK_MUSIC,1);
  zooming_backward(ablock(H_BGR_BUFF));
  hold_timer(TM_BACK_MUSIC,0);
  showview(0,0,0,0);
  }

void PodporaStitu(THUMAN *h, short *vlastnosti)
  {
  int pos[]={PO_RUKA_L,PO_RUKA_R};
  int i;
  int factor=h->kondice*100/h->vlastnosti[VLS_KONDIC];
  for (i=0;i<2;i++)
    {
    if (h->wearing[pos[i]]!=0)
      {
      TITEM *it=glob_items+h->wearing[pos[i]]-1;
      if (it->zmeny[VLS_OBRAN_L] || it->zmeny[VLS_OBRAN_H])
        {
        vlastnosti[VLS_OBRAN_L]-=it->zmeny[VLS_OBRAN_L];
        vlastnosti[VLS_OBRAN_H]-=it->zmeny[VLS_OBRAN_H];
        vlastnosti[VLS_OBRAN_L]+=(2*it->zmeny[VLS_OBRAN_L])*factor/100;
        vlastnosti[VLS_OBRAN_H]+=(2*it->zmeny[VLS_OBRAN_H])*factor/100;
        }
      }
    }
  if (factor<20)
    {
    vlastnosti[VLS_OBRAT]=vlastnosti[VLS_OBRAT]*factor/20;
    }
  if (h->kondice) h->kondice--;
  }

void mobs_hit(TMOB *p)
  {
  int asect;
  int obet;
  int i,pocet;
  THE_TIMER *tt;
  int spec;char rr=1;char dead;
  THUMAN *h;
  short vlastnosti[VLS_MAX];

  if (p->mode==MBA_SPELL)
    {
     mob_cast(p->casting,p,p-mobs);
     p->dostal=0;
    }
  else
     if (p->stay_strategy & MOB_ROGUE) mob_strelba(p);
  else
     {
     asect=p->csektor;
     if (!(map_coord[asect].flags & MC_PLAYER)) return;
     pocet=0;
     for(i=0;i<POCET_POSTAV;i++)
     {
     THUMAN *p=&postavy[i];
     if (p->used && p->sektor==asect && p->lives && p->inmaphash == current_map_hash) pocet++;
     }
  if (!pocet) abort();
  obet=1+rnd(pocet);
  for(i=0;obet>0;)
     {
     THUMAN *p;
     i++;
     if (i>=POCET_POSTAV) i=0;
     p=&postavy[i];
     if (p->used && p->sektor==asect && p->lives && p->inmaphash == current_map_hash) obet--;
     }
  h=&postavy[i];
  if (h->utek)
     {
     pocet=10;
     h->utek--;
     }
  memcpy(vlastnosti,h->vlastnosti,sizeof(vlastnosti));
  spec=vlastnosti[VLS_KOUZLA];
  if (game_extras & EX_SHIELD_BLOCKING) PodporaStitu(h,vlastnosti);
  else uprav_podle_kondice(h,&pocet);
  h->dostal=vypocet_zasahu(p->vlastnosti,vlastnosti,pocet,0,0,1);  //vypocet zasahu
  if (h->dostal) p->dostal=0;
  if (spec & SPL_OKO)                    //oko za oko pro hrace
     {
     vybrana_zbran=-1;
     mob_hit(p,h->dostal);
     if (log_combat) wzprintf("%s was hit (eye for an eye): %d\n", p->name, h->dostal);
     mob_check_death(p-mobs,p);
     }
  if (h->dostal && p->vlastnosti[VLS_KOUZLA] & SPL_KNOCK) knock_player_back(h,p->dir);
  if (p->vlastnosti[VLS_KOUZLA] & SPL_DRAIN)  //energy drain pro potvoru
     {
     p->lives+=h->dostal;
     if (p->lives>p->vlastnosti[VLS_MAXHIT])p->lives=p->vlastnosti[VLS_MAXHIT];
     if (log_combat) wzprintf("%s drained HP : %d, lives %d\n", p->name, h->dostal, p->lives);
     }
  dead=player_hit(h,h->dostal,1);
  if (h->lives>h->vlastnosti[VLS_MAXHIT])  h->lives=h->vlastnosti[VLS_MAXHIT];
  if ((spec & SPL_TVAR) && (spec & SPL_OKO)) //nedovolena kombinace
     {
     char s[200];
     h->lives=0;
     sprintf(s,texty[73+(postavy[i].female==1)],postavy[i].jmeno);
     bott_disp_text(s);
     rr=0;
     dead=player_check_death(&postavy[i],0);
     }
  tt=add_to_timer(TM_CLEAR_ZASAHY,100,1,vymaz_zasahy);tt->userdata[0]=i;tt->userdata[1]=postavy[i].dostal;
  if (dead && hlubina_level>0)
    {
    select_player=i;
    vybrana_zbran=-1;
    mob_hit(p,p->lives); //hlubina (potvora je mrtva);
    }
  bott_draw(rr);
     }
  }


void mobs_live(int num)
  {
  TMOB *p;
  int xs,ys;

  p=&mobs[num];
  if (p->vlastnosti[VLS_KOUZLA] & SPL_STONED && p->lives>0)
    {
      p->vlajky &= ~MOB_IN_BATTLE;
      return;
    }
  if (p->sector>=mapsize)
     {
     char buff[256];

     sprintf(buff,"Potvora v neexistujicim sektoru (%d, %d) ",num,p->sector);
     display_error(buff);
     exit(1);
     }
  if (p->headx==p->locx && p->heady==p->locy && !p->anim_phase)
     {
     //zde se bude rozhodovat co dal;
     p->anim_counter=-1;
     stop_track_free(num+256);
     if (battle)
        {
        if (p->mode!=MBA_NONE && p->mode!=MBA_FLEE)
           {
           neco_v_pohybu=1;
           p->anim_phase=MOB_ATTACK;
           p->anim_counter=0;
           mob_sound_event(p,MBS_ATTACK);
           mobs_live(num);
           return;
           }
        if (p->mode==MBA_NONE) return;
        rozhodni_o_smeru(p);
        return;
        }
     if (p->stay_strategy & MOB_PICK)
        {
        if (!seber_predmet(p))  return;
        if (pick_item_corpse(p,PK_PICK)) return;
        p->stay_strategy&=~MOB_PICK;
        }
     if (p->stay_strategy & MOB_WALK)
        {
        if (p->walk_data>=224)
           if (++p->walk_data<255)
           {
           p->anim_counter=-1;
           return;
           }
        p->anim_counter=0;
        rozhodni_o_smeru(p);
        }
     else
        if (mob_map[p->sector]==num+MOB_START && (!p->next) )
        {
        p->headx=128;p->heady=128;
        }
     }
  else
     {
     if (p->anim_phase<MOB_ATTACK)
        {
        int spd;
        if (p->mode==MBA_FLEE)
           {
           int xr=abs(map_coord[p->sector].x-map_coord[viewsector].x);
           int yr=abs(map_coord[p->sector].y-map_coord[viewsector].y);
           if (xr>3 || yr>3) spd=100;else spd=2*p->speed;
           }
        else spd=p->speed;
        xs=p->headx-p->locx;
        ys=p->heady-p->locy;
		if (!(game_extras & EX_WALKDIAGONAL) || p->stay_strategy & MOB_BIG)
		  {
		  if (ys!=0 && xs!=0) {
			 if (p->dir==1 || p->dir==3) xs=0;else ys=0;
		  }}
        if (xs>spd) xs=spd;
        else if (xs<-spd) xs=-spd;
        if (ys>spd) ys=spd;
        else if (ys<-spd) ys=-spd;
        p->locx+=xs;
        p->locy+=ys;
		if (xs!=0 || ys!=0) neco_v_pohybu=1;
        if (p->locx>192 || p->locx<64 || p->locy>192 || p->locy<64) mob_check(num,p);
        }
     p->anim_counter++;
     if (p->anim_phase==MOB_ATTACK)
        {
		neco_v_pohybu=1;
        if (p->anim_counter==p->hit_pos) mobs_hit(p);
        if (p->anim_counter>=p->anim_counts[4])
           {
           if (p->lives<1) p->anim_phase=MOB_TO_HIT;else p->anim_phase=0;
           p->anim_counter=-1;
           p->mode=MBA_NONE;
           }
        }
     else
     if (p->anim_phase==MOB_TO_HIT && p->anim_counter>=p->anim_counts[5])
        {
		neco_v_pohybu=1;
        p->anim_phase=0;
        p->anim_counter=-1;
        mob_check_death(num,p);
        }
     else
     if (p->anim_phase==MOB_DEATH)
        {
		neco_v_pohybu=1;
        if (p->anim_counter==2) mob_check_death(num,p);
        else if (p->anim_counter>12)
           {
           p->anim_phase=0;
           p->anim_counter=-1;
           }
        }
     }
  }

void calc_mobs()
  {
  int i;

  neco_v_pohybu=0;
  for(i=0;i<MAX_MOBS;i++) if (mobs[i].vlajky & MOB_LIVE) mobs_live(i);
  if (folow_mode)
     {
     viewsector=mobs[(uint8_t)folow_mob].sector;
     viewdir=mobs[(uint8_t)folow_mob].dir;
     }
  }

void check_all_mobs()
  {
  int i;
  TMOB *p;
  char b=0;

  for(i=0;i<MAX_MOBS;i++)
     {
     p=&mobs[i];
     if (p->vlajky & MOB_LIVE)
        {
        mob_test_na_bitvu(p);
        if (p->vlajky & MOB_IN_BATTLE) b=1;
        }
     }
  battle=b;
  }

void check_all_mobs_battle() //kontroluje zda je nekdo v battle
  {
  int i;
  TMOB *p;
  char b=0;

  for(i=0;i<MAX_MOBS;i++)
     {
     p=&mobs[i];
     if (p->vlajky & MOB_LIVE)
        if (p->vlajky & MOB_IN_BATTLE)
          b=1;
     }
  battle=b;
  }


#define Hi(x) ((x)>>16)
#define Lo(x) ((x)& 0xffff)

int q_kolik_je_potvor(int sector)
  {
    if (mob_map[sector]) {
        if (mobs[mob_map[sector] - MOB_START].next) {
            return 2;
        }
        else if (mobs[mob_map[sector] - MOB_START].stay_strategy & MOB_BIG) {
            return 2;
        }
        else {
            return 1;
        }
    }
    return 0;
}

void najdi_cestu(word start,word konec,int flag,word **cesta, int iamcnt)
  {
  longint *stack;
  longint *stk_free;
  longint *stk_cur;
  char *ok_flags;

  *cesta=NULL;
  stk_free=stk_cur=stack=getmem(4*(mapsize+2));
  memset(ok_flags=getmem((mapsize+8)/8),0,(mapsize+8)/8);
  ok_flags[start>>3]|=1<<(start & 0x7);
  for(*stk_free++=start;stk_free!=stk_cur;stk_cur++)
     {
     uint8_t i;word s,d=0xFFFF,ss;
     s=(ss=Lo(*stk_cur))<<2;
     for(i=0;i<4;i++) if (!(map_sides[s+i].flags & flag))
        {
        char c;
        word w;
        d=map_sectors[ss].step_next[i];
        c=1<<(d & 0x7);
        w=d>>3;
        if (!(ok_flags[w] & c) && q_kolik_je_potvor(d)<iamcnt)
           {
           ok_flags[w]|=c;
          *stk_free++=d | ((stk_cur-stack)<<16);
           }
        if (d==konec) break;
        }
     if (d==konec) break;
     }
  if (stk_free!=stk_cur)
     {
     int count=0;
     longint *p,*z;
     word *x;

     z=p=stk_free-1;
     while (Lo(*p)!=start)
        {
        int l;
        count++;
        l=*p;
        p=Hi(l)+stack;
        *z--=Lo(l);
        }
     x=*cesta=getmem(count*2+2);
     z++;
     while (count--)
        {
        *x++=(word)(*z++);
        }
     *x++=0;
     }
  free(stack);
  free(ok_flags);
  }

void reakce_na_hluk(int mob,int smer)
  {
  if (mobs[mob].stay_strategy & MOB_LISTEN && !(mobs[mob].vlajky & MOB_IN_BATTLE))
     {
     mobs[mob].dir=smer;
     if (!battle)
        {
        mobs[mob].headx=mob_go_x[smer];
        mobs[mob].heady=mob_go_y[smer];
        }
     mobs[mob].stay_strategy|=MOB_WALK;
     }
  }

void sirit_zvuk(word start)
  {
  longint *stack;
  longint *stk_free;
  longint *stk_cur;
  char *ok_flags;

  stk_free=stk_cur=stack=getmem(4*(mapsize+2));
  memset(ok_flags=getmem((mapsize+8)/8),0,(mapsize+8)/8);
  ok_flags[start>>3]|=1<<(start & 0x7);
  for(*stk_free++=start;stk_free!=stk_cur;stk_cur++)
     {
     uint8_t i;word s,d,ss;
     s=(ss=Lo(*stk_cur))<<2;
     for(i=0;i<4;i++) if (!(map_sides[s+i].flags & SD_SOUND_IMPS))
        {
        char c;
        word w;
        d=map_sectors[ss].step_next[i];
        c=1<<(d & 0x7);
        w=d>>3;
        if (!(ok_flags[w] & c))
           {
           int mob;

           ok_flags[w]|=c;
           *stk_free++=d | ((stk_cur-stack)<<16);
           mob=mob_map[d]-MOB_START;
           if (mob>=0)
              {
              reakce_na_hluk(mob,(i+2)&3);
              if ((mob=mobs[mob].next-MOB_START)>=0) reakce_na_hluk(mob,(i+2)&3);
              }
           }
        }
     }
  free(stack);
  free(ok_flags);
  }

void refresh_mob_map()
  {
  int i,s;

  memset(mob_map,0,mapsize);
  for(i=0;i<MAX_MOBS;i++)
     if (mobs[i].vlajky & MOB_LIVE && ~mobs[i].vlajky & MOB_MOBILE)
        {
        s=mobs[i].sector;
        mobs[i].next=mob_map[s];
        mob_map[s]=i+MOB_START;
        }
  }

char track_mob(int sect,int dir)
  {
  if  (!(map_sides[(sect<<2)+dir].flags & SD_THING_IMPS))
    sect=map_sectors[sect].step_next[dir];else return 0;
  do
     {
     int i=-1;
     int m=mob_map[sect];

     if (map_coord[sect].flags & MC_PLAYER) return 1;
     while (m)
      {
      TMOB *mm=mobs+m-MOB_START;
      if (mm->stay_strategy & MOB_BIG) return 0;
      if (mm->dir!=i && i!=-1) return 0;
      stop_mob(mm);
      if (!mm->next && i==-1)
        if (mm->headx==128 || mm->heady==128) smeruj_moba(mm,1);
      i=mm->dir;m=mm->next;
      }
     if  (!(map_sides[(sect<<2)+dir].flags & SD_THING_IMPS))
       sect=map_sectors[sect].step_next[dir];
      else return 1;
     }
  while (1);
  }


//---------------------------------------------------------------------
/* Nasledujici procedury a funkce se volaji pro chovani potvory v bitve */


typedef struct flee_monster_context {
    word last_sector;
    TMOB *fleeing_mob;

} TFLEE_MONSTER_CONTEXT;

static char valid_sectors(word sector, void *ctx)
  {
  int pp;
  TFLEE_MONSTER_CONTEXT *fmc = (TFLEE_MONSTER_CONTEXT *)ctx;

  fmc->last_sector=sector;
  if (map_coord[sector].flags & MC_MARKED) return 0; //nevyhovujici
  pp=q_kolik_je_potvor(sector);
  if (pp==2) return 0; //moc potvor - nevyhovujici
  if (fmc->fleeing_mob->stay_strategy & MOB_BIG && pp) return 0;
  pp=map_sectors[sector].sector_type;
  if (pp==S_DIRA || ISTELEPORT(pp)) return 0;
  return 1;
  }


char flee_monster_zac(TMOB *m)
  {
  int ss,s;
  int i,j;
  word *cesta,*c,cntr;
  for(j=0;j<POCET_POSTAV;j++)
     if (postavy[j].used && postavy[j].lives && postavy[j].inmaphash == current_map_hash)
     {
     ss=(s=postavy[j].sektor)<<2;
     for(i=0;i<4;i++,ss++)
        if (~map_sides[ss].flags & SD_PLAY_IMPS)
          map_coord[map_sectors[s].step_next[i]].flags |= MC_MARKED; //oznac sektor jako nevyhovujici
     map_coord[s].flags |= MC_MARKED;
     }
  TFLEE_MONSTER_CONTEXT fmc;
  fmc.last_sector = 0;
  fmc.fleeing_mob = m;
  labyrinth_find_path(m->sector,65535,SD_MONST_IMPS,valid_sectors,NULL,&fmc);
  i=labyrinth_find_path(m->sector,fmc.last_sector,SD_MONST_IMPS,valid_sectors,&cesta,&fmc);
  for(j=0;j<mapsize;j++)map_coord[j].flags &= ~MC_MARKED;
  if (!i) return 0;
  for(cntr=0,c=cesta;cntr<6;cntr++,c++) if (!*c) break;
  free_path(m-mobs);
  register_mob_path(m-mobs,cesta);
  m->mode=MBA_FLEE;
  m->headx=m->locx+1;
  m->actions=6;
  m->dostal=0;
  return 1;
  }

char akce_moba_zac(TMOB *m)
  {
  int sect,flg,i,j;
  THUMAN *h;
  char flee;
  int perlives,dper,corlives;

  if (~m->vlajky & MOB_LIVE) return 1;
  dper=m->dostal*100/(m->lives+m->dostal);
  corlives=m->vlastnosti[VLS_MAXHIT]-m->flee_num*(m->vlastnosti[VLS_MAXHIT]-m->lives)/100;
  perlives=(100-m->flee_num)*corlives*q_kolik_je_potvor(m->sector)/(m->vlastnosti[VLS_MAXHIT]);
  perlives+=rnd(m->flee_num);
  dper+=rnd(m->flee_num);
  flee=dper>perlives || m->vlastnosti[VLS_KOUZLA] & SPL_FEAR;
     if (flee)
        {
        if (flee_monster_zac(m)) return 1;
        m->dostal=0;
        }
  if (call_mob_event(m->specproc,SMPR_ATTACK,m))
     {
     mob_walk_sound(m);
     return 0;
     }
  sect=map_sectors[m->sector].step_next[m->dir];
  flg=map_sides[(m->sector<<2)+m->dir].flags;
  if (!(flg & SD_PLAY_IMPS))
     if (map_coord[sect].flags & MC_PLAYER)
       for(i=0;i<POCET_POSTAV;i++)
           {
           THUMAN *p=&postavy[i];

           if (p->used && p->lives && p->sektor==sect && p->inmaphash == current_map_hash)
              {
              if (((m->vlajky & MOB_CASTING) && get_spell_track(m->casting))|| m->stay_strategy & MOB_ROGUE)
                    {stop_all_mobs_on_sector(m->sector);smeruj_moba(m,0);}
              else stop_mob(m);
              viewsector=sect;
              viewdir=(m->dir+2) &3;
              m->csektor=sect;
              if ((m->vlajky & MOB_CASTING)&& rnd(100)<=(unsigned int)m->vlastnosti[VLS_SMAGIE]) m->mode=MBA_SPELL;else m->mode=MBA_ATTACK;
              bott_draw(1);
              return 0;
              }
           }
  for(i=0;i<4;i++)
     {
     sect=map_sectors[m->sector].step_next[i];
     flg=map_sides[(m->sector<<2)+i].flags;
     if (!(flg & SD_MONST_IMPS))
        if (map_coord[sect].flags & MC_PLAYER)
           for(j=0;j<POCET_POSTAV;j++)
              {
              THUMAN *p=&postavy[j];

              if (p->used && p->lives && p->inmaphash == current_map_hash &&p->sektor==sect && !(p->vlastnosti[VLS_KOUZLA] & SPL_INVIS))
                 {
                 m->dir=i;
                 stop_mob(m);
                 return 1;
                 }
              }
     }
  sect=m->sector;
  i=q_vidis_postavu(m->sector,m->dir,m,&j,1);
  h=postavy+i;
  if (i>-1) {
     if (((m->vlajky & MOB_CASTING && get_spell_track(m->casting))|| m->stay_strategy & MOB_ROGUE) &&
        (map_coord[m->sector].x==map_coord[h->sektor].x || map_coord[m->sector].y==map_coord[h->sektor].y)
        && track_mob(m->sector,m->dir))
        {
        m->dir=j;
        stop_all_mobs_on_sector(m->sector);
        if (~m->stay_strategy & MOB_ROGUE)
          m->mode=MBA_SPELL;
        else
          m->mode=MBA_ATTACK;
        smeruj_moba(m,0);
        viewsector=h->sektor;
        viewdir=(m->dir+2) & 3;
        return 0;
        }
     else
         {
         word *cesta;

         najdi_cestu(m->sector,postavy[i].sektor,SD_MONST_IMPS,&cesta,(m->stay_strategy & MOB_BIG)?1:2);
         if (cesta!=NULL)
           {
           for(j=0;j<4 && map_sectors[sect].step_next[j]!=cesta[0];j++);
           m->dir=j & 3;
           free(cesta);
           if (m->dir & 1)m->headx=mob_go_x[m->dir];else m->heady=mob_go_y[m->dir];
           mob_sound_event(m,MBS_WALK);
           return 1;
           }
         else
           return 1;
         }
  }
  rozhodni_o_smeru(m);
  if (m->dir & 1)m->headx=mob_go_x[m->dir];else m->heady=mob_go_y[m->dir];
//  m->headx=mob_go_x[m->dir];
//  m->heady=mob_go_y[m->dir];
  m->vlajky&=~MOB_IN_BATTLE;
  return 1;
  }

void mob_animuj()
  {
  int mob;

  for(mob=0;mob<MAX_MOBS;mob++)
     {
     if (!(mobs[mob].vlajky & MOB_LIVE)) continue;
     mobs_live(mob);
     }
  }

int vyber_potvoru(int sect,int dir,int *z)
  {
  TMOB *m1,*m2;
  int ww;
  int m,ch,x;

  if (map_sides[ww=((sect<<2)+dir)].flags & SD_PLAY_IMPS)
     {
     call_macro(ww,MC_WALLATTACK);
     return -1;
     }
  sect=map_sectors[sect].step_next[dir];
  if (!sect) return -1;
  if (!mob_map[sect]) return -1;
  m=mob_map[sect]-MOB_START;
  ch=mobs[m].next;*z=(ch!=0)+1;
  if (!ch) return m;
  m1=mobs+m;
  ch-=MOB_START;
  m2=mobs+ch;
  switch (dir)
     {
     case 2: if (m1->locy<m2->locy) return m;else if (m1->locy>m2->locy) return ch;break;
     case 1: if (m1->locx<m2->locx) return m;else if (m1->locx>m2->locx) return ch;break;
     case 0: if (m1->locy>m2->locy) return m;else if (m1->locy<m2->locy) return ch;break;
     case 3: if (m1->locx>m2->locx) return m;else if (m1->locx<m2->locx) return ch;break;
     }
  x=rnd(2);
  if (x) return ch;
  return m;
  }

static void knock_mob_back(TMOB *mm,int dir)
  {
  char chk;
  int i,sek,mnum,mms;

  if (call_mob_event(mm->specproc,SMPR_KNOCK,mm)) return;
  mms=mm->sector;mnum=mm-mobs+MOB_START;
  chk=mob_check_next_sector(mms,dir,mm->stay_strategy,mm->vlajky);
  if (chk) return;
  sek=map_sectors[mms].step_next[dir];
  i=mob_map[sek];
  if (mob_map[mms]!=mnum) mobs[mob_map[mms]-MOB_START].next=0;else mob_map[mms]=mm->next;
  if (i)
     {
     mm->next=i;
     uhni_mobe(1,i-1,0);
     }
  else mm->next=0;
  mob_map[sek]=mm-mobs+MOB_START;mm->sector=sek;
  recheck_button(mms,1);
  recheck_button(sek,1);
  }

  static void remove_other_weapon_from_calc(THUMAN *p,  short *vlastnosti, int poz) {
   int otherpoz = poz == PO_RUKA_L?PO_RUKA_R:PO_RUKA_L;
   int itnum = p->wearing[otherpoz];
   if (itnum == 0) return;
   const TITEM *it = glob_items + itnum-1;
   if (it->druh != TYP_UTOC) return;
   int vls[] = {VLS_UTOK_H,VLS_UTOK_L,VLS_DAMAGE};
   for (size_t i = 0; i < countof(vls); ++i) {
      vlastnosti[vls[i]]  -= it->zmeny[vls[i]];
   }
 }


int utok_na_sektor(THUMAN *p,TMOB *mm,int ch,int bonus, int ruka)
  {
  int dostal;
  short vlastnosti[VLS_MAX];
  const short *use_vls = p->vlastnosti;

  if (ruka == PO_RUKA_L || ruka == PO_RUKA_R) {
      memcpy(vlastnosti, p->vlastnosti, sizeof(vlastnosti));
      remove_other_weapon_from_calc(p, vlastnosti, ruka);
      use_vls = vlastnosti;
  }
  dostal=vypocet_zasahu(use_vls,mm->vlastnosti,ch,bonus,0,ruka != PL_OBOUR);
  mob_hit(mm,dostal);
  if (dostal && p->vlastnosti[VLS_KOUZLA] & SPL_KNOCK) knock_mob_back(mm,p->direction);
  if (mm->vlastnosti[VLS_KOUZLA] & SPL_OKO)  //oko za oko pro potvoru
     {
     p->lives-=dostal;
     if (log_combat) wzprintf("%s was hit (eye for an eye): %d\n", p->jmeno, dostal);
     player_check_death(p,0);
     }
  if (dostal)
     {
     mm->dir=(p->direction+2)&3;
     play_sample_at_sector(H_SND_SWHIT1+rnd(2),viewsector,viewsector,0,0);
     if (p->vlastnosti[VLS_KOUZLA] & SPL_DRAIN)
        {
         int drain_roll = rnd(16)+16;
         int drain = dostal*8/drain_roll;
        p->lives+=drain;
        if (log_combat) wzprintf("%s received (live drain): %d(hit) x 8 / %d(drain_roll) = %d HP\n",
                p->jmeno, dostal, drain_roll, drain);
        if (p->lives>p->vlastnosti[VLS_MAXHIT]) p->lives=p->vlastnosti[VLS_MAXHIT];
        }
     }
  else
     {
     dostal=0;
     play_sample_at_sector(H_SND_SWMISS1+rnd(2),viewsector,viewsector,0,0);
     }
  mm->vlajky|=MOB_IN_BATTLE;
  neco_v_pohybu=1;
  return dostal;
  }

void sleep_enemy(char regen)
  {
  int i;

  for(i=0;i<MAX_MOBS;i++)
     {
     TMOB *m=&mobs[i];

     if (m->vlajky & MOB_LIVE && m->lives>0)
        {
        if (regen)
           {
           m->lives+=m->vlastnosti[VLS_HPREG];
           if (m->lives>m->vlastnosti[VLS_MAXHIT]) m->lives=m->vlastnosti[VLS_MAXHIT];
           }
        if (m->stay_strategy & MOB_WALK)
           {
           m->locx=m->headx;
           m->locy=m->heady;
           mob_check(i,m);
           if (m->locx<64 || m->locx>192) m->locx=128;
           if (m->locy<64 || m->locy>192) m->locy=128;
           if (m->locx==m->headx && m->locy==m->heady) rozhodni_o_smeru(m);
           }
        }

     }
  }

static int mob_mob_alter(int num)
  {
  int i;
  num-=MOB_START;
  if (num<0) return 0xff;
  if (mobs[num].dialog>-1 && ~mobs[num].vlajky & MOB_PASSABLE) start_dialog(mobs[num].dialog,num);
  else if (mobs[num].stay_strategy & MOB_WATCH)
     {
     stop_mob(mobs+num);
     if (!battle) battle=1;
     }
  i=mob_mob_alter(mobs[num].next);
  return (mobs[num].vlajky & MOB_PASSABLE) & i;
  }

int mob_alter(int sect)
  {
  char p;
  att_player=0xff;
  p=mob_mob_alter(mob_map[sect]);
/*  if (p)
    {
    int i;THUMAN *h;
    for (i=0;i<POCET_POSTAV;i++)
      {
      h=postavy+group_sort[i];if (h->used && h->lives && h->groupnum==cur_group)
        {att_player=group_sort[i];break;}
      }
    }*/
  return p;
  }

void regen_all_mobs()
  {
  int i;
  TMOB *m;

  for(i=0,m=mobs;i<MAX_MOBS;i++,m++) if (m->vlajky & MOB_LIVE && m->vlastnosti[VLS_HPREG])
    m->lives=m->vlastnosti[VLS_MAXHIT];
  }


void load_enemy_to_map(int i, int sector, int dir, const TMOB *t) {

    char can_save = 0;
    for (int x = 0; x<MAX_MOBS; ++x) {
        if (strcmp(t->mobs_name, mobs[x].mobs_name) == 0 && !(mobs[x].vlajky2 & MOB_F2_DONT_SAVE)) {
            can_save = 1;
            break;
        }
    }


    mobs[i]=*t;
    if (~mobs[i].vlajky & MOB_MOBILE) mob_map[ sector]=i+MOB_START;
    if (mobs[i].palette>0)mobs[i].palette=rnd(mobs[i].palette);else mobs[i].palette=abs(mobs[i].palette);
    mobs[i].sector=sector;
    mobs[i].dir=dir;
    mobs[i].home_pos=sector;
    mobs[i].vlajky|=MOB_LIVE;
    if (!can_save) mobs[i].vlajky2|=MOB_F2_DONT_SAVE;


    char s[20];

    sprintf(s,"%s.SEQ",mobs[i].mobs_name);
    int h = find_handle(s, NULL);
    int *grptr = &h;
    if (h == -1) {
        grptr = &end_ptr;
        def_handle(grptr[0]++,s,NULL,SR_ENEMIES);
        mobs[i].cislo_vzoru=*grptr-monster_block;
        register_mob_graphics(*grptr,mobs[i].mobs_name,mobs[i].anim_counts,ablock(grptr[0]-1));
        grptr[0]+=6*16;
        register_mob_sounds(*grptr,mobs[i].sounds);
        grptr[0]+=4;
        sprintf(s,"%s.COL",mobs[i].mobs_name);
        def_handle(grptr[0],s,col_load,SR_ENEMIES);
        grptr[0]++;
    } else {
        mobs[i].cislo_vzoru=(h+1)-monster_block;
    }
}
