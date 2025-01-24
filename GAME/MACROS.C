#include <skeldal_win.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <bios.h>
#include <mem.h>
#include <types.h>
#include <event.h>
#include <memman.h>
#include <devices.h>
#include <bmouse.h>
#include <bgraph.h>
#include <zvuk.h>
#include <strlite.h>
#include "engine1.h"
#include <pcx.h>
#include "globals.h"
#include "specproc.h"

int **macros=NULL;
void *macro_block;
int macro_block_size;
long sound_side_flags=0; //kopie flagu steny pro zvuk
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

void load_macros(int size,void *data)
  {
  int *r;

  if (macros!=NULL) free(macros);
  macros=(int **)getmem(mapsize*sizeof(int *)*4);
  memset(macros,0,mapsize*sizeof(char *)*4);
  memset(codelock_memory,0,sizeof(codelock_memory));
  r=data;
  while (*r)
     {
     macros[*r]=r+1;
     r++;
     while(*r) r=(int *)((char *)r+*r+4);
     r++;
     }
  macro_block=data;
  macro_block_size=size;
  }

void macro_disp_text(int text,char glob)
  {
  if (glob) bott_disp_text(texty[text]);
  else bott_disp_text(level_texts[text]);
  }

void macro_fireball(TMA_FIREBALL *z,int sector,int dir)
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

void macro_sound(TMA_SOUND *p,int psect,int pdir,int sect,int dir)
  {
  char up=4;
  if (sound_side_flags & SD_PRIM_FORV) up=2;
  if (~(p->bit16) & up)
     if (psect)
       play_effekt(map_coord[sect].x,map_coord[sect].y,map_coord[psect].x,map_coord[psect].y,dir,pdir,p);
     else
       play_effekt(0,0,0,0,-1,-1,p);
  }

void macro_send_act(TMA_SEND_ACTION *p)
  {
  delay_action(p->s_action,p->sector,p->side,p->change_bits<<24,0,p->delay);
  }

void macro_load_another_map(TMA_LOADLEV *z)
  {
  int i,j=0;

  if (battle) return;
  group_all();
  for(i=0;i<POCET_POSTAV;i++)
     if (postavy[i].sektor!=viewsector && postavy[i].used && postavy[i].lives)
        {
        bott_disp_text(texty[66]);
        return;
        }
  if (!GlobEvent(MAGLOB_LEAVEMAP,viewsector,viewdir)) return;
  for(i=0;i<POCET_POSTAV;i++)
     if (postavy[i].groupnum)
        if (i!=j)memcpy(&postavy[j++],&postavy[i],sizeof(postavy[i]));else j++;
  if (j<POCET_POSTAV) memset(&postavy[j],0,sizeof(THUMAN)*(POCET_POSTAV-j));
  loadlevel=*z;
  send_message(E_CLOSE_MAP);
  save_map=1;
  reg_grafiku_postav();
  reroll_all_shops();
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].used) postavy[i].sektor=z->start_pos;
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


static char decode_lock(char znak,char *string,char codenum)
  {
  char *memory;
  char *endm;
  char *ends;
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

char if_lock(int side,int key_id,int level,TMA_LOCK *lk)
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
        if (rnd(100)<=level-min)
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
        //if (abs(level-thlev)<10 && h->vlastnosti[VLS_THIEF]<100)
          // {
           //h->vlastnosti[VLS_THIEF]++;
           //h->stare_vls[VLS_THIEF]++;
           //}
        lk->thieflevel=1;
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

void xchg_block(void *b1,void *b2,int leng)
//#pragma aux xchg_block parm[edi][esi][ecx]=
{
__asm
  {
     mov edi,b1
     mov esi,b2
     mov ecx,leng
 lp1: mov  al,[edi]
     mov  ah,[esi]
     mov  [edi],ah
     mov  [esi],al
     inc  edi
     inc  esi
     dec  ecx
     jnz  lp1
  }
}

static void propadnout(int sector)
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

static void swap_sectors(TMA_SWAPS *sws)
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

static void hit_1_player(int postava,TMA_WOUND *w,int chaos)
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
     dostal=vypocet_zasahu(vls,h->vlastnosti,chaos,0,0);
     }
  else
     {
     short vls[24];

     memset(vls,0,sizeof(vls));
     vls[VLS_MGSIL_L]=w->minor;
     vls[VLS_MGSIL_H]=w->major;
     vls[VLS_MGZIVEL]=zivel;
     dostal=vypocet_zasahu(vls,h->vlastnosti,chaos,0,0);
     }
  player_hit(h,dostal,0);
  }

static void hit_player(TMA_WOUND *w,int sector)
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

static TMULTI_ACTION *go_macro(side,abs_pos)
  {
  int *r;
  int mcsiz;


  program_counter=abs_pos;
  r=macros[side];
  if (r==NULL) return NULL;
  mcsiz=*r++;
  while (abs_pos--)
     {
     r=(int *)((char *)r+mcsiz);
     mcsiz=*r++;
     if (!mcsiz) return NULL;
     }
  return (TMULTI_ACTION *)r;
  }

static char monster_in_game(void)
  {
  int i;
  for(i=0;i<MAX_MOBS;i++) if (mobs[i].vlajky & MOB_LIVE && ~mobs[i].vlajky & MOB_MOBILE) return 1;
  return 0;
  }

static char monster_test;

static char is_monster(word sector)
  {
  int m1,m2;
  m1=mob_map[sector]-1;
  if (m1>=0)
     {
     m2=mobs[m1].next-1;
     if (~mobs[m1].vlajky & MOB_MOBILE && (m2<0 || ~mobs[m2].vlajky & MOB_MOBILE)) monster_test=1;
     }
  return !monster_test;
  }

static char monster_in_room(int sector)
  {
  monster_test=0;
  is_monster(sector);
  if (!monster_test) labyrinth_find_path(sector,65535,SD_MONST_IMPS,is_monster,NULL);
  return monster_test;
  }

static int if_jump(TMA_TWOP *i,int side,int abs_pos)
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

static int if_have_item(TMA_TWOP *i,int abs_pos)
  {
  int go,test,ip;
  char ok=0;

  test=abs(i->parm1);
  go=i->parm2;
  for(ip=0;ip<POCET_POSTAV && !ok;ip++) if (get_player_triggered(ip)) ok=(q_item_one(ip,test)!=NULL);
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }

static int ma_randjmp(TMA_TWOP *i,int abs_pos)
  {
  int go,test;
  char ok=0;

  test=i->parm1;
  go=i->parm2;
  if (rand_value==-1) rand_value=rnd(100);
  ok=rand_value<test;
  if (ok) return go+abs_pos;else return -1;
  }


static int ma_test_action(TMA_TWOP *i,int act,int abs_pos)
  {
  int go,test;
  char ok=0;

  test=abs(i->parm1)-1;
  go=i->parm2;
  ok=(test==act);
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }


static int ma_if_flag(TMA_TWOP *i,int abs_pos)
  {
  int go,test;
  char ok=0;

  test=abs(i->parm1)-1;
  go=i->parm2;
  ok=test_flag(test);
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }

static int ma_picki(TMA_TWOP *i,int abs_pos)
  {
  int go,test;
  char ok=0;

  test=abs(i->parm1);
  go=i->parm2;
  if (picked_item!=NULL) ok=picked_item[0]==test;else ok=0;
  if (i->parm1<0) ok=!ok;
  if (ok) return go+abs_pos;else return -1;
  }

static void ma_wbook(TMA_LOADLEV *l)
  {
  char *s;
	s=find_map_path(l->name);
  add_text_to_book(s,l->start_pos);
  play_fx_at(FX_BOOK);
	free(s);
  }

static void ma_send_experience(long what)
  {
  int maxl,i;
  THUMAN *h;

  for(i=0,maxl=0,h=postavy;i<POCET_POSTAV;i++,h++)
     if (h->used && maxl<h->level) maxl=h->level;
  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
     if (h->used && h->lives)
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
     int i;
     int sctr;
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
                       if (h->used && h->sektor==side) trig_group|=1<<i;
                     break;
     }
  }

static ma_play_anim(char *filename,char cls)
  {
  char *a;

  unwire_main_functs();
  concat(a,pathtable[SR_VIDEO],filename);
  curcolor=0;
  if (cls)
     {
     bar(0,0,639,479);
     showview(0,0,0,0);
     }
  mute_all_tracks(1);
  cancel_render=0;
  cancel_pass=0;
  play_movie_seq(a,cls?60:SCREEN_OFFLINE);
  wire_main_functs();
  }

static char ma_control_mob_control(word sector)
  {
  sector;
  return 1;
  }

static void ma_control_mob(int from,int to)
  {
  word *path;
  int m;

  if (mob_map[from]==0) return;
  if (labyrinth_find_path(from,to,SD_MONST_IMPS,ma_control_mob_control,&path)==0) return;
  m=mob_map[from]-1;
  send_mob_to(m,path);
  }

static void ma_drop_money(int sect,int side,TMULTI_ACTION *q)
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
  char *nextTrack;

  create_playlist(trackdef);
  play_next_music(&nextTrack);
  change_music(nextTrack);
}

void macro_register_global_event(TMULTI_ACTION *q)
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
      long den=24*60*6;
      long cas=((-GlobEventList[q->globe.event].param/100)*60+(-GlobEventList[q->globe.event].param%100))*6;
      long curtm=game_time % den;
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



void call_macro_ex(int side,int flags, int runatside)
  {
  TMULTI_ACTION *z,*p;
  int *r;
  int mcsiz;
  int c;
  short saved_trigger;
  short ls=last_send_action;
  short save_rand;

  if (side>=mapsize*4) return;
  if (runatside>=mapsize*4) return;
  save_rand=rand_value;rand_value=-1;
  r=macros[runatside];
  program_counter=0;
  if (r==NULL) return;
  SEND_LOG("(MULTIACTIONS) Start: Side %.1f Call %X",(float)(runatside/4)+((float)(runatside & 3)/10),flags);
  saved_trigger=save_load_trigger(-1);
  if (flags & (MC_PASSSUC | MC_PASSFAIL | MC_EXIT)) build_trig_group(TRIG_GROUP,0);else build_trig_group(TRIG_SECTOR,side);
  while ((mcsiz=*r)!=0)
     {
     r++;
     zde:
     z=(TMULTI_ACTION *)r;
     if (z->general.flags & flags)
        {
        c=-1;
        switch (z->general.action)
          {
          case MA_GEN:break;
          case MA_SOUND:macro_sound(&z->sound,side>>2,side & 3,viewsector,viewdir);break;
          case MA_TEXTG:macro_disp_text(z->text.textindex,1);break;
          case MA_TEXTL:macro_disp_text(z->text.textindex,0);break;
          case MA_SENDA:macro_send_act(&z->send_a);break;
          case MA_FIREB:macro_fireball(&z->fireball,side>>2,side & 3);break;
          case MA_DESTI:if (picked_item!=NULL)
                             {
                             destroy_items(picked_item);
                             free(picked_item);
                             picked_item=NULL;
                             pick_set_cursor();
                             }
                        break;
          case MA_LOADL:macro_load_another_map(&z->loadlev);break;
          case MA_DROPI:macro_drop_item(side>>2,side & 0x3,z->dropi.item);break;
          case MA_CREAT:macro_create_item(z->dropi.item);break;
          case MA_DIALG:start_dialog(z->text.textindex,-1);break;
          case MA_SSHOP:enter_shop(z->text.textindex);break;
          case MA_CLOCK:z->general.cancel=decode_lock(z->clock.znak,z->clock.string,z->clock.codenum);break;
          case MA_CACTN:cancel_action(z->cactn.sector,z->cactn.dir);break;
          case MA_LOCK :z->general.cancel=if_lock(side,z->lock.key_id,z->lock.thieflevel,&z->lock);break;
          case MA_SWAPS:swap_sectors(&z->swaps);break;
          case MA_WOUND:hit_player(&z->wound,side>>2);break;
          case MA_IFJMP:c=if_jump(&z->twop,side,program_counter);break;
          case MA_STORY:write_story_text(level_texts[z->text.textindex]);break;
          case MA_HAVIT:c=if_have_item(&z->twop,program_counter);break;
          case MA_SNDEX:ma_send_experience(z->twop.parm1);break;
          case MA_IFACT:c=ma_test_action(&z->twop,ls,program_counter);break;
          case MA_CALLS:if (call_map_event(z->twop.parm1,side>>2,side & 3,z->twop.parm2,flags)) call_macro(side,MC_SPEC_SUCC);break;
          case MA_MOVEG:ma_move_group(z->twop.parm1,z->twop.parm2 & 3,z->twop.parm2>>7);break;
          case MA_PLAYA:ma_play_anim(z->loadlev.name,z->loadlev.dir);break;
          case MA_ISFLG:c=ma_if_flag(&z->twop,program_counter);break;
          case MA_CHFLG:change_flag(z->twop.parm1,(char)z->twop.parm2);break;
          case MA_CUNIQ:macro_drop_item(side>>2,side & 0x3,create_unique_item(&z->uniq.item)-1);break;
          case MA_MONEY:ma_drop_money(side>>2,side & 0x3,z);break;
          case MA_GUNIQ:macro_create_item(create_unique_item(&z->uniq.item)-1);break;
          case MA_PICKI:c=ma_picki(&z->twop,program_counter);break;
          case MA_WBOOK:ma_wbook(&z->loadlev);break;
          case MA_RANDJ:c=ma_randjmp(&z->twop,program_counter);break;
          case MA_ENDGM:unwire_proc();send_message(E_CLOSE_MAP,(void *)255);break;
          case MA_GOMOB:ma_control_mob(z->twop.parm1,z->twop.parm2);break;
		  case MA_SHRMA:call_macro_ex(side,flags,z->twop.parm1*4+z->twop.parm2);break;
		  case MA_MUSIC:macro_change_music(z->text.textindex);break;
          case MA_GLOBE:macro_register_global_event(z);break;
          }
       if (c!=-1) p=go_macro(runatside,c);else p=NULL;
       if (p!=NULL) {r=(int *)p;program_counter=c;mcsiz=r[-1];goto zde;}
       if (z->general.once)
          {
          z->general.action=0;
          z->general.once=0;
          if (z->general.cancel)
             {
             z->general.cancel=0;
             goto end;
             }
          }
        if (z->general.cancel) return;
       }
     r=(int *)((char *)r+mcsiz);
     program_counter++;
     }
  end:
  rand_value=save_rand;
  save_load_trigger(saved_trigger);
  SEND_LOG("(MULTIACTIONS) End: Side %.1f Call %X",(float)(runatside/4)+((float)(runatside & 3)/10),flags);
  }

static char lock_saved=255;
static char lock_empty=254;


char save_codelocks(FILE *fsta)
  {
  char *c;
  int i;

  c=(char *)&codelock_memory;
  i=sizeof(codelock_memory);
  while(*c==0 && i) c++,i--;
  if (!i)
     {
     SEND_LOG("(SAVELOAD) Codelocks wasn't used in this map... not saved",0,0);
     return !fwrite(&lock_empty,sizeof(lock_empty),1,fsta);
     }
  SEND_LOG("(SAVELOAD) Storing code-locks...",0,0);
  if (!fwrite(&lock_saved,sizeof(lock_saved),1,fsta)) return 1;
  return !fwrite(codelock_memory,sizeof(codelock_memory),1,fsta);
  }


char load_codelocks(FILE *fsta)
  {
  char c;

  if (!fread(&c,sizeof(lock_empty),1,fsta)) return 1;
  if (c!=lock_saved)
     {
     if (c!=lock_empty)
        {
        fseek(fsta,-1,SEEK_CUR); //uprav pripadne stare verze savegamu ktere nemaji codelocky
        SEND_LOG("(ERROR) Invalid value for codelocks... may be it's old version of savegame",0,0);
        }
     memset(codelock_memory,0,sizeof(codelock_memory));
     return 0;
     }
  SEND_LOG("(SAVELOAD) Restoring code-locks for this map...",0,0);
  return !fread(codelock_memory,sizeof(codelock_memory),1,fsta);
  }

