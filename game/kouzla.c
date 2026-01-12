#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <libs/bgraph.h>
#include <libs/memman.h>

#include <libs/mgifmem.h>
#include <libs/event.h>
#include <math.h>
#include <libs/bmouse.h>
#include <stdarg.h>
#include "engine1.h"
#include "globals.h"
#include "lang.h"

#include <string.h>

#define S_jmeno 128
#define S_kouzlo 129
#define S_zivel 131
#define S_level 133
#define S_mge 134
#define S_hpnorm_min 135
#define S_hpnorm_max 136
#define S_hpzivl_min 137
#define S_hpzivl_max 138
#define S_vlastnost 139
#define S_vls_kolik 140
#define S_trvani 141
#define S_cil 142
#define S_throw_item 142
#define S_create_item 143
#define S_backfire 144
#define S_povaha 145
#define S_special 146
#define S_pvls  147
#define S_animace 148
#define S_zvuk 149
#define S_wait 150
#define S_set 151
#define S_reset 152
#define S_drain_min 153
#define S_drain_max 154
#define S_accnum 155
#define S_kondice 156
#define S_mana 157
#define S_create_weapon 158
#define S_mana_clip 159
#define S_mana_steal 160
#define S_rand_min 161
#define S_rand_max 162
#define S_location_sector 163
#define S_location_map 164
#define S_location_dir 165
#define S_location_x 166
#define S_location_y 167

#define C_kouzelnik 0
#define C_postava 1
#define C_policko 2
#define C_druzina 2
#define C_policko_pred 3
#define C_mrtva_postava 4
#define C_postava_jinde 5
#define C_nahodna_postava 6
#define C_jiny_cil 7

#define SP_AUTOMAP4  1
#define SP_AUTOMAP8  2
#define SP_AUTOMAP15 3
#define SP_PRIPOJENI1 4
#define SP_PRIPOJENI3 5
#define SP_PRIPOJENIA 6
#define SP_CHVENI 7
#define SP_DEFAULT_EFFEKT 8
#define SP_TRUE_SEEING 9
#define SP_SCORE 10
#define SP_HALUCINACE 11
#define SP_TELEPORT 12
#define SP_SUMMON 13
#define SP_HLUBINA1 14
#define SP_HLUBINA2 15
#define SP_MANABAT 16
#define SP_VAHY  17
#define SP_RYCHLOST 18
#define SP_VIR 19
#define SP_DEMON1 20
#define SP_DEMON2 21
#define SP_DEMON3 22
#define SP_VZPLANUTI1 23
#define SP_VZPLANUTI2 24
#define SP_VZPLANUTI3 25
#define SP_PHASEDOOR 26
#define SP_TELEPORT_SECT 27
#define SP_OPEN_TELEPORT 28

#define SS_invis 1
#define SS_oko 2
#define SS_tvar 4

#define FLG_TRUESEEING 0x10000 //zapnuty TRUESEEING
#define FLG_HLUBINA1   0x20000 //zapnuta HLUBINA pro vsechny
#define FLG_HLUBINA2   0x40000 //zapnuta HLUBINA pro potvory
#define FLG_SCORE      0x80000 //zapnute ukazovani score nad potvorama.
#define FLG_HALUCINACE 0x100000 // zapne halucinaci

static inline word _impl_get_word(unsigned char **c) {
    word r = (*c)[0] + 256* (*c)[1];
    (*c)+=2;
    return r;
}
#define GET_WORD(c) _impl_get_word(&c)

#define MAX_SPELLS 500

char running_anm=0;

char hlubina_level=0;
char dead_food = 0; //cheat - likvidační kouzlu

word *anim_render_buffer;

short teleport_target=0; //cil teleportace

typedef struct tteleportlocation
{
  short loc_x;
  short loc_y;
  const char *map;
  word sector;
  word dir;
}TTELEPLOCATION;

static TTELEPLOCATION TelepLocation;

typedef struct tkouzlo
  {
  word num,um,mge;
  word pc;  //8
  short owner,accnum;     //accnum = akumulacni cislo, owner = kdo kouzlo seslal //4
  int start;  //4
  short cil;  //2  //kladna cisla jsou postavy zaporna potvory (0 je bez urceni postavy)
  char povaha;
  char traceon;    //jinak noanim - neprehravaji se animace a zvuky
  union {
    word backfire; //backfire 
    word demon;     // 1 = demon , 0 = bez demona
  } bkdm;
  word wait;   //wait - cekani pocet animaci
  word delay;  //delay - cekani pocet kol
  char spellname[28];
  word teleport_target;
  }TKOUZLO;

TKOUZLO *spell_table[MAX_SPELLS];
short *vls_table[MAX_SPELLS];   //nove vlastnosti postav
                                   //pokud je cislo vetsi nez 0x7f00 pak dolni byte uvadi percentualni pomer
static int32_t _flag_map[MAX_SPELLS];      //tabulka nastavenych priznaku pro kouzlo.
                               //prvnich 16 bitu je pro postavu
                               //hornich 16 bitu je globalne

short parm1,parm2;
unsigned char twins;

static short rand_value;

static word paleta[256];

void show_full_lfb12e(void *target,const void *buff,const void *paleta);
//#pragma aux show_full_lfb12e parm[edi][esi][ebx] modify [eax ecx]
void show_delta_lfb12e(void *target,const void *buff,const void *paleta);
//#pragma aux show_delta_lfb12e parm[edi][esi][ebx] modify [eax ecx]
char mob_check_next_sector(int sect,int dir,const char alone,char passable);

void call_spell(int i);
int calculatePhaseDoor(int sector, int dir, int um);


static void animace_kouzla(MGIF_HEADER_T *_,int act,const void *data, int ssize)
  {
  switch (act)
     {
     case MGIF_LZW:
     case MGIF_COPY:show_full_lfb12e(anim_render_buffer,data,paleta);break;
     case MGIF_DELTA:show_delta_lfb12e(anim_render_buffer,data,paleta);break;
     case MGIF_PAL:memcpy(paleta,data,sizeof(paleta));paleta[0]|=BGSWITCHBIT;break;
     }
  }


const void *load_spells_legacy_format(const void *p, int32_t *s, int h) {
    TSTRINGTABLE *strtable = lang_load("spells.csv");
    void *np = getmem(*s);
    memcpy(np,p,*s);
    TKOUZLO *k = (np);
    TKOUZLO *end = (TKOUZLO *)((char *)(np) + k->start);
    ++k;
    int count = 1;
    while (k < end) {
        ++count;
        TKOUZLO *end2 = (TKOUZLO *)((char *)(np) + k->start);
        if (end2 < end) end = end2;
        ++k;
    }
    SEND_LOG("(SPELL) Loading spell table: count %d", count);
    k = (np);
    for (int i = 0; i < count; ++i) {
        char *b = (char *)k;
        char traceon = b[offsetof(TKOUZLO, spellname)-1];    //traceon was there;
        size_t bofs = offsetof(TKOUZLO, traceon);
        size_t eofs = offsetof(TKOUZLO, spellname)-1;
        memmove(b+bofs+1, b+bofs, eofs-bofs);\
        k->traceon = traceon;
        const char *new_name = stringtable_find(strtable, i, NULL);
        if (new_name) {
            strcopy_n(k->spellname,new_name,sizeof(k->spellname)-1);
        }
        ++k;
    }
    stringtable_free(strtable);
    return np;
}


static void play_anim(va_list args) //tasked animation
//#pragma aux play_anim parm []
  {
  int block=va_arg(args,int);
#define ANIM_SIZE (320*180*2)
  const void *anm;
  int32_t *l,c;

  if (running_anm)
     {
     SEND_LOG("(ERROR)(ANIM) Animation's mutex is already in use!");
     return;
     }

  anim_render_buffer=getmem(ANIM_SIZE);
  mgif_install_proc(animace_kouzla);
  running_anm=1;
  l=(void *)anim_render_buffer;
  c=ANIM_SIZE/4;do *l++=0x80008000; while (--c);
  alock(block);
  anm=open_mgif(ablock(block));
  c=0;
  SEND_LOG("(ANIM) Buffer is now ready...");
  char f = 1;
  while (f)
     {
     task_wait_event(E_KOUZLO_ANM);
     c++;
     SEND_LOG("(ANIM) Rendering frame %d in animation %xh",c,block);
     f=mgif_play(anm);
     neco_v_pohybu=1;
     }
  task_wait_event(E_KOUZLO_ANM);
  close_mgif(anm);
  running_anm=0;
  free(anim_render_buffer);

  aunlock(block);
  }

void play_big_mgif_animation(int block)
  {
  add_task(2048,play_anim,block);
  task_sleep();
  }

int get_spell_mana(int num)
  {
  TKOUZLO *p;

  p=(TKOUZLO *)ablock(H_KOUZLA);
  return p[num].mge;
  }

int get_spell_um(int num)
  {
  TKOUZLO *p;

  p=(TKOUZLO *)ablock(H_KOUZLA);
  return p[num].um;
  }


int get_spell_used(int num)
  {
  TKOUZLO *p;

  p=(TKOUZLO *)ablock(H_KOUZLA);
  return (p[num].start!=0);
  }

char get_spell_track(int num)
  {
  TKOUZLO *p;

  p=(TKOUZLO *)ablock(H_KOUZLA);
  return (p[num].traceon & 1);
  }

char get_spell_teleport(int num)
  {
  TKOUZLO *p;

  p=(TKOUZLO *)ablock(H_KOUZLA);
  return (p[num].traceon & 2);
  }


int get_spell_color(THUMAN *p,int num)
  {
  TKOUZLO *z;

  z=(TKOUZLO *)ablock(H_KOUZLA);
  z+=num;
  if (!z->start) return 1;
  if (z->mge>p->mana) return 1;
  if (z->um<=p->vlastnosti[VLS_SMAGIE]) return 0;
  if (z->um<=(p->vlastnosti[VLS_SMAGIE]*2)) return 2;
  return 1;
  }

  int get_spell_info(int num, char *txt, size_t sz)
  {
  TKOUZLO *z;

  z=(TKOUZLO *)ablock(H_KOUZLA);
  z+=num;
  if (!z->start) return 1;
  snprintf(txt, sz, "%s %d, %s %d", texty[47],z->um, texty[16], z->mge);
  return 1;
  }

char get_rune_enable(THUMAN *p,int strnum)
  {
  int i;
  for(i=0;i<3;i++) if (get_spell_color(p,strnum+i)!=1) return 1;
  return 0;
  }

char *get_rune_name(int strnum)
  {
  TKOUZLO *z;
  z=(TKOUZLO *)ablock(H_KOUZLA);
  z+=strnum;
  return z->spellname;
  }

void spell_anim(char *name)
  {
  int i;
  i=find_handle(name,NULL);
  if (i==-1) i=end_ptr++;
  def_handle(i,name,NULL,SR_ITEMS);
  add_task(8196,play_anim,i);
  }

void spell_sound(char *name)
  {
  int i;
  i=find_handle(name,wav_load);
  if (i==-1) i=end_ptr++;
  def_handle(i,name,wav_load,SR_ZVUKY);
  play_sample_at_channel(i,0,100);
  }

void get_sector_dir(int cil,word *sector,char *dir)
  {
  if (cil>0)
     {
     cil--;
     if (postavy[cil].used) *sector=postavy[cil].sektor,*dir=postavy[cil].direction;
     }
  else if (cil<0)
     {
     cil=-cil-1;
     *sector=mobs[cil].sector;
     *dir=mobs[cil].dir;
     }
  }

static void spell_vzplanuti3(int ss,int hit,int zivel)
  {
    if (map_coord[ss].flags & MC_PLAYER)
      {
      THUMAN *h;int i;

      for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && h->lives && h->sektor==ss && h->inmaphash == current_map_hash)
        {
        int ochrana=mgochrana(h->vlastnosti[VLS_OHEN+zivel]);
        player_hit(h,hit*ochrana/100,1);
        }
      }
    if (mob_map[ss])
      {
      int i=mob_map[ss];
      while(i)
        {
        TMOB *m=mobs+i-1;
        int ochrana=mgochrana(m->vlastnosti[VLS_OHEN+zivel]);
        vybrana_zbran=-1;
        mob_hit(m,hit*ochrana/100);
        i=m->next;
        }
      }

  }

static void spell_vzplanuti2(THE_TIMER *tt)
  {
  int ss,ss1,ss2,i,dp,dl,du;
  int zivel;

  i=tt->userdata[2];if (i<1) i++;
  du=tt->userdata[1] & 0xff;
  ss=tt->userdata[0];
  zivel=tt->userdata[1] >> 8;
  if (map_sides[(ss<<2)+du].flags & SD_PLAY_IMPS) return;
  ss1=ss2=ss=map_sectors[ss].step_next[du];
  if (ss==0) return;
  dp=(du+1)&3;
  dl=(du+3)&3;
  do
    {
    if (ss1!=0)
      {
      add_spectxtr(ss1,H_ARMAGED,H_ARMA_CNT,1,0);
      spell_vzplanuti3(ss1,tt->userdata[3],zivel);
      }
    if (ss2!=ss1 && ss2!=0)
      {
      add_spectxtr(ss2,H_ARMAGED,H_ARMA_CNT,1,0);
      spell_vzplanuti3(ss2,tt->userdata[3],zivel);
      }
    if (~map_sides[(ss1<<2)+dp].flags & SD_PLAY_IMPS) ss1=map_sectors[ss1].step_next[dp];
    if (~map_sides[(ss2<<2)+dl].flags & SD_PLAY_IMPS) ss2=map_sectors[ss2].step_next[dl];
    }
  while(--i);
  if (tt->userdata[2]) tt->userdata[2]++;
  //tt->userdata[3]=tt->userdata[3]*2/3;
  tt->userdata[0]=ss;
  }

static void spell_vzplanuti(int cil,int count,int hit,char mode,char zivel)
  {
  THE_TIMER *tt;
  int sector = 0,smer = 0;
  int i,o,d;
  if (cil<0)
    {
    TMOB *m=&mobs[-cil-1];
    sector=m->sector;
    smer=m->dir;
    }
  else if (cil>0)
    {
    THUMAN *h=postavy+cil-1;
    sector=h->sektor;
    smer=h->direction;
    }
  if (mode) {o=smer;d=smer+1;}else o=0,d=4;
  for(i=o;i<d;i++)
    {
    THE_TIMER tts;
    if (count>1) tt=add_to_timer(TM_VZPLANUTI,25,count-1,spell_vzplanuti2);
    else tt=&tts;
    tt->userdata[0]=sector;
    tt->userdata[1]=i+(zivel<<8);
    tt->userdata[2]=!mode;
    tt->userdata[3]=hit;
    spell_vzplanuti2(tt);
    }
  neco_v_pohybu=1;
  }

void spell_create(int cil,int what)
  {
  word sector=0;
  char dir = 0;
  short p[2];

  if (dead_food) {
      for (int i = 0; i < MAX_MOBS; ++i) {
          if ((mobs[i].vlajky & (MOB_IN_BATTLE | MOB_LIVE))==(MOB_IN_BATTLE | MOB_LIVE))  {
              mob_hit(mobs+i, mobs[i].lives);
          }
      }
  } else {
      get_sector_dir(cil,&sector,&dir);
      p[0]=what+1; p[1]=0;
      push_item(sector,dir,p);
  }
  }

void spell_create_weapon(int cil,int what)
  {
  THUMAN *h=postavy+cil-1;
  short lr,pr;

  lr=h->wearing[PO_RUKA_L];
  pr=h->wearing[PO_RUKA_R];
  if (lr && glob_items[lr-1].umisteni==PL_OBOUR) pr=lr;
  else if (pr && glob_items[pr-1].umisteni==PL_OBOUR) lr=pr;
  if (lr && pr)
     {
     char s[256];

     sprintf(s,texty[86],h->jmeno);
     bott_disp_text(s);
     return;
     }
  if (!pr) h->wearing[PO_RUKA_R]=what+1;
  else if (!lr) h->wearing[PO_RUKA_L]=what+1;
  prepocitat_postavu(h);
  }

void spell_throw(int cil,int what)
  {
  word sector=0;
  char dir = 0;
  LETICI_VEC *fly;

  get_sector_dir(cil,&sector,&dir);
  fly=create_fly();
  fly->item=what+1;
  fly->items=NULL;
  fly->xpos=-63;
  fly->ypos=0;
  fly->zpos=80;
  fly->speed=32;
  fly->velocity=0;
  fly->flags=FLY_NEHMOTNA | FLY_DESTROY;
  fly->sector=sector;
  fly->smer=dir;
  fly->owner=cil;
  fly->hit_bonus=0;
  fly->damage=0;
  fly->lives=glob_items[what].user_value;
  fly->counter = 0;
  add_fly(fly);
  }


void zmen_vlastnost(int num,int cil,int what,int how)
  {
  if (!how) return;
  if (cil<0)
     {
     cil=-cil-1;
     if (mobs[cil].vlastnosti[what]+how<0) how=-mobs[cil].vlastnosti[what];
     vls_table[num][what]-=how;
     mobs[cil].vlastnosti[what]+=how;
     }
  else if(cil>0)
     {
     THUMAN *p;

     vls_table[num][what]-=how;
     cil--;
     p=&postavy[cil];
     postavy[cil].stare_vls[what]+=how;
     prepocitat_postavu(&postavy[cil]);
     if (p->lives>p->vlastnosti[VLS_MAXHIT]) p->lives=p->vlastnosti[VLS_MAXHIT];
     //if (p->mana>p->vlastnosti[VLS_MAXMANA]) p->lives=p->vlastnosti[VLS_MAXMANA];
     if (p->kondice>p->vlastnosti[VLS_KONDIC]) p->lives=p->vlastnosti[VLS_KONDIC];

     }
  }

void zmen_vlastnost_percent(int num,int cil,int what,int how)
  {
  int x;
  int c;
  if (cil<0)
     {
     c=-cil-1;
     x=mobs[c].vlastnosti[what];
     }
  else
     {
     c=cil-1;
     x=postavy[c].vlastnosti[what];
     }
  x=x*abs(how)/100;if (how<0) x=-x;
  zmen_vlastnost(num,cil,what,x);
  }

char hod_na_uspech(int cil,TKOUZLO *k)
  {
  if (!k->povaha) return 1;
  if (cil)
     {
     int z,zv;
     short *p;

     if (cil<0)
        {
        cil=-cil-1;
        p=mobs[cil].vlastnosti+VLS_OHEN;
        }
     else
        {
        cil--;
        p=postavy[cil].vlastnosti+VLS_OHEN;
        }
     zv=mgochrana(p[k->pc]);
     z=rnd(100);
     return (z<=zv);
     }
  return 0;
  }

void spell_end_global()
  {
  int32_t l=0;
  int i;
  for(i=0;i<MAX_SPELLS;i++) l|=_flag_map[i];
  if (!(l & FLG_TRUESEEING)) true_seeing=0;
  if (!(l & FLG_SCORE)) show_lives=0;
  if (!(l & FLG_HALUCINACE)) set_halucination=0;
  if (l & (FLG_HLUBINA1 | FLG_HLUBINA2))
     {
     hlubina_level=1;
     if (l & FLG_HLUBINA2) hlubina_level<<=1;
     }
  else hlubina_level=0;
  }

static void zmena_na_demona(int hrac,int demon)
  {
  THUMAN *p=postavy+hrac;
  THUMAN *q;

  demon+=3;
  q=New(THUMAN);
  *q=*p;
  *p=postavy_2[demon];
  p->sektor=q->sektor;
  p->direction=q->direction;
  p->groupnum=q->groupnum;
  p->inmaphash=q->inmaphash;
  p->zvolene_akce=q->zvolene_akce;
  q->zvolene_akce = NULL;
  p->provadena_akce = q->provadena_akce;
  q->provadena_akce = NULL;
  p->demon_save=q;
  }

static void zmena_z_demona(int hrac)
  {

  THUMAN *q=postavy+hrac;
  THUMAN *p=q->demon_save;

  p->sektor=q->sektor;
  p->direction=q->direction;
  p->groupnum=q->groupnum;
  p->inmaphash=q->inmaphash;
  p->zvolene_akce = p->provadena_akce = NULL;
  p->programovano = 0;
  *q=*p;
  free(p);
  }

static void zmena_demona(int hrac,int demon,char smer)
  {
  THUMAN *p=postavy+hrac;

  if (postavy[hrac].stare_vls[VLS_KOUZLA] & SPL_DEMON && smer!=0) return;
  if (~postavy[hrac].stare_vls[VLS_KOUZLA] & SPL_DEMON && smer==0) return;
  if (smer!=0) zmena_na_demona(hrac,demon);else zmena_z_demona(hrac);
  if (smer!=0)
     {
     p->lives=p->vlastnosti[VLS_MAXHIT];
     p->kondice=p->vlastnosti[VLS_KONDIC];
     p->mana=p->vlastnosti[VLS_MAXMANA];
     p->jidlo=MAX_HLAD(p);
     p->voda=MAX_ZIZEN(p);
     }
  reg_grafiku_postav();
  }

static void unaffect_after_demon(int cil)
  {
  int i;
  char a;


  SEND_LOG("(SPELLS) Unaffecting after demon...");
  do
     {
     a=0;
     for(i=0;i<MAX_SPELLS;i++) {
       TKOUZLO *spl = spl=spell_table[i];
       if (spl!=NULL && spl->cil==cil && spl->bkdm.demon==1)
       {
           if (spl->wait)
              {
              spl->wait=0;call_spell(i);
              a=1;
              }
           if (spl->cil>0)
              {
              spl->delay=0;call_spell(i);
              a=1;
              }
       }
     }
    }
  while(a);
  }

void spell_end(int num,int ccil,int owner)
  {
  int i,l;
  int cil=ccil;
  for(i=0;i<VLS_MAX;i++) zmen_vlastnost(num,cil,i,vls_table[num][i]);
  free(spell_table[num]);
  spell_table[num]=NULL;
  free(vls_table[num]);
  vls_table[num]=NULL;
  _flag_map[num]&=~SPL_INVIS;
  if (cil>0)
     {
     cil--;
     if (_flag_map[num] & SPL_DEMON)
        {
        unaffect_after_demon(ccil);
        zmena_demona(cil,owner,0);
        _flag_map[num]&=~SPL_DEMON;
        SEND_LOG("(SPELLS) Spell 'Demon' has ended...");
        }
     postavy[cil].stare_vls[VLS_KOUZLA]&=~_flag_map[num];
     if (cil>=0 && cil<POCET_POSTAV)
      {
      prepocitat_postavu(postavy+cil);
      postavy[cil].spell=0;
      }
     bott_draw(0);
     }
  else
  if (cil<0)
     {
     cil=-cil-1;
     mobs[cil].vlastnosti[VLS_KOUZLA]&=~_flag_map[num];
     }
  for(i=0;i<MAX_SPELLS;i++) if ( spell_table[i]!=NULL && spell_table[i]->cil==ccil && ccil>0 &&  spell_table[i]->owner>=0)
     {
     postavy[cil].spell=1;
     bott_draw(0);
     break;
     };
  l=_flag_map[num];
  _flag_map[num]=0;
  if (l>0xffff) spell_end_global();

  }

static void spell_demon(int num,TKOUZLO *spl,int cil,int demon)
  {
  cil--;
  if (postavy[cil].stare_vls[VLS_KOUZLA] & SPL_DEMON) return;
  spl->owner=demon;
  spl->bkdm.demon = 1;
  zmena_demona(cil,demon,1);
  postavy[cil].stare_vls[VLS_KOUZLA]|=SPL_DEMON;
  _flag_map[num]|=SPL_DEMON;
  bott_draw(1);
  }

void spell_hit(int cil,int min,int max,int owner)
  {
  if (cil) {
     if (cil<0)
     {
     TMOB *m;
     cil=-cil-1;
     m=&mobs[cil];
     select_player=owner;
     vybrana_zbran=-1;
     mob_hit(m,min+rnd(max-min));
     }
     else
     {
     THUMAN *h;
     int vysl;

     cil--;
     h=&postavy[cil];
     vysl=min+rnd(max-min+1);
     if (vysl<0)
      {
         if (vysl < 0 && h->lives == 0 && h->sektor == 0) {
             h->sektor = viewsector;
             h->inmaphash = current_map_hash;
         }
          h->lives-=vysl;
          if (h->lives < 0) h->lives = 0;
          if (h->lives > h->vlastnosti[VLS_MAXHIT]) h->lives = h->vlastnosti[VLS_MAXHIT];
          if (h->groupnum==0) h->groupnum=cur_group;
      }
      else player_hit(h,vysl,1);

     bott_draw(0);
     }
  }
  }

void spell_hit_zivel(int cil,int min,int max,int owner,int zivel)
  {
  int ochrana;
  if (cil) {
     if (cil<0)
     {
     TMOB *m;
     cil=-cil-1;
     m=&mobs[cil];
     select_player=owner;
     ochrana=mgochrana(m->vlastnosti[VLS_OHEN+zivel]);
     vybrana_zbran=-1;
     mob_hit(m,(min+rnd(max-min))*ochrana/100);
     }
     else
     {
     THUMAN *h;

     cil--;
     h=&postavy[cil];
     ochrana=mgochrana(h->vlastnosti[VLS_OHEN+zivel]);
     player_hit(h,(min+rnd(max-min))*ochrana/100,0);
     bott_draw(0);
     }
  }
  }


void set_flag(int num,int cil,int flag,int what)
  {
  if (cil>0)
     {
     cil--;
     if (what)
        {
        postavy[cil].stare_vls[VLS_KOUZLA]|=flag;
        _flag_map[num]|=flag;
        }
     else
        {
        postavy[cil].stare_vls[VLS_KOUZLA]&=~flag;
        _flag_map[num]&=~flag;
        }
     zneplatnit_block(cil+H_CHARS);
     prepocitat_postavu(postavy+cil);
     }
  if (cil<0)
     {
     cil=-cil-1;
     if (what)
        {
        mobs[cil].vlastnosti[VLS_KOUZLA]|=flag;
        _flag_map[num]|=flag;
        }
     else
        {
        mobs[cil].vlastnosti[VLS_KOUZLA]&=~flag;
        _flag_map[num]&=~flag;
        }
     }
  }

void spell_automap(int kolik,int cil)
  {
  int x1,y1;
  int xx,yy;
  int i,layer;
  THUMAN *p;

  if (cil<=0) return;
  cil--;
  p=&postavy[cil];
  x1=map_coord[p->sektor].x;
  y1=map_coord[p->sektor].y;
  layer=map_coord[p->sektor].layer;
  for(i=1;i<mapsize;i++) if (map_sectors[i].sector_type!=0)
     {
     int l;
     xx=map_coord[i].x;
     yy=map_coord[i].y;
     l=map_coord[i].layer;
     xx=(x1-xx);yy=(y1-yy);
     if (abs(xx)<=kolik && abs(yy)<=kolik && sqrt(xx*xx+yy*yy)<=kolik && layer==l && ~map_coord[i].flags & MC_NOAUTOMAP)
        map_coord[i].flags|=MC_DISCLOSED;
     }
  }


void spell_pripojeni(int kolik,int cil,int owner)
  {
  int i;
  THUMAN *p;
  int from_sect;

  if (cil<=0) return;
  cil--;
  destroy_player_map();
  p=&postavy[cil];
  from_sect=p->sektor;
  if (map_coord[from_sect].flags & MC_NOSUMMON)
     {
     char *s;

     s=(char *)alloca(strlen(texty[87])+30);
     sprintf(s,texty[87],p->jmeno);
     bott_disp_text(s);
     return;
     }
  p->sektor=postavy[owner].sektor;
  p->inmaphash = postavy[owner].inmaphash;
  kolik--;
  for(i=0;i<POCET_POSTAV && kolik;i++)
     if (postavy[i].groupnum==p->groupnum && postavy[i].sektor==from_sect)
        {
        postavy[i].sektor=viewsector;
        kolik--;
        }
  for(i=0;i<POCET_POSTAV && kolik;i++)
     if (postavy[i].sektor==from_sect)
        {
        p->sektor=viewsector;
        kolik--;
        }
  add_spectxtr(viewsector,H_TELEP_PCX,14,1,0);
  auto_group();
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==viewsector) cur_group=postavy[i].groupnum;
  bott_draw(0);
  build_player_map();
  }

void spell_pripojenia(int owner)
  {
  int i;
  THUMAN *h=NULL;
  char more=0;
  destroy_player_map();
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].used)
     {
     int sect;
     sect=postavy[i].sektor;
     if (sect > 0 && sect < mapsize && map_coord[sect].flags & MC_NOSUMMON) {
        if (h==NULL) h=postavy+i;else more=1;
     } else {
        postavy[i].sektor=postavy[owner].sektor;
        postavy[i].direction=postavy[owner].direction;
        postavy[i].inmaphash = postavy[owner].inmaphash;
        }
      }
  auto_group();
  if (more) bott_disp_text(texty[89]);
  else if (h!=NULL)
     {
     char *s;

     s=(char *)alloca(strlen(texty[87])+30);
     sprintf(s,texty[87],h->jmeno);
     bott_disp_text(s);
     }
  bott_draw(0);
  build_player_map();
  }

void spell_teleport(int cil,int owner, int teleport_target)
  {
  if (teleport_target==-1)
  {
    int sektor;
    int dir;
    int um = 255;

        if (cil > 0) {
            sektor = postavy[cil - 1].sektor;
            dir = postavy[cil - 1].direction;
        } else if (cil < 0) {
            sektor = mobs[-cil - 1].sector;
            dir = mobs[-cil - 1].dir;
        } else {
            return ;
        }
        if (owner >= 0) {
            um = postavy[owner].vlastnosti[VLS_SMAGIE];
        }
        teleport_target = calculatePhaseDoor(sektor, dir, um);
    }
  if (map_coord[teleport_target].flags & MC_NOSUMMON)
     {
     if (owner>=0) bott_disp_text(texty[88]);
     return;
     }
  if (cil>0)
     {
     if (mob_map[teleport_target])
        {
        if (owner>=0) bott_disp_text(texty[85]);
        return;
        }
     destroy_player_map();
     cil--;
     postavy_teleport_effect(teleport_target,postavy[cil].direction,1<<cil,owner==cil);
     cur_group=postavy[cil].groupnum;
     build_player_map();
     add_to_group(cil);
     if (owner>=0)
        zmen_skupinu(postavy+owner);
     }
  else if (cil<0)
     {
     if (map_coord[teleport_target].flags & MC_PLAYER)
        {
        if (owner>=0) bott_disp_text(texty[85]);
        return;
        }
     cil=-cil-1;
     play_sample_at_sector(H_SND_TELEPOUT,viewsector,mobs[cil].sector,0,0);
     add_spectxtr(mobs[cil].sector,H_TELEP_PCX,14,1,0);
     mobs[cil].sector=teleport_target;
     play_sample_at_sector(H_SND_TELEPOUT,viewsector,teleport_target,0,0);
     add_spectxtr(teleport_target,H_TELEP_PCX,14,1,0);
     mobs[cil].next=0;
     refresh_mob_map();
     }
  //schovej_mysku();
  }

void spell_teleport_sector(int cil,int owner)
{
  if (cil<0)
  {
        if (owner>=0) bott_disp_text(texty[85]);
        return;
  }
  if (cil>0)
  {
    cil--;
    if (TelepLocation.map)
    {
      destroy_player_map();
      if (istrcmp(TelepLocation.map,level_fname)!=0)
      {


        if (cil!=owner) return;
        postavy_teleport_effect(0,0,0,1);
        strcopy_n(loadlevel.name,TelepLocation.map,12);
        loadlevel.name[12]=0;
        loadlevel.start_pos=TelepLocation.sector;
        loadlevel.dir=TelepLocation.dir;
        save_map=1;
        send_message(E_CLOSE_MAP);
        battle=0;

      }
      else
      {
        postavy_teleport_effect(TelepLocation.sector,TelepLocation.dir,1<<cil,owner==cil);
        cur_group=postavy[cil].groupnum;
        build_player_map();
        add_to_group(cil);
        if (owner>=0)
          zmen_skupinu(postavy+owner);
      }
    }
    else
    {
      int sector=postavy[cil].sektor;
      int dir=postavy[cil].direction;
      int x=map_coord[sector].x;
      int y=map_coord[sector].y;
      int stpx=0,stpy=0,diffx=0,diffy=0;
      switch (dir)
      {
      case 0: stpy=-1;diffx=1;break;
      case 1: stpx=1;diffy=1;break;
      case 2: stpy=1;diffx=-1;break;
      case 3: stpx=-1;diffy=-1;break;
      };
      {
        int newx=x+TelepLocation.loc_x*stpx+TelepLocation.loc_y*diffx;
        int newy=y+TelepLocation.loc_x*stpy+TelepLocation.loc_y*diffy;
        int i;
        int dist;
        int nearest=0;
        int nearestdst=0x7FFFFFFF;

        for (i=1;i<mapsize;i++) if (map_coord[i].flags & MC_AUTOMAP)
        {
          x=map_coord[i].x-newx;
          y=map_coord[i].y-newy;
          dist=x*x+y*y;
          if(dist<nearestdst)
          {
            nearestdst=dist;
            nearest=i;
          }
        }
        sector=nearest;
      }
     if (map_coord[sector].flags & MC_NOSUMMON)
      {
        if (owner>=0) bott_disp_text(texty[88]);
        return;
      }
      destroy_player_map();
      postavy_teleport_effect(sector,postavy[cil].direction,1<<cil,owner==cil);
      cur_group=postavy[cil].groupnum;
      build_player_map();
      add_to_group(cil);
      if (owner>=0)
        zmen_skupinu(postavy+owner);
    }
  }

}

static void spell_summon(int cil)
  {
  short sector = 0,i,rn,rno,slc;
  int stdir,p;

  if (cil>0) sector=postavy[cil-1].sektor;
  if (cil<0) sector=mobs[-cil-1].sector;
  for(i=0;i<MAX_MOBS;i++)
     {
     TMOB *m=&mobs[i];

     if (~m->vlajky & MOB_LIVE && ~m->vlajky & MOB_RELOAD) break;
     }
  if (i==MAX_MOBS) return;
  slc=i;
  rno=rn=rnd(256)+1;
  do
     {
     for(i=0;i<MAX_MOBS;i++)
        {
        TMOB *m=mobs+i;
        if ((m->stay_strategy & (MOB_WATCH|MOB_WALK)) == (MOB_WATCH|MOB_WALK) &&
           (m->vlajky & MOB_LIVE) && (~m->vlajky & MOB_PASSABLE) && (~m->vlajky & MOB_MOBILE))
           {
           rn--;if (!rn) break;
           }
        }
     }
  while (i==MAX_MOBS && rn!=rno);
  if (i==MAX_MOBS) return;
  memcpy(mobs+slc,mobs+i,sizeof(TMOB));
  p=map_coord[sector].flags & MC_PLAYER;
  if (!p)
     {
     int m;
     m=mob_map[sector]-1;
     p=mobs[m].stay_strategy & MOB_BIG || mobs[m].next;
     }
  if (p)
     {
     int i;
     stdir=rnd(4);

     for(i=0;i<4;i++,stdir=(stdir+1)&3)
        if (!mob_check_next_sector(sector,stdir,mobs[slc].stay_strategy,0)) break;
     if (i==4)
        {
        mobs[slc].vlajky&=~MOB_LIVE;
        return;
        }
     sector=map_sectors[sector].step_next[stdir];
     }
  mobs[slc].sector=sector;
  if (cil>0) mobs[slc].dir=(postavy[cil-1].direction+2)&3;
  if (cil<0) mobs[slc].dir=mobs[-cil-1].dir;
  refresh_mob_map();
  }

static void spell_manabat(int cil)
  {
  if (cil>0)
     {
     cil--;
     if (postavy[cil].mana_battery>postavy[cil].mana) postavy[cil].mana_battery=postavy[cil].mana;
     else postavy[cil].mana=postavy[cil].mana_battery;
     }
  }

static void spell_vahy_osudu(int zivel,char povaha)
  {
  int i;
  int min;
  THUMAN *h;
  TMOB *m;

  min=32767;
  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
     if (h->used && h->lives && min>h->lives && h->sektor == viewsector && h->inmaphash == current_map_hash) min=h->lives;
  for(i=0,m=mobs;i<MAX_MOBS;m++,i++)
     if (m->vlajky & MOB_LIVE && m->vlajky & MOB_IN_BATTLE && m->lives<min) min=m->lives;
  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && h->lives  && h->sektor == viewsector && h->inmaphash == current_map_hash)
     {
     int obr=mgochrana(h->vlastnosti[VLS_OHEN+zivel]);
     if (!povaha || (int)rnd(100)<=obr)
           {
           h->lives=min;
           display_spell_in_icone(H_SPELLDEF,1<<i);
           }
     }
  for(i=0,m=mobs;i<MAX_MOBS;m++,i++)
     if (m->vlajky & MOB_LIVE && m->vlajky & MOB_IN_BATTLE && m->lives<min)
        {
        int obr=mgochrana(m->vlastnosti[VLS_OHEN+zivel]);
        if (!povaha || (int)rnd(100)<=obr)
           m->lives=min;
        }
  bott_draw(1);
  }

static void spell_open_teleport(int cil, int owner)
{
  int sector;
  int dir;
  if (cil<0) {sector=mobs[-cil-1].sector;dir=mobs[-cil-1].dir;}
  else if (cil>0) {sector=postavy[cil-1].sektor;dir=postavy[cil-1].direction;}
  else return;

  if (map_sectors[sector].step_next[dir] && (~map_sides[sector*4+dir].flags & SD_THING_IMPS))
    sector=map_sectors[sector].step_next[dir];

  if (map_coord[sector].flags & MC_NOSUMMON)
  {
      if (owner>=0) bott_disp_text(texty[88]);
      return;
  }

  if (mob_map[sector])
  {
      if (owner>=0) bott_disp_text(texty[85]);
      return;
  }


  if (map_sectors[sector].sector_type>=S_USERTELEPORT &&
    map_sectors[sector].sector_type<=S_USERTELEPORT_END)
  {
    int i;
    int otherside;
    map_sectors[sector].sector_type-=S_USERTELEPORT ;
    otherside=map_sectors[sector].sector_tag;
    for (i=0;i<4;i++)  {map_sides[sector*4+i].flags&=~SD_SEC_VIS;map_sides[sector*4+i].sec_anim=0;map_sides[sector*4+i].sec=0;}
    map_sectors[sector].sector_tag=0;
    add_spectxtr(sector,H_TELEP_PCX,14,1,0);
    play_sample_at_sector(H_SND_TELEPOUT,viewsector,sector,0,0);
    if (otherside!=sector && otherside)
    {
      map_sectors[otherside].sector_type-=S_USERTELEPORT ;
      for (i=0;i<4;i++)  {map_sides[otherside*4+i].flags&=~SD_SEC_VIS;map_sides[otherside*4+i].sec_anim=0;map_sides[otherside*4+i].sec=0;}
      map_sectors[otherside].sector_tag=0;
      add_spectxtr(otherside,H_TELEP_PCX,14,1,0);
      play_sample_at_sector(H_SND_TELEPOUT,viewsector,otherside,0,0);
    }
  }
  else
  {
    int type=map_sectors[sector].sector_type;
    int i;
    char allowed=1;
    if (type!=S_NORMAL && (type<S_SMER || type>=S_VODA)) allowed=0;
    if (allowed && map_sectors[sector].sector_tag!=0) allowed=0;
    if (allowed)
      for (i=0;i<4;i++)
        if (map_sides[sector*4+i].sec) allowed=0;
    if (allowed)
    {
      TSTENA *st,*stt;
      int templateSect=0;
      int i,j;
      for (i=0;templateSect==0 && i<mapsize;i++)
      {
        if (ISTELEPORTSECT(i))
          for (j=0;j<4 && templateSect==0;j++)
          {
            int sd=i*4+j;
            if (map_sides[sd].sec && (map_sides[sd].flags & SD_SEC_VIS) && map_sides[sd].sec_anim!=0)
              templateSect=i;
          }
      }
      if (templateSect==0)
      {
        if (owner>=0) bott_disp_text(texty[85]);
        return;
      }
      map_sectors[sector].sector_type+=S_USERTELEPORT;
      st=map_sides+4*sector;
      stt=map_sides+4*templateSect;
      for (i=0;i<4;i++)
      {
        st->flags|=SD_SEC_VIS|SD_SEC_ANIM;
        st->sec_anim=stt->sec_anim;
        st->sec=stt->sec;
        st++;
      }
      for (i=0;i<mapsize;i++)
        if (map_sectors[i].sector_type>=S_USERTELEPORT && map_sectors[i].sector_type<=S_USERTELEPORT_END)
          if (map_sectors[i].sector_tag==i) break;
      if (i!=mapsize)
      {
        map_sectors[sector].sector_tag=i;
        map_sectors[sector].side_tag=map_sectors[i].side_tag;
        map_sectors[i].sector_tag=sector;
        map_sectors[i].side_tag=(dir+2)&3;
      }
      else
      {
        map_sectors[sector].sector_tag=sector;
        map_sectors[sector].side_tag=(dir+2)&3;
      }
      add_spectxtr(sector,H_TELEP_PCX,14,1,0);
      play_sample_at_sector(H_SND_TELEPOUT,viewsector,sector,0,0);
    }
    else
      if (owner>=0) bott_disp_text(texty[85]);
  }
}

static void spell_rychlost(int num,int cil)
  {
  short *c;
  if (cil>0) c=postavy[cil-1].vlastnosti;
  else c=mobs[-cil-1].vlastnosti;
  if (c[VLS_POHYB]<15) zmen_vlastnost(num,cil,VLS_POHYB,15-c[VLS_POHYB]);
  }

void spell_special(int num,TKOUZLO *spl,int spc)
  {
  switch (spc)
     {
     case SP_AUTOMAP4:spell_automap(4,spl->cil);break;
     case SP_AUTOMAP8:spell_automap(8,spl->cil);break;
     case SP_AUTOMAP15:spell_automap(15,spl->cil);break;
     case SP_PRIPOJENI1:spell_pripojeni(1,spl->cil,spl->owner);break;
     case SP_PRIPOJENI3:spell_pripojeni(3,spl->cil,spl->owner);break;
     case SP_PRIPOJENIA:spell_pripojenia(spl->owner);break;
     case SP_CHVENI:chveni(100);break;
     case SP_DEFAULT_EFFEKT:
                 if (spl->cil>0) display_spell_in_icone(H_SPELLDEF,1<<(spl->cil-1));
                 break;
     case SP_TRUE_SEEING: true_seeing=1;_flag_map[num]|=FLG_TRUESEEING;break;
     case SP_SCORE:show_lives=1;_flag_map[num]|=FLG_SCORE;break;
     case SP_HALUCINACE:set_halucination=1;_flag_map[num]|=FLG_HALUCINACE;
                       hal_sector=rnd(mapsize-1)+1;hal_dir=rnd(4);
                       break;
     case SP_TELEPORT:if (hod_na_uspech(spl->cil,spl)) spell_teleport(spl->cil,spl->owner,spl->teleport_target);break;
     case SP_PHASEDOOR:if (hod_na_uspech(spl->cil,spl)) spell_teleport(spl->cil,spl->owner,-1);break;
     case SP_SUMMON: spell_summon(spl->cil); break;
     case SP_HLUBINA1:if (hlubina_level==0) hlubina_level=1;_flag_map[num]|=FLG_HLUBINA1;
                      break;
     case SP_HLUBINA2:hlubina_level=2;_flag_map[num]|=FLG_HLUBINA2;
                      break;
     case SP_MANABAT:spell_manabat(spl->cil);
                      break;
     case SP_VAHY:spell_vahy_osudu(spl->pc,spl->povaha);break;
     case SP_RYCHLOST:spell_rychlost(num,spl->cil);break;
     case SP_DEMON1:spell_demon(num,spl,spl->cil,0);break;
     case SP_DEMON2:spell_demon(num,spl,spl->cil,1);break;
     case SP_DEMON3:spell_demon(num,spl,spl->cil,2);break;
     case SP_VZPLANUTI1:spell_vzplanuti(spl->cil,1,rand_value,1,spl->pc);break;
     case SP_VZPLANUTI2:spell_vzplanuti(spl->cil,5,rand_value,1,spl->pc);break;
     case SP_VZPLANUTI3:spell_vzplanuti(spl->cil,5,rand_value,0,spl->pc);break;
     case SP_TELEPORT_SECT: if (hod_na_uspech(spl->cil,spl)) spell_teleport_sector(spl->cil,spl->owner);break;
     case SP_OPEN_TELEPORT: spell_open_teleport(spl->cil,spl->owner);break;
     }
  }

void spell_drain(TKOUZLO *p,int cil,int min,int max)
  {
  int drw;
  int sect,dir;

  drw=min+rnd(max-min);
  if (cil>0)
     {
     cil--;
     sect=postavy[cil].sektor;
     dir=postavy[cil].direction;
     if (map_sides[(sect<<2)+dir].flags & SD_PLAY_IMPS)
        {
        postavy[cil].lives-=drw;
        player_check_death(&postavy[cil],0);
        }
     else
        {
        int chaos;int potvora,ochrana;
        TMOB *m;

        potvora=vyber_potvoru(sect,dir,&chaos);
        if (potvora==-1) return;
        m=mobs+potvora;
        ochrana=mgochrana(m->vlastnosti[VLS_OHEN+p->pc]);
        drw=ochrana*drw/100;
        vybrana_zbran=-1;
        mob_hit(m,drw);
        battle=1;
        postavy[cil].lives+=drw/4;
        if (postavy[cil].lives>postavy[cil].vlastnosti[VLS_MAXHIT])
           postavy[cil].lives=postavy[cil].vlastnosti[VLS_MAXHIT];
        }
     }
  }

static void set_kondice_mana(int kolik,TKOUZLO *p,int what,char clip)
  {
  int cil=p->cil;
  if (cil>0)
     {
     THUMAN *p;
     cil--;
     p=postavy+cil;
     if (what==S_kondice)
        {
        p->kondice+=kolik;
        if (p->kondice<0) p->kondice=0;
        if (clip)
         if (p->kondice>p->vlastnosti[VLS_KONDIC]) p->kondice=p->vlastnosti[VLS_KONDIC];
        }
     if (what==S_mana)
        {
        p->mana+=kolik;
        if (p->mana<0) p->mana=0;
        if (clip)
         if (p->mana>p->vlastnosti[VLS_MAXMANA]) p->mana=p->vlastnosti[VLS_MAXMANA];
        }
     }
  }

static void spell_mana_steal(int kolik,int cil,int owner)
  {
  if (cil>0)
     {
     THUMAN *h=postavy+cil-1;
     h->mana-=h->vlastnosti[VLS_MAXMANA]*kolik/100;
     if (h->mana<0) h->mana=0;
     }
  if (cil<0)
     {
     if (owner>=0)
        {
        THUMAN *h=postavy+owner;
        if (h->mana<h->vlastnosti[VLS_MAXMANA]) h->mana+=h->vlastnosti[VLS_MAXMANA]*kolik/100;
        }
     }
  }

static void calc_rand_value(int val1,int val2)
  {
  rand_value=val1+rnd(val2-val1+1);
  }

void call_spell(int i)
  {
  TKOUZLO *p;
  unsigned char *c;
  unsigned char *start;
  int z;
  unsigned char ext=0;
  int cil;


  p=spell_table[i];
  if (p==NULL) return;
  cil=p->cil;
  if (cil>0)
     {
     cil--;
     if (postavy[cil].vlastnosti[VLS_KOUZLA] & SPL_DEMON && !p->bkdm.demon && !p->delay)
        {
        p->delay=1;        
        }
     }
  if (p->delay) return;
  if (p->wait) return;
  c=(unsigned char *)ablock(H_KOUZLA);
  start = c;
  c+=p->start;
  twins=0;
  do {
      twins = twins == 3 ? 0 : twins;
        switch (*c++) {
            case S_zivel:
                p->pc = GET_WORD(c)
                ;
                if (p->owner >= 0
                        && !GlobEvent(MAGLOB_ONFIREMAGIC + p->pc,
                                postavy[p->owner].sektor,
                                postavy[p->owner].direction)) {
                    spell_end(i, p->cil, p->owner);
                    return;
                }
                break;
            case S_hpnorm_min:
                parm1 = GET_WORD(c)
                ;
                twins |= 1;
                if (twins == 3)
                    spell_hit(p->cil, parm1, parm2, p->owner);
                break;
            case S_hpnorm_max:
                parm2 = GET_WORD(c)
                ;
                twins |= 2;
                if (twins == 3)
                    spell_hit(p->cil, parm1, parm2, p->owner);
                break;
            case S_hpzivl_min:
                parm1 = GET_WORD(c)
                ;
                twins |= 1;
                if (twins == 3)
                    spell_hit_zivel(p->cil, parm1, parm2, p->owner, p->pc);
                break;
            case S_hpzivl_max:
                parm2 = GET_WORD(c)
                ;
                twins |= 2;
                if (twins == 3)
                    spell_hit_zivel(p->cil, parm1, parm2, p->owner, p->pc);
                break;
            case S_vlastnost:
                parm1 = GET_WORD(c)
                ;
                twins |= 1;
                if (twins == 3 && (hod_na_uspech(p->cil, p)))
                    zmen_vlastnost(i, p->cil, parm1, parm2);
                break;
            case S_vls_kolik:
                parm2 = GET_WORD(c)
                ;
                twins |= 2;
                if (twins == 3 && (hod_na_uspech(p->cil, p)))
                    zmen_vlastnost(i, p->cil, parm1, parm2);
                break;
            case S_trvani:
                p->delay = GET_WORD(c)
                ;
                p->wait = 0;
                ext = 1;
                break;
            case S_throw_item:
                z = GET_WORD(c)
                ;
                spell_throw(p->cil, z);
                break;
            case S_create_item:
                z = GET_WORD(c)
                ;
                spell_create(p->cil, z);
                break;
            case S_create_weapon:
                z = GET_WORD(c)
                ;
                spell_create_weapon(p->cil, z);
                break;
            case S_animace:
                if (p->owner >= 0 && !p->traceon)
                    spell_anim((char*) c);
                c = (unsigned char*) strchr((char*) c, 0);
                c++;
                break;
            case S_zvuk:
                if (p->owner >= 0 && !p->traceon)
                    spell_sound((char*) c);
                c = (unsigned char*) strchr((char*) c, 0);
                c++;
                break;
            case S_wait:
                p->wait = GET_WORD(c)
                ;
                if (p->owner >= 0)
                    ext = 1;
                break;
            case 0xff:
                spell_end(i, p->cil, p->owner);
                return;
            case S_pvls:
                parm2 = GET_WORD(c)
                ;
                twins |= 2;
                if (twins == 3 && (hod_na_uspech(p->cil, p)))
                    zmen_vlastnost_percent(i, p->cil, parm1, parm2);
                break;
            case S_set:
                parm2 = GET_WORD(c)
                ;
                if (hod_na_uspech(p->cil, p))
                    set_flag(i, p->cil, parm2, 1);
                break;
            case S_reset:
                parm2 = GET_WORD(c)
                ;
                if (hod_na_uspech(p->cil, p))
                    set_flag(i, p->cil, parm2, 0);
                break;
            case S_special:
                parm2 = GET_WORD(c)
                ;
                spell_special(i, p, parm2);
                break;
            case S_drain_min:
                parm1 = GET_WORD(c)
                ;
                twins |= 1;
                if (twins == 3)
                    spell_drain(p, p->cil, parm1, parm2);
                break;
            case S_drain_max:
                parm2 = GET_WORD(c)
                ;
                twins |= 2;
                if (twins == 3)
                    spell_drain(p, p->cil, parm1, parm2);
                break;
            case S_rand_min:
                parm1 = GET_WORD(c)
                ;
                twins |= 1;
                if (twins == 3)
                    calc_rand_value(parm1, parm2);
                break;
            case S_rand_max:
                parm2 = GET_WORD(c)
                ;
                twins |= 2;
                if (twins == 3)
                    calc_rand_value(parm1, parm2);
                break;
            case S_mana:
                parm1 = GET_WORD(c)
                ;
                set_kondice_mana(parm1, p, S_mana, 0);
                break;
            case S_kondice:
                parm1 = GET_WORD(c)
                ;
                set_kondice_mana(parm1, p, S_kondice, 1);
                break;
            case S_mana_clip:
                parm1 = GET_WORD(c)
                ;
                set_kondice_mana(parm1, p, S_mana, 1);
                break;
            case S_mana_steal:
                parm1 = GET_WORD(c)
                ;
                spell_mana_steal(parm1, p->cil, p->owner);
                break;
            case S_location_sector:
                parm1 = GET_WORD(c)
                ;
                TelepLocation.sector = parm1;
                TelepLocation.loc_x = 0;
                TelepLocation.loc_y = 0;
                break;
            case S_location_map:
                TelepLocation.map = (char*) c;
                c = (unsigned char*) strchr((char*) c, 0);
                c++;
                break;
            case S_location_dir:
                parm1 = GET_WORD(c)
                ;
                TelepLocation.dir = parm1;
                break;
            case S_location_x:
                TelepLocation.loc_x = GET_WORD(c)
                ;
                TelepLocation.map = 0;
                break;
            case S_location_y:
                TelepLocation.loc_y = GET_WORD(c)
                ;
                TelepLocation.map = 0;
                break;
            default: {
                const char *d =
                        "Chyba v popisu kouzel: Program narazil na neznamou instrukci %d (%02X) pri zpracovani kouzla s cislem %d. Kouzlo bylo ukon�eno";
                char *buff = alloca(strlen(d) + 20);
                sprintf(buff, d, *(c - 1), *(c - 1), p->num);
                bott_disp_text(buff);
                spell_end(i, p->cil, p->owner);
                return;
            }

        }
    }
  while(!ext);
  p->start=c-start;
  }

int add_spell(int num,int cil,int owner,char noanim)
  {
  int i,nl=-1;
  TKOUZLO *p;
  TKOUZLO *q;
  int accnum;
  char time_acc=1;


  alock(H_KOUZLA);
  q=(TKOUZLO *)ablock(H_KOUZLA)+num;
  accnum=q->accnum;
  if (accnum<0)
     {
     time_acc=0;
     accnum=abs(accnum);
     }
  if (!accnum) accnum=-1;
  for(i=0;i<MAX_SPELLS  && (spell_table[i]==NULL || abs(spell_table[i]->accnum)!=accnum || spell_table[i]->cil!=cil);i++)
     if (spell_table[i]==NULL) nl=i;
  if (i==MAX_SPELLS) i=nl;
  if (i==-1)
     {
     SEND_LOG("(ERROR) Too many spells in game!");
     return -1;
     }
  if (spell_table[i]!=NULL)
     {
     if (!time_acc) return -1;
     spell_end(i,spell_table[i]->cil,spell_table[i]->owner);
     }
  SEND_LOG("(SPELLS) Current spell number %d was assigned to ID number : %d",num,i);
  p=New(TKOUZLO);
  vls_table[i]=NewArr(short,24);
  memset(vls_table[i],0,2*24);
  memcpy(p,q,sizeof(TKOUZLO));
  p->cil=cil;
  p->num=num;
  p->owner=owner;
  p->traceon=noanim;
  p->teleport_target=teleport_target;
  if (cil>0) p->bkdm.demon=(postavy[cil-1].stare_vls[VLS_KOUZLA] & SPL_DEMON)!=0;
  aunlock(H_KOUZLA);
  spell_table[i]=p;
  if (cil>0 && owner>=0) postavy[cil-1].spell=1;
  call_spell(i);
  return i;
  }

void kouzla_kola(EVENT_MSG *msg,void **unused)
  {
  unused;
  if (msg->msg==E_KOUZLO_KOLO)
     {
     int i;

     for(i=0;i<MAX_SPELLS;i++)
        if (spell_table[i]!=NULL)
           if (spell_table[i]->delay)
              {
              neco_v_pohybu=1;
              if (!(--spell_table[i]->delay)) call_spell(i);
              }
     }
  }

void kouzla_anm(EVENT_MSG *msg,void **unused)
  {
  unused;
  if (msg->msg==E_KOUZLO_ANM)
     {
     int i;

     for(i=0;i<MAX_SPELLS;i++)
        if (spell_table[i]!=NULL)
           if (spell_table[i]->wait)
              {
              neco_v_pohybu=1;
              if (!(--spell_table[i]->wait)) call_spell(i);
              }
     }
  }


char add_group_spell(int num,int sector,int owner,int mode,char noanim)
  {
  char c=1;
  if (mob_map[sector])
     {
     int m;
     m=mob_map[sector];
     add_spell(num,-m,owner,noanim);
     c=0;
     if (mobs[m-1].next) add_spell(num,-mobs[m-1].next,owner,noanim);
     }
  if (map_coord[sector].flags & MC_PLAYER)
     {
     int i,j;
     if (mode==C_nahodna_postava)
        {
        i=0;j=rnd(POCET_POSTAV)+1;
        do
           {
           if (i>=POCET_POSTAV) i=0;
           if (postavy[i].sektor==sector) j--;
           if (j) i++;
           }
        while (j);
        add_spell(num,i+1,owner,noanim);
        }
     else
        {
        for(i=0;i<POCET_POSTAV;i++)
           if (postavy[i].used && postavy[i].sektor==sector && postavy[i].inmaphash == current_map_hash) add_spell(num,i+1,owner,noanim),c=0;
        }
     }
  return c;
  }

char ask_who(int num)
  {
  TKOUZLO *k=((TKOUZLO *)ablock(H_KOUZLA))+num;

  if (k->cil==C_kouzelnik) return 1;
  if (k->cil==C_postava) return 2;
  if (k->cil==C_mrtva_postava) return 3;
  if (k->cil==C_postava_jinde) return 4;
  return 0;
  }


static char get_valid_sector(word sector, void *ctx)
  {
    word *last_sector = (word *)ctx;
    *last_sector=sector;
    return 1;
  }


void cast(int num,THUMAN *p,int owner, char backfire)
  {
  int um,cil,num2;
  TKOUZLO *k;

  if (num>511)
     {
     cil=num>>9;
     num2=num & 511;
     }
  else
     {
     cil=0;
     num2=num;
     }


  SEND_LOG("(SPELLS) Cast num %d cil %d",num2,cil);
  k=((TKOUZLO *)ablock(H_KOUZLA))+num2;


  if (cil>0 && k->cil!=C_postava_jinde)
        {
        THUMAN *h1=postavy+cil-1;
        char s[256];
        if ((h1->lives)
           && ((abs(map_coord[h1->sektor].x-map_coord[p->sektor].x)>5) ||
           (abs(map_coord[h1->sektor].y-map_coord[p->sektor].y)>5) ))
           {
           sprintf(s,texty[37+(h1->female==1)],h1->jmeno,p->jmeno);
           bott_disp_text(s);
           return;
           }
        }
  if (battle && k->traceon & 1 && trace_path(p->sektor,p->direction)==-255) return;
  if (!backfire && p->mana<k->mge) return;
  if (p->vlastnosti[VLS_KOUZLA] & SPL_INVIS)
     {
     p->stare_vls[VLS_KOUZLA]&=~SPL_INVIS;
     prepocitat_postavu(p);
     build_all_players();
     }
  if (!backfire && (um=p->vlastnosti[VLS_SMAGIE])<k->um)
     {
     int per1,per2;
     if (um*2<k->um) return;
     per1=(um-k->um/2)*128/k->um;
     per2=rnd(64);
     if ((per1/2+32)<per2 && (k->bkdm.backfire || (game_extras & EX_RANDOM_BACKFIRES)!=0))
           {
           p->mana-=k->mge;
		   if ((game_extras & EX_RANDOM_BACKFIRES)!=0)
			 {
		       word last_sector = p->sektor;
			 labyrinth_find_path(p->sektor,65535,SD_PLAY_IMPS,get_valid_sector,NULL,&last_sector);
			 teleport_target=last_sector;
			 cast(rand()*105/RAND_MAX+(cil*512),p,p-postavy,1);
			 return;
			 }
           cast(k->bkdm.backfire+(cil<<9),p,owner,1);
           return;
           }
     if(per1<per2)
        {
        p->mana-=k->mge/2;
        return;
        }
     per1=(64-per1)/2;
     per2=rnd(64);
     if(per1>per2) p->stare_vls[VLS_SMAGIE]++;
     }

  if (!GlobEvent(MAGLOB_BEFOREMAGIC,p->sektor,p->direction)) return;
  if (!GlobEvents(MAGLOB_ONSPELLID1,MAGLOB_ONSPELLID9,p->sektor,p->direction,num2)) return;

  if (cil && (k->cil==C_postava || k->cil==C_mrtva_postava || k->cil==C_postava_jinde)) {
      add_spell(num2,cil,owner,0);
  } else
     {
     if (k->cil==C_policko) if (add_group_spell(num2,p->sektor,owner,C_policko,0)) goto end;
     if (k->cil==C_kouzelnik) add_spell(num2,p-postavy+1,owner,0);
     if (k->cil==C_policko_pred || k->cil==C_nahodna_postava)
        {
        int s;
        s=p->sektor;
        if (!(map_sides[(s<<2)+p->direction].flags & SD_PLAY_IMPS) && !backfire)
           s=map_sectors[s].step_next[p->direction];
        if (add_group_spell(num2,s,owner,k->cil,0)) goto end;
        }
     }
  if (!backfire) p->mana-=k->mge;
  p->exp+=k->mge;
  check_player_new_level(p);
  if (p->mana>p->mana_battery) {
     p->mana=p->mana_battery;
  }
  p->mana_battery=32767;
end:
	 GlobEvent(MAGLOB_AFTERMAGIC,p->sektor,p->direction);
  }

void mob_cast(int num,TMOB *m,int mob_num)
  {
  TKOUZLO *k;

  SEND_LOG("(SPELLS) Enemy tries to cast the spell (mob: %d, spell: %d",mob_num,num);
  k=((TKOUZLO *)ablock(H_KOUZLA))+num;
  switch (k->cil)
     {
     case C_postava:
     case C_kouzelnik:add_spell(num,-(mob_num+1),-1,1);break;
     case C_policko:add_group_spell(num,m->sector,-1,C_policko,1);break;
     case C_policko_pred:
     case C_nahodna_postava:
        {
        int s;
        s=m->sector;
        if (!(map_sides[(s<<2)+m->dir].flags & SD_PLAY_IMPS))
           s=map_sectors[s].step_next[m->dir];
        add_group_spell(num,s,-1,k->cil,1);
        }
        break;
     }
  }

int calculatePhaseDoor(int sector, int dir, int um)
{
  int x=map_coord[sector].x;
  int y=map_coord[sector].y;
  int stpx=0,stpy=0,diffx=0,diffy=0;
  switch (dir)
  {
  case 0: stpy=-1;diffx=1;break;
  case 1: stpx=1;diffy=1;break;
  case 2: stpy=1;diffx=1;break;
  case 3: stpx=-1;diffy=1;break;
  };
  {
    int dist=1+rand()*(um/4)/RAND_MAX;
    int difs=rand()*2*dist/RAND_MAX-dist;
    int newx=x+dist*stpx+difs*diffx;
    int newy=y+dist*stpy+difs*diffy;
    int i;
    int nearest=0;
    int nearestdst=0x7FFFFFFF;

    for (i=1;i<mapsize;i++) if (map_coord[i].flags & MC_AUTOMAP)
    {
      x=map_coord[i].x-newx;
      y=map_coord[i].y-newy;
      dist=x*x+y*y;
      if(dist<nearestdst)
      {
        nearestdst=dist;
        nearest=i;
      }
    }
    sector=nearest;
  }
  return sector;
}

void thing_cast(int num,int postava,int sector,TMOB *victim,char noanim)
  {
  TKOUZLO *k;
  THUMAN *h=postavy+postava;
  char telep=get_spell_teleport(num);

  SEND_LOG("(SPELLS) Item casts the spell (sector: %d, spell: %d)",sector,num);
  k=((TKOUZLO *)ablock(H_KOUZLA))+num;
  if (telep)
  {
    teleport_target=calculatePhaseDoor(h->sektor,h->direction,h->vlastnosti[VLS_SMAGIE]);
  }
  switch (k->cil)
     {
     case C_nahodna_postava:
     case C_postava:
     case C_kouzelnik:if (postavy[postava].lives) add_spell(num,postava+1,postava,noanim);break;
     case C_mrtva_postava:if (!postavy[postava].lives) add_spell(num,postava+1,postava,noanim);break;
     case C_policko:add_group_spell(num,sector,postava,k->cil,noanim);break;
     case C_policko_pred:
        {
        int s;
        s=sector;
        if (~map_sides[(s<<2)+h->direction].flags & SD_PLAY_IMPS)
           s=map_sectors[s].step_next[h->direction];
        add_group_spell(num,s,postava,k->cil,noanim);
        }
        break;
     case C_jiny_cil:if (victim!=NULL) add_spell(num,-(victim-mobs+1),postava,noanim);break;
     }
  }

void area_cast(int num,int sector,int owner,char noanim)
  {
  SEND_LOG("(SPELLS) Area casts the spell (sector: %d, spell: %d)",sector,num);
  add_group_spell(num,sector,owner,C_policko,noanim);
  }



void kouzla_init()
  {
  SEND_LOG("(SPELLS) Init...");
  send_message(E_ADD,E_KOUZLO_ANM,kouzla_anm);
  send_message(E_ADD,E_KOUZLO_KOLO,kouzla_kola);
  memset(spell_table,0,sizeof(spell_table));
  memset(vls_table,0,sizeof(vls_table));
  memset(_flag_map,0,sizeof(_flag_map));
  true_seeing=0;
  hlubina_level=0;
  show_lives=0;
  set_halucination=0;
  }

void reinit_kouzla_full()
  {
  int i;

  SEND_LOG("(SPELLS) Reinit...");
  for(i=0;i<MAX_SPELLS;i++) if (spell_table[i]!=NULL) free(spell_table[i]);
  for(i=0;i<MAX_SPELLS;i++) if (vls_table[i]!=NULL) free(vls_table[i]);
  memset(spell_table,0,sizeof(spell_table));
  memset(vls_table,0,sizeof(vls_table));
  memset(_flag_map,0,sizeof(_flag_map));
  true_seeing=0;
  hlubina_level=0;
  show_lives=0;
  set_halucination=0;
  }

void remove_all_mob_spells()
  {
  int i;
  char a;

  SEND_LOG("(SPELLS) Removing spells from enemies...");
  do
     {
     a=0;
     for(i=0;i<MAX_SPELLS;i++)
       if (spell_table[i]!=NULL)
       {
           if (spell_table[i]->wait)
              {
              spell_table[i]->wait=0;call_spell(i);
              a=1;
              }
           if (spell_table[i]->cil<0)
              {
              spell_table[i]->delay=0;call_spell(i);
              a=1;
              }
       }

     }
  while(a);
  }


int save_spells(TMPFILE_WR *f)
  {
  int i,s;

  SEND_LOG("(SPELLS) Saving spell table...");
  for(i=0,s=0;i<MAX_SPELLS;i++)
     if (spell_table[i]!=NULL) s++;
  temp_storage_write(&s,1*sizeof(s),f);
  for(i=0;i<MAX_SPELLS;i++)
     if (spell_table[i]!=NULL)
        {
        temp_storage_write(spell_table[i],1*sizeof(TKOUZLO),f);
        temp_storage_write(vls_table[i],1*2*24,f);
        temp_storage_write(_flag_map+i,1*4,f);
        }
  return 0;
  }

int load_spells(TMPFILE_RD *f)
  {
  char res=0;

  int i,s;

  SEND_LOG("(SPELLS) Loading saved spell table...");
  reinit_kouzla_full();
  res|=(temp_storage_read(&s,1*sizeof(s),f)!=sizeof(s));
  for(i=0;i<s && !res;i++)
     {
     spell_table[i]=New(TKOUZLO);
     vls_table[i]=NewArr(short,24);
     res|=(temp_storage_read(spell_table[i],1*sizeof(TKOUZLO),f)!=sizeof(TKOUZLO));
     res|=(temp_storage_read(vls_table[i],1*2*24,f)!=2*24);
     res|=(temp_storage_read(_flag_map+i,1*4,f)!=4);
     }
 true_seeing=1;
 hlubina_level=2;
 show_lives=1;
 set_halucination=1;
 spell_end_global();
 return res;
  }

void unaffect()
  {
  int i;
  char a;

  SEND_LOG("(WIZARD) Unaffect / dispel_magic");
  do
     {
     a=0;
     for(i=0;i<MAX_SPELLS;i++)
       if (spell_table[i]!=NULL)
        if (~_flag_map[i] & SPL_DEMON)
       {
           if (spell_table[i]->wait)
              {
              spell_table[i]->wait=0;call_spell(i);
              a=1;
              }
           if (spell_table[i]->cil>0)
              {
              spell_table[i]->delay=0;call_spell(i);
              a=1;
              }
       }

     }
  while(a);
  SEND_LOG("(WIZARD) Unaffect... done");
  }

void unaffect_demon(int cil)
  {
  int i;
  TKOUZLO *spl;



  cil++;
  SEND_LOG("(SPELLS) Demon returns to astral spaces...");
  for(i=0;spl=spell_table[i],i<MAX_SPELLS;i++) if (spl!=NULL && _flag_map[i] & SPL_DEMON && spl->cil==cil && spl->bkdm.demon)
     {
     while (spell_table[i]!=NULL)
        {
           if (spell_table[i]->wait)
              {
              spell_table[i]->wait=0;call_spell(i);

              }
           if (spell_table[i]->cil>0)
              {
              spell_table[i]->delay=0;call_spell(i);

              }
        }
     }
  }


