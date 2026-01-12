#include <platform/platform.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libs/pcx.h>
#include <libs/types.h>
#include <libs/bgraph.h>
#include <libs/event.h>
#include <libs/strlite.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/memman.h>
#include <fcntl.h>
#include <platform/sound.h>
#include <stdarg.h>
#include "globals.h"
#include "temp_storage.h"

#include <assert.h>
#include <sys/stat.h>
#include <time.h>
#define STATE_CUR_VER 2

#define _GAME_ST "_GAME.TMP"
#define _GLOBAL_ST "_GLOBEV.TMP"
#define _SLOT_SAV "slot%02d.SAV"
#define SLOTS_MAX 10

#define GM_MAPENABLE 0x1
#define GM_FASTBATTLES 0x2
#define GM_GAMESPEED_SHIFT 2
#define GM_GAMESPEED_MASK 0x1F

#define SAVE_SLOT_S 34
#define LOAD_SLOT_S (372+34)
#define SAVE_SLOT_E (34+203)
#define LOAD_SLOT_E (372+34+203)

#define SSAVE_VERSION 0

//static TMPFILE_WR *story=NULL;
static char load_another;
static unsigned long current_campaign = 0;
static long prev_game_time_save = -999;
static long play_time = 0;    //current play time
static long load_game_time = 0;    //time when game has been loaded (to calculate play_time)

#define AUTOSAVE_SUFFIX "-autosave"

char reset_mobiles=0;

typedef struct s_save
  {
  int viewsector;
  char viewdir;
  short version;
  char not_used;
  int gold;
  short cur_group;
  char autosave;
  char enable_sort;
  char shownames;
  char showlives;
  char zoom_speed;
  char turn_speed;
  char autoattack;
  char music_vol;
  char sample_vol;
  char xbass;
  char bass;
  char treble;
  char stereing;
  char swapchans;
  char out_filter;
  int32_t glob_flags;
  int32_t game_time;
  char runes[5];
  char level_name[13];
  short picks;  //pocet_sebranych predmetu v mysi
  short items_added; //pocet_pridanych predmetu
  int sleep_long;
	int game_flags;
  }S_SAVE;


static int get_list_count();

static int unable_open_temp(char *c)
  {
  char d[]="Unable to open the file : ",*e;

  concat(e,d,c);
  display_error(e);
  SEND_LOG("(SAVELOAD) Open temp error detected (%s)",c);
  exit(1);
  }

static void unable_write_temp(char *c)
  {
  char d[]="Unable to write to the temp file : ",*e;

  concat(e,d,c);
  display_error(e);
  SEND_LOG("(SAVELOAD) Open temp error detected (%s)",c);
  exit(1);
  }


int load_org_map(const char *filename,TSTENA **sides,TSECTOR **sectors,TMAP_EDIT_INFO **coords,int *mapsize)
  {
  FILE *f;
  void *temp;
  int sect;
  int32_t size,r;
  char nmapend=1;


	const char *c=build_pathname(2, gpathtable[SR_MAP],filename);
	f=fopen_icase(c,"rb");
  if (f==NULL) return -1;
  do
     {
     r=load_section(f,&temp,&sect,&size);
     if (r==size)
        switch (sect)
         {
         case A_SIDEMAP:
                  *sides=temp;
                  break;
         case A_SECTMAP:
                  *sectors=temp;
                  if (mapsize!=NULL) *mapsize=size/sizeof(TSECTOR);
                  break;
         case A_MAPINFO:
                  if (coords!=NULL) *coords=temp;else free(temp);
                  break;
         case A_MAPGLOB:
                  //memcpy(&mglob,temp,min(sizeof(mglob),size));
                  free(temp);
                  break;
         case A_MAPEND :
                  nmapend=0;
                  free(temp);
                  break;
         default: free(temp);
         }
     else
        {
        if (temp!=NULL)free(temp);
        fclose(f);
        return -1;
        }
     }
  while (nmapend);
  fclose(f);
  return 0;
  }

void save_daction(TMPFILE_WR *f,int count,D_ACTION *ptr)
  {
  if (ptr!=NULL)
     {
     save_daction(f,count+1,ptr->next);
     temp_storage_write(ptr,sizeof(D_ACTION),f);
     }
  else
      temp_storage_write(&count,2,f);
  }

void load_daction(TMPFILE_RD *fsta)
  {
  uint16_t i,j;
  i=0;
  while (d_action!=NULL) //vymaz pripadne delaited actions
     {
     D_ACTION *p;
     p=d_action; d_action=p->next;free(p);
     }
  temp_storage_read(&i,2,fsta);d_action=NULL;
  for(j=0;j<i;j++)
     {
     D_ACTION *p;

     p=(D_ACTION *)getmem(sizeof(D_ACTION));
     temp_storage_read(p,sizeof(D_ACTION),fsta);
     p->next=d_action;
     d_action=p;
     }
  }

void save_items(TMPFILE_WR *f)
  {
  int32_t i,j;
  short *c;

  for(i=0;i<mapsize*4;i++)
     if (map_items[i]!=NULL)
        {
        for(j=1,c=map_items[i];*c;c++,j++);
        temp_storage_write(&i,sizeof(i),f);
        temp_storage_write(&j,sizeof(j),f);
        temp_storage_write(map_items[i],2*j,f);
        }
  i=-1;
  temp_storage_write(&i,sizeof(i),f);
  }

void restore_items(TMPFILE_RD *f)
  {
  int32_t i,j;

  short **new_item_map = getmem(mapsize*4*sizeof(*map_items));
  memset(new_item_map,0,mapsize*4*sizeof(*map_items));
  while(temp_storage_read(&i,sizeof(i),f) && i!=-1) {
     temp_storage_read(&j,sizeof(j),f);
     new_item_map[i]=(short *)getmem(j*2);
     temp_storage_read(new_item_map[i],2*j,f);
     short *v = new_item_map[i];
     while (*v) { //sanitize map items
      int vv = abs(*v);
      if (vv > item_count) {
        free(new_item_map[i]);
        new_item_map[i] = map_items[i];
        map_items[i] = NULL;
        break;
      }
      ++v;
     }
  }

     for(i=0;i<mapsize*4;i++) if (map_items[i]!=NULL) free(map_items[i]);
     free(map_items);
     map_items = new_item_map;

}



void save_vyklenky(TMPFILE_WR *fsta)
  {
    temp_storage_write(&vyk_max,1*sizeof(vyk_max),fsta);
  if (vyk_max)
      temp_storage_write(map_vyk,vyk_max*sizeof(TVYKLENEK),fsta);
  }

int load_vyklenky(TMPFILE_RD *fsta)
  {
  int i=0;
  temp_storage_read(&i,1*sizeof(vyk_max),fsta);
  if (vyk_max)
     {
     if (i>vyk_max) return -2;
     temp_storage_read(map_vyk,vyk_max*sizeof(TVYKLENEK),fsta);
     }
  return 0;
  }


void save_all_fly(TMPFILE_WR *fsta)
  {
  LETICI_VEC *f;
  int32_t sz = 1;

  f=letici_veci;
  while (f!=NULL)
     {
     short *c;

     sz = sizeof(*f);
     temp_storage_write(&sz,sizeof(sz),fsta);
     temp_storage_write(f,sz,fsta);
     c=f->items;
     if (c!=NULL) do temp_storage_write(c,1*2,fsta); while (*c++);
     f=f->next;
     }
  sz = 0;
  temp_storage_write(&sz,sizeof(sz),fsta);
  }

int load_all_fly(TMPFILE_RD *fsta)
  {
  LETICI_VEC *n;
  int32_t sz;

  destroy_all_fly();

  temp_storage_read(&sz,sizeof(sz),fsta);
  while (sz == sizeof(LETICI_VEC))
     {
     short items[100],*c;
     n=New(LETICI_VEC);
     c=items;memset(items,0,sizeof(items));
     if (temp_storage_read(n,1*sizeof(*n),fsta)!=sizeof(*n))
        {
        free(n);
        return -2;
        }
     if (n->items!=NULL)
        {
        do
            temp_storage_read(c,2,fsta);
        while (*c++);
        n->items=NewArr(short,c-items);
        memcpy(n->items,items,(c-items)*sizeof(short));
        }
     add_fly(n);
     temp_storage_read(&sz,sizeof(sz),fsta);
     }
  return sz != 0;
  }




int save_map_state() //uklada stav mapy pro savegame (neuklada aktualni pozici);
  {
  char sta[200];
  char *bf = NULL;
  TMPFILE_WR *fsta;
  int32_t i;
  int32_t siz;
 TSTENA *org_sides;
 TSECTOR *org_sectors;
  short res=-1;
  unsigned char ver=0;

  strcpy(sta,level_fname);
  fsta=temp_storage_create(sta);if (fsta==NULL) unable_open_temp(sta);
  SEND_LOG("(SAVELOAD) Saving map state for current map");
  if (load_org_map(level_fname,&org_sides,&org_sectors,NULL,NULL)) goto err;
  siz=(mapsize+7)/8;
  bf=(char *)getmem(siz);
  ver=STATE_CUR_VER;
  temp_storage_write(&ver,sizeof(ver)*1,fsta);  //<-------------------------
  temp_storage_write(&mapsize,sizeof(mapsize)*1,fsta);  //<-------------------------
  memset(bf,0,siz);
  temp_storage_write(&siz,1*sizeof(siz),fsta);          //<-------------------------
  for(i=0;i<mapsize;i++)  //save automap
    if (map_coord[i].flags & MC_AUTOMAP) bf[i>>3]|=1<<(i & 7);
  temp_storage_write(bf,siz*1,fsta);
  for(i=0;i<mapsize;i++)  //save disclosed
    if (map_coord[i].flags & MC_DISCLOSED) bf[i>>3]|=1<<(i & 7);
  temp_storage_write(bf,siz*1,fsta);
  save_map_description(fsta);
  for(i=0;i<mapsize*4;i++)  //save changed sides
     if (memcmp(map_sides+i,org_sides+i,sizeof(TSTENA)))
        {
         temp_storage_write(&i,sizeof(i),fsta);
         temp_storage_write(map_sides+i,1*sizeof(TSTENA),fsta);
        }
  i=-1;
  temp_storage_write(&i,sizeof(i),fsta);
  for(i=0;i<mapsize;i++)   //save changed sectors
     if (memcmp(map_sectors+i,org_sectors+i,sizeof(TSECTOR)))
        {
         temp_storage_write(&i,sizeof(i),fsta);
         temp_storage_write(map_sectors+i,1*sizeof(TSECTOR),fsta);
        }
  i=-1;
  temp_storage_write(&i,sizeof(i),fsta);
  for(i=0;i<MAX_MOBS;i++) if ((mobs[i].vlajky & MOB_LIVE) && !(mobs[i].vlajky2 & MOB_F2_DONT_SAVE))
     {
      temp_storage_write(&i,sizeof(i),fsta);
      temp_storage_write(mobs+i,sizeof(TMOB),fsta); //save_mobmap
     }
  i=-1;
  temp_storage_write(&i,sizeof(i),fsta);
  i=mapsize*4;
  temp_storage_write(&i,sizeof(i),fsta); //save flag maps //<-------------------------
  temp_storage_write(flag_map,i,fsta);
  save_daction(fsta,0,d_action); //save dactions//<-------------------------
  temp_storage_write(&macro_state_block.count,1*sizeof(macro_state_block.count),fsta);
  if (macro_state_block.count)temp_storage_write(macro_state_block.states,1*macro_state_block.count,fsta);//save_macros
  if (save_codelocks(fsta)) goto err;
  save_items(fsta);
  save_vyklenky(fsta);
  save_all_fly(fsta);
  save_enemy_paths(fsta);
  res=0;
  err:
  SEND_LOG("(SAVELOAD) State of current map saved (err:%d)",res);
  temp_storage_close_wr(fsta);
  free(org_sectors);
  free(org_sides);
  free(bf);
  if (res)
     {
     remove(sta);
     unable_write_temp(sta);
     }
  return res;
  }

int load_map_state_ex(const char *level_name, int mapsize, char partial);
int load_map_state() {
    return load_map_state_ex(level_fname, mapsize, 0);
}
int load_map_state_ex(const char *level_fname, int mapsize, char partial)
  {
  char sta[200];
  char *bf = NULL;
  TMPFILE_RD *fsta;
  int32_t i;
  int32_t siz;
  short res=-2;
  unsigned char ver=0;

  strcpy(sta,level_fname);
  fsta=temp_storage_open(sta);if (fsta==NULL) return -1;
  i=0;
    if (!temp_storage_read(&ver,sizeof(ver)*1,fsta)) goto err;
    if (ver>STATE_CUR_VER) goto err;
    if (!temp_storage_read(&i,sizeof(mapsize)*1,fsta)) goto err;
    if (mapsize!=i) goto err;
    SEND_LOG("(SAVELOAD) Loading map state for current map");
    temp_storage_read(&siz,1*sizeof(siz),fsta);
    bf=(char *)getmem(siz);
    if (!temp_storage_read(bf,siz*1,fsta)) goto err;
    for (i=0;i<mapsize;i++)
      if ((bf[i>>3]>>(i & 7)) & 1) map_coord[i].flags|=MC_AUTOMAP;
    if (!temp_storage_read(bf,siz*1,fsta)) goto err;
    for (i=0;i<mapsize;i++)
      if ((bf[i>>3]>>(i & 7)) & 1) map_coord[i].flags|=MC_DISCLOSED;
  load_map_description(fsta);
  while (temp_storage_read(&i,sizeof(i),fsta) && i >= 0 && i<=mapsize*4)
     if (temp_storage_read(map_sides+i,1*sizeof(TSTENA),fsta)!=sizeof(TSTENA)) goto err;
  while (temp_storage_read(&i,sizeof(i),fsta) && i >= 0 && i<=mapsize)
     if (temp_storage_read(map_sectors+i,1*sizeof(TSECTOR),fsta)!=sizeof(TSECTOR)) goto err;
  if (partial) {
      res = 0;
      goto err;
  }
  if (reset_mobiles)  //reloads mobiles if flag present
    {
    char mm[MAX_MOBS];
    for(i=0;i<MAX_MOBS;mobs[i].vlajky &=~MOB_LIVE,i++)
      if (mobs[i].vlajky & MOB_LIVE) mm[i]=1;else mm[i]=0;
    while (temp_storage_read(&i,1*2,fsta) && i<=MAX_MOBS)
      {
      if (mm[i]) mobs[i].vlajky |=MOB_LIVE;
      temp_storage_skip(fsta,sizeof(TMOB));
      }
    reset_mobiles=0;
    }
  else
    {
    for(i=0;i<MAX_MOBS;(mobs[i].vlajky &=~MOB_LIVE),i++);
    while (temp_storage_read(&i,sizeof(i),fsta) && i>=0 && i<=MAX_MOBS) {
       if (temp_storage_read(mobs+i,1*sizeof(TMOB),fsta)!=sizeof(TMOB)) goto err;
       mobs[i].vlajky2 = 0;
    }

    }
  for(i=0;i<MAX_MOBS;i++) mobs[i].vlajky &=~MOB_IN_BATTLE;
  refresh_mob_map();
  temp_storage_read(&i,sizeof(i),fsta);
  temp_storage_read(flag_map,i,fsta);
  load_daction(fsta);
  size_t stsz;
  temp_storage_read(&stsz,sizeof(macro_state_block.count),fsta);
  if (macro_state_block.count == stsz) {
      temp_storage_read(macro_state_block.states,macro_state_block.count,fsta);
  }
  else
     {
     temp_storage_skip(fsta,stsz);
     SEND_LOG("(ERROR) Multiaction: Sizes mismatch %lu != %lu",stsz,macro_state_block.count);
     }
  if (load_codelocks(fsta)) goto err;
  restore_items(fsta);
  res=0;
  res|=load_vyklenky(fsta);
  res|=load_all_fly(fsta);
  res|=load_enemy_paths(fsta);

  err:
  SEND_LOG("(SAVELOAD) State of current map loaded (err:%d)",res);
  temp_storage_close_rd(fsta);
  free(bf);
  return res;
  }

void restore_current_map() //pouze obnovuje ulozeny stav aktualni mapy
  {
  int i;

  SEND_LOG("(SAVELOAD) Restore map...");
  kill_all_sounds();
  for(i=0;i<mapsize;i++) map_coord[i].flags&=~0x7f; //vynuluj flags_info
  free(map_sides);        //uvolni informace o stenach
  free(map_sectors);      //uvolni informace o sektorech
  free(map_coord);       //uvolni minfo informace
  load_org_map(level_fname,&map_sides,&map_sectors,&map_coord,NULL); //nahrej originalni mapu
  load_map_state(); //nahrej ulozenou mapu
  for(i=1;i<mapsize*4;i++) call_macro(i,MC_STARTLEV);
  }

static void add_status_file(FILE *f, const char *name, size_t sz, void *data) {
    size_t name_size = strlen(name);
    uint8_t name_size_b;
    if (name_size > 255) name_size_b = 255; else name_size_b = (uint8_t)name_size;
    fwrite(&name_size_b,1,1,f);
    fwrite(name, 1, name_size_b, f);
    uint32_t data_size = sz;
    fwrite(&data_size,1,sizeof(data_size),f);
    if (data && data_size) fwrite(data,1,data_size, f);
}

static void pack_status_file_cb(const char *name, void *ctx) {
    FILE *f = ctx;
    int32_t sz = temp_storage_find(name);
    assert(sz >= 0);
    if (sz == 0) {
        add_status_file(f, name, 0, 0);
    } else {
        void *data = getmem(sz);
        temp_storage_retrieve(name, data, sz);
        add_status_file(f, name, sz, data);
        free(data);
    }
}

int pack_all_status(FILE *f)
  {
    char c = 0;
    temp_storage_list(pack_status_file_cb, f);
    fwrite(&c,1,1,f);
    return 0;
  }


typedef enum enum_all_status_callback_result_t {

    enum_status_cont_skip,       //continue, skip this block
    enum_status_cont_read,       //continue, i read this block
    enum_status_skip_stop,       //stop, skip this block
    enum_status_read_stop,       //stop, don't skip i read this block
    enum_status_error            //error

} ENUM_ALL_STATUS_CALLBACK_RESULT;

//return 1 success, 0 stopped, -1 error
static int enum_all_status(FILE *f, ENUM_ALL_STATUS_CALLBACK_RESULT (*cb)(FILE *, const char *, size_t , void *), void *ctx) {
    while(1) {
        uint8_t name_size_b = 0;
        if (fread(&name_size_b, 1,1,f)==0) return 1;
        if (name_size_b == 0)  {
            return -1;
        }
        char *name = (char *)alloca(name_size_b+1);
        if (fread(name, 1, name_size_b, f) != name_size_b) return -1;
        name[name_size_b] = 0;
        uint32_t data_size;
        if (fread(&data_size,1,sizeof(uint32_t),f) != sizeof(uint32_t)) return -1;
        ENUM_ALL_STATUS_CALLBACK_RESULT st = cb(f, name, data_size, ctx);
        switch (st) {
            case enum_status_error: return 0;
            case enum_status_cont_skip: fseek(f, data_size, SEEK_CUR);break;
            case enum_status_cont_read: break;
            case enum_status_skip_stop: fseek(f, data_size, SEEK_CUR);return 0;
            case enum_status_read_stop: return 0;
        }
    }
}

static ENUM_ALL_STATUS_CALLBACK_RESULT unpack_status_callback(FILE *f, const char *name, size_t datasize, void *_) {
    void *buff = getmem(datasize);
    if (fread(buff, 1, datasize, f) != datasize) {
        free(buff);
        return enum_status_error;;
    }
    temp_storage_store(name, buff, datasize);
    free(buff);
    return enum_status_cont_read;
}

int unpack_all_status(FILE *f)
  {
    int r = enum_all_status(f, &unpack_status_callback, NULL);
    if (r) return 0;
    return -1;
  }

static void save_destroyed_items(TMPFILE_WR *f) {
  short c = (short)count_items_total(destroyed_items);
  temp_storage_write(&c,sizeof(c), f);
  if (c) {
    temp_storage_write(destroyed_items,sizeof(short)*c, f);
  }
}

int save_basic_info()
  {
  TMPFILE_WR *f;
  S_SAVE s;
  short *p;
  int i;
  char res=0;
  THUMAN *h;

  SEND_LOG("(SAVELOAD) Saving basic info for game (file:%s)",_GAME_ST );
  f=temp_storage_create(_GAME_ST);
  if (f==NULL) return 1;
  s.viewsector=viewsector;
  s.viewdir=viewdir;
  s.version=SSAVE_VERSION;
  s.not_used=0;
  s.gold=money;
  s.cur_group=cur_group;
  s.shownames=show_names;
  s.showlives=show_lives;
  s.autoattack=autoattack;
  s.turn_speed=turn_speed(-1);
  s.zoom_speed=zoom_speed(-1);
  s.game_time=game_time;
  s.enable_sort=enable_sort;
  s.sleep_long=sleep_ticks;
  s.sample_vol=MIN(get_snd_effect(SND_GFX),255);
  s.music_vol=MIN(get_snd_effect(SND_MUSIC),255);
  s.xbass=MIN(get_snd_effect(SND_XBASS),255);
  s.bass=MIN(get_snd_effect(SND_BASS),255);
  s.treble=MIN(get_snd_effect(SND_TREBL),255);
  s.stereing=MIN(get_snd_effect(SND_LSWAP),255);
  s.swapchans=MIN(get_snd_effect(SND_SWAP),255);
  s.out_filter=MIN(get_snd_effect(SND_OUTFILTER),255);
  s.autosave=autosave_enabled;
	s.game_flags=0;

  if (enable_glmap!=0) s.game_flags |= GM_MAPENABLE;
  if (gamespeedbattle<gamespeed) s.game_flags |= GM_FASTBATTLES;
  if (timerspeed_val <= GM_GAMESPEED_MASK) {
      s.game_flags |= (timerspeed_val & GM_GAMESPEED_MASK) << GM_GAMESPEED_SHIFT;
  }

  strcopy_n(s.level_name,level_fname,sizeof(s.level_name));
  for(i=0;i<5;i++) s.runes[i]=runes[i];
  if (picked_item!=NULL)
     for(i=1,p=picked_item;*p;i++,p++);else i=0;
  s.picks=i;
  s.items_added=item_count-it_count_orgn;
  temp_storage_write(&s,1*sizeof(s),f);
  if (i)
     temp_storage_write(picked_item,2*i,f);
  if (s.items_added)
     temp_storage_write(glob_items+it_count_orgn,sizeof(TITEM)*s.items_added,f);
  save_spells(f);
  temp_storage_write(postavy,1*sizeof(postavy),f);
  for(i=0,h=postavy;i<POCET_POSTAV;h++,i++) if (h->demon_save!=NULL)
     temp_storage_write(h->demon_save,sizeof(THUMAN)*1,f);       //ulozeni polozek s demony
  res|=save_dialog_info(f);
  save_destroyed_items(f);
  temp_storage_close_wr(f);
  SEND_LOG("(SAVELOAD) Done... Result: %d",res);
  return res;
  }

static int load_destroyed_items(TMPFILE_RD *f) {
  int res = 0;
  short destroyed_items_count = 0;
  temp_storage_read(&destroyed_items_count,sizeof(destroyed_items_count), f);
  free(destroyed_items);
  destroyed_items = NULL;
  if (destroyed_items_count) {
    destroyed_items = NewArr(short, destroyed_items_count+1);
    res|=temp_storage_read(destroyed_items,destroyed_items_count*sizeof(short), f) != destroyed_items_count*sizeof(short);
    destroyed_items[destroyed_items_count] = 0;
  }
  return res;
}

int load_basic_info()
  {
  TMPFILE_RD *f;
  S_SAVE s;
  int i;
  char res=0;
  TITEM *itg;
  THUMAN *h;

  SEND_LOG("(SAVELOAD) Loading basic info for game (file:%s)",_GAME_ST);
  f=temp_storage_open(_GAME_ST);
  if (f==NULL) return 1;
  res|=(temp_storage_read(&s,1*sizeof(s),f)!=sizeof(s));
	if (s.game_flags & GM_MAPENABLE) enable_glmap=1;else enable_glmap=0;
  gamespeedbattle =s.game_flags & GM_FASTBATTLES? GAMESPEED_FASTBATTLE:gamespeed;
  int tmsp = (s.game_flags >> GM_GAMESPEED_SHIFT) & GM_GAMESPEED_MASK;
  if (tmsp) timerspeed_val = tmsp;
  i=s.picks;
  if (picked_item!=NULL) free(picked_item);
  if (i)
     {
     picked_item=NewArr(short,i);
     res|=(temp_storage_read(picked_item,2*i,f)!=(unsigned)i);
     }
  else picked_item=NULL;
  itg=NewArr(TITEM,it_count_orgn+s.items_added);
  memcpy(itg,glob_items,it_count_orgn*sizeof(TITEM));
  free(glob_items);glob_items=itg;
  if (s.items_added)
     res|=(temp_storage_read(glob_items+it_count_orgn,sizeof(TITEM)*s.items_added,f)!=(unsigned)s.items_added*sizeof(TITEM));
  item_count=it_count_orgn+s.items_added;
  res|=load_spells(f);
  for(i=0,h=postavy;i<POCET_POSTAV;h++,i++) if (h->demon_save!=NULL) free(h->demon_save);
  if (!res) res|=(temp_storage_read(postavy,1*sizeof(postavy),f)!=sizeof(postavy));
  for(i=0,h=postavy;i<POCET_POSTAV;h++,i++)
        {
        h->programovano=0;
        h->provadena_akce=h->zvolene_akce=NULL;
        h->dostal=0;
        if (h->demon_save!=NULL)
          {
          h->demon_save=New(THUMAN);
          temp_storage_read(h->demon_save,sizeof(THUMAN)*1,f);//obnova polozek s demony
          h->demon_save->programovano = 0;
          h->demon_save->provadena_akce=h->zvolene_akce=NULL;
          }
        }
  res|=load_dialog_info(f);
  res|=load_destroyed_items(f);
  temp_storage_close_rd(f);
  viewsector=s.viewsector;
  viewdir=s.viewdir;
  cur_group=s.cur_group;
  show_names=s.shownames;
  show_lives=s.showlives;
  autoattack=s.autoattack;
  turn_speed(s.turn_speed);
  zoom_speed(s.zoom_speed);
  game_time=s.game_time;
  sleep_ticks=s.sleep_long;
  enable_sort=s.enable_sort;
  autosave_enabled=s.autosave;
  money=s.gold;
  for(i=0;i<5;i++) runes[i]=s.runes[i];
  set_snd_effect(SND_GFX,s.sample_vol);
  set_snd_effect(SND_MUSIC,s.music_vol);
  set_snd_effect(SND_XBASS,s.xbass);
  set_snd_effect(SND_BASS,s.bass);
  set_snd_effect(SND_TREBL,s.treble);
  set_snd_effect(SND_LSWAP,s.stereing);
  set_snd_effect(SND_SWAP,s.swapchans);
  set_snd_effect(SND_OUTFILTER,s.out_filter);
  if (level_fname==NULL || strcmp(s.level_name,level_fname))
     {
     strcopy_n(loadlevel.name,s.level_name,sizeof(loadlevel.name));
     loadlevel.start_pos=viewsector;
     loadlevel.dir=viewdir;
     send_message(E_CLOSE_MAP,NULL);
     load_another=1;
     }
  else load_another=0;
  for(i=0;i<POCET_POSTAV;i++) postavy[i].dostal=0;
  SEND_LOG("(SAVELOAD) Done... Result: %d",res);
  return res;
  }


static int save_global_events()
{
  TMPFILE_WR *f;
  f=temp_storage_create(_GLOBAL_ST);
  if (f==NULL) return 1;
  temp_storage_write(GlobEventList,1*sizeof(GlobEventList),f);
  temp_storage_close_wr(f);
  return 0;
}

static int load_global_events()
{
  TMPFILE_RD *f;
  memset(GlobEventList,0,sizeof(GlobEventList));

  f=temp_storage_open(_GLOBAL_ST);
  if (f==NULL) return 1;
  temp_storage_read(GlobEventList,1*sizeof(GlobEventList),f);
  temp_storage_close_rd(f);
  return 0;
}

int save_game(long game_time,char *gamename, char is_autosave)
  {
  char *gn;
  FILE *svf;
  int r;

  if (current_campaign == 0) {
      time_t t;
      time(&t);
      current_campaign = (unsigned long) t;
  }

  char str_buff[50];
  snprintf(str_buff,sizeof(str_buff),"sav.%08lx.%08lx%s", current_campaign, game_time, is_autosave?AUTOSAVE_SUFFIX:"");
  SEND_LOG("(SAVELOAD) Saving game slot %ld",game_time);
  save_map_state();

  const char *sn = build_pathname(2,gpathtable[SR_SAVES],str_buff);
  sn = local_strdup(sn);
  create_directories(gpathtable[SR_SAVES]);
  gn=alloca(SAVE_NAME_SIZE);
  strcopy_n(gn,gamename,SAVE_NAME_SIZE);
  if ((r=save_shops())!=0) return r;
  if ((r=save_basic_info())!=0) return r;
  save_book();
  save_global_events();

  long new_play_time = play_time + get_game_tick_count()/1000 - load_game_time;
  temp_storage_store("playtime",&new_play_time, sizeof(new_play_time));

  svf=fopen_icase(sn,"wb");
  if (svf==NULL){
    if (!is_autosave) {
      char buff[256];
      sprintf(buff,"Failed to create savegame at path %s", sn);    
      message(1,0,0,"",buff,texty[80]);
    }
  }
  else
  {
	fwrite(gn,1,SAVE_NAME_SIZE,svf);
	close_story_file();
	r=pack_all_status(svf);
	open_story_file();
	fclose(svf);
  }
  SEND_LOG("(SAVELOAD) Game saved.... Result %d",r);
  play_fx_at(FX_SAVE);
  return r;
  }

extern char running_battle;

int load_game(const char *fname)
  {
  FILE *svf;
  int r,t;

  sscanf(fname,"sav.%lx.%lx", &current_campaign, &prev_game_time_save);
  prev_game_time_save = get_game_tick_count()/1000;

  SEND_LOG("(SAVELOAD) Loading file: %s",fname);
  if (battle) konec_kola();
  battle=0;
  close_story_file();
  purge_temps(0);
  const char *sn = build_pathname(2, gpathtable[SR_SAVES], fname);
  svf=fopen_icase(sn,"rb");
  if (svf==NULL) return 1;
  fseek(svf,SAVE_NAME_SIZE,SEEK_CUR);
  r=unpack_all_status(svf);
  fclose(svf);
  open_story_file();
  if (r>0)
     {
     SEND_LOG("(ERROR) Error detected during unpacking game... Loading stopped (result:%d)",r);
     return r;
     }
  load_book();
  load_global_events();
  if ((t=load_saved_shops())!=0) return t;
  if ((t=load_basic_info())!=0) return t;

  if (temp_storage_find("playtime") == sizeof(long)) {
    temp_storage_retrieve("playtime", &play_time, sizeof(play_time));
  } else {
    play_time = 0;
  }

  load_game_time = get_game_tick_count() / 1000;

  running_battle=0;
  norefresh=0;
  if (!load_another) restore_current_map();
        else
           {
           save_map=0;
           norefresh=1;
           }
  for(t=0;t<POCET_POSTAV;t++) postavy[t].zvolene_akce=NULL;
  SEND_LOG("(SAVELOAD) Game loaded.... Result %d",r);
//  if (GetKeyState(VK_CONTROL) & 0x80) correct_level();
  return r;
  }

typedef struct load_specific_file_callback_data_t {
    const char *name;
    size_t size;
    void *data;

} LOAD_SPECIFIC_FILE_CALLBACK_DATA;

static ENUM_ALL_STATUS_CALLBACK_RESULT load_specific_file_callback(FILE *f, const char *name, size_t datasize, void *ctx) {
    LOAD_SPECIFIC_FILE_CALLBACK_DATA *me = ctx;
    if (istrcmp(name, me->name) == 0) {
        void *d = getmem(datasize);
        if (fread(d, 1, datasize, f) != datasize) {
            free(d);
            return enum_status_error;
        }
        me->size = datasize;
        me->data = d;
        return enum_status_read_stop;
    } else {
        return enum_status_cont_skip;
    }

}

//------------------------ SAVE LOAD DIALOG ----------------------------
static char force_save;
//static TSTR_LIST slot_list=NULL;
static int last_select=-1;

static TSTR_LIST story_text=NULL;
static void *back_texture=NULL;
static int cur_story_pos=0;
//static char load_mode;

#define SLOT_SPACE 33
#define SLOT_FONT H_FBOLD
#define SLOT_AUTOSAVE_FONT H_FLITT5
#define SLOT_AUTOSAVE_TEXT "auto"
#define SELECT_COLOR RGB555(31,31,31)
#define NORMAL_COLOR RGB555(16,31,16)
#define STORY_X 57
#define STORY_Y 50
#define STORY_XS (298-57)
#define STORY_YS (302-50)

#if 0
void read_slot_list()
  {
  int i;
  char *name;
  char slotname[SAVE_NAME_SIZE];
  if (slot_list==NULL) slot_list=create_list(SLOTS_MAX);
  const char *mask = build_pathname(2, gpathtable[SR_SAVES],_SLOT_SAV);
  name=alloca(strlen(mask)+1);
  for(i=0;i<SLOTS_MAX;i++)
     {
     FILE *f;
     sprintf(name,mask,i);
     f=fopen_icase(name,"rb");
     if (f!=NULL)
        {
        fread(slotname,1,SAVE_NAME_SIZE,f);
        fclose(f);
        used_pos[i]=1;
        }
     else
        {
        strcpy(slotname,texty[75]);
        used_pos[i]=0;
        }
     str_replace(&slot_list,i,slotname);
     }
  }

#endif
typedef struct {
    TSTR_LIST files;
    TSTR_LIST names;
    char *autosave_flags;
    size_t count;
} TSAVEGAME_LIST;

static TSAVEGAME_LIST current_game_slot_list = {};
static int current_slot_list_top_line = 0;

typedef struct {
    TSTR_LIST files;
    const char *prefix;
    size_t prefix_len;
    size_t count;
    char skip_autosave;
} TSAVEGAME_CB_STATE;

/*
static char is_same_prefix(const char *name, const char *prev_name) {
    const char *sep1 = strrchr(name, '.');
    const char *sep2 = strrchr(prev_name, '.');
    if (!sep1 || !sep2) return 0;
    if ((sep1 - name) != (sep2 - prev_name)) return 0;
    if (strncmp(name, prev_name,sep1-name) == 0) return 1;
    return 0;
}
*/
static int get_all_savegames_callback(const char *name, LIST_FILE_TYPE  type , size_t tm, void *ctx) {
    if (istrncmp(name, "sav.", 4) != 0
            && istrcmp(name+strlen(name)-4,".sav") != 0)
            return 0;
    TSAVEGAME_CB_STATE *st = (TSAVEGAME_CB_STATE *)ctx;
    if (st->prefix_len == 0 || strncmp(name, st->prefix, st->prefix_len) == 0) {
        char is_autosave = !!strstr(name, AUTOSAVE_SUFFIX);
        if (is_autosave && st->skip_autosave) return 0;
        size_t nlen = strlen(name);
        if (st->count == (size_t)str_count(st->files)) {
          TSTR_LIST nw = create_list(st->count * 3/2);
          str_move_list(nw, st->files);;
          release_list(st->files);
          st->files = nw;
        }
        char *buff = NewArr(char, nlen + 2);
        strncpy(buff, name, nlen+1);
        buff[nlen+1] = is_autosave?'\x1f':0;
        st->files[st->count] = buff;
        ++st->count;
    }
    return 0;
}
/*
static int compare_strings (const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}
*/
static int compare_strings_third_back (const void *a, const void *b) {
  const char *sa = *(const char **)a;
  const char *sb = *(const char **)b;
  const char *ba = strrchr(sa,'.');
  const char *bb = strrchr(sb,'.');
  if (!ba) ba = sa;else ba++;
  if (!bb) bb = sb;else bb++;
  if (istrcmp(ba,"SAV") == 0) {
    ba = concat2("0000",ba-3);
  }
  if (istrcmp(bb,"SAV") == 0) {
    bb = concat2("0000",bb-3);
  }
  return -strcmp(ba,bb);
}

/*
static int dedup_strings_prefix(TSTR_LIST lst, int count) {
    int j = -1;
    for (int i = 0; i < count; ++i) {
        if (j < 0 || !is_same_prefix(lst[j], lst[i])) {
            ++j;
        }
        if (j != i) lst[j] = lst[i];
    }
    ++j;
    int r = j;
    while (j != count) {
        lst[j] = NULL;
        ++j;
    }
    return r;
}



static void load_specific_file(int slot_num,char *filename,void **out,int32_t *size) //call it in task!
  {
  FILE *slot;

  if (current_game_slot_list.count <= (size_t)slot_num) {
      *out = NULL;
      return;
  }

  slot=fopen_icase(build_pathname(2, gpathtable[SR_SAVES], current_game_slot_list.files[slot_num]),"rb");
  if (slot==NULL)
     {
     *out=NULL;
     return;
     }
  fseek(slot,SAVE_NAME_SIZE,SEEK_CUR);

  LOAD_SPECIFIC_FILE_CALLBACK_DATA ctx;
  ctx.name = filename;

  *out = NULL;
  *size = 0;
  if (enum_all_status(slot, &load_specific_file_callback, &ctx) == 0){
      *out = ctx.data;
      *size = ctx.size;
  }
  fclose(slot);
  }
*/
static void load_specific_file(const char *filename,const char *name, void **out,int32_t *size) //call it in task!
  {
  FILE *slot;


  slot=fopen_icase(build_pathname(2, gpathtable[SR_SAVES], filename),"rb");
  if (slot==NULL)
     {
     *out=NULL;
     return;
     }
  fseek(slot,SAVE_NAME_SIZE,SEEK_CUR);

  LOAD_SPECIFIC_FILE_CALLBACK_DATA ctx;
  ctx.name = name;

  *out = NULL;
  *size = 0;
  if (enum_all_status(slot, &load_specific_file_callback, &ctx) == 0){
      *out = ctx.data;
      *size = ctx.size;
  }
  fclose(slot);
  }


static const char * get_savegame_name(TSAVEGAME_LIST *lst, unsigned int index) {
  if (!lst->names) {
    lst->names = create_list(lst->count);
  }
  if (!lst->names[index]) {
        FILE *f = NULL;
        if (lst->files[index] != NULL) {
            f=fopen_icase(build_pathname(2, gpathtable[SR_SAVES], lst->files[index]), "rb");
        }
        if (f!=NULL) {
            char slotname[SAVE_NAME_SIZE+1];
            fread(slotname,1,SAVE_NAME_SIZE,f);
            slotname[SAVE_NAME_SIZE] = 0;
            fclose(f);
            str_replace(&lst->names, index, slotname);
        } else {
            str_replace(&lst->names, index, texty[75]);
        }
    }
    return lst->names[index];
}

static TSAVEGAME_LIST get_all_savegames(unsigned long kampan, char skip_autosave) {
    //sav.creation_time.game_save_time
    char prefix[50];
    snprintf(prefix,50,"sav.%08lx.",kampan);
    TSAVEGAME_CB_STATE st;
    st.files = create_list(32);
    st.prefix = kampan?prefix:NULL;
    st.prefix_len = kampan?strlen(prefix):0;
    st.count = 0;
    st.skip_autosave = skip_autosave;
    list_files(gpathtable[SR_SAVES], file_type_just_name|file_type_need_timestamp|file_type_normal, get_all_savegames_callback, &st);
//    qsort(st.files, st.count, sizeof(char *), compare_strings);
//    if (kampan == 0) {
//        st.count =dedup_strings_prefix(st.files, (int)st.count);
//    }
    qsort(st.files, st.count, sizeof(char *), compare_strings_third_back);

    TSTR_LIST names = NULL;
    char *autosave_list = NewArr(char, st.count);
    for (size_t i = 0; i < st.count; ++i) {
        size_t fns = strlen(st.files[i]);
        autosave_list[i] = st.files[i][fns+1];
/*        FILE *f=fopen_icase(build_pathname(2, gpathtable[SR_SAVES], st.files[i]), "rb");
        if (f!=NULL) {
            char slotname[SAVE_NAME_SIZE+1];
            fread(slotname,1,SAVE_NAME_SIZE,f);
            slotname[SAVE_NAME_SIZE] = 0;
            fclose(f);
            str_replace(&names, i, slotname);
        } else {
            str_replace(&names, i, texty[75]);
        }*/
    }


    TSAVEGAME_LIST out;
    out.files = st.files;
    out.names = names;
    out.count = st.count;
    out.autosave_flags = autosave_list;
    return out;
}

static void free_savegame_list(TSAVEGAME_LIST *lst) {
    release_list(lst->files);
    release_list(lst->names);
    lst->files = 0;
    lst->names = 0;
    if (lst->autosave_flags) {
      free(lst->autosave_flags);
      lst->autosave_flags = NULL;
    }
}

static void place_name(int c,int i,char show, char sel)
  {
  int z,x;
  int p = i + current_slot_list_top_line;
  if ((size_t)p >= current_game_slot_list.count) return;
  if (c) x=SAVE_SLOT_S;else x=LOAD_SLOT_S;
  if (show) schovej_mysku();
  const char *name = get_savegame_name(&current_game_slot_list,p);
  set_font(SLOT_FONT,sel?SELECT_COLOR:NORMAL_COLOR);
  int w = text_width(name);
  int spc = 0;
  if (w > 210) {
      int len = strlen(name);
      int extra = (w-210)/len;
      spc = -extra-1;
  }
  position(x,z=i*SLOT_SPACE+21+SCREEN_OFFLINE);
  outtext_ex(name,spc);
  if (current_game_slot_list.autosave_flags[p]) {
      set_font(SLOT_AUTOSAVE_FONT,sel?SELECT_COLOR:NORMAL_COLOR);
      set_aligned_position(x+204+strlen(SLOT_AUTOSAVE_TEXT), z, 2, 1, SLOT_AUTOSAVE_TEXT);
      outtext_ex(SLOT_AUTOSAVE_TEXT,-1);
  }
  if (show)
     {
     ukaz_mysku();
     showview(x,z-2,204,20);
     }
  }

static void redraw_save()
  {
  int i;
  schovej_mysku();
  put_picture(0,SCREEN_OFFLINE,ablock(H_SAVELOAD));
  put_picture(274,SCREEN_OFFLINE,ablock(H_SVITEK));
  for(i=0;i<SLOTS_MAX;i++) {
      place_name(1,i,0,0);
  }
  ukaz_mysku();
  }

static void redraw_load()
  {
  int i;
  schovej_mysku();
  put_picture(0,SCREEN_OFFLINE,ablock(H_SVITEK));
  put_picture(372,SCREEN_OFFLINE,ablock(H_SAVELOAD));
  for(i=0;i<SLOTS_MAX;i++) place_name(0,i,0,0);
  ukaz_mysku();
  }

static void redraw_story_bar(int pos)
  {
  int i,y,ys,x,count;
  schovej_mysku();
  if (force_save) x=STORY_X+274;else x=STORY_X;
  if (back_texture==NULL)
     {
     back_texture=getmem(STORY_XS*STORY_YS*2+6);
     get_picture(x,STORY_Y+SCREEN_OFFLINE,STORY_XS,STORY_YS,back_texture);
     }
  else
     put_picture(x,STORY_Y+SCREEN_OFFLINE,back_texture);
  if (story_text!=NULL)
     {
     y=SCREEN_OFFLINE+STORY_Y;
     ys=STORY_YS;
     count=str_count(story_text);
       set_font(H_FONT6,NOSHADOW(0));
     for(i=pos;i<count;i++) if (story_text[i]!=NULL)
       {
       int h;

       h=text_height(story_text[i]);
       if (ys<2*h) break;
       position(x,y);outtext(story_text[i]);
       ys-=h;
       y+=h;
       }
     }
  ukaz_mysku();
  showview(x,STORY_Y+SCREEN_OFFLINE,STORY_XS,STORY_YS);
  }
/*
static void read_story_task(va_list args)
  {
  int slot=va_arg(args,int);

  TSTR_LIST ls;
  void *text_data;
  char *c,*d;
  int32_t size;

  load_specific_file(slot,STORY_BOOK,&text_data,&size);
  if (text_data!=NULL)
     {
     ls=create_list(2);
     c=text_data;
     set_font(H_FONT6,0);
     while (size>0)
       {
       int xs,ys;
       d=c;
       while (size>0 && *d!='\r' && *d!='\n') {d++;size--;};
       if (!size) break;
       *d=0;
       {
       char *e,*or;
       or=e=getmem(strlen(c)+2);
       zalamovani(c,e,STORY_XS,&xs,&ys);
       while (*e)
          {
          str_add(&ls,e);
          if (text_width(e)>STORY_XS) abort();
          e=strchr(e,0)+1;
          }
        c=d+1;size--;
       if (size > 0 &&(*c=='\n' || *c=='\r')) {c++;size--;};
       free(or);
       }
       }
     free(text_data);
     }
  else ls=NULL;
  if (story_text!=NULL) release_list(story_text);
  story_text=ls;
  cur_story_pos=get_list_count();if (cur_story_pos<0) cur_story_pos=0;
  redraw_story_bar(cur_story_pos);
  }

static void read_story(int slot)
  {
  static int task_num=-1;

  if (task_num!=-1) term_task(task_num);
  if (slot!=-1)
     task_num=add_task(8196,read_story_task,slot);
  }
*/

static void read_story(const char *filename) {
    void *text_data;
    int32_t size = 0;
    TSTR_LIST ls;
    char *c,*d;

    if (filename) {
        load_specific_file(filename,STORY_BOOK,&text_data,&size);
    } else {
        int32_t sz = temp_storage_find(STORY_BOOK);
        if (sz < 0) {
            text_data = NULL;
        } else {
            text_data = getmem(sz);
            temp_storage_retrieve(STORY_BOOK, text_data, sz);
            size = sz;
        }
    }
    if (text_data!=NULL)
       {
       ls=create_list(2);
       c=text_data;
       set_font(H_FONT6,0);
       while (size>0)
         {
         int xs,ys;
         d=c;
         while (size>0 && *d!='\r' && *d!='\n') {d++;size--;};
         if (!size) break;
         *d=0;
         {
         char *e,*or;
         or=e=getmem(strlen(c)+2);
         zalamovani(c,e,STORY_XS,&xs,&ys);
         while (*e)
            {
            str_add(&ls,e);
            if (text_width(e)>STORY_XS) abort();
            e=strchr(e,0)+1;
            }
          c=d+1;size--;
         if (size > 0 &&(*c=='\n' || *c=='\r')) {c++;size--;};
         free(or);
         }
         }
       free(text_data);
       }
    else ls=NULL;
    if (story_text!=NULL) release_list(story_text);
    story_text=ls;
    cur_story_pos=get_list_count();if (cur_story_pos<0) cur_story_pos=0;
    redraw_story_bar(cur_story_pos);

}

static int get_list_count()
  {
  int count,i,max=0;

  if (story_text==NULL) return 0;
  count=str_count(story_text);
  for(i=0;i<count;i++) if (story_text[i]!=NULL) max=i;
  return max-20;
  }

  static void select_slot(int i);

static int bright_slot(int yr)
  {
  int id;

  id=yr/SLOT_SPACE+current_slot_list_top_line;
  select_slot(id);
  return id;
  }

char updown_scroll(int id,int xa,int ya,int xr,int yr);

static char updown_noinst=0;

static void updown_scroll_hold(EVENT_MSG *msg,void **_)
  {
  if (msg->msg == E_MOUSE)
    {
    MS_EVENT *ms;

    ms=get_mouse(msg);
    if (ms->event_type==0x4 || !ms->tl1 || ms->tl2 || ms->tl3)
      {
      send_message(E_DONE,E_MOUSE,updown_scroll_hold);
      send_message(E_DONE,E_TIMER,updown_scroll_hold);
      updown_noinst=0;
      }
    }
  if (msg->msg == E_TIMER)
    {
    MS_EVENT *ms;

    updown_noinst=1;
    ms=&ms_last_event;
    ms->event_type=0x2;
    send_message(E_MOUSE,ms);
    if (updown_noinst)
      {
      send_message(E_DONE,E_MOUSE,updown_scroll_hold);
      send_message(E_DONE,E_TIMER,updown_scroll_hold);
      updown_noinst=0;
      }
    else
      updown_noinst=1;
    }
  }

char updown_scroll(int id,int xa,int ya,int xr,int yr)
  {
  int count;
  xr,yr,xa,ya;
  if (story_text==NULL) return 0;
  cur_story_pos+=id;
  count=get_list_count();
  if (cur_story_pos>count) cur_story_pos=count;
  if (cur_story_pos<0) cur_story_pos=0;
  redraw_story_bar(cur_story_pos);
  if (updown_noinst)
    {
    updown_noinst=0;
    return 1;
    }
  send_message(E_ADD,E_MOUSE,updown_scroll_hold);
  send_message(E_ADD,E_TIMER,updown_scroll_hold);
  return 1;
  }

static char close_saveload(int id,int xa,int ya,int xr,int yr)
  {
  xa;ya;xr;yr;id;
  if (ms_last_event.event_type & 0x8)
     {
     unwire_proc();
     wire_proc();
     }
  return 1;
  }

char clk_load_konec(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;xr;yr;
  send_message(E_CLOSE_MAP,NULL);
  return 1;
  }

static char clk_load_proc(int id,int xa,int ya,int xr,int yr);

#define CLK_LOAD_ERROR 5
T_CLK_MAP clk_load_error[]=
  {
  {-1,59,14+SCREEN_OFFLINE,306,46+SCREEN_OFFLINE,updown_scroll,2,H_MS_ZARE},
  {1,59,310+SCREEN_OFFLINE,306,332+SCREEN_OFFLINE,updown_scroll,2,H_MS_ZARE},
  {-1,LOAD_SLOT_S,SCREEN_OFFLINE,LOAD_SLOT_E,350,clk_load_proc,3,H_MS_DEFAULT},
  {-1,30,0,85,14,clk_load_konec,2,H_MS_DEFAULT},
  {-1,0,0,639,479,empty_clk,0xff,H_MS_DEFAULT},
  };


//static const char *find_autosave(const char *name);

static char clk_load_proc_menu(int id,int xa,int ya,int xr,int yr)
  {
  id=bright_slot(yr-18);
  xa;ya;xr;yr;
  if (ms_last_event.event_type & 0x2 && id>=0 && (size_t)id < current_game_slot_list.count) {
     send_message(E_CLOSE_MAP,current_game_slot_list.files[id]);
  }
  return 1;
  }

#define CLK_LOAD_MENU 5
T_CLK_MAP clk_load_menu[]=
  {
  {-1,59,14+SCREEN_OFFLINE,306,46+SCREEN_OFFLINE,updown_scroll,2,H_MS_ZARE},
  {1,59,310+SCREEN_OFFLINE,306,332+SCREEN_OFFLINE,updown_scroll,2,H_MS_ZARE},
  {-1,LOAD_SLOT_S,SCREEN_OFFLINE,LOAD_SLOT_E,350,clk_load_proc_menu,3,H_MS_DEFAULT},
  {-1,0,0,639,479,clk_load_konec,8,H_MS_DEFAULT},
  {-1,0,0,639,479,empty_clk,0xff,H_MS_DEFAULT},
  };

static void load_save_pos_ingame(int id) {
  if (load_game(current_game_slot_list.files[id]))
  {
  message(1,0,0,"",texty[79],texty[80]);
  redraw_load();
  showview(0,0,0,0);
  change_click_map(clk_load_error,CLK_LOAD_ERROR);
  }
else
  {
  unwire_proc();
  wire_proc();
  if (battle) konec_kola();
  unwire_proc();
  if (!load_another)
{
wire_main_functs();
cur_mode=MD_GAME;
bott_draw(1);
pick_set_cursor();
for(id=0;id<mapsize;id++) map_coord[id].flags&=~MC_DPLAYER;
  build_player_map();
}
reg_grafiku_postav();
build_all_players();
  }

}

static char clk_load_proc(int id,int xa,int ya,int xr,int yr)
  {
  id=bright_slot(yr-18);
  xa;ya;xr;yr;
  if (ms_last_event.event_type & 0x2 && id>=0 && (size_t)id < current_game_slot_list.count)
     {
      load_save_pos_ingame(id);
     }
  return 1;
  }

static char global_gamename[SAVE_NAME_SIZE];
static int slot_pos;

void save_step_next(EVENT_MSG *msg,void **unused)
  {
  int c;

  unused;
  if (msg->msg==E_KEYBOARD)
     {
     c=va_arg(msg->data, int);
     if (c==13)
        {
        send_message(E_KEYBOARD,c);
        save_game(slot_pos,global_gamename,0);
        wire_proc();
//        read_slot_list();
        msg->msg=-2;
        }
     else if(c==27)
        {
        send_message(E_KEYBOARD,c);
        msg->msg=-2;
        wire_save_load(1);
        }
     }
  }
static char clk_askname_stop(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  if (ms_last_event.event_type & 0x2)
     {
     send_message(E_KEYBOARD,13);
     return 1;
     }
  else
     {
     send_message(E_KEYBOARD,27);
     return 1;
     }
  }


static void save_it(char ok)
  {
  if (ok)
     {
     save_game(slot_pos,global_gamename,0);
//     read_slot_list();
     wire_proc();
	 GlobEvent(MAGLOB_AFTERSAVE,viewsector,viewdir);
     }
  else
     {
     wire_save_load(force_save);
     }
  }

#define CLK_ASK_NAME 2
T_CLK_MAP clk_ask_name[]=
  {
  {-1,0,0,639,479,clk_askname_stop,8+2,H_MS_DEFAULT},
  {-1,0,0,639,479,empty_clk,0xff,H_MS_DEFAULT},
  };



void wire_ask_gamename(int id)
  {
  int x,y;

  x=SAVE_SLOT_S;
  y=id*SLOT_SPACE+21+SCREEN_OFFLINE;
  slot_pos=id;
  schovej_mysku();
  put_picture(x,y,ablock(H_LOADTXTR));
  //strcpy(global_gamename,slot_list[id]);
  clk_ask_name[0].id=add_task(16384,type_text_v2,global_gamename,x,y,SAVE_SLOT_E-SAVE_SLOT_S,SAVE_NAME_SIZE,H_FBOLD,RGB555(31,31,0),save_it);
  change_click_map(clk_ask_name,CLK_ASK_NAME);
  ukaz_mysku();
  }

static void save_as_dialog(int pos);



#define CLK_SAVELOAD 11
T_CLK_MAP clk_load[]=
  {
  {-1,LOAD_SLOT_S,SCREEN_OFFLINE,LOAD_SLOT_E,350,clk_load_proc,3,H_MS_DEFAULT},
  {-1,59,14+SCREEN_OFFLINE,306,46+SCREEN_OFFLINE,updown_scroll,2,H_MS_ZARE},
  {1,59,310+SCREEN_OFFLINE,306,332+SCREEN_OFFLINE,updown_scroll,2,H_MS_ZARE},
  {-1,54,378,497,479,start_invetory,2+8,-1},
  {-1,337,0,357,14,go_map,2,H_MS_DEFAULT},

  {-1,291,0,313,14,go_book,2,H_MS_DEFAULT},
  {-1,87,0,142,14,game_setup,2,H_MS_DEFAULT},
  {-1,30,0,85,14,konec,2,H_MS_DEFAULT},
  {1,147,0,205,14,clk_saveload,2,H_MS_DEFAULT},
  {-1,267,0,289,15,clk_sleep,2,H_MS_DEFAULT},
  {-1,0,0,639,479,close_saveload,9,H_MS_DEFAULT},
  };


static char clk_save_proc(int id,int xa,int ya,int xr,int yr)
  {
  id=bright_slot(yr-18);
  xa;ya;xr;yr;
  if (ms_last_event.event_type & 0x2 && id>=0)
     {
      save_as_dialog(id);
     }
  return 1;
  }


T_CLK_MAP clk_save[]=
  {
  {-1,SAVE_SLOT_S,SCREEN_OFFLINE,SAVE_SLOT_E,350,clk_save_proc,3,H_MS_DEFAULT},
  {-1,59+274,14+SCREEN_OFFLINE,306+274,46+SCREEN_OFFLINE,updown_scroll,2,H_MS_ZARE},
  {1,59+274,310+SCREEN_OFFLINE,306+274,332+SCREEN_OFFLINE,updown_scroll,2,H_MS_ZARE},
  {-1,54,378,497,479,start_invetory,2+8,-1},
  {-1,337,0,357,14,go_map,2,H_MS_DEFAULT},
  {-1,291,0,313,14,go_book,2,H_MS_DEFAULT},
  {-1,87,0,142,14,game_setup,2,H_MS_DEFAULT},
  {-1,30,0,85,14,konec,2,H_MS_DEFAULT},
  {0,207,0,265,14,clk_saveload,2,H_MS_DEFAULT},
  {-1,267,0,289,15,clk_sleep,2,H_MS_DEFAULT},
  {-1,0,0,639,479,close_saveload,9,H_MS_DEFAULT},
  };

static void select_slot(int i) {
    int rel = i-current_slot_list_top_line;
    while (rel < 0) {
      --current_slot_list_top_line;
      schovej_mysku();
      if (force_save) redraw_save(); else redraw_load();
      showview(0,0,0,0);
      ukaz_mysku();
      rel++;
    }
    while (rel > SLOTS_MAX-1) {
      ++current_slot_list_top_line;
      schovej_mysku();
      if (force_save) redraw_save(); else redraw_load();
      showview(0,0,0,0);
      ukaz_mysku();
      rel--;
    }
    int m = force_save != 0?1:0;
    if (last_select != i) {
        if (last_select != -1) place_name(m,last_select-current_slot_list_top_line,1,0);
        place_name(m,i-current_slot_list_top_line,1,1);
        last_select = i;
        if (last_select != -1 && last_select < (int)current_game_slot_list.count) {
            read_story(current_game_slot_list.files[last_select]);
        }
    }
  }

static void saveload_keyboard(EVENT_MSG *msg,void **_)
  {
  if (msg->msg == E_KEYBOARD)
     {
      int v = quit_request_as_escape(va_arg(msg->data, int));
     switch (v>>8)
        {
        case 1:unwire_proc();wire_proc();break;
        case 17:
        case 'H':if (last_select>0) select_slot(last_select-1);break;
        case 31:
        case 'P':if (last_select<(int)current_game_slot_list.count-1) select_slot(last_select+1);break;
        case 28:if (last_select>=0) {
            if (force_save) {
                save_as_dialog(last_select);
            } else {
                load_save_pos_ingame(last_select);break;
            }
        }break;
        case 60:if (force_save) {
                    unwire_proc();wire_proc();
                } else {
                    clk_saveload(1, 0, 0, 0, 0);
                }
                break;
        case 61:if (!force_save) {
                        unwire_proc();wire_proc();
                  } else {
                    clk_saveload(0, 0, 0, 0, 0);
                  }
                break;
        case 59:game_setup(0,0,0,0,0);break;
        case 64:go_book(0, 0, 0, 0, 0);break;

        }
     }
  }

static void saveload_keyboard_menu(EVENT_MSG *msg,void **_)
  {
  if (msg->msg == E_KEYBOARD)
     {
      int v = quit_request_as_escape(va_arg(msg->data, int));
     switch (v>>8)
        {
        case 1:send_message(E_CLOSE_MAP,NULL);break;
        case 17:
        case 'H':if (last_select>0) select_slot(last_select-1);break;
        case 31:
        case 'P':if (last_select<(int)current_game_slot_list.count-1) select_slot(last_select+1);break;
        case 28:if (last_select>= 0 && last_select < (int)current_game_slot_list.count) {
                send_message(E_CLOSE_MAP, current_game_slot_list.files[last_select]);
                break;
        }

     }
    }
  }

void unwire_save_load(void)
  {
  term_task_wait(clk_ask_name[0].id);
  if (back_texture!=NULL) free(back_texture);
  back_texture=NULL;
  if (story_text!=NULL)release_list(story_text);
  story_text=NULL;
  cancel_pass=0;
  send_message(E_DONE,E_KEYBOARD,saveload_keyboard);
  send_message(E_DONE,E_KEYBOARD,saveload_keyboard_menu);
  free_savegame_list(&current_game_slot_list);
  change_click_map(0,0);
  }



void wire_save_load(char save) {
    current_slot_list_top_line = 0;
    last_select = -1;
    schovej_mysku();
    mute_all_tracks(0);
    force_save=save & 1;
    current_game_slot_list = get_all_savegames(current_campaign, save & 1);
    curcolor = RGB555(0,0,0);
    bar32(0, 17, 639, 17 + 360);
    if (save == 1) {
        current_game_slot_list.count++;
        str_insline(&current_game_slot_list.files, 0, NULL);
        release_list(current_game_slot_list.names);
        current_game_slot_list.names = NULL;
        free(current_game_slot_list.autosave_flags);
        current_game_slot_list.autosave_flags = NewArr(char, current_game_slot_list.count);
        memset(current_game_slot_list.autosave_flags,0,current_game_slot_list.count);
        change_click_map(clk_save,CLK_SAVELOAD);
        redraw_save();
        send_message(E_ADD, E_KEYBOARD, saveload_keyboard);
        effect_show();

    } else {
        curcolor = 0;
        redraw_load();
        effect_show();
        if (save == 2)
            change_click_map(clk_load_error, CLK_LOAD_ERROR);
        else if (save == 4) {
            change_click_map(clk_load_menu, CLK_LOAD_MENU);
            send_message(E_ADD, E_KEYBOARD, saveload_keyboard_menu);
        }
        else {
            send_message(E_ADD, E_KEYBOARD, saveload_keyboard);
            change_click_map(clk_load, CLK_SAVELOAD);
        }
    }
    redraw_story_bar(cur_story_pos);
    cancel_pass = 1;
    if (last_select != -1) {
        int x = last_select * SLOT_SPACE + 1;
        last_select = -1;
        bright_slot(x);
    }
    ukaz_mysku();
    update_mysky();
    unwire_proc = unwire_save_load;
}


void open_story_file()
  {
  }


void write_story_text(char *text)
  {
    int l = strlen(text);
    TMPFILE_WR *story = temp_storage_append(STORY_BOOK);
    temp_storage_write( text, l, story);
    temp_storage_write("\n", 1, story);
    temp_storage_close_wr(story);
  }

void close_story_file()
  {
  }

#if 0
static int load_map_state_partial(char *level_fname,int mapsize) //obnovuje stav mapy; castecne
  {
  char *bf = NULL;
  TMPFILE_RD *fsta;
  int i;
  int32_t siz;
  short res=-2;
  unsigned char ver;


  fsta=temp_storage_open(level_fname);
  if (fsta==NULL) return -1;
  if (!temp_storage_read(&ver,sizeof(ver)*1,fsta)) goto err;
  if (ver>STATE_CUR_VER) goto err;
  if (!temp_storage_read(&i,sizeof(mapsize)*1,fsta)) goto err;
  if (mapsize!=i) goto err;
  SEND_LOG("(SAVELOAD) Partial restore for map: %s (%s)",level_fname,"START");
  temp_storage_read(&siz,1*sizeof(siz),fsta);
  bf=(char *)getmem(siz);
  if (!temp_storage_read(bf,siz*1,fsta)) goto err;
  for (i=0;i<mapsize;i++)
    if ((bf[i>>3]>>(i & 7)) & 1) map_coord[i].flags|=MC_AUTOMAP;
  if (!temp_storage_read(bf,siz*1,fsta)) goto err;
  for (i=0;i<mapsize;i++)
    if ((bf[i>>3]>>(i & 7)) & 1) map_coord[i].flags|=MC_DISCLOSED;
  load_map_description(fsta);
  while (temp_storage_read(&i,1*2,fsta) && i<=mapsize*4)
     if (temp_storage_read(map_sides+i,1*sizeof(TSTENA),fsta)!=sizeof(TSTENA)) goto err;
  while (temp_storage_read(&i,1*2,fsta) && i<=mapsize)
     if (temp_storage_read(map_sectors+i,1*sizeof(TSECTOR),fsta)!=sizeof(TSECTOR)) goto err;
  res=0;
  err:
  free(bf);
  temp_storage_close_rd(fsta);
  SEND_LOG("(SAVELOAD) Partial restore for map: %s (%s)",level_fname,"DONE");
  return res;
  }
#endif

int load_map_automap(char *mapfile)
  {
  int i;

  SEND_LOG("(SAVEGAME) CRITICAL SECTION - Swapping maps: %s <-> %s",level_fname,mapfile);
  kill_all_sounds();
  for(i=0;i<mapsize;i++) map_coord[i].flags&=~0x7f; //vynuluj flags_info
  free(map_sides);        //uvolni informace o stenach
  free(map_sectors);      //uvolni informace o sektorech
  free(map_coord);       //uvolni minfo informace
  load_org_map(mapfile,&map_sides,&map_sectors,&map_coord,&mapsize); //nahrej originalni mapu
  return load_map_state_ex(mapfile,mapsize,1); //nahrej ulozenou mapu
  }


#define DEFAULT_GAME_NAME(extra)    \
            char game_name[100];\
            long cur_time = get_game_tick_count()/1000;\
            long game_time = play_time + cur_time - load_game_time;\
            if (game_time < 60) snprintf(game_name, sizeof(game_name), "%s" extra, mglob.mapname);\
            else snprintf(game_name, sizeof(game_name), "%s %02lu:%02lu" extra, mglob.mapname, game_time/3600, (game_time/60)%60);

long get_save_game_slot_id() {
  time_t tm;
  time(&tm);
  return tm-1700000000;
}

void do_autosave() {
    DEFAULT_GAME_NAME("");
    char prefix[50];
    snprintf(prefix,50,"sav.%08lx.",current_campaign);
    char isdead = 0;
    for (int i = 0; i < POCET_POSTAV; ++i) {
      isdead = isdead || (postavy[i].used && postavy[i].lives == 0 && postavy[i].inmaphash == current_map_hash);
    }
    if (!isdead) {
      TSAVEGAME_CB_STATE st;
      st.files = create_list(32);
      st.prefix = prefix;
      st.prefix_len = strlen(prefix);
      st.count = 0;
      st.skip_autosave = 0;
      list_files(gpathtable[SR_SAVES], file_type_just_name|file_type_need_timestamp|file_type_normal, get_all_savegames_callback, &st);
      for (size_t i = 0; i < st.count; ++i) {
          const char *n = st.files[i];
          if (strstr(n, AUTOSAVE_SUFFIX)) {
              remove(build_pathname(2, gpathtable[SR_SAVES],n));
          }
      }
      release_list(st.files);
    }
    save_game(get_save_game_slot_id(), game_name,1);
}

void do_save_dialog() {
    /*
    DEFAULT_GAME_NAME("");
    if (ask_save_dialog(game_name, sizeof(game_name))) {
      prev_game_time_save = cur_time;
      save_game(get_save_game_slot_id(), game_name);
    }
    */


}
/*
static int find_autosave_callback(const char *name, LIST_FILE_TYPE  type , size_t tm, void *ctx) {
    static char *autosave_name = NULL;
    char **s = (char **)ctx;
    const char *sep1 = strrchr(*s, '.');
    const char *sep2 = strrchr(name, '.');
    if ((sep2 - name) != (sep1 - *s)) return 0;
    if (memcmp(*s, name, (sep2 - name)) == 0 && strstr(name, AUTOSAVE_SUFFIX) && strcmp(*s, name) < 0) {
        free(autosave_name);
        autosave_name = strdup(name);
        *s = autosave_name;
    }
    return 0;
}

static const char *find_autosave(const char *name) {
    list_files(gpathtable[SR_SAVES], file_type_just_name|file_type_need_timestamp|file_type_normal, find_autosave_callback, &name);
    return name;

}
*/
static void save_as_dialog(int pos) {
    DEFAULT_GAME_NAME("");
    const char *todel = current_game_slot_list.files[pos];
    const char *name = get_savegame_name(&current_game_slot_list,pos);
    if (todel != NULL)  {
        strcopy_n(game_name, name, sizeof(game_name));
        todel = build_pathname(2,gpathtable[SR_SAVES],todel);
        todel = local_strdup(todel);
    }
    unwire_proc();
    char r =  ask_save_dialog(game_name, sizeof(game_name), todel != NULL);
    if (r==1) {
        prev_game_time_save = cur_time;
        save_game(get_save_game_slot_id(), game_name,0);
        if (todel) {
            remove(todel);
        }
        wire_proc();
        return;
    } else if (r == 2 && todel) {
        remove(todel);
    }
    wire_save_load(1);
}
