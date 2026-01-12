#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include <libs/types.h>
#include <libs/event.h>
#include <libs/memman.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/bgraph.h>
#include <platform/sound.h>
#include <libs/strlite.h>
#include "engine1.h"
#include <libs/pcx.h>
#include "globals.h"
#include "specproc.h"
#include "temp_storage.h"

#include <assert.h>
#include <string.h>
TMULTI_ACTION_RECORD *macros;                  //tabulka make
TMULTI_ACTION_STATE  macro_state_block;
static const TMULTI_ACTION *first_macro;
void *macro_block = NULL;
int macro_block_size = 0;
int32_t sound_side_flags=0; //kopie flagu steny pro zvuk
static char codelock_memory[16][8];
static short rand_value;
static int program_counter=0;
static char trig_group;

SGlobalEventDef GlobEventList[MAGLOB_NEXTID];

void propadnout(int sector);

#define TRIG_GROUP 1
#define TRIG_SECTOR 2

char get_player_triggered(int p)
  {
  return (trig_group & (1<<p))!=0;
  }

char save_load_trigger(short load)
  {
  if (load>=0) trig_group=(char)load;
  return trig_group;
  }

static inline size_t read_4b(const char *c) {
    size_t b1 = c[0];
    size_t b2 = c[1];
    size_t b3 = c[2];
    size_t b4 = c[3];
    return b1 | (b2<<8) | (b3 << 16) | (b4 << 24);
}

static void read_macro_item(const char *iter, size_t sz, TMULTI_ACTION *target) {
    assert (sz <= sizeof(TMULTI_ACTION));
    char action = ((const TMULTI_ACTION *)iter)->general.action;
    char fixpad = action == MA_FIREB
               || action == MA_LOADL
               || action == MA_DROPI
               || action == MA_CREAT
               || action == MA_LOCK
               || action == MA_CUNIQ
               || action == MA_GUNIQ
               || action == MA_IFJMP
               || action == MA_HAVIT
               || action == MA_RANDJ
               || action == MA_ISFLG
               || action == MA_PICKI
               || action == MA_SNDEX
               || action == MA_IFACT
               || action == MA_CALLS
               || action == MA_MOVEG
               || action == MA_CHFLG
               || action == MA_WBOOK
               || action == MA_GOMOB
               || action == MA_SHRMA
               || action == MA_MONEY
               || action == MA_PLAYA;
    if (fixpad) {
        char *src = (char *)(iter)+3;
        char *dst = (char *)(target)+4;
        memcpy(target, iter, 3);
        memcpy(dst, src, sz - 3);
    } else {
        memcpy(target, iter, sz);
    }
}

void load_macros(int size,void *data)
  {

  char *iter = data;
  size_t count_s = mapsize*4;
  size_t count_m = 0;
  size_t i;
  while ((i = read_4b(iter)) != 0) {
      iter += 4;
      while ((i = read_4b(iter)) != 0) {
          iter += 4 + i;
          ++count_m;
      }
      iter +=4;
  }

  //records[count_s] + states[count_m] + data[count_m]

  size_t total_size = sizeof(TMULTI_ACTION_RECORD) * count_s + count_m + sizeof(TMULTI_ACTION)*count_m;
  macro_block = getmem(total_size);
  memset(macro_block,0, total_size);
  macros = macro_block;
  macro_state_block.states = (uint8_t *)(macros+count_s);
  macro_state_block.count = count_m;
  TMULTI_ACTION *m_iter = (TMULTI_ACTION *)(macro_state_block.states + count_m);
#ifndef NDEBUG
  TMULTI_ACTION *m_end = m_iter + count_m;
#endif
  first_macro = m_iter;

  iter = data;

  while ((i = read_4b(iter)) != 0) {
      assert(m_iter < m_end);
      assert(i < (size_t)mapsize*4);
      TMULTI_ACTION_RECORD *r = macros+i;
      size_t count = 0;
      r->action_list = m_iter;
      iter += 4;
      i = read_4b(iter);
      iter += 4;
      if (i) { do {
              read_macro_item(iter, i, m_iter);
              ++m_iter;
              iter += i;
              i = read_4b(iter);
              iter += 4;
              ++count;
          } while (i != 0);
      }
      r->count = count;
  }

  assert((char *)m_iter == (char *)macro_block+total_size);
  macro_block_size = total_size;
  }

void macro_disp_text(int text,char glob)
  {
  if (glob) bott_disp_text(texty[text]);
  else bott_disp_text(level_texts[text]);
  }

void macro_fireball(const TMA_FIREBALL *z,int sector,int dir)
  {
  LETICI_VEC *fly;
  TITEM *it;

  fly=create_fly();
  it=glob_items+z->item-1;
  fly->items=NULL;
  fly->item=z->item;
  fly->xpos=z->xpos;
  fly->ypos=z->ypos*128/500-64;
  fly->zpos=z->zpos*128/320;
  fly->speed=z->speed;
  fly->velocity=0;
  fly->flags=FLY_IN_FLY | (!it->hmotnost?FLY_NEHMOTNA:0) | (it->flags & ITF_DESTROY?FLY_DESTROY:0)|FLY_NEDULEZITA;
  fly->sector=sector;
  fly->smer=(dir+2)&3;
  fly->owner=0;
  fly->hit_bonus=0;
  fly->damage=0;
  fly->counter=0;
  if (fly->flags & FLY_DESTROY)fly->lives=it->user_value;
  add_fly(fly);
  }

void macro_sound(const TMA_SOUND *p,int psect,int pdir,int sect,int dir)
  {
  char up=4;
  if (sound_side_flags & SD_PRIM_FORV) up=2;
  if (~(p->bit16) & up) {
     if (psect) {
       play_effekt(map_coord[sect].x,map_coord[sect].y,map_coord[psect].x,map_coord[psect].y,psect,pdir,p);
     }      else {
       play_effekt(0,0,0,0,-1,-1,p);
     }
  }
  }

void macro_send_act(const TMA_SEND_ACTION *p)
  {
  delay_action(p->s_action,p->sector,p->side,p->change_bits<<24,0,p->delay);
  }

char autosave_on_enter = 0;

void macro_load_another_map(const TMA_LOADLEV *z)
  {
//  int i,j=0;

  if (battle) return;
  group_all();
/*  for(i=0;i<POCET_POSTAV;i++)
     if (postavy[i].sektor!=viewsector && postavy[i].used && postavy[i].lives)
        {
        bott_disp_text(texty[66]);
        return;
        }
        */
  if (!GlobEvent(MAGLOB_LEAVEMAP,viewsector,viewdir)) return;
/*  for(i=0;i<POCET_POSTAV;i++)
     if (postavy[i].groupnum) {
        if (i!=j)memcpy(&postavy[j++],&postavy[i],sizeof(postavy[i]));else j++;
     }
  if (j<POCET_POSTAV) memset(&postavy[j],0,sizeof(THUMAN)*(POCET_POSTAV-j));
  */
  loadlevel=*z;
  save_map=1;
  autosave_on_enter = 1;
  send_message(E_CLOSE_MAP);
  }

void macro_drop_item(int sector,int smer,short item)
  {
  short itms[2];
  itms[0]=item+1;
  itms[1]=0;
  push_item(sector,(smer+rnd(2))&0x3,itms);
  }

static void macro_create_item(short item)
  {
  if (picked_item!=NULL) poloz_vsechny_predmety();
  picked_item=NewArr(short,2);
  picked_item[0]=item+1;
  picked_item[1]=0;
  pick_set_cursor();
  }


static char decode_lock(char znak,const char *string,uint8_t codenum)
  {
  char *memory;
  char *endm;
  const char *ends;
  int i;

  memory=codelock_memory[codenum];
  memmove(memory,memory+1,7);
  endm=memory+7;
  *endm=znak;
  if (!*string) return 1;
  ends=string;
  for(i=0;i<8;i++,ends++) if (!*ends) break;
  ends--;
  while (ends>=string)
     {
     if (*ends!=*endm) break;
     ends--;endm--;
     }
  if (ends<string) return 0;else return 1;
  }



void cancel_action(int sector,int dir)
  {
  D_ACTION *d,*p;

  p=NULL;d=d_action;
  while (d!=NULL)
     {
     if (d->sector==sector && d->side==dir)
        {
        if (p==NULL) d_action=d->next;else p->next=d->next;
        free(d);
        return;
        }
     p=d;
     d=d->next;
     }
  }

char if_lock(int side,int key_id,int level,const TMA_LOCK *lk)
  {
  int c;

  level;
  if (picked_item==NULL)
     {
     call_macro(side,MC_LOCKINFO);
     return 1;
     }
  c=picked_item[0]-1;
  c=glob_items[c].keynum;
  if (c==-1 && level!=-1)
     {
     int i,j=0,min=-1;
     THUMAN *h=postavy;
     int thlev;
     char s[100];

     for(i=0;i<POCET_POSTAV;h++,i++) if (h->used && h->groupnum==cur_group && h->vlastnosti[VLS_OBRAT]>min)
        {min=h->vlastnosti[VLS_OBRAT];j=i;};
     h=postavy+j;
     if (level==0) level=100;
     if (level>=min)
        if ((int)rnd(100)<=level-min)
           {
           sprintf(s,texty[158+h->female],h->jmeno);
           bott_disp_text(s);
           destroy_items(picked_item);
           free(picked_item);
           picked_item=NULL;pick_set_cursor();
           return 1;
           }
     thlev=rnd(min);
     if (thlev>level)
        {
        sprintf(s,texty[154+h->female],h->jmeno);
        bott_disp_text(s);
        return 0;
        }
     sprintf(s,texty[156+h->female],h->jmeno);
     bott_disp_text(s);
     return 1;
     }
  if (c!=key_id || !c)
     {
     call_macro(side,MC_TOUCHFAIL);
     return 1;
     }
  return 0;
  }

void xchg_block(void *b1, void *b2, int leng) {
    uint8_t *p1 = (uint8_t *)b1;
    uint8_t *p2 = (uint8_t *)b2;

    for (int i = 0; i < leng; i++) {
        uint8_t temp = p1[i];
        p1[i] = p2[i];
        p2[i] = temp;
    }
}

void propadnout(int sector)
  {
  short *i,c,m1,m2;

   for(c=0;c<4;c++)
     {
     pop_item(sector,c,0,&i);
     while(i!=NULL)
        {
        push_item(sector,c,i);
        pop_item(sector,c,0,&i);
        }
     }
  if (mob_map[sector])
     {
     m1=mob_map[sector]-1;
     m2=mobs[m1].next-1;
     mob_map[sector]=0;
     if (map_sectors[sector].sector_type==S_DIRA)
        {
        mobs[m1].sector=map_sectors[sector].sector_tag;
        if (m2>=0) mobs[m2].sector=mobs[m1].sector;
        }
     mob_map[mobs[m1].sector]=m1+1;
     }
  postavy_propadnout(sector);
  }

static void swap_sectors(const TMA_SWAPS *sws)
  {
  TSECTOR *ss1=&map_sectors[sws->sector1],*ss2=&map_sectors[sws->sector2];
  TSTENA *sd1=&map_sides[sws->sector1<<2],*sd2=&map_sides[sws->sector2<<2];
  char c=4;
  char st1=ss2->sector_type,st2=ss1->sector_type;

  for(c=0;c<4;c++) xchg_block(sd1+c,sd2+c,sizeof(TSTENA));
  xchg_block(ss1,ss2,sizeof(TSECTOR));
  if (st1==S_DIRA || st1==S_VODA) propadnout(sws->sector1);
  if (st2==S_DIRA || st2==S_VODA) propadnout(sws->sector2);
  recheck_button(sws->sector1,0);
  recheck_button(sws->sector2,0);
  }

static void hit_1_player(int postava,const TMA_WOUND *w,int chaos)
  {
  int mode=w->pflags>>1;
  int zivel=mode-2;
  int dostal;
  THUMAN *h=postavy+postava;

  if (mode==0)
     {
     dostal=w->minor+rnd(w->major-w->minor+1);
     }
  else if (mode==1)
     {
     short vls[24];

     memset(vls,0,sizeof(vls));
     vls[VLS_UTOK_L]=w->minor;
     vls[VLS_UTOK_H]=w->major;
     dostal=vypocet_zasahu(vls,h->vlastnosti,chaos,0,0,0);
     }
  else
     {
     short vls[24];

     memset(vls,0,sizeof(vls));
     vls[VLS_MGSIL_L]=w->minor;
     vls[VLS_MGSIL_H]=w->major;
     vls[VLS_MGZIVEL]=zivel;
     dostal=vypocet_zasahu(vls,h->vlastnosti,chaos,0,0,0);
     }
  player_hit(h,dostal,0);
  }

static void hit_player(const TMA_WOUND *w,int sector)
  {
  int i,pocet,r;

  for(i=0,pocet=0;i<POCET_POSTAV;i++) if (get_player_triggered(i)) pocet++;
  if (!pocet) return;
  if (~w->pflags & 1)
     {
     r=rnd(pocet)+1;
     for(i=0;i<POCET_POSTAV && r>0;i++) if (get_player_triggered(i)) r--;
     i--;
     hit_1_player(i,w,pocet);
     }
  else
     for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==sector) hit_1_player(i,w,pocet);
  bott_draw(1);
  }

static TMULTI_ACTION_RECORD go_macro(int side,int abs_pos)
  {
  TMULTI_ACTION_RECORD ret = macros[side];


  program_counter=abs_pos;
  if (ret.action_list!=NULL && ret.count > (size_t)abs_pos) {
      ret.action_list += abs_pos;
      ret.count -= abs_pos;
  }
  return ret;
  }

static char monster_in_game(void)
  {
  int i;
  for(i=0;i<MAX_MOBS;i++) if (mobs[i].vlajky & MOB_LIVE && ~mobs[i].vlajky & MOB_MOBILE) return 1;
  return 0;
  }



static char is_monster(word sector, void *flag)
  {
    char *monster_test = (char *)flag;
  int m1,m2;
  m1=mob_map[sector]-1;
  if (m1>=0)
     {
     m2=mobs[m1].next-1;
     if (~mobs[m1].vlajky & MOB_MOBILE && (m2<0 || ~mobs[m2].vlajky & MOB_MOBILE)) *monster_test=1;
     }
  return !*monster_test;
  }

static char monster_in_room(int sector)
  {
  char monster_test=0;
  is_monster(sector, &monster_test);
  if (!monster_test) labyrinth_find_path(sector,65535,SD_MONST_IMPS,is_monster,NULL, &monster_test);
  return monster_test;
  }

static int if_jump(const TMA_TWOP *i,int side,int abs_pos)
  {
  TSTENA *sd=map_sides+side;
  int go,test,flag;
  char ok=0;

  test=abs(i->parm1)-1;
  go=i->parm2;
  flag=sd->flags;
  if (test<32) ok=(flag & (1<<test))!=0;
  else
     switch(test)
     {
     case 32:ok=monster_in_game();break;
     case 33:ok=monster_in_room(side>>2);break;
     }
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }

static int if_have_item(const TMA_TWOP *i,int abs_pos)
  {
  int go,test,ip;
  char ok=0;

  test=abs(i->parm1);
  go=i->parm2;
  for(ip=0;ip<POCET_POSTAV && !ok;ip++) if (get_player_triggered(ip)) ok=(q_item_one(ip,test)!=NULL);
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }

static int ma_randjmp(const TMA_TWOP *i,int abs_pos)
  {
  int go,test;
  char ok=0;

  test=i->parm1;
  go=i->parm2;
  if (rand_value==-1) rand_value=rnd(100);
  ok=rand_value<test;
  if (ok) return go+abs_pos;else return -1;
  }


static int ma_test_action(const TMA_TWOP *i,int act,int abs_pos)
  {
  int go,test;
  char ok=0;

  test=abs(i->parm1)-1;
  go=i->parm2;
  ok=(test==act);
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }


static int ma_if_flag(const TMA_TWOP *i,int abs_pos)
  {
  int go,test;
  char ok=0;

  test=abs(i->parm1)-1;
  go=i->parm2;
  ok=test_flag(test);
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }

static int ma_picki(const TMA_TWOP *i,int abs_pos)
  {
  int go,test;
  char ok=0;

  test=abs(i->parm1);
  go=i->parm2;
  if (picked_item!=NULL) ok=picked_item[0]==test;else ok=0;
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }

static void ma_wbook(const TMA_LOADLEV *l)
  {
  const char *s = build_pathname(2, gpathtable[SR_MAP], l->name);
  add_text_to_book(s,l->start_pos);
  play_fx_at(FX_BOOK);
  }

static void ma_send_experience(int32_t what)
  {
  int maxl,i;
  THUMAN *h;

  for(i=0,maxl=0,h=postavy;i<POCET_POSTAV;i++,h++)
     if (h->used && maxl<h->level) maxl=h->level;
  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
     if (h->used)
        {
        h->exp+=what*h->level/maxl;
        check_player_new_level(h);
        }
  bott_draw(0);
  }

static void ma_move_group(int where,int turn,char effect)
  {
  if (!save_load_trigger(-1)) return;
  if (!effect)
     {
     int i;
     THUMAN *h=postavy;
     for(i=0;i<POCET_POSTAV;i++,h++)
        if (get_player_triggered(i)) h->sektor=where,h->direction=turn;
     viewsector=where;
     viewdir=turn;
     }
  else
     {
     THUMAN *h=postavy;
     int i=0;
     int sctr=0;
     char kdo=0;
     for(i=0;i<POCET_POSTAV;i++,h++)
        if (get_player_triggered(i)) kdo|=1<<i,sctr=postavy[i].sektor;
     postavy_teleport_effect(where,turn,kdo,viewsector==sctr);
     }
  }

static void build_trig_group(char mode,int side)
  {
  int i;
  THUMAN *h;

  trig_group=0;
  switch (mode)
     {
     case TRIG_GROUP:if (battle && select_player>=0) trig_group|=1<<select_player;
                     else for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
                            if (h->used && h->groupnum==cur_group) trig_group|=1<<i;
                     break;
     case TRIG_SECTOR:side>>=2;
                      for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
                       if (h->used && h->sektor==side && h->inmaphash == current_map_hash) trig_group|=1<<i;
                     break;
     }
  }

static int  ma_play_anim(const char *filename,char cls)
  {


  unwire_main_functs();
  const char *a = build_pathname(2, gpathtable[SR_VIDEO],filename);
  curcolor=0;
  if (cls)
     {
     bar32(0,0,639,479);
     showview(0,0,0,0);
     }
  mute_all_tracks(1);
  cancel_pass=0;
  play_movie_seq(a,cls?60:SCREEN_OFFLINE);
  wire_main_functs();
  return 0;
  }

static void ma_control_mob(int from,int to)
  {
  word *path;
  int m;

  if (mob_map[from]==0) return;
  if (labyrinth_find_path(from,to,SD_MONST_IMPS,NULL,&path,NULL)==0) return;
  m=mob_map[from]-1;
  send_mob_to(m,path);
  }

static void ma_drop_money(int sect,int side,const TMULTI_ACTION *q)
  {
  int x;
  x=rnd(q->twop.parm2-q->twop.parm1+1);
  x+=q->twop.parm1;
  x=create_item_money(x)-1;
  if (x) macro_drop_item(sect,side,x);
  }

void macro_change_music(int textindex)
{
  char *trackdef=level_texts[textindex];

  create_playlist(trackdef);
  change_music(get_next_music_from_playlist());
}

void macro_register_global_event(const TMULTI_ACTION *q)
{
  GlobEventList[q->globe.event].cancel=q->globe.cancel;
  GlobEventList[q->globe.event].sector=q->globe.sector;
  GlobEventList[q->globe.event].side=q->globe.side;
  GlobEventList[q->globe.event].param=q->globe.param;
  if (q->globe.event>=MAGLOB_ONTIMER1 && q->globe.event<=MAGLOB_ONTIMER4)
  {
    if (GlobEventList[q->globe.event].param>0)
      GlobEventList[q->globe.event].param+=game_time;
    else
    {
      int32_t den=24*60*6;
      int32_t cas=((-GlobEventList[q->globe.event].param/100)*60+(-GlobEventList[q->globe.event].param%100))*6;
      int32_t curtm=game_time % den;
      if (cas<=curtm) cas+=den;
      GlobEventList[q->globe.event].param=game_time-curtm+cas;
    }
  }
}

void call_macro_ex(int side,int flags, int runatside);

void call_macro(int side,int flags)
{
  call_macro_ex(side,flags,side);
}




void call_macro_ex(int side, int flags, int runatside) {
    const TMULTI_ACTION *z;
    short saved_trigger;
    short ls = last_send_action;
    short save_rand;

    if (side >= mapsize * 4)
        return;
    if (runatside >= mapsize * 4)
        return;
    save_rand = rand_value;
    rand_value = -1;
    TMULTI_ACTION_RECORD mrec = macros[runatside];
    program_counter = 0;
    int count_actions = 0;
    if (mrec.action_list == NULL)
        return;
    SEND_LOG("(MULTIACTIONS) Start: Side %.1f Call %X",(float)(runatside/4)+((float)(runatside & 3)/10),flags);
    saved_trigger = save_load_trigger(-1);
    if (flags & (MC_PASSSUC | MC_PASSFAIL | MC_EXIT))
        build_trig_group(TRIG_GROUP, 0);
    else
        build_trig_group(TRIG_SECTOR, side);
    while (mrec.count != 0) {
        z = mrec.action_list;
        if (z->general.flags & flags) {
            int jmp_to = -1;
            int stindex = z - first_macro;
            char cancel = 0;
            if (!z->general.once || !macro_state_block.states[stindex]) {
                macro_state_block.states[stindex] = 1;
                cancel = z->general.cancel;
                count_actions++;
                switch (z->general.action) {
                    default:
                        break;
                    case MA_GEN:
                        break;
                    case MA_SOUND:
                        macro_sound(&z->sound, side >> 2, side & 3, viewsector,
                                viewdir);
                        break;
                    case MA_TEXTG:
                        macro_disp_text(z->text.textindex, 1);
                        break;
                    case MA_TEXTL:
                        macro_disp_text(z->text.textindex, 0);
                        break;
                    case MA_SENDA:
                        macro_send_act(&z->send_a);
                        break;
                    case MA_FIREB:
                        macro_fireball(&z->fireball, side >> 2, side & 3);
                        break;
                    case MA_DESTI:
                        if (picked_item != NULL) {
                            destroy_items(picked_item);
                            free(picked_item);
                            picked_item = NULL;
                            pick_set_cursor();
                        }
                        break;
                    case MA_LOADL:
                        macro_load_another_map(&z->loadlev);
                        break;
                    case MA_DROPI:
                        macro_drop_item(side >> 2, side & 0x3, z->dropi.item);
                        break;
                    case MA_CREAT:
                        macro_create_item(z->dropi.item);
                        break;
                    case MA_DIALG:
                        start_dialog(z->text.textindex, -1);
                        break;
                    case MA_SSHOP:
                        enter_shop(z->text.textindex);
                        break;
                    case MA_CLOCK:
                        cancel = decode_lock(z->clock.znak, z->clock.string,
                                z->clock.codenum);
                        break;
                    case MA_CACTN:
                        cancel_action(z->cactn.sector, z->cactn.dir);
                        break;
                    case MA_LOCK:
                        cancel = if_lock(side, z->lock.key_id,
                                z->lock.thieflevel, &z->lock);
                        break;
                    case MA_SWAPS:
                        swap_sectors(&z->swaps);
                        break;
                    case MA_WOUND:
                        hit_player(&z->wound, side >> 2);
                        break;
                    case MA_IFJMP:
                        jmp_to = if_jump(&z->twop, side, program_counter);
                        break;
                    case MA_STORY:
                        write_story_text(level_texts[z->text.textindex]);
                        break;
                    case MA_HAVIT:
                        jmp_to = if_have_item(&z->twop, program_counter);
                        break;
                    case MA_SNDEX:
                        ma_send_experience(z->twop.parm1);
                        break;
                    case MA_IFACT:
                        jmp_to = ma_test_action(&z->twop, ls, program_counter);
                        break;
                    case MA_CALLS:
                        if (call_map_event(z->twop.parm1, side >> 2, side & 3,
                                z->twop.parm2, flags))
                            call_macro(side, MC_SPEC_SUCC);
                        break;
                    case MA_MOVEG:
                        ma_move_group(z->twop.parm1, z->twop.parm2 & 3,
                                z->twop.parm2 >> 7);
                        break;
                    case MA_PLAYA:
                        ma_play_anim(z->loadlev.name, z->loadlev.dir);
                        break;
                    case MA_ISFLG:
                        jmp_to = ma_if_flag(&z->twop, program_counter);
                        break;
                    case MA_CHFLG:
                        change_flag(z->twop.parm1, (char) z->twop.parm2);
                        break;
                    case MA_CUNIQ:
                        macro_drop_item(side >> 2, side & 0x3,
                                create_unique_item(&z->uniq.item) - 1);
                        break;
                    case MA_MONEY:
                        ma_drop_money(side >> 2, side & 0x3, z);
                        break;
                    case MA_GUNIQ:
                        macro_create_item(create_unique_item(&z->uniq.item) - 1);
                        break;
                    case MA_PICKI:
                        jmp_to = ma_picki(&z->twop, program_counter);
                        break;
                    case MA_WBOOK:
                        ma_wbook(&z->loadlev);
                        break;
                    case MA_RANDJ:
                        jmp_to = ma_randjmp(&z->twop, program_counter);
                        break;
                    case MA_ENDGM:
                        unwire_proc();
                        send_message(E_CLOSE_MAP, (void*) 255);
                        break;
                    case MA_GOMOB:
                        ma_control_mob(z->twop.parm1, z->twop.parm2);
                        break;
                    case MA_SHRMA:
                        call_macro_ex(side, flags,
                                z->twop.parm1 * 4 + z->twop.parm2);
                        break;
                    case MA_MUSIC:
                        macro_change_music(z->text.textindex);
                        break;
                    case MA_GLOBE:
                        macro_register_global_event(z);
                        break;
                }
            }
            if (jmp_to != -1) {
                mrec = go_macro(runatside, jmp_to);
                program_counter = jmp_to;
            } else if (cancel) {
                break;
            } else {
                program_counter++;
                ++mrec.action_list;
                --mrec.count;
            }
        } else {
            program_counter++;
            ++mrec.action_list;
            --mrec.count;
        }
    }
    rand_value = save_rand;
    save_load_trigger(saved_trigger);
    (void)count_actions;
    SEND_LOG("(MULTIACTIONS) End: Sector %d, Side %d,  Call %X, Actions: %d",runatside/4,runatside & 3,flags,count_actions);

}

char save_codelocks(TMPFILE_WR *fsta)
  {
  temp_storage_write(codelock_memory,sizeof(codelock_memory)*1,fsta);
  return 0;
  }


char load_codelocks(TMPFILE_RD *fsta)
  {
  return !temp_storage_read(codelock_memory,sizeof(codelock_memory)*1,fsta);
  }


