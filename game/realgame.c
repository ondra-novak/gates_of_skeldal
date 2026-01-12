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
#include <libs/inicfg.h>

#include "lang.h"
#include <ctype.h>
#include <string.h>


#define A_OPEN_DOOR 1
#define A_CLOSE_DOOR 2
#define A_OPEN_CLOSE 3
#define A_RUN_PRIM 4
#define A_SHOW_PRIM 5
#define A_HIDE_PRIM 6
#define A_SHOW_HIDE_PRIM 7
#define A_RUN_SEC 8
#define A_SHOW_SEC 9
#define A_HIDE_SEC 10
#define A_SHOW_HIDE_SEC 11
#define A_HIDE_PRIM_SEC 12
#define A_DISPLAY_TEXT 13
#define A_CODELOCK_LOG 14
#define A_OPEN_TELEPORT 15
#define A_CLOSE_TELEPORT 16
#define A_CODELOCK_LOG2 17
#define A_CODELOCK_LOG3 18

#define MAX_FLY_SECT 4

#define DIS (char *)0x1
#define START_HANDLE hl_ptr





char pass_zavora=0;

char map_with_password=0;

MAPGLOBAL mglob={
        {"","","",""},0,0,0,1,0,"",0,0,0
};
TSTENA *map_sides;
TSECTOR *map_sectors;
TVYKLENEK *map_vyk;         //mapa vyklenku
word vyk_max;               //pocet vyklenku v mape
short **map_items = 0;
short *destroyed_items = 0;;
char *flag_map;
TMAP_EDIT_INFO *map_coord;
TSTR_LIST level_texts;
int mapsize;
int num_ofsets[10]; //tabulka ofsetu pro cisla sten k levelu
int num_ofsets_count[10]; //count of items in each table
char sekceid[]="<BLOCK>";
char datapath;
D_ACTION *d_action={NULL};
int end_ptr;
char pass_all_mobs = 0; //cheat make all mobs passable
uint8_t cur_group=1;
uint8_t group_select=1;
char cancel_pass=0;
char break_sleep=0;
char enable_sort=0;
int  sleep_ticks=MAX_SLEEP;
char side_touched=1;
char save_map=1;
//char formace=0;
char last_send_action;      //naposled vyslana akce
char insleep=0;
short moving_player=0;

char force_start_dialog=0;
int start_dialog_number=0;
int start_dialog_mob=0;
uint32_t current_map_hash;

int32_t money=0;

char runes[5]={0,0,0,0,0};

char group_sort[POCET_POSTAV]={0,1,2,3,4,5};

int32_t load_section(FILE *f,void **section, int *sct_type,int32_t *sect_size)
//
  {
  int32_t s;
  char c[20];

  *section=NULL;
  fread(c,1,sizeof(sekceid),f);
  if (strcmp(c,sekceid)) return -1;
  fread(sct_type,1,sizeof(*sct_type),f);
  fread(sect_size,1,sizeof(*sect_size),f);
  fread(&s,1,sizeof(s),f);
  *section=getmem(*sect_size);
  s=fread(*section,1,*sect_size,f);
  return s;
  }


int prepare_graphics(int *ofs,char *names,int32_t size,ABLOCK_DECODEPROC  decomp,int class)
  {
  char *p,*end;
  int count = 0;

  end=names+size;
  p=names;
  while (p<end)
     {
     def_handle(*ofs,p,decomp,class);
     p=strchr(p,'\0');
     p++;
     (*ofs)++;
     ++count;
     }
     return count;
  }



int load_level_texts(const char *filename)
  {
  int err;

  level_texts=create_list(10);
  err=load_string_list_ex(&level_texts,filename);
  return err;
  }

char *change_extension_support(char *buffer, const char *filename,char *new_extension)
  {
    strcpy(buffer, filename);
    char *sep = buffer;
    char *c = buffer;
    while (*c) {
        if (*c == '/' || *c == '\\') {
            sep = c;
        }
        ++c;
    }

    char *dot = strchr(sep, '.');
    if (dot == NULL) strcat(buffer, new_extension);
    else strcpy(dot, new_extension);
    return buffer;
  }



uint32_t fnv1a_hash(const char *str) {
    uint32_t hash = 0x01000193;

    while (*str) {
        hash ^= (unsigned char)(toupper(*str));
        hash *= 0x811C9DC5;
        str++;
    }

    return hash;
}

char *map_hash_to_string_impl(char *c, uint32_t h, int sz) {
    snprintf(c, sz, "maphash_%u", h);
    return c;
}

const char *find_map_from_hash_impl(char *c, uint32_t h, int sz) {
    const char *name = map_hash_to_string(h);
    int32_t tsz = temp_storage_find(name);
    if (tsz < 0 || tsz+1 > sz) return NULL;
    temp_storage_retrieve(name, c, tsz);
    c[tsz] = 0;
    return c;
}

void sanitize_map() {
   for(int i = 1; i < mapsize; ++i) {
      for (int j = 0; j < 4;++j) {
         int nx = map_sectors[i].step_next[j];
         if (nx > mapsize) {
            map_sectors[i].step_next[j] = i;
         }
      }
      TSECTOR *sctr = map_sectors+i;
      if (sctr->floor > num_ofsets_count[FLOOR_NUM]) {
         sctr->floor = 0;
      } if (sctr->ceil > num_ofsets_count[CEIL_NUM]) {
         sctr->ceil = 0;
      }
   }
   for (int i = 1, cnt = mapsize * 4; i < cnt; ++i) {
      TSTENA *s = map_sides+i;
      int max_side = MIN(num_ofsets_count[MAIN_NUM],MIN(num_ofsets_count[LEFT_NUM],num_ofsets_count[RIGHT_NUM]));
      int max_obl = MIN(num_ofsets_count[OBL_NUM],num_ofsets_count[OBL2_NUM]);
      if (s->prim > max_side) {
         s->prim = 0;
      }
      if (s->sec > max_side) {
         s->sec = 0;
      }
      if ((s->oblouk & 0x0F)> max_obl) {
         s->oblouk &= ~0x0F;
      }
      if (s->prim + (s->prim_anim & 0xF) > max_side)  {
         s->prim_anim = max_side - (s->prim_anim & 0xF);
      }
      if (s->sec + (s->sec_anim & 0xF) > max_side)  {
         s->sec_anim = max_side - (s->sec_anim & 0xF);
      }
   }

}

void translate_map_name(const char *mapfile, MAPGLOBAL *mglob) {
   uint32_t id = fnv1a_hash(mapfile);
   const TSTRINGTABLE *stable = lang_load("mapnames.csv");
   if (stable) {
      const char *rplc = stringtable_find(stable, id, NULL);
      if (rplc) {
         strcopy_n(mglob->mapname,rplc,sizeof(mglob->mapname));
      }
   }
}

int load_map(const char *filename)
  {
  FILE *f;
  void *temp;
  int sect;
  int32_t size,r;
  char nmapend=1;
  int ofsts=START_HANDLE;
  char snd_load=0;
  const void *mob_template;
  int32_t mob_size;
  int failed = 0;

  map_with_password=0;
  const char *mpath = build_pathname(2, gpathtable[SR_MAP], filename);
  mpath = local_strdup(mpath);
  schovej_mysku();
  zobraz_mysku();
  f=fopen_icase(mpath,"rb");
  if (level_fname!=NULL) free(level_fname);
  level_fname=(char *)getmem(strlen(filename)+1);
  strcpy(level_fname,filename);
  SEND_LOG("(GAME) Loading map: '%s'",level_fname);
  strupper(level_fname);
  mob_template=NULL;
  mob_size=0;
  if (f==NULL) return -1;
  do
     {
     r=load_section(f,&temp,&sect,&size);
     if (r==size)
        switch (sect)
         {
         case A_SIDEMAP:
                  map_sides=temp;
                  break;
         case A_SECTMAP:
                  map_sectors=temp;
                  break;
         case A_MAPINFO:
                  map_coord=temp;
                  mapsize=size / sizeof(TMAP_EDIT_INFO);
                  init_items();
                  init_mobs();
                  break;
         case A_MAPEND:
                  nmapend=0;
                  free(temp);
                  break;
         case A_STRMAIN:
                  num_ofsets[MAIN_NUM]=ofsts-1;
                  num_ofsets_count[MAIN_NUM]=prepare_graphics(&ofsts,temp,size,pcx_fade_decomp,SR_GRAFIKA);
                  free(temp);
                  break;
         case A_STRLEFT:
                  num_ofsets[LEFT_NUM]=ofsts-1;
                  num_ofsets_count[LEFT_NUM]=prepare_graphics(&ofsts,temp,size,pcx_fade_decomp,SR_GRAFIKA);
                  free(temp);
                  break;
         case A_STRRIGHT:
                  num_ofsets[RIGHT_NUM]=ofsts-1;
                  num_ofsets_count[RIGHT_NUM] = prepare_graphics(&ofsts,temp,size,pcx_fade_decomp,SR_GRAFIKA);
                  free(temp);
                  break;
         case A_STRCEIL:
                  num_ofsets[CEIL_NUM]=ofsts-1;
                  num_ofsets_count[CEIL_NUM]=prepare_graphics(&ofsts,temp,size,pcx_15bit_autofade,SR_GRAFIKA);
                  free(temp);
                  break;
         case A_STRFLOOR:
                  num_ofsets[FLOOR_NUM]=ofsts-1;
                  num_ofsets_count[FLOOR_NUM]=prepare_graphics(&ofsts,temp,size,pcx_15bit_autofade,SR_GRAFIKA);
                  free(temp);
                  break;
         case A_STRARC:
                  num_ofsets[OBL_NUM]=ofsts-1;
                  num_ofsets_count[OBL_NUM]=prepare_graphics(&ofsts,temp,size,pcx_fade_decomp,SR_GRAFIKA);
                  free(temp);
                  break;
         case A_STRARC2:
                  num_ofsets[OBL2_NUM]=ofsts-1;
                  num_ofsets_count[OBL2_NUM]=prepare_graphics(&ofsts,temp,size,pcx_fade_decomp,SR_GRAFIKA);
                  free(temp);
                  break;
         case A_MAPGLOB:
                  num_ofsets[BACK_NUM]=ofsts;
                  num_ofsets_count[BACK_NUM]=1;
				      memset(&mglob,0,sizeof(mglob));
                  memcpy(&mglob,temp,MIN((int)size,(int)sizeof(mglob)));
                  free(temp);
                  for(r=0;r<4;r++) {
                        def_handle(ofsts++,mglob.back_fnames[r],pcx_fade_decomp,SR_GRAFIKA);
                  }
                  back_color=RGB888(mglob.fade_r,mglob.fade_g,mglob.fade_b);
                  translate_map_name(filename, &mglob);;
                  break;
         case A_MAPITEM:
                  SEND_LOG("(GAME) Loading items...");
                  load_item_map(temp,size);
                  free(temp);
                  break;
         case A_MAPMOBS:
                  if (snd_load==0) create_sound_table_old();
                  SEND_LOG("(GAME) Loading enemies...");
                  if (mob_template==NULL)
                    {
                    int32_t h;const char *p;

                    alock(H_ENEMY);
                    p=ablock(H_ENEMY);
                    h=get_handle_size(H_ENEMY);
                    load_enemies(temp,size,&ofsts,(TMOB *)p,h);
                    aunlock(H_ENEMY);
                    }
                  else
                    {
                    load_enemies(temp,size,&ofsts,mob_template,mob_size);
                    ablock_free(mob_template);
                    mob_template=NULL;
                    SEND_LOG("(GAME) Loading enemies from map template...");
                    }
                  free(temp);
                  break;
         case A_MAPMACR:
                  SEND_LOG("(GAME) Loading multiactions...");
                  load_macros(size,temp);
                  free(temp);
                  break;
         case A_MAPVYK:
                  map_vyk=temp;
                  vyk_max=size/sizeof(TVYKLENEK);
                  break;
         case A_MOBS:
                  mob_template=load_mob_legacy_format_direct(temp, &size,0);
                  mob_size=size;
                  free(temp);
                  break;
         case A_MOBSND:
                  snd_load=1;
                  create_sound_table(temp,size);
                  free(temp);
                  break;
		 case A_PASSW :
				  map_with_password=1;
				  free(temp);
				  break;



         default:free(temp);
         }
     else
        {
        if (temp!=NULL)free(temp);
        ablock_free(mob_template);
        fclose(f);
        return -3;
        }
     }
  while (nmapend);
  ablock_free(mob_template);
  fclose(f);
  flag_map=(char *)getmem(mapsize*4);
  memset(minimap,0,sizeof(minimap));
  end_ptr=ofsts;
  const char *tpath=set_file_extension(mpath,".txt");
  failed=load_level_texts(tpath);
  if (!failed && level_texts!=NULL) {
      lang_patch_stringtable(&level_texts, filename, "map_");
      create_playlist(level_texts[0]);
  }
  init_tracks();
  change_music(get_next_music_from_playlist());
  for(r=0;r<mapsize*4;r++) flag_map[r]=(char)map_sides[r].flags;
  if (!doNotLoadMapState && load_map_state()==-2)
     {
     display_error("Bug in temp file. Please purge some status blocks in last load savegame file.");
     exit(0);
     }
  doNotLoadMapState=0;
  current_map_hash = fnv1a_hash(filename);
  const char * hash_str = map_hash_to_string(current_map_hash);
  temp_storage_store(hash_str, filename, strlen(filename));
  sanitize_map();
  return failed;
  }

void add_leaving_place(int sector)
  {
    char *s;
    concat(s,level_fname, ".lplace");
    temp_storage_store(s, &sector, sizeof(sector));
  }

int set_leaving_place(void)
  {
  return get_leaving_place(level_fname);
  }

int get_leaving_place(char *level_name)
  {
    char *s;
    concat(s,level_name, ".lplace");
    int sector;
    if (temp_storage_retrieve(s, &sector, sizeof(sector))<0) {
        return 0;
    }
    return sector;
  }



void leave_current_map()
  {
  int i;
  TFLY *p;
  SEND_LOG("(GAME) Leaving current map ... start");
  add_leaving_place(viewsector);
  kill_all_sounds();
  remove_all_mob_spells();
  regen_all_mobs();
  if (save_map) {
      save_map_state(); //do tempu se zabali status mapy
  }
  destroy_fly_map();
  p=letici_veci;
  while (p!=NULL) {stop_fly(p,0);p=p->next;}
  calc_fly(0);
  save_map=1;
  free(map_sides);
  free(map_sectors);
  free(flag_map);
  free(map_coord);
  free(map_vyk);map_vyk=NULL;vyk_max=0;
  free_map_description();
  while (d_action!=NULL)
    {
    void *p=d_action;
    d_action=d_action->next;
    free(p);
    }
  release_list(level_texts);
  if (mob_map!=NULL) free(mob_map);
  if (macro_block!=NULL)
     {
     free(macro_block);
     macros=NULL;
     macro_block=NULL;
     macro_block_size=0;
     }
  if (map_items!=NULL)
     {
     for(i=0;i<mapsize*4;i++) if (map_items[i]!=NULL) free(map_items[i]);
     free(map_items);
     map_items=NULL;
     }
  for(i=0;i<hl_ptr;i++)
     {
     THANDLE_DATA *h;

     h=get_handle(i);
     if (h->loadproc==pcx_fade_decomp) zneplatnit_block(i);
     }
  for(i=hl_ptr;i<end_ptr;i++) undef_handle(i);
  build_all_players();
  purge_playlist();
  hold_timer(TM_BACK_MUSIC,0);
  mapsize=0;
  free(level_fname);level_fname=NULL;
  }



int32_t actn_flags(TSTENA *q,int32_t flags)
  {
  flags>>=24;
  flags&=0x1f;
  return q->flags ^ flags;
  }

void recheck_button(int sector,char auto_action)
  {
  TSECTOR *ts;
  short **ms;
  char swch;
  char flrpls[]={1,2,2,4,4,8,2,2};

  ts=map_sectors+sector;
  if (ts->sector_type!=S_TLAC_OFF && ts->sector_type!=S_TLAC_ON) return;
  ms=map_items+(sector<<2);
  swch=(map_coord[sector].flags & MC_DPLAYER || mob_map[sector] ||
     ms[0]!=NULL || ms[1]!=NULL || ms[2]!=NULL || ms[3]!=NULL);
  switch (ts->sector_type)
     {
     case S_TLAC_OFF:if (swch)
                       {
                       ts->floor+=flrpls[ts->flags & 7];
                       ts->sector_type=S_TLAC_ON;
                       if (auto_action) do_action(ts->action,ts->sector_tag,ts->side_tag,0,0);
                       }
                    break;
     case S_TLAC_ON:if (!swch)
                       {
                       ts->floor-=flrpls[ts->flags & 7];
                       ts->sector_type=S_TLAC_OFF;
                       if (auto_action) do_action(ts->action,ts->sector_tag,ts->side_tag,0,0);
                       }
                    break;
     }
  }

void stop_fly(LETICI_VEC *p,char zvuk)
  {
  if (zvuk && ~p->flags & FLY_NEDULEZITA)sirit_zvuk(p->sector);
  if (!(p->flags & FLY_DESTROY))
     {
     item_sound_event(p->items==NULL?p->item:p->items[0],p->sector);
     if (p->xpos>=0) push_item(p->sector,(p->smer+(p->ypos>0))&3,p->items);
                else push_item(p->sector,(p->smer+(p->ypos<0)+2)&3,p->items);
     }
  p->flags |= FLY_BURNT;
  }

void interrupt_fly(LETICI_VEC *p)
  {
           if (p->flags & FLY_DESTROY_SEQ) return;
           p->speed=0;
           if (p->flags & FLY_DESTROY)
              {
              p->flags|=FLY_DESTROY_SEQ|FLY_NEHMOTNA;
              p->anim_pos=3;
              p->velocity=0;
              item_sound_event(p->items==NULL?p->item:p->items[0],p->sector);
              }
           else
              if (p->velocity==0) stop_fly(p,1);else if (p->velocity<10) p->velocity=10;
  }

LETICI_VEC *create_fly()
  {
  LETICI_VEC *p,*q;

  p=letici_veci;
  q=NULL;
  while (p!=NULL) if (p->flags & FLY_UNUSED)
     {
     if (q==NULL) letici_veci=p->next;else q->next=p->next;
     free(p->items);
     break;
     }
     else
        {
        q=p;
        p=p->next;
        }
  if (p==NULL) p=New(LETICI_VEC);
  p->flags=0;
  p->items=NULL;
  p->item=0;
  p->lives=0;
  p->anim_pos = 0;
  return p;
  }

TFLY *duplic_fly(TFLY *p)
  {
  TFLY *q;

  q=create_fly();
  *q=*p;
  if (q->items!=NULL)
     {
      int s=0;
     while (q->items[s]) ++s;
     ++s;
     q->items=(short *)getmem(s);
     memcpy(q->items,p->items,s);
     }
  return q;
  }

void calc_fly(THE_TIMER *t)
  {
  LETICI_VEC *p,*q;
  short ss;

  for(p=letici_veci;p!=NULL;p=q)
     {
     p->xpos+=p->speed;
     if (p->flags & FLY_BURNT) {
      if (p->flags & FLY_UNUSED)
        {
        short ds[2];
        if (letici_veci==p) letici_veci=p->next;
           else
           {
           for(q=letici_veci;q->next!=p;q=q->next);
           q->next=p->next;
           }
        q=p->next;
        ds[0]=p->item;ds[1]=0;
        if (p->items!=NULL) destroy_items(p->items); else destroy_items(ds);
        free(p->items);
        p->items=NULL;
        free(p);
        continue;
        }
      else
        {
        q=p->next;
        continue;
        }
     }
     q=p->next;
    if (!(p->flags & FLY_NEHMOTNA))
              {
              p->velocity+=1;
              }
     p->zpos-=p->velocity;
     ss=p->sector;
     if (!(p->flags & FLY_NEDULEZITA)) neco_v_pohybu=1;
     if (p->zpos>110) p->zpos=110;
     if (p->zpos<=20)
        {
        if (map_sectors[p->sector].sector_type==S_DIRA)
           {
           p->speed=0;
           if (p->zpos<=-128)
              {
              p->sector=map_sectors[p->sector].sector_tag;
              p->zpos=256;
              }
           }
        else
           stop_fly(p,1);
        continue;
        }
     if (p->flags & FLY_IN_FLY && p->xpos>-16 && zasah_veci(p->sector,p))
           {
           interrupt_fly(p);
           continue;
           }
     if (p->xpos>63)
        {
        p->flags|=FLY_IN_FLY;
        p->xpos-=127;
        ss=map_sectors[p->sector].step_next[p->smer];
        if (p->sector != 0 && !(map_sides[p->sector*4+p->smer].flags & SD_THING_IMPS))
           {
           p->sector=ss;

           if (ISTELEPORTSECT(p->sector))
              p->sector=map_sectors[p->sector].sector_tag;
           }

        else
           {
           p->xpos+=127-p->speed;
           if (!zasah_veci(p->sector,p) && p->lives)  p->lives--;
           if (p->lives)
              {
              TFLY *q,*z;

              q=duplic_fly(p);q->smer=(p->smer+1)&3;
              z=duplic_fly(p);z->smer=(p->smer-1)&3;
              add_fly(q);
              add_fly(z);
              }
           interrupt_fly(p);
           }
        }
     else if (p->counter++ > 100) {
           zasah_veci(p->sector,p);
           interrupt_fly(p);
           continue;
     }
     }

  }

void destroy_all_fly() {
    while (letici_veci) {
        TFLY *f = letici_veci;
        letici_veci = f->next;
        if (f->items) free(f->items);
        free(f);
    }
}

extern int32_t sound_side_flags;


void calc_animations()
  {
  int i;

  for (i=0;i<mapsize*4;i++)
     {
     TSTENA *p;
     int pj,pk,sj,sk;

     p=&map_sides[i];
     sound_side_flags=p->flags;
     pj=p->prim_anim>>4;pk=p->prim_anim & 15;
     sj=p->sec_anim>>4;sk=p->sec_anim & 15;
     if (!pk && !sk) continue;
     if (p->flags & SD_PRIM_ANIM)
        {
        if (p->flags & SD_PRIM_GAB)
           if (pj==0 || pj==pk) p->flags^=SD_PRIM_FORV;
        if (p->flags & SD_PRIM_FORV) pj++; else pj--;
        if (pj>pk) pj=0;
        if (pj<0) pj=pk;
        }
      else
       {
        if (p->flags & SD_PRIM_FORV) pj++; else pj--;
        if (pk && (pj+(!(p->flags & SD_PRIM_FORV))==pk))
           {p->flags&=~0xff;p->flags|=flag_map[i];}
        if (pj>pk) pj=pk;
        else if (pj<0) pj=0;
        else call_macro(i,MC_ANIM|((pj & 1)?MC_ANIM2:0)|(pj?0:MC_CLOSEDOOR)|(pj==pk?MC_OPENDOOR:0));
        if (pk==pj && p->flags & SD_PRIM_GAB)
           p->flags&=~SD_PRIM_FORV;
       }
    if (p->flags & SD_SEC_ANIM)
        {
        if (p->flags & SD_SEC_GAB)
           if (sj==0 || sj==sk) p->flags^=SD_SEC_FORV;
        if (p->flags & SD_SEC_FORV) sj++; else sj--;
        if (sj>sk) sj=0;
        if (sj<0) sj=sk;
        }
      else
       {
        if (p->flags & SD_SEC_FORV) sj++; else sj--;
        if (!pk && sk && (sj+(!(p->flags & SD_SEC_FORV))==sk))
           {p->flags&=~0xff;p->flags|=flag_map[i];}
        if (sj>sk) sj=sk;
        else if (sj<0) sj=0;
        else if (!pk) call_macro(i,MC_ANIM|((sj & 1)?MC_ANIM2:0)|(pj?0:MC_CLOSEDOOR));
        if (sk==sj && p->flags & SD_SEC_GAB)
           p->flags&=~SD_SEC_FORV;
       }
     p->prim_anim=(pj<<4)+pk;
     p->sec_anim=(sj<<4)+sk;
     }
  sound_side_flags=0;
  }


void delay_action(int action_numb,int sector,int direct,int flags,int nosend,int delay)
  {
  D_ACTION *d;
  if (!sector && !direct) return;
  if (!delay) do_action(action_numb,sector,direct,flags,nosend);
  else
     {
     d=(D_ACTION *)getmem(sizeof(D_ACTION));
     d->action=action_numb;
     d->sector=sector;
     d->side=direct;
     d->flags=flags>>16;
     d->nocopy=nosend;
     d->delay=delay;
     d->next=d_action;
     d_action=d;
     }
  }
/*
int get_action_delay(TSTENA *q)
  {
  if (q->flags & SD_DELAY)
     {
     if (q->sec) return q->sec_anim & 0xf;
     return q->prim_anim & 0xf;
     }
  else
     return 0;
  }
*/
void check_codelock_log(int sector,uint32_t flags)
  {
  int i;
  TSTENA *p;
  TSECTOR *s;
  p=&map_sides[sector*4];
  s=&map_sectors[sector];
  for (i=0;i<4;i++) if (!(p[i].flags & SD_PRIM_VIS)) break;
  if (i==4) do_action(s->action,s->sector_tag,s->side_tag,flags,0);
  }


int do_action(int action_numb,int sector, int direct,int flags, int nosend)
  {
  TSTENA *q;
  TSECTOR *s;
  char *c;
  int sid;
  char ok=0;

  sid=sector*4+direct;
  s=&map_sectors[sector];
  q=&map_sides[sid];
  c=&flag_map[sid];
  switch (action_numb)
     {
     case 0:
              q->flags=actn_flags(q,flags);
              break;
     case A_OPEN_DOOR:
               if (!(q->flags & SD_PRIM_FORV) && !(q->flags & SD_PRIM_ANIM))
                    {
                    q->flags|=SD_PRIM_FORV;
                    *c=(char)actn_flags(q,flags);
                    ok=1;
                    }
               if (!(q->flags & SD_SEC_FORV))
                    {
                    q->flags|=SD_SEC_FORV;
                    ok=1;
                    }
                 sound_side_flags=q->flags;
               break;
     case A_CLOSE_DOOR:
               if (q->flags & SD_PRIM_FORV && !(q->flags & SD_PRIM_ANIM))
                 {
                 q->flags&=~SD_PRIM_FORV;
                 *c=(char)actn_flags(q,flags);
                 ok=1;
                 }
               if (q->flags & SD_SEC_FORV)
                 {
                 q->flags&=~SD_SEC_FORV;
                 ok=1;
                 }
                 sound_side_flags=q->flags;
              break;
     case A_OPEN_CLOSE:
               if (!(q->flags & SD_PRIM_ANIM)) q->flags^=SD_PRIM_FORV | SD_SEC_FORV;
               else q->flags^= SD_SEC_FORV ;
               *c=(char)actn_flags(q,flags);
               ok=1;
               sound_side_flags=q->flags;
              break;
     case A_RUN_PRIM:
              q->flags|=SD_PRIM_ANIM;
              ok=1;
              break;
     case A_HIDE_PRIM:
              if (q->flags & SD_PRIM_VIS)
                    {
                    q->flags&=~SD_PRIM_VIS;
                    *c=(q->flags=actn_flags(q,flags));
                    ok=1;
                    }
              break;
     case A_SHOW_PRIM:
              if (!(q->flags & SD_PRIM_VIS))
                    {
                    q->flags|=SD_PRIM_VIS;
                    *c=(q->flags=actn_flags(q,flags));
                    ok=1;
                    }
              break;
     case A_SHOW_HIDE_PRIM:
              q->flags^=SD_PRIM_VIS;
              *c=(q->flags=actn_flags(q,flags));
              ok=1;
              break;
     case A_RUN_SEC:
              q->flags|=SD_SEC_ANIM;
              ok=1;
              break;
     case A_HIDE_SEC:
              if (q->flags & SD_SEC_VIS)
                    {
                    q->flags&=~SD_SEC_VIS;
                    *c=(q->flags=actn_flags(q,flags));
                    ok=1;
                    }
              break;
     case A_SHOW_SEC:
              if (!(q->flags & SD_SEC_VIS))
                    {
                    q->flags|=SD_SEC_VIS;
                    *c=(q->flags=actn_flags(q,flags));
                    ok=1;
                    }
              break;
     case A_SHOW_HIDE_SEC:
              q->flags^=SD_SEC_VIS;
              *c=(q->flags=actn_flags(q,flags));
              ok=1;
              break;
     case A_HIDE_PRIM_SEC:
              if ((q->flags & SD_SEC_VIS)||(q->flags & SD_PRIM_VIS))
                    {
                    q->flags&=~(SD_SEC_VIS+SD_PRIM_VIS);
                    *c=(q->flags=actn_flags(q,flags));
                    ok=1;
                    }
              break;
     case A_DISPLAY_TEXT:
                 bott_disp_text(level_texts[sector]);
                 ok=1;
                 return 0;
     case A_CODELOCK_LOG3:
                 q->flags|=SD_PRIM_VIS;
                 check_codelock_log(sector,flags);
                 ok=1;
                 break;
     case A_CODELOCK_LOG2:
                 check_codelock_log(sector,flags);
                 q->flags^=SD_PRIM_VIS;
                 check_codelock_log(sector,flags);
                 ok=1;
                 break;
     case A_CODELOCK_LOG:
                 check_codelock_log(sector,0x1f000000);
                 q->flags^=SD_PRIM_VIS;
                 check_codelock_log(sector,0x1f000000);
                 ok=1;
                 break;
     case A_OPEN_TELEPORT:
                 ok=(s->sector_type!=S_TELEPORT);
                 s->sector_type=S_TELEPORT;
                 {
                 int i;
                 for(i=0;i<4;i++) map_sides[(sector<<2)+i].flags|=SD_SEC_VIS;
                 }
                 nosend=1;
                 break;
     case A_CLOSE_TELEPORT:
                 ok=(s->sector_type==S_TELEPORT);
                 s->sector_type=S_NORMAL;
                 {
                 int i;
                 for(i=0;i<4;i++) map_sides[(sector<<2)+i].flags&=~SD_SEC_VIS;
                 }
                 nosend=1;
                 break;
     }
  if (nosend) return 0;
  last_send_action=action_numb;
  call_macro(sid,MC_INCOMING);
  if (ok)
     {
     ok=0;
     call_macro(sid,MC_SUCC_DONE);
     }
  if (q->flags & SD_COPY_ACTION)
     {

     q->flags&=~SD_COPY_ACTION;
     delay_action(action_numb,q->sector_tag,q->side_tag,flags,0,0);
     q->flags|=SD_COPY_ACTION;
     }
  if (q->flags & SD_SEND_ACTION)
     {

     q->flags&=~SD_SEND_ACTION;
     delay_action(q->action,q->sector_tag,q->side_tag,q->flags,0,0);
     q->flags|=SD_SEND_ACTION;
     }
  if (q->flags & SD_APPLY_2ND && s->step_next[direct])
     do_action(action_numb,s->step_next[direct],(direct+2)&3,flags,1);
  return 0;
  }

/*
void black(int i,int sector,int dir)
  {
  int xl,y1,xr,y2;

  if (!(map_sides[sector*4+dir].flags & SD_INTERIER)) return;
     y2=MIDDLE_Y+points[i][0][VIEW3D_Z].y+16;
     y1=MIDDLE_Y+points[i][1][VIEW3D_Z].y+16;
  if (i<0)
     {
     xl=MIDDLE_X-points[i][0][VIEW3D_Z].x;
     xr=MIDDLE_X-points[i+1][0][VIEW3D_Z].x;
     }
  else
  if (i>0)
     {
     xl=MIDDLE_X+points[i-1][0][VIEW3D_Z].x;
     xr=MIDDLE_X+points[i][0][VIEW3D_Z].x;
     }
  else
     {
     xl=MIDDLE_X-points[0][0][VIEW3D_Z].x;
     xr=MIDDLE_X+points[0][0][VIEW3D_Z].x;
     }
  if (xl<0) xl=0;
  if (xr>639) xr=639;
  curcolor=back_color;
  bar(xl,y1,xr,y2);
  }
*/

void do_delay_actions()
  {
  D_ACTION *d,*p,*q;

  d=d_action;p=NULL;
  while (d!=NULL)
     {
     if (!(--d->delay))
        {
        q=d;
        if (q==d_action)
           {
           d_action=d->next;
           d=d_action;
           }
        else
           {
           p=d_action;
           while (p->next!=d) p=p->next;
           p->next=d->next;
           d=p->next;
           }
       do_action(q->action,q->sector,q->side,q->flags<<16,q->nocopy);
       free(q);
       }
     else
        {
        d=d->next;
        }
     }
  }

void mrtva_skupina()
  {
  int i=0;

  //najde prvni pouzitelnou postavu
  for(i=0;!postavy[i].groupnum || !postavy[i].used || postavy[i].inmaphash != current_map_hash;i++);
  //a zmeni
  zmen_skupinu(postavy+i);
  }
/*
static void check_pod_vodou(char mode)
  {
  short sectors[POCET_POSTAV];
  short psec;
  short pp=0;
  int i,j;


  for(i=0;i<POCET_POSTAV;i++)
     {
     psec=postavy[i].sektor;
     for(j=0;j<pp;j++) if (sectors[j]==psec) break;
     if (j==pp)
        {

        }
     }
  }
*/
static void akce_voda(THUMAN *h,int mode)
  {
  int ziv=mode?10:1;
  if (h->kondice) h->kondice-=ziv*2;
     else
        {
        player_hit(h,h->lives,0);
        }
  if (h->kondice<0) h->kondice=0;
  bott_draw(0);
  }

void check_players_place(char mode) {
    int i;
    THUMAN *h = postavy;
    char vir_eff = 0;
    static char vir_zavora = 0;
    char levitat;

    for (i = 0; i < POCET_POSTAV; i++, h++)
        if (h->used && h->lives && h->inmaphash == current_map_hash) {
            int sect;
            int u1;

            levitat = (h->vlastnosti[VLS_KOUZLA] & SPL_LEVITATION) != 0;
            sect = h->sektor;
            if (sect >= mapsize)
                continue;
            switch (map_sectors[sect].sector_type) {
                case S_ACID:
                    if (!levitat) {
                        if (h->lives > 3) {
                            h->lives -= 3;
                            bott_draw(0);
                        } else {
                            player_hit(h, 3 + 7 * mode, 0);
                        }
                    }
                    break;
                case S_VIR:
                    if (!levitat) {
                        if (mode == 0 && vir_zavora == 0) {
                            int i, smer;
                            vir_zavora = 1;
                            pass_zavora = 0;
                            smer = rnd(100) < 50 ? -1 : 1;
                            if (vir_eff == 0)
                                for (i = 0; i < 8; i++)
                                    turn_zoom(smer);
                            vir_eff = 1;
                            vir_zavora = 0;
                            cancel_pass = 1;
                        }
                    }
                    break;
                case S_LAVA:
                    if (!levitat) {
                        u1 = (h->lives);
                        player_hit(h, u1, 0);
                        bott_draw(0);
                    } else {
                        if (h->lives > 3) {
                            h->lives -= 3;
                            bott_draw(0);
                        } else
                            player_hit(h, 3 + 7 * mode, 0);
                    }

                    break;
                case S_SSMRT:
                    u1 = (h->lives);
                    player_hit(h, u1, 0);
                    bott_draw(0);
                    break;
                case S_VODA:
                    if (!levitat) {
                        akce_voda(h, mode);
                    }
                    break;
                case S_DIRA:
                    if (!pass_zavora)
                        postavy_propadnout(sect);
                    break;
                case S_LODKA:
                    if (lodka != 1 && mode) {
                        set_backgrnd_mode(1);
                        lodka = 1;
                    }
                    break;
            }
            if (mglob.map_effector == ME_PVODA
                    && ~map_coord[h->sektor].flags & MC_SAFEPLACE)
                akce_voda(h, mode);
            if (map_sectors[sect].sector_type != S_LODKA && lodka) {
                set_backgrnd_mode(0);
                lodka = 0;
                hide_boat();
            }
        }
}

static void move_lodka(int oldsect,int newsect)
  {
  if (map_sectors[oldsect].sector_type==S_LODKA &&
     map_sectors[newsect].sector_type==S_VODA)
     {
     map_sectors[oldsect].sector_type=S_VODA;
     map_sectors[newsect].sector_type=S_LODKA;
     }
  }

void calc_game()
  {
  int d;
  calc_animations();
  if (d_action!=NULL) do_delay_actions();
  calc_mobs();
  if (force_start_dialog && !norefresh)
     {
     force_start_dialog=0;
     call_dialog(start_dialog_number,start_dialog_mob);
     }
  check_players_place(0);
  if ((d=check_end_game())!=0) {
     if (d==1) wire_end_game();else mrtva_skupina();
  }
  if (battle && cur_mode!=MD_INBATTLE)
      {
      start_battle();
      }
  else if(!battle && select_player!=-1)
     {
     select_player=-1;
     bott_draw(0);
     }
  }

void a_touch(int sector,int dir)
  {
  TSTENA *q;
  int sid;

  sid=sector*4+dir;
  q=&map_sides[sid];
  if (q->flags & SD_PASS_ACTION) return;
  if (q->sec && ~q->flags & SD_SEC_VIS) return;
  side_touched=1;
  if (!q->sec) delay_action(q->action,q->sector_tag,q->side_tag,q->flags,0,0);
  else
     if (q->flags & SD_SEC_VIS)
     {
     if (q->flags & SD_AUTOANIM) do_action(A_OPEN_CLOSE,sector,dir,0,1);
     delay_action(q->action,q->sector_tag,q->side_tag,q->flags,0,0);
     }
  call_macro(sid,MC_TOUCHSUC);
  }

void a_pass(int sector, int dir)
  {
  TSTENA *q;

  q=&map_sides[sector*4+dir];
  q->flags&=~SD_SECRET;
  if (q->flags & SD_ALARM)
  {
     if (GlobEvent(MAGLOB_ALARM,viewsector,viewdir))
       sirit_zvuk(map_sectors[sector].step_next[dir]);
  }
  if (!(q->flags & SD_PASS_ACTION)) return;
  delay_action(q->action,q->sector_tag,q->side_tag,q->flags,0,0);
  }

void zmen_skupinu(THUMAN *p)
  {
  int i;

  if (p->groupnum==0 || p->inmaphash != current_map_hash) {bott_draw(0);return;}
  cur_group=p->groupnum;
  for(i=0;i<POCET_POSTAV;i++) if (!postavy[i].lives && postavy[i].groupnum!=cur_group) postavy[i].groupnum=0;
  viewsector=p->sektor;
  viewdir=p->direction;
  schovej_mysku();
  bott_draw(0);
  other_draw();
  ukaz_mysku();
  showview(0,378,640,480);
  ukaz_mysku();
  }

void sort_groups()
  {
  int i,j,t=0;

  if (cur_mode==MD_PRESUN) group_sort[0]=moving_player;
  if (enable_sort)
     {
     for(i=0;i<POCET_POSTAV && t<POCET_POSTAV;i++)
       {
       for(j=0;j<POCET_POSTAV;j++) if (postavy[j].groupnum==i+1) group_sort[t++]=j;
       }
    j=0;
    while (t<POCET_POSTAV)
       {
       while (postavy[j].groupnum) j++;
       group_sort[t++]=j++;
       }
     }
  else
     for(i=0;i<POCET_POSTAV;i++) group_sort[i]=i;
  }

void add_to_group(int num)
  {
  if (lodka)return;
  if (!postavy[num].used) return;
  if (postavy[num].groupnum!=cur_group) postavy[num].groupnum=cur_group;
  else if (!postavy[num].lives) postavy[num].groupnum=0;else
     {
     char cisla[7];
     int i;

     memset(cisla,0,sizeof(cisla));
     for(i=0;i<POCET_POSTAV;i++) cisla[postavy[i].groupnum]=1;
     for(i=1;i<7 && cisla[i]!=0;i++);
     if (i==7) return;
     postavy[num].groupnum=i;
     }
  }

void group_all(void)
  {
  char c=1;
  int i;
  THUMAN *h;

  while (chod_s_postavama(1)) c=0;
  if (c)
     {
     if (cur_group!=1)
        {
        for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && h->groupnum==1 && h->sektor!=viewsector) break;
        if (i==POCET_POSTAV) cur_group=1;
        }
     for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
        if (h->used && h->lives && h->sektor==viewsector) h->groupnum=cur_group;
     }

  bott_draw(0);
  }

void destroy_player_map() //je nutne volat pred presunem postav
  {
  int i;
  for(i=0;i<POCET_POSTAV;i++) {
      int s = postavy[i].sektor;
      if (s > 0 && s < mapsize) {
          map_coord[s].flags&=~(MC_DPLAYER | MC_SAFEPLACE);
      }
  }
  }

void build_player_map()  //je nutne volat po presunu postav
  {
  int i;
  THUMAN *p;
  for(i=0;p=&postavy[i],i<POCET_POSTAV;i++) if (p->used && p->inmaphash == current_map_hash)
     {
      int s = abs(p->sektor);
     map_coord[s].flags|=(p->lives?MC_PLAYER:MC_DEAD_PLR);
     if (mglob.map_effector==ME_PVODA)
        {
        if (q_item_one(i,water_breath+1))map_coord[s].flags|=MC_SAFEPLACE;
        }
     }
  }


char chod_s_postavama(char sekupit)
  {
  int i;
  char gatt=0;
  signed char group_nums[7];

  destroy_player_map();
  if (cur_mode==MD_PRESUN)
     {
        postavy[select_player].sektor=viewsector;
        postavy[select_player].direction=viewdir;
     }
  else
     {
     memset(group_nums,0xff,sizeof(group_nums));
     group_nums[0]=0;
     for(i=0;i<POCET_POSTAV;i++)
        {
        if (postavy[i].groupnum==cur_group)
           {
           postavy[i].sektor=viewsector;
           postavy[i].direction=viewdir;
           postavy[i].utek=0;
           postavy[i].kondice-=weigth_defect(postavy+i);
           if (postavy[i].kondice<0) postavy[i].kondice=0;
           group_nums[cur_group]=1;
           }
        }
     }
  build_player_map();
  SEND_LOG("(GAME) New position %d:%d",viewsector,viewdir);
  return gatt;
  }

void shift_zoom(char smer)
  {
  if (cancel_pass) return;
  hold_timer(TM_BACK_MUSIC,1);
  switch (smer & 3)
     {
     case 0:if (lodka)zooming_forward(ablock(H_LODKA));
             else zooming_forward(ablock(H_BGR_BUFF));
     break;
     case 1:turn_left();break;
     case 2:if (lodka)zooming_backward(ablock(H_LODKA));
             else zooming_backward(ablock(H_BGR_BUFF));
     break;
     case 3:turn_right();break;
     }
  hold_timer(TM_BACK_MUSIC,0);
  }

void hide_ms_at(int line)
  {
  if (ms_last_event.y<line)
     {
     update_mysky();
     schovej_mysku();
     }
  }

void real_krok(EVENT_MSG *msg,void **data)
  {
  if (msg->msg==E_INIT || msg->msg==E_DONE) return;
  check_all_mobs();
  calc_game();msg;data;
  SEND_LOG("(GAME) STEP");
  }

void do_halucinace()
  {
  hal_sector=rnd(mapsize-1)+1;
  hal_dir=rnd(4);
  }

void sector0(void)
  {
  int i;

  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==0 && postavy[i].used)
     {
     postavy[i].lives=0;
     player_check_death(postavy+i,0);
     }
  }

static void tele_redraw(THE_TIMER *_)
  {
  if (!running_anm) return;
  redraw_scene();
  calc_animations();
  showview(0,17,640,360);
  }

void postavy_teleport_group(int sector,int dir,int postava,char ef_mode)
              {
              int i;
              i=mob_map[sector];
              if (i)
                 {
                 mobs[--i].vlajky &= ~MOB_LIVE;
                 if ((i=mobs[i].next)!=0)
                    mobs[--i].vlajky &= ~MOB_LIVE;
                 mob_map[sector]=0;
                 }
              destroy_player_map();
              for(i=0;i<POCET_POSTAV;i++) if (postava & (1<<i))
                 {
                 if (ef_mode)
                    {
                    add_spectxtr(postavy[i].sektor,H_TELEP_PCX,14,1,0);
                    play_sample_at_sector(H_SND_TELEPOUT,viewsector,viewsector,0,0);
                    }
                 postavy[i].sektor=sector;
                 postavy[i].direction=dir;
                 if (!sector) player_hit(postavy+i,postavy[i].lives,0);
                 if (ef_mode)
                    {
                    add_spectxtr(sector,H_TELEP_PCX,14,1,0);
                    play_sample_at_sector(H_SND_TELEPOUT,viewsector,sector,0,0);
                    }
                 ef_mode=0;
                 }
              build_player_map();
              }

void postavy_teleport_effect(int sector,int dir,int postava,char effect)
  {
  char kolo=global_anim_counter;
  int counter=0;
  static int zavora=0;

  if (zavora) return;
  if (effect)
     {
     void (*c)(void);

     zavora=1;
     c=wire_proc;
     unwire_proc();
     recalc_volumes(viewsector,viewdir);
     add_to_timer(-1,gamespeed,-1,tele_redraw);
     add_spectxtr(viewsector,H_TELEP_PCX,14,1,0);
     while (running_anm) do_events();
     play_sample_at_channel(H_SND_TELEPIN,2,100);
     play_big_mgif_animation(H_TELEPORT);
     while (!running_anm) task_sleep();
     kolo=global_anim_counter;
     if (norefresh)
        {
        norefresh=0;
        cancel_pass=1;
        }
     while (running_anm)
        {
        do_events();
        if (kolo!=global_anim_counter)
           {
           counter+=(char)(global_anim_counter-kolo);
           kolo=global_anim_counter;
           if (counter==6)
              {
              postavy_teleport_group(sector,dir,postava,0);
              viewsector=sector;
              viewdir=dir&0x3;
              recalc_volumes(viewsector,viewdir);
              }
           }
        }
     kolo=global_anim_counter;
     while (kolo!=global_anim_counter) do_events();
     delete_from_timer(-1);
     wire_proc();
     wire_proc=c;
     }
  else
     {
     postavy_teleport_group(sector,dir,postava,1);
     viewsector=sector;
     viewdir=dir&0x3;
     }
  zavora=0;
  }

void check_postavy_teleport()
  {
  int nsect=viewsector;
  if (ISTELEPORTSECT(viewsector))
     {
     int i,ss=0,sid=viewsector*4+viewdir;
     for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==viewsector && postavy[i].used && postavy[i].inmaphash == current_map_hash) ss|=1<<i;
     if (map_sides[sid].flags & SD_SEC_VIS)
        postavy_teleport_effect(map_sectors[viewsector].sector_tag,map_sectors[nsect].side_tag,ss,1);
     else
        postavy_teleport_group(viewsector=map_sectors[viewsector].sector_tag,viewdir=map_sectors[nsect].side_tag,ss,0);
     }
  }

static char test_can_walk(int grp)
  {
  int i;
  THUMAN *h;

  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
     if (h->used && h->groupnum==grp && h->lives &&
        (h->vlastnosti[VLS_POHYB]==0 || h->kondice==0)) return 0;
  return 1;
  }

void step_zoom(char smer)
  {
  char nopass;
  uint8_t drs;
  int sid,nsect,sect;
  char can_go=1;

  if (running_anm) return;
  if (pass_zavora) return;
  if (lodka && (smer==1 || smer==3)) return;
  cancel_pass=0;
  drs=(viewdir+smer)&3;
  sid=viewsector*4+drs;
  sect=viewsector;
  call_macro(sid,MC_EXIT);
  nopass=(map_sides[sid].flags & SD_PLAY_IMPS);
  if (nopass) call_macro(sid,MC_PASSFAIL);else call_macro(sid,MC_PASSSUC);
  if (cur_mode==MD_PRESUN)
     {
     select_player=moving_player;
     if(!postavy[select_player].actions) nopass=1;
     }
  else can_go=test_can_walk(cur_group);
  if (!can_go)
    {
    bott_disp_text(texty[220]);
    return;
    }
  if (force_start_dialog) cancel_pass=1;
  if (cancel_pass) return;
  if (!GlobEvent(MAGLOB_ONSTEP,viewsector,viewdir)) return;
  if (viewsector!=sect) nsect=viewsector,viewsector=sect;else nsect=map_sectors[viewsector].step_next[drs];
  if (map_sectors[nsect].sector_type==S_SCHODY)
     {
     int i;
     viewdir=(viewdir+map_sectors[nsect].side_tag) & 3;
     nsect=map_sectors[nsect].sector_tag;
     i=mob_map[nsect];
     while (i!=0)
        {
        i--;
        mobs[i].sector=mobs[i].home_pos;
        i=mobs[i].next;
        }
     mob_map[nsect]=0;
     }
  else if (mob_map[nsect] && !nopass) {
     if (!battle){
         if (!pass_all_mobs && !mob_alter(nsect)) return;
     }
     else return;
  }
  if (map_sectors[nsect].sector_type==S_LODKA)
     {
     int i;
     THUMAN *h;
     group_all();can_go=1;
     for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && h->inmaphash == current_map_hash && h->groupnum!=cur_group && h->lives) break;
     if (i!=POCET_POSTAV)
        {
        bott_disp_text(texty[66]);
        return;
        }
     }
  pass_zavora=1;
  norefresh=1;
  schovej_mysku();
  anim_sipky(H_SIPKY_S+smer,1);
  anim_sipky(0,255);
  hide_ms_at(385);
  ukaz_mysku();
  redraw_ms_cursor_on_screen();
  if (set_halucination) do_halucinace();
  if (loadlevel.name[0])
     {
     if (!battle)
        {
        pass_zavora=0;
        return;
        }
     nopass=1;
     loadlevel.name[0]=0;
     exit_wait=0;
     }
  if (!can_go) nopass=1;
  if (!nopass)
     {
     a_pass(viewsector,drs);
     viewsector=nsect;
     move_lodka(sect,nsect);
     chod_s_postavama(1);
     send_message(E_KROK);
     }
  if (!cancel_pass)
     {
     render_scene(viewsector,viewdir);
     OutBuffer2nd();
     sort_groups();
     bott_draw(0);
     other_draw();
    if (!nopass) shift_zoom(smer);
    if (battle || (game_extras & EX_ALWAYS_MINIMAP)) draw_medium_map();
    sort_groups();
    bott_draw(0);
    other_draw();
     }
  update_mysky();
  ukaz_mysku();
  showview(0,0,0,0);
  norefresh=0;
  mix_back_sound(0);
  viewsector=postavy_propadnout(viewsector);
  check_postavy_teleport();
  recheck_button(sect,1);
  recheck_button(viewsector,1);
  check_players_place(1);
  cancel_pass=0;
  pass_zavora=0;
  if (force_start_dialog)
     {
     force_start_dialog=0;
     call_dialog(start_dialog_number,start_dialog_mob);
     }
  if (cur_mode==MD_GAME) recalc_volumes(viewsector,viewdir);
  }

void turn_zoom(int smer) {
    if (running_anm)
        return;
    if (pass_zavora)
        return;
    else
        pass_zavora = 1;
    if (!GlobEvent(MAGLOB_ONTURN, viewsector, viewdir))
        return;
    if (set_halucination)
        do_halucinace();
    norefresh = 1;
    hold_timer(TM_BACK_MUSIC, 1);
    viewdir = (viewdir + smer) & 3;
    recalc_volumes(viewsector, viewdir);
    render_scene(viewsector, viewdir);
    hide_ms_at(387);
    OutBuffer2nd();
    other_draw();
    bott_draw(0);
    if (smer == 1) {
        anim_sipky(H_SIPKY_SV, 1);
        anim_sipky(0, 255);
        turn_left();
    } else {
        anim_sipky(H_SIPKY_SZ, 1);
        anim_sipky(0, 255);
        turn_right();
    }
    chod_s_postavama(0);
    if (battle || (game_extras & EX_ALWAYS_MINIMAP))
        draw_medium_map();
    update_mysky();
    ukaz_mysku();
    showview(0, 0, 0, 0);
    if (!battle)
        calc_game();
    norefresh = 0;
    hold_timer(TM_BACK_MUSIC, 0);
    mix_back_sound(0);
    pass_zavora = 0;
}

int check_path(word **path,word tosect)
  {
  word *p,*n,ss;
  char ok;
  int i=0;

  p=*path;
  n=p+1;ok=0;
  while (*p!=tosect)
     {
     if (map_sectors[*n].sector_type!=S_DIRA && ISTELEPORTSECT(*n))
        {
        for(i=0;i<4 && map_sectors[*p].step_next[i]!=*n;i++) {
           if (i==4)
              {
              ss=*p;
              free(*path);*path=NULL;
              return ss;
              }
        }
           if (!(map_sides[(*p<<2)+i].flags & SD_PLAY_IMPS)) ok=1;
        }
     if (!ok)
        {
        ss=*p;
        free(*path);*path=NULL;
        najdi_cestu(*p,tosect,SD_PLAY_IMPS,path,0);
        if (*path==NULL) return ss;
        p=*path;
        n=p+1;
        a_touch(ss,i);
        }
     p++;n++;
     ok=0;
     }
  free(*path);*path=NULL;
  return tosect;
  }

static unsigned int get_path_len(const word *w) {
    unsigned int i = 0;
    while (w[i]) ++i;
    return i;
}

void recall()
  {
  int tosect = 0;
  unsigned int max,i;
  int j;
  word *paths[POCET_POSTAV];

  for(i=0;i<POCET_POSTAV;i++)
     {
     if (postavy[i].groupnum==cur_group) tosect=postavy[i].sektor;
     paths[i]=NULL;
     }
  for(i=0;i<POCET_POSTAV;i++)
     if (postavy[i].sektor!=tosect) najdi_cestu(postavy[i].sektor,tosect,SD_PLAY_IMPS,&paths[i],0);
  do
     {
     max=0xffff;j=-1;
     unsigned int l;
     for(i=0;i<POCET_POSTAV;i++) if (paths[i]!=NULL && (l=get_path_len(paths[i]))<(unsigned)max)
                                      {
                                      max=l;
                                      j=i;
                                      }
     if (j!=-1)
        {
        postavy[j].sektor=check_path(&paths[j],tosect);
        free(paths[j]);paths[j]=NULL;
        bott_draw(1);
        other_draw();
        showview(0,0,0,0);
        }
     }
   while (j!=-1);
  }

void key_break_sleep(EVENT_MSG *msg,void **unused)
  {

  unused;
  if (msg->msg==E_KEYBOARD) break_sleep=1;
  if (msg->msg==E_MOUSE)
     {
     MS_EVENT *ms;

     ms=get_mouse(msg);
     if (ms->event_type & (0x2+0x8+0x20)) break_sleep=1;
     }
  }

void sleep_players(va_list args)
  {
  int i;
  int hours=0;
  char reg;
  char enablity;

  if (!sleep_ticks) return;
  if (!GlobEvent(MAGLOB_STARTSLEEP,viewsector,viewdir)) return;
  enablity=enable_sound(0);
  mute_all_tracks(0);
  autosave();
  insleep=1;
  update_mysky();
  schovej_mysku();
  curcolor=0;bar32(0,17,639,360+16);
  send_message(E_ADD,E_KEYBOARD,key_break_sleep);
  ukaz_mysku();
  showview(0,0,0,0);
  unwire_proc();
  break_sleep=0;
  while (sleep_ticks && !break_sleep)
     {
     reg=0;
     if (!(sleep_ticks%6))
       {
       if ((reg=(sleep_ticks%HODINA==0))==1)
           {
           char s[50];
           for(i=0;i<POCET_POSTAV;i++)
           break_sleep|=sleep_regenerace(&postavy[i]);
           update_mysky();
           schovej_mysku();
           bott_draw(0);
           curcolor=0;bar32(0,120,639,140);
           sprintf(s,texty[71],hours++);
           set_font(H_FBOLD,RGB555(31,31,0));
           set_aligned_position(320,130,1,1,s);outtext(s);
           other_draw();
           ukaz_mysku();
           showview(0,120,640,20);
           showview(0,378,640,102);
           task_wait_event(E_TIMER);
           task_wait_event(E_TIMER);
           task_wait_event(E_TIMER);
           task_wait_event(E_TIMER);
           }
       sleep_enemy(reg);
       check_players_place(0);
       }
     if (battle) break_sleep|=1;
     for(i=0;i<POCET_POSTAV;i++)
        {
        break_sleep|=check_jidlo_voda(&postavy[i])|check_map_specials(&postavy[i]);
        }
     send_message(E_KOUZLO_KOLO);
     sleep_ticks--;
     tick_tack(1);
     if (!TimerEvents(viewsector,viewdir,game_time)) break;
     }
  send_message(E_DONE,E_KEYBOARD,key_break_sleep);
  wire_proc();
  bott_draw(1);
  insleep=0;
  enable_sound(enablity);
  GlobEvent(MAGLOB_ENDSLEEP,viewsector,viewdir);
  }


void game_keyboard(EVENT_MSG *msg,void **usr)
  {
  int c;

  usr;
  if (pass_zavora) return ;
  if (cur_mode==MD_END_GAME) return ;
  if (msg->msg==E_KEYBOARD)
     {
      c=quit_request_as_escape(va_arg(msg->data, int));
     while (_bios_keybrd(_KEYBRD_READY) ) _bios_keybrd(_KEYBRD_READ);
     switch (c >> 8)
        {
         case 17:
        case 'H':step_zoom(0);break;
        case 31:
        case 'P':step_zoom(2);break;
        case 30: step_zoom(3);break;
        case 32: step_zoom(1);break;
        case 16: turn_zoom(-1);break;
        case 18: turn_zoom(1);break;
        case 'M':if (get_control_key_state() & 0x80)
            step_zoom(1);
          else
            turn_zoom(1);
          break;
        case 'K':if (get_control_key_state() & 0x80)
            step_zoom(3);
          else
            turn_zoom(-1);
          break;
        case 79:
        case 's':step_zoom(3);break;
        case 81:
        case 't':step_zoom(1);break;
        case 0x21:if (q_item(flute_item,viewsector)) bott_draw_fletna();
            break;
        case 57:a_touch(viewsector,viewdir);if (cur_mode==MD_PRESUN)send_message(E_KEYBOARD,28*256);break;
        case 15:
        case 50:
   	  	      if (GlobEvent(MAGLOB_BEFOREMAPOPEN,viewsector,viewdir))
				show_automap(1);
              break;
        case 0x17:unwire_proc();
                 wire_inv_mode(human_selected);break;
//      case 'A':lodka=!lodka;set_backgrnd_mode(lodka);break;
        case 1:konec(0,0,0,0,0);break;
//        case 25:GamePause();break;
        case 28:enforce_start_battle();break;
        case 60:clk_saveload(1, 0, 0, 0, 0);break;
        case 61:clk_saveload(0, 0, 0, 0, 0);break;
        case 59:game_setup(0,0,0,0,0);break;
        case 63:do_autosave();break;
        case 62:clk_sleep(0,0,0,0,0);break;
        case 64:go_book(0, 0, 0, 0, 0);break;
        case 65:spell_casting(0, 0, 0, 0, 0);break;
        case 45:
        case 82:group_all();break;
        case 0x2E: if (get_control_key_state() && get_shift_key_state()) {
                    console_show(!console_is_visible());
                    }
                    break;
        CASE_KEY_1_6:c=group_sort[(c >> 8)-2];
                     if (postavy[c].sektor==viewsector && postavy[c].used && postavy[c].inmaphash == current_map_hash)
                       add_to_group(c);
                     zmen_skupinu(postavy+c);
                     bott_draw(1);
                     break;
/*        default:
              {
              char s[20];
              bott_disp_text(itoa(c,s,10));
              break;
              }*/
         }
     }
  }


void start_dialog(int dialog,int mob)
  {
  if (battle)
  {
    call_dialog(dialog,mob);
  }
  else
  {
    force_start_dialog=1;
    start_dialog_number=dialog;
    start_dialog_mob=mob;
  }
//  call_dialog(dialog,mob);
  }


int postavy_propadnout(int sector)
  {
  char redraw=0;
  int i,z=sector;
  mute_hit_sound=0;
  if (map_coord[sector].flags & MC_DPLAYER && map_sectors[sector].sector_type==S_DIRA)
     {
     for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==sector && postavy[i].used && postavy[i].inmaphash==current_map_hash && (postavy[i].vlastnosti[VLS_KOUZLA] & SPL_LEVITATION)==0)
        {
        int l=postavy[i].vlastnosti[VLS_MAXHIT];
        z=postavy[i].sektor=map_sectors[sector].sector_tag;
        if (postavy[i].groupnum==cur_group)  viewsector=z;
        if (z) player_hit(postavy+i,rnd(l/2)+l/3,0);else player_hit(postavy+i,postavy[i].lives,0);
        redraw=1;
        }
     if (redraw) bott_draw(1);
     }
  return z;
  }


