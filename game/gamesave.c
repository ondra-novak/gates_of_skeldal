#include <skeldal_win.h>
#include <bios.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <mem.h>
#include <pcx.h>
#include <types.h>
#include <bgraph.h>
#include <event.h>
#include <strlite.h>
#include <devices.h>
#include <bmouse.h>
#include <memman.h>
#include <fcntl.h>
#include <zvuk.h>
#include <stdarg.h>
#include "globals.h"
#include "temp_storage.h"

#include <assert.h>
#include <sys/stat.h>
#define STATE_CUR_VER 1

#define _GAME_ST "_GAME.TMP"
#define _GLOBAL_ST "_GLOBEV.TMP"
#define _SLOT_SAV "slot%02d.SAV"
#define SLOTS_MAX 10

#define GM_MAPENABLE 0x1

#define SAVE_SLOT_S 34
#define LOAD_SLOT_S (372+34)
#define SAVE_SLOT_E (34+203)
#define LOAD_SLOT_E (372+34+203)

#define SSAVE_VERSION 0

static TMPFILE_WR *story=NULL;
static char load_another;
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
  char level_name[12];
  short picks;  //pocet_sebranych predmetu v mysi
  short items_added; //pocet_pridanych predmetu
  int sleep_long;
	int game_flags;
  }S_SAVE;

#define ZAKLAD_CRC 0xC005

static int get_list_count();

static word vypocet_crc(char *data,int32_t delka)
  {
  uint32_t l=0;
  do
     {
     l=(l<<8)|(delka>0?*data++:0);delka--;
     l=(l<<8)|(delka>0?*data++:0);delka--;
     l%=ZAKLAD_CRC;
     }
  while(delka>-1);
  return l & 0xffff;
  }
static int unable_open_temp(char *c)
  {
  char d[]="Unable to open the file : ",*e;

  concat(e,d,c);
  closemode();
  display_error(e);
  SEND_LOG("(SAVELOAD) Open temp error detected (%s)",c,0);
  exit(1);
  }

static void unable_write_temp(char *c)
  {
  char d[]="Unable to write to the temp file : ",*e;

  concat(e,d,c);
  closemode();
  display_error(e);
  SEND_LOG("(SAVELOAD) Open temp error detected (%s)",c,0);
  exit(1);
  }


int load_org_map(char *filename,TSTENA **sides,TSECTOR **sectors,TMAP_EDIT_INFO **coords,int *mapsize)
  {
  FILE *f;
  void *temp;
  int sect;
  int32_t size,r;
  char nmapend=1;
  char *c;

	c=find_map_path(filename);
	f=fopen(c,"rb");free(c);
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
  int i,j;
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
  int i,j;
  short *c;

  for(i=0;i<mapsize*4;i++)
     if (map_items[i]!=NULL)
        {
        for(j=1,c=map_items[i];*c;c++,j++);
        temp_storage_write(&i,2,f);
        temp_storage_write(&j,2,f);
        temp_storage_write(map_items[i],2*j,f);
        }
  i=-1;
  temp_storage_write(&i,2,f);
  }

void restore_items(TMPFILE_RD *f)
  {
  short i,j;

  for(i=0;i<mapsize*4;i++) if (map_items[i]!=NULL) free(map_items[i]);
  memset(map_items,0,mapsize*16);
  while(temp_storage_read(&i,1*2,f) && i!=-1)
     {
     temp_storage_read(&j,1*2,f);
     map_items[i]=(short *)getmem(j*2);
     temp_storage_read(map_items[i],2*j,f);
     }
  }

extern TSTR_LIST texty_v_mape;

void save_map_description(TMPFILE_WR *f)
  {
  int count,max;
  int i;

  if (texty_v_mape==NULL) max=0;else max=str_count(texty_v_mape);
  for(i=0,count=0;i<max;i++) if (texty_v_mape[i]!=NULL) count++;
  temp_storage_write(&count,1*sizeof(count),f);
  for(i=0;i<max;i++) if (texty_v_mape[i]!=NULL)
     {
     int len;
     len=strlen(texty_v_mape[i]+12)+12+1;
     temp_storage_write(&len,1*2,f);
     temp_storage_write(texty_v_mape[i],1*len,f);
     }
  }

void load_map_description(TMPFILE_RD *f)
  {
  int count;
  int i;
  word len;

  if (texty_v_mape!=NULL)release_list(texty_v_mape);
  temp_storage_read(&count,1*sizeof(count),f);
  if (!count)
     {
     texty_v_mape=NULL;
     return;
     }
  texty_v_mape=create_list(count);
  for(i=0;i<count;i++)
     {
      temp_storage_read(&len,1*2,f);
        {
        char *s;
        s=(char *)alloca(len);
        memset(s,1,len-1);
        s[len-1]=0;
        str_replace(&texty_v_mape,i,s);
        }
        temp_storage_read(texty_v_mape[i],1*len,f);
     }
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

  f=letici_veci;
  temp_storage_write(&f,1*sizeof(f),fsta);
  while (f!=NULL)
     {
     short *c;
     temp_storage_write(f,1*sizeof(*f),fsta);
     c=f->items;
     if (c!=NULL) do temp_storage_write(c,1*2,fsta); while (*c++);
     f=f->next;
     }
  }

int load_all_fly(TMPFILE_RD *fsta)
  {
  LETICI_VEC *f=NULL,*n,*p;
  p=letici_veci;
  while (p!=NULL) {stop_fly(letici_veci,0);p=p->next;}
  temp_storage_read(&f,1*sizeof(f),fsta);
  p=letici_veci;
  while (f!=NULL)
     {
     short items[100],*c;
     n=New(LETICI_VEC);
     c=items;memset(items,0,sizeof(items));
     if (temp_storage_read(n,1*sizeof(*n),fsta)!=sizeof(*n))
        {
        free(n);
        if (p!=NULL) p->next=NULL;
        return -2;
        }
     if (n->items!=NULL)
        {
        do
            temp_storage_read(c,1*2,fsta);
        while (*c++);
        n->items=NewArr(short,c-items);
        memcpy(n->items,items,(c-items)*sizeof(short));
        }
     if (p==NULL) p=letici_veci=n;else p->next=n;
     p=n;
     f=n->next;
     n->next=NULL;
     }
  return 0;
  }




int save_map_state() //uklada stav mapy pro savegame (neuklada aktualni pozici);
  {
  char sta[200];
  char *bf;
  TMPFILE_WR *fsta;
  int i;
  int32_t siz;
 TSTENA *org_sides;
 TSECTOR *org_sectors;
  short res=-1;
  unsigned char ver=0;

  restore_sound_names();
  strcpy(sta,level_fname);
  fsta=temp_storage_create(sta);if (fsta==NULL) unable_open_temp(sta);
  SEND_LOG("(SAVELOAD) Saving map state for current map",0,0);
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
         temp_storage_write(&i,1*2,fsta);
         temp_storage_write(map_sides+i,1*sizeof(TSTENA),fsta);
        }
  i=-1;
  temp_storage_write(&i,1*2,fsta);
  for(i=0;i<mapsize;i++)   //save changed sectors
     if (memcmp(map_sectors+i,org_sectors+i,sizeof(TSECTOR)))
        {
         temp_storage_write(&i,1*2,fsta);
         temp_storage_write(map_sectors+i,1*sizeof(TSECTOR),fsta);
        }
  i=-1;
  temp_storage_write(&i,1*2,fsta);
  for(i=0;i<MAX_MOBS;i++) if (mobs[i].vlajky & MOB_LIVE)
     {
      temp_storage_write(&i,1*2,fsta);
      temp_storage_write(mobs+i,1*sizeof(TMOB),fsta); //save_mobmap
     }
  i=-1;
  temp_storage_write(&i,1*2,fsta);
  i=mapsize*4;
  temp_storage_write(&i,1*sizeof(i),fsta); //save flag maps //<-------------------------
  temp_storage_write(flag_map,1*i,fsta);
  save_daction(fsta,0,d_action); //save dactions//<-------------------------
  temp_storage_write(&macro_block_size,1*sizeof(macro_block_size),fsta);
  if (macro_block_size)temp_storage_write(macro_block,1*macro_block_size,fsta);//save_macros
  if (save_codelocks(fsta)) goto err;
  save_items(fsta);
  save_vyklenky(fsta);
  save_all_fly(fsta);
  save_enemy_paths(fsta);
  res=0;
  err:
  SEND_LOG("(SAVELOAD) State of current map saved (err:%d)",res,0);
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

int load_map_state() //obnovuje stav mapy; nutno volat po zavolani load_map;
  {
  char sta[200];
  char *bf;
  TMPFILE_RD *fsta;
  int i;
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
    SEND_LOG("(SAVELOAD) Loading map state for current map",0,0);
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
    while (temp_storage_read(&i,1*2,fsta) && i<=MAX_MOBS)
       if (temp_storage_read(mobs+i,1*sizeof(TMOB),fsta)!=sizeof(TMOB)) goto err;
    }
  for(i=0;i<MAX_MOBS;i++) mobs[i].vlajky &=~MOB_IN_BATTLE;
  refresh_mob_map();
  temp_storage_read(&i,1*sizeof(i),fsta);
  temp_storage_read(flag_map,1*i,fsta);
  load_daction(fsta);
  temp_storage_read(&i,1*sizeof(macro_block_size),fsta);
  if (macro_block_size && i==macro_block_size) {
      temp_storage_read(macro_block,1*macro_block_size,fsta);
  }
  else
     {
     temp_storage_skip(fsta,i);
     SEND_LOG("(ERROR) Multiaction: Sizes mismatch %d != %d",i,macro_block_size);
     }
  if (load_codelocks(fsta)) goto err;
  restore_items(fsta);
  res=0;
  res|=load_vyklenky(fsta);
  res|=load_all_fly(fsta);
  res|=load_enemy_paths(fsta);
  err:
  SEND_LOG("(SAVELOAD) State of current map loaded (err:%d)",res,0);
  temp_storage_close_rd(fsta);
  free(bf);
  return res;
  }

void restore_current_map() //pouze obnovuje ulozeny stav aktualni mapy
  {
  int i;

  SEND_LOG("(SAVELOAD) Restore map...",0,0);
  kill_all_sounds();
  for(i=0;i<mapsize;i++) map_coord[i].flags&=~0x7f; //vynuluj flags_info
  free(map_sides);        //uvolni informace o stenach
  free(map_sectors);      //uvolni informace o sektorech
  free(map_coord);       //uvolni minfo informace
  load_org_map(level_fname,&map_sides,&map_sectors,&map_coord,NULL); //nahrej originalni mapu
  load_map_state(); //nahrej ulozenou mapu
  for(i=1;i<mapsize*4;i++) call_macro(i,MC_STARTLEV);
  }
/*
_inline unsigned char rotate(unsigned char c)
  {
	  return (c >> 1) | (c << 7);

  __asm
    {
    mov al,c
    rol al,1;
    }
  }
*/
//#pragma aux rotate parm [al] value [al]="rol al,1";

/* errors
     -1 end of file
      1 disk error
      2 internal error
      3 checksum error
 */
int pack_status_file(FILE *f,const char *status_name)
  {
  int stt;
  char rcheck=0;
  uint32_t fsz;
  char *buffer,*c,*fullnam;
  word crc;
  unsigned char name_len;

  SEND_LOG("(SAVELOAD) Packing status file '%s'",status_name,0);
  fsz = temp_storage_find(status_name);
  if (fsz < 0) return 2;
  name_len = (unsigned char)strlen(status_name);
  uint32_t extra = 1+name_len+4;
  c=buffer=getmem(fsz+extra);
  *c = name_len;
  ++c;
  memcpy(c,status_name, name_len);
  c += name_len;
  memcpy(c,&fsz,4);

  temp_storage_retrieve(status_name, c, fsz);

  fsz+=extra;
  rcheck=(fwrite(buffer,1,fsz,f)!=(unsigned)fsz);
  free(buffer);
  return 0;
  }

int unpack_status_file(FILE *f)
  {
  int stt;
  char rcheck=0;
  uint32_t fsz;
  char *buffer;
  unsigned char namelen = 0;
  char name[256];

  fread(&namelen, 1,1, f);
  if (namelen == 0) return -1;
  fread(name, 1, namelen, f);
  SEND_LOG("(SAVELOAD) Unpacking status file '%s'",name,0);
  fread(&fsz,1,4,f);
  buffer=(char *)getmem(fsz);
  if (fread(buffer,1,fsz,f)!=(unsigned)fsz) return 1;
  temp_storage_store(name, buffer, fsz);
  free(buffer);
  return rcheck;
  }


static void pack_status_file_cb(const char *name, void *ctx) {
    pack_status_file((FILE *)ctx, name);
}

int pack_all_status(FILE *f)
  {
    char c = 0;
    temp_storage_list(pack_status_file_cb, f);
    fwrite(&c,1,1,f);
    return 0;
  }

int unpack_all_status(FILE *f)
  {
  int i;

  i=0;
  while (!i) i=unpack_status_file(f);
  if (i==-1) i=0;
  return i;
  }

int save_basic_info()
  {
  TMPFILE_WR *f;
  char *c;
  S_SAVE s;
  short *p;
  int i;
  char res=0;
  THUMAN *h;

  SEND_LOG("(SAVELOAD) Saving basic info for game (file:%s)",_GAME_ST ,0);
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
  s.sample_vol=get_snd_effect(SND_GFX);
  s.music_vol=get_snd_effect(SND_MUSIC);
  s.xbass=get_snd_effect(SND_XBASS);
  s.bass=get_snd_effect(SND_BASS);
  s.treble=get_snd_effect(SND_TREBL);
  s.stereing=get_snd_effect(SND_LSWAP);
  s.swapchans=get_snd_effect(SND_SWAP);
  s.out_filter=get_snd_effect(SND_OUTFILTER);
  s.autosave=autosave_enabled;
	s.game_flags=(enable_glmap!=0);
  strncpy(s.level_name,level_fname,12);
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
  temp_storage_close_wr(f);
  SEND_LOG("(SAVELOAD) Done... Result: %d",res,0);
  return res;
  }

int load_basic_info()
  {
  TMPFILE_RD *f;
  char *c;
  S_SAVE s;
  int i;
  char res=0;
  TITEM *itg;
  THUMAN *h;

  SEND_LOG("(SAVELOAD) Loading basic info for game (file:%s)",_GAME_ST,0);
  f=temp_storage_open(_GAME_ST);
  if (f==NULL) return 1;
  res|=(temp_storage_read(&s,1*sizeof(s),f)!=sizeof(s));
	if (s.game_flags & GM_MAPENABLE) enable_glmap=1;else enable_glmap=0;
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
     res|=(temp_storage_read(glob_items+it_count_orgn,sizeof(TITEM)*s.items_added,f)!=(unsigned)s.items_added);
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
          }
        }
  res|=load_dialog_info(f);
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
  if (level_fname==NULL || strncmp(s.level_name,level_fname,12))
     {
     strncpy(loadlevel.name,s.level_name,12);
     loadlevel.start_pos=viewsector;
     loadlevel.dir=viewdir;
     send_message(E_CLOSE_MAP);
     load_another=1;
     }
  else load_another=0;
  for(i=0;i<POCET_POSTAV;i++) postavy[i].dostal=0;
  SEND_LOG("(SAVELOAD) Done... Result: %d",res,0);
  return res;
  }

static void MakeSaveGameDir(const char *name)
{
  char *p=(char *)alloca(strlen(name)+1);
  strcpy(p,name);
  p[strlen(p)-1]=0;
  mkdir(p,0666);
}

static int save_global_events()
{
  FILE *f;
  char *c;
  concat(c,pathtable[SR_TEMP],_GLOBAL_ST );
  f=fopen(c,"wb");
  if (f==NULL) return 1;
  fwrite(GlobEventList,1,sizeof(GlobEventList),f);
  fclose(f);
  return 0;
}

static int load_global_events()
{
  FILE *f;
  char *c;
  memset(GlobEventList,0,sizeof(GlobEventList));

  concat(c,pathtable[SR_TEMP],_GLOBAL_ST );
  f=fopen(c,"rb");
  if (f==NULL) return 1;
  fread(GlobEventList,1,sizeof(GlobEventList),f);
  fclose(f);
  return 0;
}

int save_game(int slotnum,char *gamename)
  {
  char *sn,*ssn,*gn;
  FILE *svf;
  int r;

  SEND_LOG("(SAVELOAD) Saving game slot %d",slotnum,0);
  save_map_state();
  concat(sn,pathtable[SR_SAVES],_SLOT_SAV);
  MakeSaveGameDir(pathtable[SR_SAVES]);
  ssn=alloca(strlen(sn)+3);
  sprintf(ssn,sn,slotnum);
  gn=alloca(SAVE_NAME_SIZE);
  strncpy(gn,gamename,SAVE_NAME_SIZE);
  if ((r=save_shops())!=0) return r;
  if ((r=save_basic_info())!=0) return r;
  save_leaving_places();
  save_book();
  save_global_events();
  svf=fopen(ssn,"wb");
  if (svf==NULL)
  {
	char buff[256];
	sprintf(buff,"Nelze ulozit pozici na cestu: %s", ssn);
    display_error(buff);
  }
  else
  {
	fwrite(gn,1,SAVE_NAME_SIZE,svf);
	close_story_file();
	r=pack_all_status(svf);
	open_story_file();
	fclose(svf);
  }
  SEND_LOG("(SAVELOAD) Game saved.... Result %d",r,0);
  disable_intro();
  return r;
  }

extern char running_battle;

int load_game(int slotnum)
  {
  char *sn,*ssn;
  FILE *svf;
  int r,t;

  SEND_LOG("(SAVELOAD) Loading game slot %d",slotnum,0);
  if (battle) konec_kola();
  battle=0;
  close_story_file();
  purge_temps(0);
  concat(sn,pathtable[SR_SAVES],_SLOT_SAV);
  ssn=alloca(strlen(sn)+3);
  sprintf(ssn,sn,slotnum);
  svf=fopen(ssn,"rb");
  if (svf==NULL) return 1;
  fseek(svf,SAVE_NAME_SIZE,SEEK_CUR);
  r=unpack_all_status(svf);
  load_leaving_places();
  fclose(svf);
  open_story_file();
  if (r>0)
     {
     SEND_LOG("(ERROR) Error detected during unpacking game... Loading stopped (result:%d)",r,0);
     return r;
     }
  load_book();
  load_global_events();
  if ((t=load_saved_shops())!=0) return t;
  if ((t=load_basic_info())!=0) return t;
  running_battle=0;
  norefresh=0;
  if (!load_another) restore_current_map();
        else
           {
           save_map=0;
           norefresh=1;
           }
  for(t=0;t<POCET_POSTAV;t++) postavy[t].zvolene_akce=NULL;
  SEND_LOG("(SAVELOAD) Game loaded.... Result %d",r,0);
//  if (GetKeyState(VK_CONTROL) & 0x80) correct_level();
  return r;
  }

static void load_specific_file(int slot_num,char *filename,void **out,int32_t *size) //call it in task!
  {
  FILE *slot;
  char *c,*d;
  int32_t siz;
  char fname[12];
  char succes=0;

  concat(c,pathtable[SR_SAVES],_SLOT_SAV);
  d=alloca(strlen(c)+2);
  sprintf(d,c,slot_num);
  slot=fopen(d,"rb");
  if (slot==NULL)
     {
     *out=NULL;
     return;
     }
  fseek(slot,SAVE_NAME_SIZE,SEEK_CUR);
  fread(fname,1,12,slot);
  while(fname[0] && !succes)
     {
     task_sleep(NULL);
     if (task_quitmsg()) break;
     fread(&siz,1,4,slot);
     if (!strncmp(fname,filename,12)) succes=1; else
           {
           fseek(slot,siz,SEEK_CUR);
           fread(fname,1,12,slot);
           }
     }
  if (succes)
     {
     *out=getmem(siz);
     fread(*out,1,siz,slot);
     *size=siz;
     }
  else *out=NULL;
  fclose(slot);
  }

//------------------------ SAVE LOAD DIALOG ----------------------------
static char force_save;
static TSTR_LIST slot_list=NULL;
static int last_select=-1;
static char used_pos[SLOTS_MAX];
static TSTR_LIST story_text=NULL;
static void *back_texture=NULL;
static int cur_story_pos=0;
static char load_mode;

#define SLOT_SPACE 33
#define SELECT_COLOR RGB555(31,31,31)
#define NORMAL_COLOR RGB555(10,31,10)
#define STORY_X 57
#define STORY_Y 50
#define STORY_XS (298-57)
#define STORY_YS (302-50)

void read_slot_list()
  {
  int i;
  char *mask,*name;
  char slotname[SAVE_NAME_SIZE];
  if (slot_list==NULL) slot_list=create_list(SLOTS_MAX);
  concat(mask,pathtable[SR_SAVES],_SLOT_SAV);
  name=alloca(strlen(mask)+1);
  for(i=0;i<SLOTS_MAX;i++)
     {
     FILE *f;
     sprintf(name,mask,i);
     f=fopen(name,"rb");
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

static void place_name(int c,int i,char show)
  {
  int z,x;
  if (c) x=SAVE_SLOT_S;else x=LOAD_SLOT_S;
  if (show) schovej_mysku();
  position(x,z=i*SLOT_SPACE+21+SCREEN_OFFLINE);outtext(slot_list[i]);
  if (show)
     {
     ukaz_mysku();
     showview(x,z,204,18);
     }
  }

static void redraw_save()
  {
  int i;
  schovej_mysku();
  put_picture(0,SCREEN_OFFLINE,ablock(H_SAVELOAD));
  put_picture(274,SCREEN_OFFLINE,ablock(H_SVITEK));
  set_font(H_FBOLD,NORMAL_COLOR);
  for(i=0;i<SLOTS_MAX;i++) place_name(1,i,0);
  ukaz_mysku();
  }

static void redraw_load()
  {
  int i;
  schovej_mysku();
  put_picture(0,SCREEN_OFFLINE,ablock(H_SVITEK));
  put_picture(372,SCREEN_OFFLINE,ablock(H_SAVELOAD));
  set_font(H_FBOLD,NORMAL_COLOR);
  for(i=0;i<SLOTS_MAX;i++) place_name(0,i,0);
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
       while (size && *d!='\r' && *d!='\n') {d++;size--;};
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
       if (*c=='\n' || *c=='\r') {c++;size--;};
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


static int get_list_count()
  {
  int count,i,max=0;

  if (story_text==NULL) return 0;
  count=str_count(story_text);
  for(i=0;i<count;i++) if (story_text[i]!=NULL) max=i;
  return max-20;
  }

static int bright_slot(int yr)
  {
  int id;

  id=yr/SLOT_SPACE;
  if ((yr % SLOT_SPACE)<18 && yr>0)
     {
     if (id!=last_select)
     {
     set_font(H_FBOLD,NORMAL_COLOR);
     if (last_select!=-1) place_name(force_save,last_select,1);
     set_font(H_FBOLD,SELECT_COLOR);
     place_name(force_save,id,1);
     last_select=id;
     read_story(id);
     }
     }
  else
     id=-1;
  return id;
  }

char updown_scroll(int id,int xa,int ya,int xr,int yr);

static char updown_noinst=0;

static EVENT_PROC(updown_scroll_hold)
  {
  user_ptr;
  WHEN_MSG(E_MOUSE)
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
  WHEN_MSG(E_TIMER)
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
  send_message(E_CLOSE_MAP,-1);
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


static char clk_load_proc_menu(int id,int xa,int ya,int xr,int yr)
  {
  id=bright_slot(yr-18);
  xa;ya;xr;yr;
  if (ms_last_event.event_type & 0x2 && id>=0 && used_pos[id])
     send_message(E_CLOSE_MAP,id);
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

static char clk_load_proc(int id,int xa,int ya,int xr,int yr)
  {
  id=bright_slot(yr-18);
  xa;ya;xr;yr;
  if (ms_last_event.event_type & 0x2 && id>=0 && used_pos[id])
     {
     if (load_game(id))
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
  	    cancel_render=1;
        }
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
        save_game(slot_pos,global_gamename);
        wire_proc();
        read_slot_list();
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
     save_game(slot_pos,global_gamename);
     read_slot_list();
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
  strcpy(global_gamename,slot_list[id]);
  clk_ask_name[0].id=add_task(16384,type_text_v2,global_gamename,x,y,SAVE_SLOT_E-SAVE_SLOT_S,SAVE_NAME_SIZE,H_FBOLD,RGB555(31,31,0),save_it);
  change_click_map(clk_ask_name,CLK_ASK_NAME);
  ukaz_mysku();
  }


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
     unwire_proc();
     wire_ask_gamename(id);
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

static EVENT_PROC(saveload_keyboard)
  {
  user_ptr;
  WHEN_MSG(E_KEYBOARD)
     {
     switch (GET_DATA(word)>>8)
        {
        case 1:unwire_proc();wire_proc();break;
        case 'H':if (last_select>0) bright_slot((last_select-1)*SLOT_SPACE+1);break;
        case 'P':if (last_select<SLOTS_MAX-1) bright_slot((last_select+1)*SLOT_SPACE+1);break;
        case 28:ms_last_event.event_type|=0x2;
                if (force_save) clk_save_proc(0,0,0,0,last_select*SLOT_SPACE+1+18);
                else if (load_mode==4)
                  clk_load_proc_menu(0,0,0,0,last_select*SLOT_SPACE+1+18);
                else
                  clk_load_proc(0,0,0,0,last_select*SLOT_SPACE+1+18);
                break;
        }
     }
  }

void unwire_save_load()
  {
  if (back_texture!=NULL) free(back_texture);
  back_texture=NULL;
  if (story_text!=NULL)release_list(story_text);
  story_text=NULL;
  cancel_pass=0;
  send_message(E_DONE,E_KEYBOARD,saveload_keyboard);
  }

void wire_save_load(char save)
  {
  schovej_mysku();
  mute_all_tracks(0);
  force_save=save & 1;
  load_mode=save;
  if (slot_list==NULL) read_slot_list();
  curcolor=0;
  bar(0,17,639,17+360);
  if (force_save) redraw_save();else redraw_load();
  if (save==4) effect_show(NULL);else showview(0,0,0,0);
  redraw_story_bar(cur_story_pos);
  unwire_proc=unwire_save_load;
  send_message(E_ADD,E_KEYBOARD,saveload_keyboard);
  if (save==1)change_click_map(clk_save,CLK_SAVELOAD);
  else if (save==2) change_click_map(clk_load_error,CLK_LOAD_ERROR);
  else if (save==4) change_click_map(clk_load_menu,CLK_LOAD_MENU);
  else change_click_map(clk_load,CLK_SAVELOAD);
  cancel_pass=1;
  if (last_select!=-1)
     {
     int x=last_select*SLOT_SPACE+1;
     last_select=-1;
     bright_slot(x);
     }
  ukaz_mysku();
  update_mysky();
  }


void open_story_file()
  {

  story=temp_storage_append(STORY_BOOK);
  SEND_LOG("(STORY) Story temp file is opened....",0,0);
  }


void write_story_text(char *text)
  {
    int l = strlen(text);
    temp_storage_write( text, l, story);
    temp_storage_write("\n", 1, story);
  }

void close_story_file()
  {
  if (story!=NULL)  temp_storage_close_wr(story);
  story=NULL;
  SEND_LOG("(STORY) Story temp file is closed...",0,0);
  }

static int load_map_state_partial(char *level_fname,int mapsize) //obnovuje stav mapy; castecne
  {
  char sta[200];
  char *bf;
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
     map_coord[i].flags|=(bf[i>>3]>>(i & 7)) & 1;
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
  return load_map_state_partial(mapfile,mapsize); //nahrej ulozenou mapu
  }
//po teto akci se nesmi spustit TM_SCENE!!!! pokud mapfile!=level_fname
