#include <platform/platform.h>

#include <stdio.h>
#include <stdlib.h>
#include <libs/event.h>
#include <platform/sound.h>
#include <libs/bgraph.h>
#include <libs/bmouse.h>

#include <libs/memman.h>
#include <math.h>
#include "globals.h"

#include <string.h>
#define PRG_COLOR RGB555(0,31,31)
#define PRG_HELP_COLOR RGB555(31,31,0)

TMOB *attack_mob;
short init_distance;
short init_sector;
short select_player=-1;
char running_battle=0;
char att_player;
char pgm_help;
char death_play;char far_play;
short cislo_potvory;
short cislo_kola;
HUM_ACTION spell_string;
short caster;
short vybrana_zbran=-1;
static char group_flee = 0;
char plr_switcher[POCET_POSTAV];
static int pohyblivost_counter[POCET_POSTAV];
static int autostart_round=0;

char autoattack=0;
char immortality=0;
int32_t level_map[]=
  {400,                 //level 2
   1000,                     // 3
   1800,                     // 4
   2800,                     // 5
   5000,                     // 6
   8400,                     // 7
   13000,                    // 8
   20000,                    // 9
   30000,                    // 10
   45000,                    // 11
   70000,                    // 12
   110000,                   // 13
   180000,                   // 14
   300000,                   // 15
   500000,                   // 16
   800000,                   // 17
   1300000,                  // 18
   2000000,                  // 19
   3000000,                  // 20
   4500000,                  // 21
   6500000,                  // 22
   10000000,                  // 23
   11000000,                  // 24
   13000000,                  // 25
   16000000,                  // 26
   20000000,                  // 27
   25000000,                  // 28
   32000000,                  // 29
   40000000,                  // 30
   50000000,                  // 31
   60000000,                 // 32
   70000000,                 // 33
   80000000,                 // 34
   90000000,                 // 35
   100000000,                 // 36
   110000000,                 // 37
   120000000,                 // 38
   130000000,                 // 39
   140000000,                 // 40
   0x7fffffff                 // Immortal!
   };

#define MAX_WEAPON_SKILL 10

word weapon_skill[]=         //tabulka poctu uspesnych zasahu pro kazdy level
   {
   20,    //level 1
   40,        //  2
   80,        //  3
   160,       //  4
   320,       //  5
   640,       //  6
   1280,       //  7
   2560,       //  8
   5120,       //  9
   10240,      //  10
   65535,      // max
   };

char JePozdrzeno();
char mask_click(int id,int xa,int ya,int xr,int yr);
char mask_click_help(int id,int xa,int ya,int xr,int yr);
char mask_click_help_clear(int id,int xa,int ya,int xr,int yr);
char souboje_clk_throw(int id,int xa,int ya,int xr,int yr);
char runes_mask(int id,int xa,int ya,int xr,int yr);
char cancel_runes(int id,int xa,int ya,int xr,int yr);
char power(int id,int xa,int ya,int xr,int yr);
char power_info(int id,int xa,int ya,int xr,int yr);
char cancel_power(int id,int xa,int ya,int xr,int yr);
static char ask_who_proc(int id,int xa,int ya,int xr,int yr);
void wire_programming();
void souboje_vybrano(int d);
void program_draw();
static void souboje_turn(int smer);
static char clk_fly_cursor(int id,int xa,int ya,int xr,int yr);

void (*after_spell_wire)();

short *poradi=NULL;
short *prave_hraje;

void wire_programming(void);
void unwire_programming(void);
void wire_jadro_souboje(void);
void unwire_jadro_souboje(void);

uint8_t sel_zivel=0;
static char prekvapeni=0;
char powers[3]={0,1,2};
char powers_info[3][50];
HUM_ACTION *magic_data;

static int minwait=0,maxwait=-1;

#define CLK_SOUBOJE 16
T_CLK_MAP clk_souboje[]=
  {
  {-1,337,0,357,14,go_map,2,H_MS_DEFAULT},
  {-1,87,0,142,14,game_setup,2,H_MS_DEFAULT},
  {-1,30,0,85,14,konec,2,H_MS_DEFAULT},
  {1,147,0,205,14,clk_saveload,2,H_MS_DEFAULT},
  {0,207,0,265,14,clk_saveload,2,H_MS_DEFAULT},
  {-1,291,0,313,14,go_book,2,H_MS_DEFAULT},
  {-1,528,378,630,479,mask_click,2,-1},
  {1,320,303,639,376,pick_item_,2,-1},//344
  {0,0,303,320,376,pick_item_,2,-1},//344
  {3,0,200,320,340,pick_item_,2,-1},//303
  {2,320,200,639,340,pick_item_,2,-1},//303
  {-1,528,378,630,479,mask_click_help,1,-1},
  {-1,0,0,640,480,mask_click_help_clear,1,-1},
  {MS_GAME_WIN,0,17,639,377,souboje_clk_throw,2,-1},
  {MS_GAME_WIN,0,17,639,377,clk_fly_cursor,8,-1},
  {-1,54,378,497,479,start_invetory,2+8,-1},
  };


char clk_enter(int id,int xa,int ya,int xr,int yr)
  {
  int i=28*256;
  id;xa;ya;xr;yr;
  send_message(E_KEYBOARD,i);
  return 0;
  }


char clk_battle_touch(int id,int xa,int ya,int xr,int yr)
  {
  int i;
  side_touched=0;
  i=clk_touch(id,xa,ya,xr,yr);
  if (side_touched) clk_enter(id,xa,ya,xr,yr);
  return i;
  }

#define CLK_PRESUN 12
T_CLK_MAP clk_presun[]=
  {
  {H_SIPKY_S,561,378,598,407,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_SZ,530,387,560,418,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_Z,529,419,555,453,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_J,560,446,598,474,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_SV,599,387,628,418,clk_step,2,H_MS_DEFAULT},
  {H_SIPKY_V,605,420,632,454,clk_step,2,H_MS_DEFAULT},
  {MS_GAME_WIN,0,17,639,377,clk_battle_touch,2,-1},
  //{3,109,303,320,340,pick_item_,2,-1},
  //{2,320,303,531,340,pick_item_,2,-1},
  //{1,320,344,577,376,pick_item_,2,-1},
  //{0,63,344,320,376,pick_item_,2,-1},
  //{MS_GAME_WIN,0,17,639,377,clk_throw,2,-1},
  //{-1,337,0,357,14,go_map,2,H_MS_DEFAULT},
  {1,147,0,205,14,clk_saveload,2,H_MS_DEFAULT},
  {0,207,0,265,14,clk_saveload,2,H_MS_DEFAULT},
  {-1,30,0,85,14,konec,2,H_MS_DEFAULT},
  {-1,565,408,593,447,clk_enter,2,H_MS_DEFAULT},
  {-1,0,0,640,480,empty_clk,0xff,-1},
  };

#define CLK_RUNES 3
T_CLK_MAP clk_runes[]=
  {
  {-1,520,378,639,479,runes_mask,2+1,H_MS_DEFAULT},
  {-1,0,0,639,479,cancel_runes,0x8,H_MS_DEFAULT},
  {-1,0,0,639,479,empty_clk,0xff,H_MS_DEFAULT},
  };



#define CLK_POWER 8
#define CLK_POWER_WHO 7
T_CLK_MAP clk_power[]=
  {
  {-1,500,376,637,480,power_info,1,-1},
  {0,535,391,637,411,power,2,-1},
  {1,535,421,637,441,power,2,-1},
  {2,535,451,637,471,power,2,-1},
  {-1,0,0,639,377,cancel_power,2+8,-1},
  {-1,0,378,639,479,cancel_power,8,-1},
  {-1,54,378,497,479,ask_who_proc,2,-1},
  {-1,0,0,639,479,empty_clk,0xff,-1},
  };

void pc_gain_action(THUMAN *h) {
    int idx = h- postavy;
    if (idx >=0 && idx < POCET_POSTAV) {
        pohyblivost_counter[idx] += h->vlastnosti[VLS_POHYB];
    }
}



int pc_get_actions(THUMAN *h) {
    int idx = h- postavy;
    if (idx >=0 && idx < POCET_POSTAV) {
        return pohyblivost_counter[idx]/AP_MULTIPLIER;
    }
    return 0;
}

void pc_use_actions(THUMAN *h, int count) {
    int idx = h- postavy;
    if (idx >=0 && idx < POCET_POSTAV) {
        int useap = count * AP_MULTIPLIER;
        useap = MIN(useap, pohyblivost_counter[idx]);
        pohyblivost_counter[idx] -= useap;
    }

}


THUMAN *isplayer(int sector,THUMAN *h,char death)
  {
  short c=map_coord[sector].flags;
  if (c & MC_PLAYER || (death && c & MC_DEAD_PLR))
     {
     if (h==NULL) h=postavy;else h++;
     while (h-postavy<POCET_POSTAV)
        {
        if (h->used && (h->lives || death) && h->sektor==sector && h->inmaphash == current_map_hash) return h;
        h++;
        }
     }
  return NULL;
  }

int numplayers(int sector,char death)
  {
  int i,c;
  THUMAN *h;

  for(i=0,c=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && (death || h->lives) && h->sektor==sector && h->inmaphash == current_map_hash) c++;
  return c;
  }

TMOB *ismonster(int sector,TMOB *m)
  {
  if (mob_map[sector])
     {
     if (m==NULL) m=mobs+mob_map[sector]-1;else
        if (m->next) m=mobs+m->next-1;else return NULL;
     }
  else m=NULL;
  return m;
  }

void presun_krok(EVENT_MSG *msg,void **user)
  {
  user;
  if (msg->msg==E_INIT) return;
  if (msg->msg==E_DONE) return;
  postavy[select_player].actions--;
  if (hromadny_utek)
	{
	int i;
	destroy_player_map();
	for (i=0;i<POCET_POSTAV;i++) if (postavy[i].used && postavy[i].lives && postavy[i].sektor==hromadny_utek && postavy[i].inmaphash == current_map_hash)
	  postavy[i].sektor=postavy[select_player].sektor;
	build_player_map();
	hromadny_utek=postavy[select_player].sektor;
	}
  msg->msg=-1;
  }

void poloz_vsechny_predmety()
  {
  int i;

  for(i=0;i<POCET_POSTAV && picked_item!=NULL;i++) {//polozeni pripadne drzene veci v mysi.
    if (postavy[i].used && postavy[i].sektor==viewsector && postavy[i].inmaphash == current_map_hash && put_item_to_inv(&postavy[i],picked_item))
          {
          free(picked_item);
          picked_item=NULL;
          }
  }
    if (picked_item!=NULL)
       {
       push_item(viewsector,viewdir,picked_item);
       free(picked_item);
       picked_item=NULL;
       }
     pick_set_cursor();
  }

char q_zacit_souboj(TMOB *p,int d,short sector)
  {
  int ss;
  int i;

  if (p->vlastnosti[VLS_KOUZLA] & (SPL_STONED | SPL_FEAR)) return 0;
//  if (battle) return 1;
  prekvapeni=0;
  if (d>p->dosah) return 0;
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==sector && postavy[i].direction==((p->dir+2)&0x3)) break;
  if (i==POCET_POSTAV) prekvapeni=!battle;
  if (d==1)
     {
     ss=map_sectors[p->sector].step_next[p->dir];
     if (!(map_coord[ss].flags & MC_PLAYER)) return 0;
     }
  if (battle && p->vlajky & MOB_CASTING && get_spell_track(p->casting)) return 1;
  if (d!=1)
     {
     if (prekvapeni) return 0;
     }
  return 1;
  }

void zacni_souboj(TMOB *p,int d,short sector)
  {
  int i;
  //toto je jen test
  battle=1;
  attack_mob=p;
  init_distance=d;
  init_sector=sector;
  map_coord[p->sector].flags |= MC_AUTOMAP;
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].used && postavy[i].sektor==init_sector &&postavy[i].inmaphash == current_map_hash) break;
  if (i<POCET_POSTAV) att_player=i;else att_player=0xff;
  }



int vypocet_zasahu(const short *utocnik,const short *obrance, int chaos,int  zbran,int external_force, char enable_finesse_rule)
  {
  int zasah,mutok,flg;
  flg=obrance[VLS_KOUZLA];
  short attbonus = enable_finesse_rule?MAX(utocnik[VLS_SILA], utocnik[VLS_OBRAT]):utocnik[VLS_SILA];
  char finesse = utocnik[VLS_SILA] != attbonus;
  attbonus/=5;
  int utok,obrana;
  int ospod;
  int attack_roll,defense_roll,mag_att_roll,mg_def;
  int dmzhit = 0;
//  int ddostal = 0;
  int disadv = 0;
  int obrbonus = obrance[VLS_OBRAT]/5;
  int magbonus = utocnik[VLS_SMAGIE]/5;
  /*
  if (game_extras & EX_ALTERNATEFIGHT)
  	{
	int postih=(chaos+1)/2;
	int testutok=rangrnd(utocnik[VLS_UTOK_L],utocnik[VLS_UTOK_H])+utocnik[VLS_OBRAT]/4+external_force;
	int testobrana=rangrnd(obrance[VLS_OBRAN_L],obrance[VLS_OBRAN_H])+obrance[VLS_OBRAT]/(4*postih)+(flg & SPL_INVIS?rangrnd(5,15):0);
	int utok=rangrnd(utocnik[VLS_UTOK_L],utocnik[VLS_UTOK_H])+attack_attribute/10+external_force;
	int obrana=rangrnd(obrance[VLS_OBRAN_L],obrance[VLS_OBRAN_H])+obrance[VLS_OBRAT]/(10*postih);
	int zv;
	mutok=rangrnd(utocnik[VLS_MGSIL_L],utocnik[VLS_MGSIL_H]);
	dhit=testutok;
	ddef=testobrana;
	if (testutok<testobrana) utok=obrana=0;
	else
	  {
	  dhit=utok;
	  ddef=obrana;
	  }
	zv=obrance[VLS_OHEN+utocnik[VLS_MGZIVEL]];
	zv=mgochrana(zv);
	mutok=zv*mutok/100;
	if ((int)rnd(obrance[VLS_OHEN+utocnik[VLS_MGZIVEL]]/2)>mutok) mutok=0;
  	dmzhit=mutok;
	zasah=utok-obrana;
	}
  else*/
	{

	disadv=(chaos+1)/2;   //chaos - pocet postav, 0=>1, 1=>1,2=>1,3=>2,4=>2,5=>3,6=>3
	//nizsi obrana je snizena o pocet postav na poli
	//0-2=>no change, 3-4=>/2, 5-6=>/3
	ospod=obrance[VLS_OBRAN_L]/disadv;
	attack_roll=utocnik[VLS_UTOK_L]+rnd(utocnik[VLS_UTOK_H]-utocnik[VLS_UTOK_L]+1);
	defense_roll=ospod+rnd(obrance[VLS_OBRAN_H]-ospod+1);
	mag_att_roll=utocnik[VLS_MGSIL_L]+rnd(utocnik[VLS_MGSIL_H]-utocnik[VLS_MGSIL_L]+1);

	//hit=attack_roll+max(str,dex)+external_force
	utok=attack_roll+attbonus+external_force;
	//def=defense_roll+dex/5+(10 if invisible)
	obrana=defense_roll+obrbonus+(flg & SPL_INVIS?10:0);
	//mg_attack = magic_roll
	mutok=mag_att_roll+(mag_att_roll?magbonus:0);
	//mg_deffense (100-x)
	mg_def=obrance[VLS_OHEN+utocnik[VLS_MGZIVEL]];
	//adjust magic attack
	dmzhit=mgochrana(mg_def)*mutok/100;
	zasah=utok-obrana;
	}
  if (zasah<0) zasah=0;
  int damage = utocnik[VLS_DAMAGE]+zbran;
  if (zasah<=0 || damage<=0) damage = 0;
//  ddostal=zasah;
  zasah+=damage;
  if (log_combat) {
      wzprintf(">");
      if (utocnik[VLS_UTOK_H]) {
        wzprintf("Attack (physical): %d (roll %d-%d) + %d (%s/5) + %d (ext.force) = %d\n",
                  attack_roll, (int)utocnik[VLS_UTOK_L],(int)utocnik[VLS_UTOK_H],
                  attbonus, finesse?texty[13]:texty[10],
                  external_force,utok);
        } else if (external_force) {
            wzprintf("Attack (physical):  %d (ext.force)\n", external_force);
        }
      if (obrance[VLS_OBRAN_H] && utok > 0) {
           char chaos[50];
           if (disadv > 1) snprintf(chaos, 50, ", disadvantage %d", disadv);
           else strcpy(chaos,"");
           wzprintf(" Defense (physical):  %d (roll %d-%d%s) + %d (%s/5) = %d\n",
              defense_roll, ospod, obrance[VLS_OBRAN_H], chaos, obrbonus, texty[13], obrana);
      }
      if (zasah>0) {
          if (damage) {
              wzprintf(" Psychical hit: %d - %d + %d (damage) + %d (weapon) = %d\n",
                utok, obrana, utocnik[VLS_DAMAGE], zbran, zasah);
          } else {
            wzprintf(" Psychical hit: %d - %d = %d\n",
              utok, obrana, zasah);
          }
      }
  }
  if (flg & SPL_SANC) {
      int tmp = zasah;
      zasah/=2;
      if (log_combat) wzprintf( "Physical resistance applied: %d - %d (half)= %d\n", tmp,tmp - zasah, zasah);
//      ddostal = zasah;
  }
  int total_hit = zasah+dmzhit;
  if (log_combat) {
    if ((int)utocnik[VLS_MGSIL_H]) {
      wzprintf(" Attack (magical): %d (roll %d-%d) + %d (%s/5) = %d\n",
          mag_att_roll,(int)utocnik[VLS_MGSIL_L],(int)utocnik[VLS_MGSIL_H], magbonus, texty[11], mutok);
      wzprintf(" Magical hit: %d - %d (resistance %s: %d%%) = %d\n",
          mutok, mutok - dmzhit, texty[22+utocnik[VLS_MGZIVEL]], mg_def, dmzhit);
      wzprintf(" Total hit: %d (physical) + %d (magical) = %d\n",
          zasah, dmzhit, total_hit);
    }
  }
  if ((flg & SPL_HSANC) && dmzhit) {
      int tmp = dmzhit/2;
      total_hit -= tmp;
      if (log_combat) wzprintf("Spell resistance applied: %d - %d = %d\n", total_hit+tmp, tmp, total_hit);
  }
  if (flg & SPL_TVAR) {
      if (log_combat) wzprintf("Set Face spell applied: %d = -%d (heal)\n", total_hit, -total_hit);
      total_hit=-total_hit;
  }
  return total_hit;
  }

void rozhodni_o_poradi()
  {
  int celk=0;
  int i,j;
  short *r,mem;

  for(i=0;i<MAX_MOBS;i++) if (mobs[i].vlajky & MOB_LIVE && mobs[i].vlajky & MOB_IN_BATTLE) celk+=mobs[i].actions;
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].used && postavy[i].inmaphash == current_map_hash) celk+=postavy[i].programovano;
  if (poradi!=NULL) free(poradi);
  r=poradi=getmem(celk*2+4);
  for(i=0;i<MAX_MOBS;i++) if (mobs[i].vlajky & MOB_LIVE && mobs[i].vlajky & MOB_IN_BATTLE)
     {
     for(j=0;j<mobs[i].actions;j++) *r++=i+1;
     }
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].used  && postavy[i].inmaphash == current_map_hash)
     {
     for(j=0;j<postavy[i].programovano;j++) *r++=-i-1;
     }
  for(i=0;i<celk;i++)
     {
     j=rnd(celk);
     mem=poradi[j];
     poradi[j]=poradi[i];
     poradi[i]=mem;
     }
  *r++=-255;
  *r++=0;
  prave_hraje=poradi;
  }
void hrat_souboj(THE_TIMER *_)
  {
  static int counter=0;
	char cond=ms_last_event.y>378 && ms_last_event.x>510 && cur_mode!=MD_PRESUN;
  if (cond) schovej_mysku();
  redraw_scene();
  if (!norefresh)
     {
     if (cur_mode!=MD_PRESUN)
			 {
			 program_draw();
			 draw_blood(0,0,0);
			 check_players_place(0);
			 }
     showview(0,0,0,0);
     if (neco_v_pohybu==2) neco_v_pohybu=0; else neco_v_pohybu=2;
     calc_fly(NULL);mob_animuj();
     if (d_action!=NULL) do_delay_actions();
     }
  if (cond) ukaz_mysku();
  if (neco_v_pohybu)
     {
     if (++counter>=maxwait && maxwait>0) neco_v_pohybu=0;
     }
  else
     if (++counter<minwait) neco_v_pohybu=2;
  if (!neco_v_pohybu)
     {
       if (JePozdrzeno()) neco_v_pohybu=1;
     counter=0;
     maxwait=-1;
     minwait=0;
     }
  calc_animations();
  }

void auto_group()
  {
  int i,j,t=1;
  THUMAN *p,*q;
  for(i=0;i<POCET_POSTAV;i++)
     postavy[i].groupnum=0;
  for(i=0;p=&postavy[i],i<POCET_POSTAV;i++)
     if (!p->groupnum && p->lives)
     {
     p->groupnum=t++;
     for(j=i+1;q=&postavy[j],j<POCET_POSTAV;j++)
        if (p->sektor==q->sektor  && p->inmaphash == current_map_hash && q->used && q->lives)
          q->groupnum=p->groupnum;
     }

    for(i=0;p=&postavy[i],i<POCET_POSTAV;i++) {
        if (p->sektor == viewsector && p->direction == viewdir) {
            cur_group = p->groupnum;
            break;
        }
    }
  }
/*
int vyber_zacinajiciho(int att_player)
  {
  int gr;
  THUMAN *p;
  int i;

//  if (att_player==0xff)
//     for(i=0;i<pocet_postav;i++) if (postavy[i].groupnum==cur_group) att_player=i
  for(gr=0;group_sort[gr]!=att_player;gr++);
  i=0;
  do
     {
     p=&postavy[group_sort[gr]];
     gr+=(gr>=POCET_POSTAV?-POCET_POSTAV:1);
     i++;
     }
  while ((!p->used || !p->actions)&& i<6);
  return p-postavy;
  }

int vyber_prvniho(int att)
  {
  int grp;
  int i;

  if (att==0xff)
     for(i=0;i<POCET_POSTAV;i++) if (postavy[i].used && postavy[i].groupnum==cur_group) att=i;
  grp=postavy[att].groupnum;
  for(i=0;postavy[group_sort[i]].groupnum!=grp;i++);
  return i;
  }
*/
static int vyber_hrace(int att) {
    int gr;
    THUMAN *h;

    if (att > POCET_POSTAV || att < 0)
        gr = cur_group, att = 0xff;
    else
        gr = postavy[att].groupnum;
    h = postavy;
    int candidate0 = -1;
    int candidate1 = -1;
    int candidate2 = -1;
    for (int i = POCET_POSTAV; i>0;) {
        --i;
        h = postavy+i;
        if (h->used && h->lives && h->inmaphash == current_map_hash) {
            candidate0 = i;
            if (h->groupnum == gr) {
                candidate1 =i;
                if (h->actions) {
                    candidate2 = i;
                }
            }
        }
    }
    if (candidate2>=0) return candidate2;
    if (candidate1>=0) return candidate1;
    if (candidate0>=0) return candidate0;
    if (att != 0xFF) return att;
    display_error("Can't select PC for battle");
    return 0;
}

void zacatek_kola()
  {
  int i;
  THUMAN *p;

  SEND_LOG("(BATTLE) Start round");
  build_player_map();
  cislo_kola++;
  autostart_round=0;
  for(i=0;i<MAX_MOBS;i++) if (mobs[i].vlajky & MOB_LIVE)
           {
           mobs[i].actions=get_ap(mobs[i].vlastnosti);
           mobs[i].walk_data=0;
           }
  memset(plr_switcher,0,sizeof(plr_switcher));
  for(i=0;i<POCET_POSTAV;i++)
     {
     p=&postavy[i];
     if (p->used)
        {
        postavy[i].programovano=0;
        if (p->kondice && p->lives && p->inmaphash == current_map_hash)
           {
           pc_gain_action(p);
           p->actions=MAX(1, postavy[i].vlastnosti[VLS_POHYB]/15);
       //    if (p->actions) autostart_round=0;
           }
        else postavy[i].actions=0;
        if (postavy[i].zvolene_akce!=NULL)free(postavy[i].zvolene_akce);
        postavy[i].zvolene_akce=NewArr(HUM_ACTION,postavy[i].actions+1);
        memset(postavy[i].zvolene_akce,0,(postavy[i].actions+1)*sizeof(HUM_ACTION));
        postavy[i].provadena_akce=postavy[i].zvolene_akce;
        }
     }
  auto_group();
  sort_groups();
  select_player=vyber_hrace(att_player);
  //att_player=vyber_prvniho(att_player);
  //select_player=vyber_zacinajiciho(att_player);
  zmen_skupinu(&postavy[select_player]);
  viewsector=postavy[select_player].sektor;
  viewdir=postavy[select_player].direction;
  redraw_scene();
  recalc_volumes(viewsector,viewdir);
  }

char check_end_game()
  {
  THUMAN *p;
  int i;
  char end=1;

  for(i=0;i<POCET_POSTAV;i++)
     {
     p=&postavy[i];
     if (p->used && p->lives && p->inmaphash == current_map_hash)
        {
        end=2;
        if (p->groupnum==cur_group) return 0;
        }
     }
  return end;
  }

void konec_kola()
  {
  int i;THUMAN *h;TMOB *m;
  int j;

  SEND_LOG("(BATTLE) End round");
  prekvapeni=0;
  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
     if (h->used)
        {
        if (h->zvolene_akce!=NULL) free(h->zvolene_akce);
        h->zvolene_akce=NULL;
        h->programovano=0;
        if (h->vlastnosti[VLS_KOUZLA] & SPL_REGEN && h->lives)
           h->lives+=3;
		player_check_death(h,1);
        if (h->lives>h->vlastnosti[VLS_MAXHIT]) h->lives=h->vlastnosti[VLS_MAXHIT];
        }
  for(i=0,m=mobs;i<MAX_MOBS;i++,m++)
     if (m->vlajky & MOB_LIVE && m->vlastnosti[VLS_KOUZLA] & SPL_REGEN)
        {
        m->lives+=3;if (m->lives>m->vlastnosti[VLS_MAXHIT]) m->lives=m->vlastnosti[VLS_MAXHIT];
        }
  auto_group();
  sort_groups();
  select_player=-1;
  GlobEvent(MAGLOB_ONROUND,viewsector,viewdir);
  sleep_ticks+=MAX_SLEEP/12;
  if (sleep_ticks>MAX_SLEEP) sleep_ticks=MAX_SLEEP;
  for(i=0,j=0;i<POCET_POSTAV;i++)
    if (postavy[i].lives || ~postavy[i].vlastnosti[VLS_KOUZLA]&SPL_STONED)
    {
      if (i!=j)memcpy(&postavy[j++],&postavy[i],sizeof(postavy[i]));
      else j++;
    }
  if (j<POCET_POSTAV) memset(&postavy[j],0,sizeof(THUMAN)*(POCET_POSTAV-j));
  destroy_player_map();
  tick_tack(1);
  send_message(E_KOUZLO_KOLO);
  TimerEvents(viewsector,viewdir,game_time);
  }

static void kbd_end_game(EVENT_MSG *msg,void *unused)
  {
  unused;
  if (msg->msg==E_KEYBOARD && !pass_zavora)
     {
     msg->msg=-2;
     delete_from_timer(TM_SCENE);
     delete_from_timer(TM_FLY);
     wire_save_load(2);
     bott_draw(1);
     }
  }

static char clk_goon(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  send_message(E_KEYBOARD,13);
  return 1;
  }

#define CLK_END_GAME 2
T_CLK_MAP clk_end_game[]=
  {
  {-1,0,0,639,479,clk_goon,8+2,H_MS_DEFAULT},
  {-1,0,0,639,479,empty_clk,0xff,H_MS_DEFAULT},
  };


void end_game_end_phase(EVENT_MSG *msg,void **_)
{
  static int wait=0;
  if (msg->msg == E_TIMER) {
   if (pass_zavora) return;
     if (wait==2)
     {
     send_message(E_ADD,E_KEYBOARD,kbd_end_game);
     send_message(E_DONE,E_TIMER,end_game_end_phase);
     change_click_map(clk_end_game,CLK_END_GAME);
     }
     else wait++;
   }
  if (msg->msg == E_INIT) {
     wait=0;
  }
}

void wire_end_game()
  {
  int i;
  if (cur_mode==MD_END_GAME) return;
  konec_kola();
  battle=0;running_battle=0;
  unwire_proc();
  for(i=0;i<MAX_MOBS;i++) if (mobs[i].vlajky & MOB_LIVE) mobs[i].vlajky&=~MOB_IN_BATTLE;

  for (int i = 0; i < POCET_POSTAV; ++i) {
      if (postavy[i].used && postavy[i].inmaphash != current_map_hash && postavy[i].lives) {
          const char *mname = find_map_from_hash(postavy[i].inmaphash);
          if (mname != NULL) {
              cur_group = postavy[i].groupnum;
              TMA_LOADLEV lv;
              strcopy_n(lv.name,mname, sizeof(lv.name));
              lv.start_pos = -postavy[i].sektor;
              lv.dir = postavy[i].direction;
              macro_load_another_map(&lv);
              return;
          }
      }
  }

/*  bott_disp_text(texty[65]);
  bott_text_forever();*/
  add_to_timer(TM_SCENE,gamespeed,-1,refresh_scene);
  add_to_timer(TM_FLY,gamespeed,-1,calc_fly);
  disable_click_map();
  send_message(E_ADD,E_TIMER,end_game_end_phase);
  cur_mode=MD_END_GAME;
  build_player_map();
  GlobEvent(MAGLOB_ONDEADALL,viewsector,viewdir);
  GlobEventList[MAGLOB_ONDEADALL].sector=0;
  GlobEventList[MAGLOB_ONDEADALL].side=0;
  }


void konec_presunu(EVENT_MSG *msg,void **unused)
  {
  unused;
  int d = va_arg(msg->data, int);
  if (msg->msg==E_KEYBOARD && (d>>8)==28 && !pass_zavora)
     {
     unwire_proc();
     wire_jadro_souboje();
     msg->msg=-1;
     }
  }

void wire_presun_postavy(void);
void unwire_presun_postavy(void)
  {
  disable_click_map();
  send_message(E_DONE,E_KEYBOARD,game_keyboard);
  send_message(E_DONE,E_KROK,presun_krok);
  send_message(E_DONE,E_KEYBOARD,konec_presunu);
  delete_from_timer(TM_SCENE);
  cur_mode=MD_INBATTLE;
  sort_groups();
  wire_proc=wire_presun_postavy;
  hromadny_utek=0;
  }

void wire_presun_postavy(void)
  {
  unwire_proc();

  moving_player=select_player;
  change_click_map(clk_presun,CLK_PRESUN);
  send_message(E_ADD,E_KEYBOARD,game_keyboard);
  send_message(E_ADD,E_KROK,presun_krok);
  send_message(E_ADD,E_KEYBOARD,konec_presunu);
  cur_mode=MD_PRESUN;
  auto_group();
  sort_groups();
  bott_draw(1);
  redraw_scene();
  add_to_timer(TM_SCENE,gamespeed,-1,hrat_souboj);
  showview(0,0,0,0);
  unwire_proc=unwire_presun_postavy;
  }

static uint32_t SPozdrzeno=0;

char JePozdrzeno()
{
  return get_game_tick_count()<SPozdrzeno;
}

void pozdrz_akci()
{
  int battlespeed=gamespeedbattle;
  SPozdrzeno=get_game_tick_count()+battlespeed*2000/6;
}

/*void pozdrz_akci_proc(THE_TIMER *x)
  {
  x;
  neco_v_pohybu=1;
  }

void pozdrz_akci()
  {
  add_to_timer(0,1,75,pozdrz_akci_proc);
  }
*/
void souboje_prezbrojeni(int postava)
  {
  unwire_proc();
  bott_draw(1);
  wire_inv_mode(&postavy[postava]);
  battle_mode=MD_PREZBROJIT;
  }

void prejdi_na_pohled(THUMAN *p)
  {
                 viewsector=p->sektor;
                 viewdir=p->direction;
                 recalc_volumes(viewsector,viewdir);
                 pozdrz_akci();
                 hold_timer(TM_SCENE,1);
                 redraw_scene();
                 program_draw();
                 showview(0,0,0,0);
                 hold_timer(TM_SCENE,0);

  }

int hromadny_utek;

void utek_postavy(THUMAN *p, char group)
  {
  int minact=0;
  p->actions=p->utek;
  if (group)
	{
	int i;
    minact = 5;
	p->actions=minact;
	hromadny_utek=p->sektor;
	for (i=0;i<POCET_POSTAV;i++) if (postavy[i].used && postavy[i].sektor==p->sektor && postavy[i].inmaphash == current_map_hash)
	  {
      int wf=weigth_defect(postavy+i)+2;
	  postavy[i].kondice-=minact*wf;
	  if (postavy[i].kondice<0) postavy[i].kondice=0;
	  if (postavy+i!=p) postavy[i].programovano=0;
	  }
	}
  else
	{
    int wf=weigth_defect(p)+2;
    hromadny_utek=0;
	if (p->actions)
	   {
	   p->kondice-=p->actions*wf;
	   if (p->kondice<0)
		  {
		  p->actions+=p->kondice/wf;
		  p->kondice=0;
		  }
	  }
	}
  wire_presun_postavy();
  prejdi_na_pohled(p);
  }

int trace_path(int sector,int dir)
  {
  int mm,p=0,c=5;
  int r=rnd(2);
  do
     {
     if ((mm=mob_map[sector])!=0)
       {
       if (mobs[mm-1].stay_strategy & MOB_BIG) return 0;
       if (mobs[mm-1].next!=0 && r==1) mm=mobs[mm-1].next;
       switch (dir)
          {
          case 0: p=mobs[mm-1].locx-128;break;
          case 1: p=mobs[mm-1].locy-128;break;
          case 2: p=-(mobs[mm-1].locx-128);break;
          case 3: p=-(mobs[mm-1].locy-128);break;
          }
       if (p<-12) return -24;
       else if (p>12) return +24;
       else return 0;
       }/* return rnd(3)-1;*/
     if (map_sides[(sector<<2)+dir].flags & SD_THING_IMPS) return -255;
     sector=map_sectors[sector].step_next[dir];
     c--;
     }
  while (c);
  return -255;
  }

void hod_dykou(THUMAN *p,int where,int bonus)
  {
  short *pp;
  int i;
  int ps;
  LETICI_VEC *v;

  ps=trace_path(p->sektor,p->direction);
  if (ps==-255) return;
  pp=picked_item;
  picked_item=getmem(2*sizeof(short));
  picked_item[0]=p->wearing[where];
  picked_item[1]=0;
  v=throw_fly(320,100,0);
  v->ypos=ps;
  v->hit_bonus=p->vlastnosti[VLS_OBRAT]/10+bonus;
  v->damage=0;
  for(i=0;i<p->inv_size;i++)
     {
     int it;

     it=p->inv[i];
     if (it--)
        {
        if (glob_items[it].druh==TYP_VRHACI && glob_items[it].umisteni==PL_RUKA)
           {
           p->wearing[where]=it+1;
           p->inv[i]=0;
           break;
           }
        }
     }
  if(i==p->inv_size) p->wearing[where]=0;
  picked_item=pp;
  if (log_combat) {
      wzprintf("%s throws: ext.force: %d(%s)/10+%d(bonus) = %d\n",
              p->jmeno, p->vlastnosti[VLS_OBRAT], texty[13],
              bonus, v->hit_bonus);
  }
  }

void vystrel_sip(THUMAN *p,int bonus)
  {
  short *pp;
  int ps;
  int i;
  int attack_roll;
  LETICI_VEC *v;
  TITEM *t;

  ps=trace_path(p->sektor,p->direction);
  if (ps==-255) return;
  if (!p->sipy)
     {
     char s[100];

     sprintf(s,texty[72],p->jmeno);
     bott_disp_text(s);
     return;
     }
  for(i=0;i<item_count;i++) if (glob_items[i].umisteni==PL_SIP && glob_items[i].user_value==0) break;
  if (i==item_count) return;
  t=glob_items+i;
  pp=picked_item;
  picked_item=getmem(2*sizeof(short));
  picked_item[0]=i+1;
  picked_item[1]=0;
  p->sipy--;
  v=throw_fly(320,100,1);
  v->ypos=ps;
  attack_roll=p->vlastnosti[VLS_UTOK_L]+rnd(p->vlastnosti[VLS_UTOK_H]-p->vlastnosti[VLS_UTOK_L]);
  v->hit_bonus=attack_roll+(p->vlastnosti[VLS_OBRAT]/5)+bonus;
  v->damage=p->vlastnosti[VLS_DAMAGE];
  picked_item=pp;
  t->zmeny[VLS_MGSIL_H]=p->vlastnosti[VLS_MGSIL_H]; //adjust zmen v magickem utoku
  t->zmeny[VLS_MGSIL_L]=p->vlastnosti[VLS_MGSIL_L];
  t->zmeny[VLS_MGZIVEL]=p->vlastnosti[VLS_MGZIVEL];
  play_sample_at_sector(H_SND_SIP1+rnd(2),0,0,0,0);
  neco_v_pohybu=1;

  if (log_combat) {
      wzprintf("%s shoots: attack_roll (%d-%d)=%d, ext.force: %d + %d(%s)/5+%d(bonus) = %d\n",
              p->jmeno, p->vlastnosti[VLS_UTOK_L], p->vlastnosti[VLS_UTOK_H],
              attack_roll,
              attack_roll, p->vlastnosti[VLS_OBRAT], texty[13],
              bonus, v->hit_bonus);
  }

  }

char is_useable_weapon(int i)
  {
  if (!i) return 0;
  i--;
  switch (glob_items[i].druh)
     {
     case TYP_UTOC:
     case TYP_VRHACI:
     case TYP_STRELNA:
     case TYP_SVITEK:return 1;
     default: return 0;
     }
  }

int select_weapon(THUMAN *p,char ask)
  {
  char lp,rp;
  int li,ri;
  char *c;

  li=p->wearing[PO_RUKA_L];
  ri=p->wearing[PO_RUKA_R];
  lp=is_useable_weapon(li);
  rp=is_useable_weapon(ri);
  if (!lp && !rp) return 2;
  if (lp && !rp) return 0;
  if (!lp && rp) return 1;
  li--;
  ri--;
  if (glob_items[li].druh==glob_items[ri].druh && glob_items[li].druh==TYP_UTOC) return 2;
  if (!ask) return rnd(2);
  unwire_proc();
  c=alloca(strlen(p->jmeno)+strlen(texty[82])+1);
  sprintf(c,texty[82],p->jmeno);
  lp=message(2,0,1,"",c,texty[83],texty[84]);
  wire_proc();
  return lp;
  }

static int vypocti_bonus(THUMAN *p,int vybrana_zbran)
  {
  int bonus;
  if (vybrana_zbran>-1)
        if (vybrana_zbran>0) bonus=p->bonus_zbrani[glob_items[vybrana_zbran-1].typ_zbrane];
        else bonus=p->bonus_zbrani[TPW_OST];
  else bonus=0;
  return bonus;
  }

static void pouzij_svitek(THUMAN *p,int ruka)
  {
  int it=p->wearing[ruka]-1;
  short s[2];

  thing_cast(glob_items[it].spell,p-postavy,p->sektor,NULL,0);
  if (glob_items[it].magie==1)
     {
     s[0]=it+1;
     s[1]=0;
     destroy_items(s);
     p->wearing[ruka]=0;
     }
  else
     {
     it=(p->wearing[ruka]=duplic_item(it+1));
     it--;
     glob_items[it].magie--;
     }
  }

static void play_weapon_anim(int anim_num,int hitpos)
  {
  char count_save=global_anim_counter;
  int battlespeed=gamespeedbattle;

  if (anim_num==0) return;
  hold_timer(TM_SCENE,1);
  if (battlespeed<1) battlespeed=1;
  add_to_timer(TM_SCENE2,battlespeed,-1,hrat_souboj);
  play_big_mgif_animation(anim_num+face_arr[4]);
  do
     {
     while (global_anim_counter==count_save)
        {
        do_events();
        }
     count_save=global_anim_counter;
     }
  while (hitpos-- && running_anm);
  delete_from_timer(TM_SCENE2);
  hold_timer(TM_SCENE,0);
  }

void pouzij_zbran(THUMAN *p,int ruka)
  {
  int itm,where;
  TITEM *it;
  int bonus;
  int wf=weigth_defect(p)+1;


  p->kondice-=wf;
  if (ruka==2) ruka=plr_switcher[p-postavy];
  where=PO_RUKA_L+ruka;
  itm=p->wearing[where];
  SEND_LOG("(BATTLE) Player uses weapon %d.in %s hand",itm,where?"right":"left");
  if (p->stare_vls[VLS_KOUZLA] & SPL_INVIS)
     {
     p->stare_vls[VLS_KOUZLA]&=~SPL_INVIS;
     prepocitat_postavu(p);
     build_all_players();
     }
  vybrana_zbran=itm;
  bonus=vypocti_bonus(p,itm);
  if (itm>0) {
     memcpy(&p->vlastnosti[VLS_MGSIL_L],&glob_items[itm-1].zmeny[VLS_MGSIL_L],3*sizeof(short));
  } else {
     memset(&p->vlastnosti[VLS_MGSIL_L],0,3*sizeof(short));
  }
  if (!itm || (it=&glob_items[itm-1])->druh==TYP_UTOC)
     {
     TMOB *m;int mm,chaos;
     mm=vyber_potvoru(p->sektor,p->direction,&chaos);
     if (mm>=0)
        {
        TITEM *it=itm?glob_items+itm-1:NULL;
        m=mobs+mm;
        bott_draw(1);
        anim_mirror=ruka==0;
        prejdi_na_pohled(p);
        if (itm) play_weapon_anim(it->weapon_attack,it->hitpos);
        if (utok_na_sektor(p,m,chaos,bonus,it==NULL || it->umisteni == PL_OBOUR?0:where)>0 && itm ) {
          int roll = (int)rnd(100)+1;
          char cast = roll <it->magie;
          if (cast) {
            if (log_combat) wzprintf(" Weapon cast: %s - %d (roll) > %d (probability)\n",it->jmeno, roll, it->magie);
            thing_cast(it->spell,p-postavy,p->sektor,m,1);
          }
        }


        }
     }
  else
     {
     prejdi_na_pohled(p);
     switch(it->druh)
       {
        case TYP_VRHACI:hod_dykou(p,where,bonus);break;
        case TYP_STRELNA:vystrel_sip(p,bonus);break;
        case TYP_SVITEK:pouzij_svitek(p,where);break;
       }
     }
  bott_draw(0);
  }

static char valid_sectors(word sector, void *ctx)
  {
  int pp;
  int i;
  word *last_sector = (word *)ctx;

  *last_sector=sector;
  if (mob_map[sector]) return 0; //nevyhovujici
  pp=map_sectors[sector].sector_type;
  if (pp==S_DIRA || ISTELEPORT(pp)) return 0;
  for (i=0;i<4;i++) if (map_sectors[sector].step_next[i] && mob_map[map_sectors[sector].step_next[i]]) return 0;
  return 1;
  }


static char StrachPostavy(THUMAN *p)
{
  word *cesta;
  int i;
  word last_sector = 0;

  int wf=weigth_defect(p)+1;

  prejdi_na_pohled(p);
  cur_group=p->groupnum;
  for(select_player=0;select_player<6;select_player++) if (postavy+select_player==p) break;
  bott_draw(0);
  labyrinth_find_path(p->sektor,65535,SD_PLAY_IMPS,valid_sectors,NULL,&last_sector);
  labyrinth_find_path(p->sektor,last_sector,SD_PLAY_IMPS,valid_sectors,&cesta,&last_sector);
  if (cesta[0]==0) {free(cesta);return 0;}
  for (i=0;i<6 && cesta[i] && p->kondice ;i++)
  {
    int dir;
    for (dir=0;dir<4;dir++) if (map_sectors[p->sektor].step_next[dir]==cesta[i]) break;
    destroy_player_map();
    p->direction=dir;
    p->sektor=cesta[i];
    build_player_map();
    prejdi_na_pohled(p);
    sleep_ms(200);
    p->kondice-=wf;
  }
  p->provadena_akce+=p->programovano-1;
  p->programovano=1;
  free(cesta);
  return 1;
}

void jadro_souboje(EVENT_MSG *msg,void **unused) //!!!! Jadro souboje
  {
  static char nowait=0;
  unused;

  if (msg->msg==E_IDLE && (!neco_v_pohybu || !battle || nowait)&& !norefresh)
     {
     short nxt;

     if (check_end_game()==1)
        {
        wire_end_game();
        return;
        }
     vybrana_zbran=-1;
     nxt=*prave_hraje++;
     anim_mirror=0;
     if (nxt)
        {
        if (nxt>0)
           {
           nxt--;
           SEND_LOG("(BATTLE) Mobile action (%d. %s)",nxt,mobs[nxt].name);
           nowait=akce_moba_zac(&mobs[nxt]) && *prave_hraje>0;
           neco_v_pohybu=1;
           cislo_potvory=nxt;
           }
        else
           if(nxt==-255)
              {
              int i;
//              SEND_LOG("(BATTLE) Ending round...(nxt=%d,mob=%s)",nxt,mobs[nxt].name);
              delete_from_timer(TM_SCENE);
              add_to_timer(TM_SCENE,gamespeed,-1,refresh_scene);
              for(i=0;i<MAX_MOBS;i++)
                 if (~mobs[i].vlajky & MOB_IN_BATTLE)
                    {
                    mob_test_na_bitvu(mobs+i);
                    if (mobs[i].vlajky & MOB_LIVE && mobs[i].stay_strategy & 1)
                       {rozhodni_o_smeru(&mobs[i]);krok_moba(&mobs[i]);}
                    }
              check_all_mobs_battle();
              minwait=16;
              maxwait=32;
              neco_v_pohybu=1;
              nowait=1;
              }
        else if (nxt<0)
           {
           THUMAN *p;

           nowait=0;
           nxt=abs(nxt)-1;
           p=postavy+nxt;
           select_player=nxt;
           if (p->programovano && p->lives)
              {
              plr_switcher[select_player]=!plr_switcher[select_player];
              cur_group=p->groupnum;
              if (p->kondice || p->provadena_akce->action==AC_STAND)
               {
                 if (p->vlastnosti[VLS_KOUZLA] & SPL_FEAR &&  StrachPostavy(p))
                 {}
                 else
                 {
               SEND_LOG("(BATTLE) Player Action '%s', number: %d",p->jmeno,p->provadena_akce->action);
               if (group_flee) {
                   group_flee = 0;
                   utek_postavy(p, 1);
               } else switch(p->provadena_akce->action)
                {
                case AC_MOVE:
                  {
                             int wf=weigth_defect(p)+1;
                             p->actions++;
                             p->kondice-=p->actions*wf;
                             if (p->kondice<0)
                                {
                                p->actions+=p->kondice*wf;
                                p->kondice=0;
                                if (p->actions<0)
                                   {
                                   p->actions=0;
                                   break;
                                   }
                                }
                             viewsector=p->sektor;
                             viewdir=p->direction;
                             wire_presun_postavy();
                             break;
                  }
                case AC_ATTACK:pouzij_zbran(p,p->provadena_akce->data1);
                            if (p->provadena_akce->data1 == 2 && plr_switcher[p-postavy] == 1) {
                                prave_hraje--;
                                neco_v_pohybu = 1;
                                return;
                            }
                            break;
                case AC_ARMOR:souboje_prezbrojeni(nxt);
                             bott_draw(1);
                             other_draw();
                             break;
                case AC_THROW:
                             {
                             int x,y;
                             picked_item = getmem(sizeof(short)*2);
                             picked_item[0] = p->provadena_akce->data2;
                             picked_item[1] = 0;
                             x=p->provadena_akce->data1;
                             y=(x>>8)*2;x=(x & 0xff)*4;
                             prejdi_na_pohled(p);
                             throw_fly(x,y,0);
                             cislo_potvory=-2;
                             neco_v_pohybu=1;
                             }
                             break;
                case AC_STAND:pomala_regenerace_postavy(p);break;
                case AC_RUN:utek_postavy(p,0);break;
                case AC_MAGIC:
                             prejdi_na_pohled(p);
                             bott_draw(1);
                             teleport_target=p->provadena_akce->data2;
                             cast(p->provadena_akce->data1,p,select_player,0);
                             cislo_potvory=-2;
                             break;
                }
                }
               }
               neco_v_pohybu=1;
              p->provadena_akce++;
              p->programovano--;
              }
           }
        }
     else
        {
        nowait=0;
        konec_kola();
        check_all_mobs_battle();
        mouse_set_default(H_MS_DEFAULT);
        if (battle)
           {
           delete_from_timer(TM_SCENE);
           zacatek_kola();
           unwire_proc();
           wire_programming();
           }
        else
           {
           int i;
           THUMAN *p;

           SEND_LOG("(BATTLE) Leaving battle");
           prekvapeni=0;
           unwire_proc();
           wire_main_functs();
           bott_draw(1);
           running_battle=0;
           for(i=0;p=&postavy[i],i<POCET_POSTAV && (p->sektor!=viewsector || !p->used || p->inmaphash != current_map_hash || !p->groupnum);i++);
           if (i==POCET_POSTAV)
              for(i=0;p=&postavy[i],i<POCET_POSTAV && (!p->used || !p->groupnum || p->inmaphash != current_map_hash );i++);
           cur_group=postavy[i].groupnum;
           viewsector=postavy[i].sektor;
           viewdir=postavy[i].direction;
           build_player_map();
           recalc_volumes(viewsector,viewdir);
		   GlobEvent(MAGLOB_AFTERBATTLE,viewsector,viewdir);
		   autosave();
           }
        msg->msg=-2;
        }
     }
 }

void wire_jadro_souboje(void)
  {
  int battlespeed=gamespeedbattle;
  recalc_volumes(viewsector,viewdir);
  if (battlespeed<1) battlespeed=1;
  add_to_timer(TM_SCENE,battlespeed,-1,hrat_souboj);
  send_message(E_ADD,E_IDLE,jadro_souboje);
  mouse_set_default(H_MS_SOUBOJ);
  unwire_proc=unwire_jadro_souboje;
  cur_mode=MD_INBATTLE;
  pgm_help=10;
  }

void unwire_jadro_souboje(void)
  {
  delete_from_timer(TM_SCENE);
  send_message(E_DONE,E_IDLE,jadro_souboje);
  mouse_set_default(H_MS_DEFAULT);
  wire_proc=wire_jadro_souboje;
  pgm_help=0;
  }

void fill_rune(char *d,int i)
  {
  int x,y;char *dd;

  dd=((char *)d)+6+512;
        for(y=378;y<480;y++)
           {
           word *z;
           int32_t scr_linelen2 = GetScreenPitch();
           z=GetScreenAdr()+y*scr_linelen2;
           for(x=520;x<640;x++)
              if (*dd++==i) z[x]=z[x]-((z[x] & RGB555(28,28,28))>>2);
           }

  }

static void *runebar;
static char *rune_name=NULL;
static char *rune_info=NULL;

void display_power_bar(char);
void display_rune_bar(THE_TIMER *_)
  {
  short coords[][2]={{3,26},{32,26},{61,26},{90,26},{18,64},{47,64},{76,64}};
  char c;
  int i;

  schovej_mysku();
  if (runebar!=NULL) put_picture(520,378,runebar);
  else
     {
     put_picture(520,378,ablock(H_RUNEBAR1+sel_zivel));
     c=runes[sel_zivel];
     for(i=0;i<7;i++,c>>=1)
       if (!(c & 1)) put_picture(520+coords[i][0],378+coords[i][1],ablock(H_RUNEHOLE));
       else if (!get_rune_enable(&postavy[select_player],(sel_zivel*7+i)*3)) fill_rune((char *)ablock(H_RUNEMASK),i+6);
     if (sel_zivel) trans_bar(520,378,sel_zivel*24,22,0);
     if (sel_zivel!=4)trans_bar(544+sel_zivel*24,378,96-sel_zivel*24,22,0);
     runebar=getmem(120*102*2+6);
     get_picture(520,378,120,102,runebar);
     }
  ukaz_mysku();
  if (je_myska_zobrazena())showview(520,378,120,102);
  }



void rune_bar_redrawing(THE_TIMER *_)
  {
  redraw_scene();
  if (!norefresh )
     {
     schovej_mysku();
     program_draw();
     display_rune_bar(NULL);
     ukaz_mysku();
     showview(0,0,0,0);
     }
  }
  void power_bar_redrawing(THE_TIMER *_)
  {
  redraw_scene();
  if (!norefresh )
     {
     schovej_mysku();
     program_draw();
     display_power_bar(0);
     ukaz_mysku();
     showview(0,0,0,0);
     }
  }


void display_power_bar(char drw)
  {
  int coords[][2]={{20,11},{20,41},{20,71}};
  int i;

  int32_t scr_linelen2 = GetScreenPitch();
  schovej_mysku();
  put_picture(520,378,ablock(H_POWERBAR));
  for(i=0;i<3;i++)
     put_8bit_clipped(ablock(H_POWERLED),520+coords[i][0]+(378+coords[i][1])*scr_linelen2+GetScreenAdr(),24*powers[i],21,24);
  ukaz_mysku();
  if (drw) showview(520,378,120,102);
  }

void display_power_bar_tm(THE_TIMER *tm)
  {
  tm;
  display_power_bar(1);
  }

void wire_select_rune(void);
void unwire_select_rune(void);
void wire_select_power(void);

char cancel_power(int id,int xa,int ya,int xr,int yr)
  {
  xa;ya;xr;yr;id;
  schovej_mysku();
  unwire_proc();
  magic_data->action=0;
  after_spell_wire();
  mouse_set_default(H_MS_DEFAULT);
  ukaz_mysku();
  return 1;
  }

static char ask_who_proc(int id,int xa,int ya,int xr,int yr)
  {
  THUMAN *p;
  int i;
  const word *xs;

  xs=ablock(H_OKNO);
  i=xr/xs[0];yr;xa;ya;id;
  if (i<POCET_POSTAV)
     {
     i=group_sort[i];
     p=&postavy[i];
     if (can_select_player(p, death_play, far_play)) {
           {
           if (get_spell_teleport(magic_data->data1)) {
              if ((magic_data->data2=select_teleport_target(0))==0)
                 {
                 cancel_power(id,xa,ya,xr,yr);
                 return 1;
                 }
           }
           magic_data->data1+=(i+1)<<9;
           magic_data->action=AC_MAGIC;
           if (battle) souboje_vybrano(AC_MAGIC);
           unwire_proc();
           after_spell_wire();
           mouse_set_default(H_MS_DEFAULT);
           return 1;
           }
     }
     }
  return 0;
  }

void vyber_cil(int typ)
  {
  death_play=(typ==3)?1:0;
  far_play=(typ==4)?1:0;
  change_click_map(clk_power,CLK_POWER_WHO);
  mouse_set_default(H_MS_WHO);
  }

char power_info(int id,int xa,int ya,int xr,int yr) {
  rune_info = NULL;
  int x = ms_last_event.x;
  int y = ms_last_event.y;
for (int i = 0; i < CLK_POWER; ++i) {
      const T_CLK_MAP *m = clk_power+i;
      if (m->proc == &power && m->xlu <= x && m->xrb >= x && m->ylu <= y && m->yrb >= y) {
        rune_info = powers_info[m->id];
      }
    }
    return 1;
}
char power(int id,int xa,int ya,int xr,int yr)
  {
  xa;ya;xr;yr;
  if (powers[id]==1) return 1;
  schovej_mysku();
  display_power_bar(0);
  trans_bar(520+44,378+11+30*id,76,24,RGB555(31,31,31));
  mouse_set_default(H_MS_DEFAULT);
  ukaz_mysku();
  showview(520,378,120,102);
  magic_data->data1-=magic_data->data1 % 3;
  magic_data->data1+=id;
  id=magic_data->data1;
  if ((id=ask_who(id))>1)
     {
     vyber_cil(id);
     return 1;
     }
  magic_data->action=AC_MAGIC;
  if (get_spell_teleport(magic_data->data1))
     if ((magic_data->data2=select_teleport_target(0))==0)
         {
         cancel_power(id,xa,ya,xr,yr);
         return 1;
         }
  if (id==1) magic_data->data1+=(select_player+1)<<9;
  schovej_mysku();
  if (battle) souboje_vybrano(AC_MAGIC);
  unwire_proc();
  after_spell_wire();
  ukaz_mysku();
  return 1;
  }

char runes_mask(int id,int xa,int ya,int xr,int yr)
  {
  char *c;
  int cc;
  const short *d;

  id;ya;xa;
  d=ablock(H_RUNEMASK);
  c=((char *)d)+6+512+xr+yr*d[0];
  cc=*c-6;
  if (*c) {
     if (*c<6 && ms_last_event.event_type & 0x2) {
         sel_zivel=*c-1;
     } else if (runes[sel_zivel] & (1<<cc))
        {
        int x=(sel_zivel*7+(cc))*3;
        if (ms_last_event.event_type & 0x2)
           {
           magic_data->data1=x;
           unwire_select_rune();
           wire_select_power();
           schovej_mysku();
           fill_rune((char *)d,*c);
           ukaz_mysku();
           showview(520,378,120,102);
           return 1;
           }
        else
           {
           rune_name=get_rune_name(x);
           }
        }
  }
  if (cc<0) rune_name=NULL;
  rune_info = NULL;
  free(runebar);runebar=NULL;
  display_rune_bar(NULL);
  return 1;
  }

char cancel_runes(int id,int xa,int ya,int xr,int yr)
  {
  xa;ya;xr;yr;id;
  rune_name=NULL;
  schovej_mysku();
  unwire_select_rune();
  magic_data->action=0;
  after_spell_wire();
  ukaz_mysku();
  return 1;
  }


  static void select_rune_kbd(EVENT_MSG *ev, void **user_ptr) {
    if (ev->msg == E_KEYBOARD) {
        int v = quit_request_as_escape(va_arg(ev->data, int));
        switch (v>>8) {
          default: return;
          case 1: cancel_runes(0,0,0,0,0);return;
          case 32:
          case 'M':sel_zivel = (sel_zivel +1) % 5;break;
          case 30:
          case 'K':sel_zivel = (sel_zivel +4) % 5;break;
        }
        free(runebar);runebar=NULL;
        display_rune_bar(NULL);
    }
  }

void unwire_select_rune(void)
  {
  wire_proc=wire_select_rune;
  delete_from_timer(TM_DELAIER);
  delete_from_timer(TM_SCENE);
  free(runebar);runebar=NULL;
  send_message(E_DONE,E_KEYBOARD, select_rune_kbd);
  }

void wire_select_rune(void)
  {
  THUMAN *p;
  HUM_ACTION *c;

  mute_all_tracks(0);
  p=&postavy[select_player];
  c=p->zvolene_akce;while (c->action) c++;
  magic_data=c;
  c++;
  c->action=0;
  unwire_proc();
  change_click_map(clk_runes,CLK_RUNES);
  add_to_timer(TM_DELAIER,12,1,display_rune_bar);
  add_to_timer(TM_SCENE,gamespeed,-1,rune_bar_redrawing);
  send_message(E_ADD,E_KEYBOARD, select_rune_kbd);
  unwire_proc=unwire_select_rune;
  }

void wire_select_rune_fly()
  {
  mute_all_tracks(0);
  unwire_proc();
  change_click_map(clk_runes,CLK_RUNES);
  add_to_timer(TM_DELAIER,12,1,display_rune_bar);
  add_to_timer(TM_SCENE,gamespeed,-1,rune_bar_redrawing);
  send_message(E_ADD,E_KEYBOARD, select_rune_kbd);
  unwire_proc=unwire_select_rune;
  }

void unwire_select_power(void)
  {
  rune_name=NULL;
  delete_from_timer(TM_DELAIER);
  delete_from_timer(TM_SCENE);
  }

void wire_select_power(void)
  {
  THUMAN *p;
  int i;

  p=&postavy[select_player];
  mute_all_tracks(0);
  unwire_proc();
  for(i=0;i<3;i++) {
    powers[i]=get_spell_color(p,magic_data->data1+i);
    get_spell_info(magic_data->data1+i, powers_info[i],sizeof(powers_info[i]));
  }
  change_click_map(clk_power,CLK_POWER);
  unwire_proc=unwire_select_power;
  add_to_timer(TM_SCENE,gamespeed,-1,power_bar_redrawing);
  add_to_timer(TM_DELAIER,12,1,display_power_bar_tm);
  }



void program_draw()
  {
  int x=54+74/2;
  int i,j,maxy=0;

  lodka_battle_draw = 1;

  maxy;
  schovej_mysku();
  if (group_flee) {
      trans_bar(0,377-15,640,15,0);
      for (j = 0; j < POCET_POSTAV; ++j) if (postavy[j].groupnum == cur_group) {
          set_font(H_FLITT5,PRG_COLOR);
          int y=386-10;
          set_aligned_position(x,y,1,2,texty[AC_RUN+40]);
          outtext(texty[40+AC_RUN]);
          x+=74;
      }
  } else {
      for(j=0;j<POCET_POSTAV;j++) {
         if (postavy[i=group_sort[j]].used)
         {
         int y;

         y=postavy[i].programovano*10;
         if (y>maxy) maxy=y;
         }
      }
         int m = 0;
      if (pgm_help || rune_name!=NULL) m=10*(rune_info?2:1);
      if (m > maxy) maxy = m;
      if (maxy)
         {
         maxy+=5;
         trans_bar(0,377-maxy,640,maxy,0);
         }
      for(j=0;j<POCET_POSTAV;j++)
         if (postavy[i=group_sort[j]].used)
         {
         HUM_ACTION *c;
         int y,j;
         c=postavy[i].provadena_akce;
         if (c==NULL) continue;
         y=386-10*postavy[i].programovano;
         set_font(H_FLITT5,PRG_COLOR);
         for(j=0;j<postavy[i].programovano;j++)
            {
            set_aligned_position(x,y,1,2,texty[c->action+40]);
            outtext(texty[c->action+40]);
            c++;
            y+=+10;
            }
         x+=74;
         }
  }
 if(pgm_help || rune_name!=NULL)
     {
     char *c;

     set_font(H_FLITT5,PRG_HELP_COLOR);
     if (rune_name!=NULL) {
        c=rune_name;
     } else {
        c=texty[40+pgm_help];
     }
     set_aligned_position(580,376,1,2,c);
     outtext(c);

     if (rune_name && rune_info) {
        set_aligned_position(580,362,1,2,rune_info);
        outtext(rune_info);
     }

    }


     ukaz_mysku();
  }


void souboje_redrawing(THE_TIMER *_)
  {
  if (neco_v_pohybu) calc_mobs();
  calc_animations();
  redraw_scene();
  if (!norefresh)
     {
     schovej_mysku();
     program_draw();
     ukaz_mysku();
     showview(0,0,0,0);
     }
  }


void souboje_stisknout(int d)
  {
    int32_t scr_linelen2 = GetScreenPitch();
  update_mysky();
  schovej_mysku();
  d--;
  d*=105;
  put_8bit_clipped(ablock(H_BATTLE_CLICK),378*scr_linelen2+520+GetScreenAdr(),d,120,102);
  ukaz_mysku();
  showview(520,378,120,102);
  }

static void souboje_dalsi()
  {
   int i,j=12,cd;
   for(i=0;group_sort[i]!=select_player;i++);
   cd=postavy[select_player].groupnum;
   do
      {
      i++;
      if (i>=POCET_POSTAV) i=0;
      select_player=group_sort[i];
      j--;
      }
   while ((!postavy[select_player].used || postavy[select_player].inmaphash != current_map_hash || !postavy[select_player].actions || (postavy[select_player].groupnum!=cd && j>6)) && j);
   viewsector=postavy[select_player].sektor;
   viewdir=postavy[select_player].direction;
   recalc_volumes(viewsector,viewdir);
   cur_group=postavy[select_player].groupnum;
  }

static void souboje_dalsi_user() {
    int i,j=6,k;
    for(i=0;group_sort[i]!=select_player;i++);
    do
       {
       i++;
       if (i>=POCET_POSTAV) i=0;
       k=group_sort[i];
       j--;
       }
    while ((!postavy[k].used
            || !postavy[k].lives
            || postavy[k].inmaphash != current_map_hash) && j);
    select_player = k;
    viewsector=postavy[select_player].sektor;
    viewdir=postavy[select_player].direction;
    cur_group=postavy[select_player].groupnum;
    recalc_volumes(viewsector,viewdir);
}

void souboje_vybrano(int d)
  {
                       if (d==AC_STAND || d==AC_RUN) postavy[select_player].actions=0;
                       else postavy[select_player].actions--;
                       postavy[select_player].programovano++;
                       if (!postavy[select_player].actions)
                          souboje_dalsi();
  bott_draw(1);
  }

void zrusit_akce()
  {
  HUM_ACTION *c;

  c=postavy[select_player].zvolene_akce;
  while (c->action)
     if (c->action==AC_THROW)
        {
        poloz_vsechny_predmety();
        picked_item = getmem(sizeof(short)*2);
        picked_item[0] = c->data2;
        picked_item[1] = 0;
        c++;
        }
     else c++;
  postavy[select_player].zvolene_akce->action=0;
  postavy[select_player].actions=get_ap(postavy[select_player].vlastnosti);;
  postavy[select_player].programovano=0;
  postavy[select_player].utek=0;
  pick_set_cursor();
  bott_draw(1);
  }

char souboje_clk_throw(int id,int xa,int ya,int xr,int yr)
  {
  HUM_ACTION *c;

  if (postavy[select_player].actions==0) return 0;
  if (picked_item==NULL) return 0;
  if (picked_item[1] != 0) return 0;
  postavy[select_player].direction=viewdir;
  c=postavy[select_player].zvolene_akce;while (c->action) c++;
  c->action=AC_THROW;
  c->data2 = picked_item[0];
  free(picked_item);
  picked_item = NULL;
  c->data1=xa/4+(ya/2)*256;
  c++;
  c->action=0;
  pick_set_cursor();id;xr;yr;
  souboje_vybrano(AC_THROW);
  return 1;
  }


char mask_click_help(int id,int xa,int ya,int xr,int yr)
  {
  char *c;
  int d;
  word *mask;

  id;xa;ya;
  mask=(word *)ablock(H_BATTLE_MASK);
  c=(char *)mask+6+512;
  c+=yr*mask[0]+xr;
  d=*c;
  if (d) pgm_help=d;
  return 1;
  }

char mask_click_help_clear(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;
  xr;yr;
  pgm_help=0;
  return 1;
  }

  static char clk_fly_cursor(int id,int xa,int ya,int xr,int yr)
  {
  id;

  if (spell_cast)
     {
     spell_cast=0;
     pick_set_cursor();
     return 1;
     }
  if (xr<160) {
    souboje_turn(-1);
  } else if (xr > 480) {
    souboje_turn(1);
  } else {
    return 0;
  }
  return 1;
  }


static void zahajit_kolo(char prekvapeni)
  {
  int i,j;

  for(i=0;i<POCET_POSTAV;i++)
                          {
                          THUMAN *p=&postavy[i];
                          if (p->inmaphash != current_map_hash) continue;
                          int sect=p->sektor,dir=p->direction;
                          char monster=0;
                          char monster_far=0;
                          char lnear=1;
                          int counter=5;
                          short w1,w2,dw1,dw2,w;

                          pc_use_actions(p, pc_get_actions(p));

                          while (~map_sides[(sect<<2)+dir].flags & SD_PLAY_IMPS)
                             {
                             int m1,m2;
                             sect=map_sectors[sect].step_next[dir];
                             if (numplayers(sect,0)>2) break;
                             m1=mob_map[sect]-1;if (m1>=0) m2=mobs[m1].next-1;else m2=-1;
                            if ((m1 >= 0 && (mobs[m1].vlajky & MOB_IN_BATTLE))
                                    || (m2 >= 0 && (mobs[m2].vlajky & MOB_IN_BATTLE))) {
                                if (lnear) {
                                    monster = 1;
                                } else {
                                    monster_far = 1;
                                }
                            }
                             lnear=0;counter--;if(!counter) break;
                             }
                          w1=p->wearing[PO_RUKA_L];w2=p->wearing[PO_RUKA_R];
                          if (w1) dw1=glob_items[w1-1].druh;else dw1=-1;
                          if (w2) dw2=glob_items[w2-1].druh;else dw2=-1;
                          w=0;
                          if ((dw1==TYP_STRELNA && p->sipy) || dw1==TYP_VRHACI) w|=1;
                          if ((dw2==TYP_STRELNA && p->sipy) || dw2==TYP_VRHACI) w|=2;
                          if (w==0) w=select_weapon(p,0);
                          else if (w==3) w=select_weapon(p,0),monster|=monster_far;
                          else w--,monster|=monster_far;
                          if (p->used && group_flee) {
                              p->programovano = 1;
                              p->zvolene_akce->action=AC_RUN;
                          } else if (p->used && !p->programovano && p->lives && p->inmaphash == current_map_hash) {
                             if (prekvapeni || !p->actions || !autoattack || !monster)
                             {
                             p->programovano++;p->zvolene_akce->action=AC_STAND;
                             }
                          else
                             {
                             for(j=0;j<p->actions;j++)
                                {
                                p->zvolene_akce[j].action=AC_ATTACK;
                                p->zvolene_akce[j].data1=w;
                                }
                             p->programovano=(char)p->actions;
                             }
                          }  else if (p->used && !p->lives) {
                            umirani_postavy(p);
                          }
                          }
  rozhodni_o_poradi();
  unwire_proc();
  wire_jadro_souboje();
  
  }

static char add_pc_action(int d) {
       souboje_stisknout(d);
       switch(d)
          {
          case AC_RUN:
              if (lodka) {
                  group_flee = 1;break;
              } else {
                  postavy[select_player].utek=5+postavy[select_player].actions;
              }
                  CASE_FALLTHROUGH;
          case AC_ATTACK:
          case AC_STAND:
          case AC_ARMOR:
          case AC_MOVE:
          case AC_MAGIC:if (postavy[select_player].actions && (d != AC_MOVE || !lodka))
                         {
                         HUM_ACTION *c;
                         postavy[select_player].direction=viewdir;
                         c=postavy[select_player].zvolene_akce;while (c->action) c++;
                         if (d==AC_MAGIC)
                            {
                            wire_select_rune();
                            return 1;
                            }
                         c->action=d;
                         if (d==AC_ATTACK) c->data1=select_weapon(&postavy[select_player],1);
                         c++;
                         c->action=0;
                         souboje_vybrano(d);
                         }
                       break;
          case AC_CANCEL:zrusit_akce();group_flee = 0;break;
          case AC_START:zahajit_kolo(0);
                        souboje_stisknout(d);
                        return 0;
                        break;
          }
       return 0;

}

char mask_click(int id,int xa,int ya,int xr,int yr)
  {
  char *c;
  int d;
  word *mask;

  id;xa;ya;
  mask=(word *)ablock(H_BATTLE_MASK);
  c=(char *)mask+6+512;
  c+=yr*mask[0]+xr;
  d=*c;
  if (d) {
      return add_pc_action(d);
  }
  bott_draw(1);
  return 1;
  }
void fix_group_direction()
  {
  int i,g;

  g=postavy[select_player].groupnum;
  for(i=0;i<POCET_POSTAV;i++)
     if (postavy[i].used && postavy[i].groupnum==g && !postavy[i].programovano) postavy[i].direction=viewdir;
  }

static void souboje_turn(int smer)
  {
    if (pass_zavora) return;
  norefresh=1;
  hold_timer(TM_BACK_MUSIC,1);
                 viewdir=(viewdir+smer)&3;
                 if (get_shift_key_state()) fix_group_direction();
                 else postavy[select_player].direction=viewdir;
                 render_scene(viewsector,viewdir);
                 hide_ms_at(387);
                 if (smer==1) turn_left();
                 else turn_right();
  OutBuffer2nd();
  bott_draw(0);
  other_draw();
  program_draw();
  draw_medium_map();
  ukaz_mysku();
  showview(0,0,0,0);
  schovej_mysku();
  CopyBuffer2nd();
  ukaz_mysku();
  norefresh=0;
  hold_timer(TM_BACK_MUSIC,0);
  recalc_volumes(viewsector,viewdir);
  mix_back_sound(0);
  }



void programming_keyboard(EVENT_MSG *msg,void **unused)
  {
  uint8_t c;

  unused;
  if (msg->msg==E_KEYBOARD)
     {
     c=quit_request_as_escape(va_arg(msg->data, int))>>8;
     while (_bios_keybrd(_KEYBRD_READY) ) _bios_keybrd(_KEYBRD_READ);
     switch (c)
        {
        case 1:konec(0,0,0,0,0);break;
        case 18:
        case 'M':souboje_turn(1);break;
        case 16:
        case 'K':souboje_turn(-1);break;
        case 61:clk_saveload(0, 0, 0, 0, 0);break;
        case 60:clk_saveload(1, 0, 0, 0, 0);break;
        case 64:go_book(1, 0, 0, 0, 0);break;
        case 65:add_pc_action(AC_MAGIC);break;
        case 66:add_pc_action(AC_ATTACK);break;
        case 67:add_pc_action(AC_MOVE);break;
        case 68:add_pc_action(AC_ARMOR);break;
        case 133:add_pc_action(AC_RUN);break;
        case 134:add_pc_action(AC_STAND);break;
        case 14:add_pc_action(AC_CANCEL);break;
        case 59:game_setup(0,0,0,0,0);break;
        case 57:souboje_dalsi_user();bott_draw(1);break;
        case 28:zahajit_kolo(0);
                souboje_stisknout(AC_START);
                break;
        case 15:
        case 50:
		      if (GlobEvent(MAGLOB_BEFOREMAPOPEN,viewsector,viewdir))
				show_automap(1);
              break;
        case 0x17:unwire_proc();
                 wire_inv_mode(human_selected);
                 break;
        case 82:group_all();break;
        case 0x2E: if (get_control_key_state() && get_shift_key_state()) {
                    console_show(!console_is_visible());
                    }
                    break;
        CASE_KEY_1_6:c=group_sort[c-2];
                     if (postavy[c].used)
                       {
                       select_player=c;
                       zmen_skupinu(postavy+c);
                       bott_draw(1);
                       }
                     break;
        }
     }

  }

void unwire_programming(void)
  {
  disable_click_map();
  send_message(E_DONE,E_KEYBOARD,programming_keyboard);
  delete_from_timer(TM_SCENE);
  wire_proc=wire_programming;
  }



void wire_programming(void)
  {
  schovej_mysku();
  after_spell_wire=wire_programming;
  cur_mode=MD_INBATTLE;
  battle_mode=0;
  change_click_map(clk_souboje,CLK_SOUBOJE);
  send_message(E_ADD,E_KEYBOARD,programming_keyboard);
  add_to_timer(TM_SCENE,gamespeed,-1,souboje_redrawing);
  ukaz_mysku();
  unwire_proc=unwire_programming;
  bott_draw(1);
  showview(0,0,0,0);
  recalc_volumes(viewsector,viewdir);
  if (autostart_round) zahajit_kolo(1);
  }

void wait_to_stop(EVENT_MSG *msg,void **unused)
  {

  unused;
  if (msg->msg==E_IDLE)
     if (!neco_v_pohybu)
        {
        unwire_proc();
        calc_mobs();
        mouse_set_default(H_MS_DEFAULT);
        refresh_scene(0);
        if (prekvapeni) zahajit_kolo(1);else wire_programming();
        msg->msg=-2;
        }
  }


void start_battle()
  {
  spell_cast=0;
  if (check_end_game())
     {
     wire_end_game();
     return;
     }
  if (!GlobEvent(MAGLOB_BEFOREBATTLE,viewsector,viewdir))
  {
    int i;
	battle=0;
    for (i=0;i<MAX_MOBS;i++) mobs[i].vlajky &= ~MOB_IN_BATTLE;
	return;
  }
  neco_v_pohybu=1;
  cur_mode=MD_INBATTLE;
  send_message(E_DONE,E_KEYBOARD,game_keyboard);
  disable_click_map();
  if (!running_battle)
     {
     SEND_LOG("(BATTLE) Battle started (monster: %s)",attack_mob!=NULL?attack_mob->name:"(NULL)");
     memset(pohyblivost_counter,0,sizeof(pohyblivost_counter));
     poloz_vsechny_predmety();
     zacatek_kola();
     running_battle=1;
//     select_player=att_player;
     cislo_kola=0;
     if (prekvapeni)
        {
        unwire_proc();
        mouse_set_default(H_MS_SOUBOJ);
        zahajit_kolo(1);
        }
     else
        {
        unwire_proc();
        wire_programming();
        }
     }
  }

int pocet_zivych(int sector)
  {
  char z=0;
  int i;
  for(i=0;i<POCET_POSTAV;i++)
     {
     THUMAN *p=&postavy[i];

     if (p->used && p->lives && p->sektor==sector && p->inmaphash == current_map_hash) z++;
     }
  return z;
  }

void manashield_check(short *vls,short *lives,short *mana,int dostal)
  {
  if (vls[VLS_KOUZLA] & SPL_MANASHIELD)
     {
     *mana-=*mana>dostal?dostal:*mana;
     if (!*mana) vls[VLS_KOUZLA]&=~SPL_MANASHIELD;
     }
  else
     *lives-=dostal;
  }



char zasah_veci(int sector,TFLY *fl)
  {
  int mob1,mob2;
  TMOB *m1,*m2;
  TITEM *it;

  m1=NULL;
  m2=NULL;
  if (fl->items==NULL && fl->item==0) return 0;
  if (fl->items==NULL) it=glob_items+fl->item-1;else it=&glob_items[*(fl->items)-1];
  if (fl->flags & FLY_DESTROY_SEQ || !fl->speed) return 0;
  if (fl->flags & FLY_DESTROY)
  {
  if (mob_map[sector] && fl->owner>=0)
     {
            if (fl->owner >= 0)
                select_player = fl->owner - 1;
            if (it->druh != TYP_VRHACI)
                return 1;
            if (it->magie)
                area_cast(it->spell, sector, fl->owner, 1);
            mob1 = mob_map[sector] - MOB_START;
            m1 = &mobs[mob1];
            mob2 = m1->next - MOB_START;
            if (mob2 >= 0) {
                m2 = &mobs[mob2];
                if (m2->vlajky & MOB_PASSABLE) {
                    m2 = NULL; //pruchozi nestvury nemaji affekt na hozenou vec
                }
            } else {
                m2 = NULL;
            }
            if (m1->vlajky & MOB_PASSABLE) {
                if (m2 != NULL) {
                    m1 = m2;
                } else {
                    return 0;
                }
            }
            if (m2 == NULL) {
                mob_hit(m1,
                        vypocet_zasahu(it->zmeny, m1->vlastnosti, 1, fl->damage,
                                fl->hit_bonus,0));
                m1->dir = (fl->smer + 2) & 3;
            } else {
                mob_hit(m1,
                        vypocet_zasahu(it->zmeny, m1->vlastnosti, 2, fl->damage,
                                fl->hit_bonus,0));
                mob_hit(m2,
                        vypocet_zasahu(it->zmeny, m1->vlastnosti, 2, fl->damage,
                                fl->hit_bonus,0));
                m1->dir = (fl->smer + 2) & 3;
                m2->dir = (fl->smer + 2) & 3;
            }
            return 1;
        }
  else if (map_coord[sector].flags & MC_PLAYER && (fl->owner<=0 || pocet_zivych(sector)>2))
     {
     int kolik,i,c=0;
     int owner=fl->owner;

     if (it->druh!=TYP_VRHACI) return 1;
     if (it->magie) area_cast(it->spell,sector,fl->owner,1);
     for(i=0,kolik=0;i<POCET_POSTAV;kolik+=(postavy[i].sektor==sector?1:0),i++);
     for(i=0;i<POCET_POSTAV;i++)
        {
        THUMAN *p=&postavy[i];
        if (sector==p->sektor && p->lives && p->used && p->inmaphash == current_map_hash)
           {
           char death;
           short vlastnosti[VLS_MAX];
           memcpy(vlastnosti,p->vlastnosti,sizeof(vlastnosti));
		   if (game_extras & EX_SHIELD_BLOCKING) PodporaStitu(p, vlastnosti);
           death=player_hit(p,vypocet_zasahu(it->zmeny,vlastnosti,kolik,fl->damage,fl->hit_bonus,0),1);
           if (death && owner && hlubina_level) mobs[-owner-1].lives=0; //hlubina - nestvura je mrtva
           c=1;
           }
        bott_draw(1);
        }
     return c;
     }
  }
  else
  if (mob_map[sector] && fl->owner>=0)
     {
     if (fl->owner>=0) select_player=fl->owner-1;
     if (it->druh!=TYP_VRHACI) return 1;
     mob1=mob_map[sector]-MOB_START;m1=&mobs[mob1];
     mob2=m1->next-MOB_START;
     if (mob2>=0)
        {
        int x1=0,y1=0;
        m2=&mobs[mob2];
        switch (fl->smer)
           {
           case 0:x1=fl->ypos;y1=32;break;
           case 1:x1=-32;y1=fl->ypos;break;
           case 2:x1=-fl->ypos;y1=-32;break;
           case 3:x1=32;y1=-fl->ypos;break;
           }
        if (abs(x1-m1->locx+128)+abs(y1-m1->locy+128)>abs(x1-m2->locx+128)+abs(y1-m2->locy+128)) m1=m2;
        }
     if (m1->vlajky & MOB_PASSABLE) return 0;
     mob_hit(m1,vypocet_zasahu(it->zmeny,m1->vlastnosti,(m2!=NULL)+1,fl->damage,fl->hit_bonus,0));
     if (it->druh==TYP_VRHACI) fl->flags|=FLY_DESTROY;
     if (it->umisteni!=PL_SIP && !(it->flags & ITF_DESTROY))
           {
           int i;
           for(i=0;i<MOBS_INV;i++) if (m1->inv[i]==0) { m1->inv[i]=it-glob_items+1;break;}
           if (i==MOBS_INV) fl->flags &=FLY_DESTROY;
           }
     m1->dir=(fl->smer+2)&3;
     return 1;
     }
  else if (map_coord[sector].flags & MC_PLAYER && (fl->owner<=0 || pocet_zivych(sector)>2))
     {
     int kolik,i,j,c=0,r;
     int owner=fl->owner;

     if (it->druh!=TYP_VRHACI) return 1;else fl->flags|=FLY_DESTROY;
     fl->speed=0;
     for(i=0,kolik=0;i<POCET_POSTAV;kolik+=((postavy[i].sektor==sector && postavy[i].lives)?1:0),i++);
     if (kolik) r=rnd(kolik)+1;else r=0;
     for(i=0,j=0;i<r;i+=((postavy[j].sektor==sector && postavy[j].lives)?1:0),j++);
     j--;
        {
        THUMAN *p=&postavy[j];
           {
           char death;
           short vlastnosti[VLS_MAX];
           memcpy(vlastnosti,p->vlastnosti,sizeof(vlastnosti));
           if (game_extras & EX_SHIELD_BLOCKING) PodporaStitu(p, vlastnosti);else  uprav_podle_kondice(p,&kolik);
           death=player_hit(p,vypocet_zasahu(it->zmeny,vlastnosti,kolik,fl->damage,fl->hit_bonus,0),1);
           if (death && owner && hlubina_level) mobs[-owner-1].lives=0; //hlubina - nestvura je mrtva
           c=1;
           }
        bott_draw(1);
        }
     return c;
     }
  return 0;
  }

void cast_wait(EVENT_MSG *msg,void **unused)
  {
  unused;

  if (msg->msg==E_TIMER)
     if (!neco_v_pohybu)
        {
        send_message(E_DONE,E_TIMER,cast_wait);
        delete_from_timer(TM_SCENE);
        wire_main_functs();
        }
  }

void wire_cast_spell()
  {
  if (spell_string.action)
     {
     teleport_target=spell_string.data2;
     select_player=caster;
     cast(spell_string.data1,&postavy[caster],caster,0);
     /*add_to_timer(TM_SCENE,gamespeed,-1,hrat_souboj);
     neco_v_pohybu=1;
     send_message(E_ADD,E_TIMER,cast_wait);*/
     }
  wire_main_functs();
  bott_draw(0);
  }

void wire_fly_casting(int i)
  {
  if (!postavy[i].used || !postavy[i].lives || postavy[i].inmaphash != current_map_hash) return;
  magic_data=&spell_string;
  memset(&spell_string,0,sizeof(spell_string));
  after_spell_wire=wire_cast_spell;
  select_player=i;
  wire_select_rune_fly();
  select_player=caster=i;
  }

static void rozdelit_skryte_bonusy(THUMAN *hu)
  {
  short *vls,*vls2;
  short p[]={VLS_SILA,VLS_SMAGIE,VLS_OBRAT};
  short h[3];
  int i;

  vls=hu->vlastnosti;
  vls2=hu->stare_vls;
  if (vls[p[0]]<vls[p[1]]) {p[1]^=p[0];p[0]^=p[1];p[1]^=p[0];}
  if (vls[p[1]]<vls[p[2]]) {p[2]^=p[1];p[1]^=p[2];p[2]^=p[1];}
  if (vls[p[0]]<vls[p[1]]) {p[1]^=p[0];p[0]^=p[1];p[1]^=p[0];}
  h[0]=rnd(5)+1;h[1]=rnd(3)+1;h[2]=1;
  for(i=0;i<3;i++)
     {
     switch (p[i])
        {
        case VLS_SILA: vls2[VLS_MAXHIT]+=h[i];hu->lives+=h[i];break;
        case VLS_SMAGIE: if (vls2[VLS_SMAGIE]) {vls2[VLS_MAXMANA]+=h[i];hu->mana+=h[i];}break;
        case VLS_OBRAT: vls2[VLS_KONDIC]+=h[i];hu->kondice+=h[i];break;
        }
     }
  prepocitat_postavu(hu);
  }


void check_player_new_level(THUMAN *p)
  {
  int u;

  u=p->level-1;
  while (level_map[u]<=p->exp)
     {
     u++;p->bonus+=5;
     rozdelit_skryte_bonusy(p);
     prepocitat_postavu(p);
     SEND_LOG("(GAME) Character '%s' raised a level %d",p->jmeno,u);
     }
  p->level=u+1;
  }


void send_experience(TMOB *p,int dostal)
  {
  if (select_player<0) return;
  if (isdemon(postavy+select_player)) return;
  if (p->lives<=0)
     {
     int i;
     if (log_combat && p->bonus) {
         wzprintf("Group experience: %d\n", p->bonus);
     }
     for(i=0;i<POCET_POSTAV;i++) if (postavy[i].used && postavy[i].lives)
                                   {
                                   postavy[i].exp+=p->bonus;
                                   check_player_new_level(postavy+i);
                                   }
     if (hlubina_level==1)
        {
        postavy[select_player].lives=0;
        player_check_death(postavy+select_player,0);
        }
     }
  if (dostal>0) {
      int exp = ((float)p->experience*(float)dostal/p->vlastnosti[VLS_MAXHIT]);
      if (log_combat && p->bonus) {
          wzprintf("%s experience: %d\n", postavy[select_player].jmeno, exp);
      }

      postavy[select_player].exp+=(int32_t)exp;
  }
  check_player_new_level(&postavy[select_player]);
  }


void send_weapon_skill(int druh)
  {
  THUMAN *p=&postavy[select_player];

  if (isdemon(p)) return;
  if (p->bonus_zbrani[druh]<MAX_WEAPON_SKILL)
     {
     short *c=&p->bonus_zbrani[druh];
     if (c[0]<MAX_WEAPON_SKILL-1)
        {
        p->weapon_expy[druh]++;
        if (p->weapon_expy[druh]>=weapon_skill[c[0]])
           {
           c[0]++;
           SEND_LOG("(GAME) Character '%s' raised new weaponskill in '%s'",p->jmeno,texty[91+druh]);
           }
        }
     }
  }

char player_check_death(THUMAN *p, char afterround)
  {
  p->used&=~0x80;
  if (p->lives<=0 && p->groupnum) {
	if (!battle || afterround)
     {
     int mp;
     int i;
     if (isdemon(p))
        {
        unaffect_demon(p-postavy);
        return 0;
        }
     for(i=0;i<HUMAN_PLACES;i++)
        {
        int j=p->wearing[i];
        if (j)
           {
           short it[2];
           it[1]=0;it[0]=j;j--;
           if (glob_items[j].flags & ITF_NOREMOVE)
              {
              destroy_items(it);
              p->wearing[i]=0;
              }
           }
        }
     p->groupnum=0;
     p->lives=0;
//     p->kondice=0;
     p->mana=0;
     SEND_LOG("(GAME) Character '%s' died. R.I.P.",p->jmeno);
     if (numplayers(p->sektor,0)==0) map_coord[p->sektor].flags &=~MC_PLAYER;
     mp=map_sectors[p->sektor].sector_type;
     if (mp==S_VODA || mp==S_LAVA || mp==S_VIR) p->sektor=0;
     else  map_coord[p->sektor].flags |= MC_DEAD_PLR;
	 GlobEvent(MAGLOB_ONDEADMAN+p->female,viewsector,viewdir);
     return 1;
     }
  else
	{
	if (p->lives<0) p->lives=0;
	p->used|=0x80;
	}
  }
  if (p->lives>p->vlastnosti[VLS_MAXHIT]) p->lives=p->vlastnosti[VLS_MAXHIT];
  return 0;
  }

char mute_hit_sound=0;

char player_hit(THUMAN *p,int zraneni,char manashield)
  {
  char check=0;

  if (!p->lives) return check;
  if (zraneni>0)
     {
     THE_TIMER *tt;int h;

     if (log_combat) wzprintf("%s was hit: %d, lives: %d\n", p->jmeno, zraneni, p->lives-zraneni);
     if (zraneni>p->lives) zraneni=p->lives;
     p->dostal=zraneni;
     if (manashield) manashield_check(p->vlastnosti,&p->lives,&p->mana,p->dostal);    //manashield pro hrace
     else p->lives-=zraneni;
     if (p->female) h=H_SND_HEK1F;else h=H_SND_HEK1M;
     h+=rnd(2);
     if (!mute_hit_sound)
        {
          if (GlobEvent(MAGLOB_ONHITMAN+p->female,p->sektor,p->direction))
          play_sample_at_sector(h,p->sektor,viewsector,0,0);
        tt=add_to_timer(TM_CLEAR_ZASAHY,100+rnd(100),1,vymaz_zasahy);tt->userdata[0]=p-postavy;tt->userdata[1]=p->dostal;
        }
     mute_hit_sound=0;
     if (immortality) p->lives=p->vlastnosti[VLS_MAXHIT];
     check=player_check_death(p,0);
     }
  else
     {
     p->lives-=zraneni;
     if (p->lives>p->vlastnosti[VLS_MAXHIT]) p->lives=p->vlastnosti[VLS_MAXHIT];
     }
  return check;
  }

void enforce_start_battle()
  {
  if (!battle && see_monster)
     {
     int i;THUMAN *h;
     stop_all_mobs();battle=1;
     for(i=0,h=postavy;i<POCET_POSTAV && (!h->used || !h->lives || h->groupnum!=cur_group);i++,h++);
     if (i>=POCET_POSTAV) h=postavy,i=0;
     att_player=i;
     }
  }

void  uprav_podle_kondice(THUMAN *p,int *chaos)
  {
  if (p->kondice<(p->vlastnosti[VLS_POHYB]/2) || p->vlastnosti[VLS_POHYB]<4)
     {
     *chaos=999;
     }
  else
     p->kondice--;
  }


void correct_level()
  {
  THUMAN *h;
  int i;

  puts("\x7");
  h=postavy;
  for (i=0;i<POCET_POSTAV;i++)
    if (h->used)
      {
      int j;
      if (h->level>37 || (h->level>1 && h->exp<level_map[h->level-2]))
        {
        poloz_vsechny_predmety();
        picked_item=NewArr(short,2);
        picked_item[0]=h->level;
        picked_item[1]=0;
        pick_set_cursor();
        }
      h->level=1;
      for (j=0;;j++) if (h->exp>level_map[j]) h->level=j+2;else break;
      }
  }
