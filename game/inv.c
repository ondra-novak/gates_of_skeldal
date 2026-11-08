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
#include <libs/gui.h>
#include <libs/basicobj.h>
#include "engine1.h"
#include <libs/pcx.h>
#include "globals.h"

#include "lang.h"
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "ach_events.h"

#define neprezbrojit() (battle && (battle_mode!=MD_PREZBROJIT || select_player!=human_selected-postavy))

#define _SHOP_ST "_SHOPS.TMP"

#define SV_ITLIST 0x8001
#define SV_SNDLIST 0x8002
#define SV_END    0x8000
#define IT_LIBS   27
#define IT_LIB_NAME "IKONY%02d.LIB"
#define IT_LIB_SIZE 18
#define SHOP_NAME "SHOPS.DAT"


#define BUYBOX_X 0
#define BUYBOX_Y 197

#define IMAP_SIZE 8

#define SHP_ICSIZX 54
#define SHP_ICSIZY 60
#define SHP_ICPLCX 30
#define SHP_ICPLCY 29


THUMAN postavy_2[20];
int face_arr[IT_FACES];
static int sound_handle;
int ikon_libs;
int item_count,it_count_orgn;
LETICI_VEC *letici_veci=NULL;

short *picked_item=NULL;
short water_breath=-1;       //vec pro dychani pod vodou
short flute_item=-1;
int item_in_cursor=0;

void (*inv_redraw)();

TSHOP *cur_shop;
TSHOP **shop_list=NULL;
int max_shops=0; //shop_list=prima spojeni s obchody
void *shop_hacek=NULL; //hacek za ktery visi cely shop strom (free(shop_hacek) - odalokuje shopy)
                       //hacek lze ulozit do savegame -> ulozi se cely stav obchodu
static TSHOP_ALL_STATE shop_all_state;

#define ico_extract(icnnum) (((char*)ablock(ikon_libs+(icnnum)/IT_LIB_SIZE))+IT_ICONE_SIZE*((icnnum)%IT_LIB_SIZE))

#define TOP_OFS 17
#define HUMAN_X 35
#define HUMAN_Y 348
#define INV_X 285
#define INV_Y (TOP_OFS+29)
#define INV_DESC_X 325
#define INV_DESC_Y (TOP_OFS+50)
#define INV_XS 55
#define INV_YS 60
#define INV_NAME_X 129
#define INV_NAME_Y 349
#define INV_NAME_COL (RGB555(10,31,31)|FONT_TSHADOW)
#define INV_DESK 266
#define INV_INFO_X 298
#define INV_INFO_Y 343
#define INV_INFO_XS 300
#define INV_INFO_YS 20
#define INV_INFO_XP 12
#define INV_INFO_YC (INV_INFO_YS+INV_INFO_XP)
#define INV_BRIEF_COL1 (NOSHADOW(0))
#define INV_BRIEF_COL2 (NOSHADOW(0))
#define INV_LEVEL_COL1 (NOSHADOW(RGB555(0,0,15)))
#define INV_LEVEL_COL2 (RGB555(31,24,0))

unsigned short butt_plus[7]={0x0,(RGB555(25,23,16)),(RGB555(18,17,14)),(RGB555(10,10,5)),(RGB555(31,27,14))};

#define PO_XS 194
#define PO_YS 340
#define PO_XSS (PO_XS>>1)
#define PO_YSS (PO_YS>>1)

TITEM *glob_items=NULL;
char inv_view_mode=0;

void redraw_inventory();
void zkontroluj_postavu();

void place_human_item(word *obrazek,int x,int y,int item)
  {
  const word *p;

  p=ablock(item);
  put_picture2picture(p,obrazek,PO_XSS-p[0]/2+x,PO_YS-p[1]-y-20);
  }

/*
void init_item_sounds(int *ptr)
  {
  int i;
  char *c;

  for(i=0;i<item_count;i++)
     if (glob_items[i].sound)
        {
        c=(char *)ablock(H_SOUND_DAT)+sound_table[glob_items[i].sound-1];
        if (c==NULL || c[0]==0)
           {

           puts("Invalid Sound Table integrity - rebuild SOUND.DAT using MapEdit");
           exit(1);
           }
        def_handle(glob_items[i].sound_handle=ptr[0]++,c,wav_load,SR_ZVUKY);
        }
  }
*/
void item_sound_event(int item,int sector)
  {
  if (!item) return;
  item--;
  if (!glob_items[item].sound) return;
  play_sample_at_sector(glob_items[item].sound+sound_handle,viewsector,sector,0,0);
  }


void load_items()
  {
  const char *name;
  FILE *f;
  int sect,i,hs;
  int32_t size;
  void *p;

  f=NULL;i=0;
  ikon_libs=hl_ptr;
  free(glob_items);
    do {
        char name[200];
        sprintf(name, IT_LIB_NAME, i++);
        if (test_file_exist(SR_ITEMS, name)) {
            def_handle(hl_ptr++, name, NULL, SR_ITEMS);
        } else {
            break;
        }
    } while (1);
	name=build_pathname(2, gpathtable[SR_MAP],ITEM_FILE);
  f=fopen_icase(name,"rb");
  if (f==NULL)
     {

	    display_error("Selhalo otevreni souboru ITEMS.DAT. Zkotroluj zda vubec existuje.");
        _Exit(1);
     }
  do
     {
     load_section(f,&p,&sect,&size);
     switch (sect)
        {
        case 1:
        case 4:
           face_arr[sect-1]=hl_ptr-1;
           prepare_graphics(&hl_ptr,(char *)p,size,pcx_fade_decomp,SR_ITEMS);
           free(p);
           break;
        case 2:
        case 3:
           face_arr[sect-1]=hl_ptr-1;
           prepare_graphics(&hl_ptr,(char *)p,size,pcx_fade_decomp,SR_ITEMS);
           free(p);
           break;
        case 5:
           face_arr[sect-1]=hl_ptr-1;
           prepare_graphics(&hl_ptr,(char *)p,size,NULL,SR_ITEMS);
           free(p);
           break;
        case SV_ITLIST:
           glob_items=p;
           it_count_orgn=item_count=size/sizeof(TITEM);
           for (int i = 0; i < it_count_orgn; ++i) {
               if (glob_items[i].cena_high != 0) {
                   SEND_LOG("(ITEMS) Over-priced item %i %s %d > 65535", i, glob_items[i].jmeno, glob_items[i].cena + 65536*glob_items[i].cena_high);
               }
           }
           break;
        case SV_SNDLIST:
           hs=hl_ptr;
           prepare_graphics(&hl_ptr,(char *)p,size,wav_load,SR_ZVUKY);
           sound_handle=hs-1;
           free(p);
           break;
        default:
           free(p);
           break;
        }
     }
  while (sect!=SV_END);
  fclose(f);
     {
     TITEM *t;
     for(i=0,t=glob_items;i<it_count_orgn;i++,t++) if (t->druh==TYP_SPECIALNI)
        {
        if (t->user_value==TSP_WATER_BREATH)  water_breath=i;
        else if (t->user_value==TSP_FLUTE) flute_item=i;
        }
     }

     if (lang_get_folder()) {
         TSTRINGTABLE *str_table = lang_load("items.csv");
         if (str_table) {
             for (int i = 0; i < item_count; ++i) {
                 const char *trn = stringtable_find(str_table, i, NULL);
                 if (trn) {
                     char *trnw = local_strdup(trn);
                     char *sep = strchr(trnw, '\n');
                     if (sep != NULL) {
                         *sep = 0;
                         char *nx = sep+1;
                         --sep;
                         while (sep > trnw && isspace(*sep)) {
                             *sep = 0;
                             --sep;
                         }
                         sep = strchr(nx,0);
                         --sep;
                         while (sep > nx && isspace(*sep)) {
                             *sep = 0;
                             --sep;
                         }
                         strcopy_n(glob_items[i].popis, nx, sizeof(glob_items[i].popis)-1);
                     }
                     strcopy_n(glob_items[i].jmeno, trnw, sizeof(glob_items[i].jmeno)-1);
                 }
             }
             stringtable_free(str_table);
         }
     }
  }

void init_items()
  {
  map_items=(short **)getmem(mapsize*4*sizeof(short **));
  memset(map_items,0,mapsize*4*sizeof(short **));
  }

static short expand_itemlist(void)
  {
  int nwit=item_count;

  if (item_count>32766)
     {
     int i,j = 0;

     for(i=it_count_orgn;i<item_count;i++)
        {
        for(j=0;j<POCET_POSTAV;j++) if (postavy[j].used && q_item_one(j,i+1)!=NULL) break;
        if (j==POCET_POSTAV) break;
        }
     glob_items[i].flags|=ITF_FREE;
     nwit=j;
     }
  else
     {
     TITEM *it;
     item_count++;
     it=(TITEM *)getmem(sizeof(TITEM)*item_count);
     memcpy(it,glob_items,sizeof(TITEM)*nwit);
     free(glob_items);
     glob_items=it;
     }
  return nwit;
  }

/*static short add_dupl_item(short item)
  {
  TITEM *it;
  int nwit;


  if (!item) return item;
  if (glob_items[item-1].flags & ITF_DUPLIC) return item;
  item--;
  nwit=item_count;
  item_count++;
  if (item_count>32766)
     {
     nwit=32766;
     memmove(glob_items+it_count_orgn,glob_items+it_count_orgn,(item_count-it_count_orgn-1)*sizeof(TITEM));
     item_count=32766;
     memcpy(&it[nwit],&glob_items[item],sizeof(TITEM));
     }
  else
     {
     it=(TITEM *)getmem(sizeof(TITEM)*item_count);
     memcpy(it,glob_items,sizeof(TITEM)*nwit);
     memcpy(&it[nwit],&glob_items[item],sizeof(TITEM));
     free(glob_items);
     }
  it[nwit].flags|=ITF_DUPLIC;
  glob_items=it;
  return nwit+1;
  }*/

short duplic_item(short item)
  {
  int i;

  if (!item) return item;
  item--;
  for(i=0;i<item_count;i++) if (glob_items[i].flags & ITF_FREE) break;
  if (i==item_count) i=expand_itemlist();
  memcpy(glob_items+i,glob_items+item,sizeof(TITEM));
  glob_items[i].flags|=ITF_DUPLIC;
  glob_items[i].flags&=~ITF_FREE;
  return i+1;
  }

short create_unique_item(const TITEM *it)
  {
  int i;

  if (it==NULL) return 0;
  for(i=0;i<item_count;i++) if (glob_items[i].flags & ITF_FREE) break;
  if (i==item_count) i=expand_itemlist();
  memcpy(glob_items+i,it,sizeof(TITEM));
  glob_items[i].flags|=ITF_DUPLIC;
  glob_items[i].flags&=~ITF_FREE;
  return i+1;
  }



void destroy_items(short *items)
  {
  if (items==NULL) return;
  while (*items)
     {
     if (glob_items[*items-1].flags & ITF_DUPLIC)
        glob_items[*items-1].flags|=ITF_FREE;
     items++;
     }
  }

short create_item_money(int obnos)
  {
  int i,ls,max=99999,z;

  ls=-1;
  for(i=0;i<item_count;i++) if (glob_items[i].druh==TYP_PENIZE && (z=abs(glob_items[i].cena-obnos))<max) ls=i,max=z;
  if (ls==-1) return 0;
  i=duplic_item(ls+1)-1;
  glob_items[i].cena=obnos;
  return i+1;
  }

void load_item_map(void *p,int32_t s)
  {
  word itmc;
  int sect;
  short *c,*d;

  c=(short *)p;
  while (s>0)
     {
     sect=*(int *)c;
     c+=sizeof(sect)/sizeof(short);s-=sizeof(sect);
     d=c;itmc=1;
     while (*d++) itmc++;
     map_items[sect]=(short *)getmem(itmc*sizeof(short));
     d=c;
     itmc=0;
     do
        {
        map_items[sect][itmc++]=*d;
        s-=sizeof(short);
        }
     while (*d++);
     c=d;
     }
  }

char possx[]={0,1,1,0};
char possy[]={1,1,0,0};

void draw_placed_items_normal(int celx,int cely,int sect,int side)
  {
  int i,j,k;
  short *c;
  short nl=0,vzh;
  short cnt;


  sect<<=2;
  cnt=(cely==0)?2:4;
  for(i=0;i<cnt;i++)
     {
     j=(i+side)&3;
     if (map_items[sect+j]!=NULL) c=map_items[sect+j];else c=&nl;
     k=0;
     while (*c)
        {
        if (*c>0)
           {
           vzh=glob_items[(*c)-1].vzhled;
           if (vzh) draw_item(celx,cely,possx[i],possy[i],ablock(vzh+face_arr[0]),k);
           }
        c++;k++;
        }
     }
  }

int count_items_total(const short *place)
  {
  int c=0;

  if (place==NULL) return 0;
  while (*place++) c++;
  return c;
  }

int count_items_visible(const short *place)
  {
  int c=0;

  if (place==NULL) return 0;
  while (*place)
     {
     if (*place>0) c++;
     place++;
     }
  return c;
  }


int count_items_inside(const short *place)
  {
  int c=1;

  if (place==NULL) return 0;
  place++;
  while (*place++<0) c++;
  return c;
  }

int find_item(short *place,int mask)
  {
  int lastitem=-1;
  int i=0;

  if (place==NULL) return -1;
  while (*place)
     {
     if ((*place & 0xC000)==mask) lastitem=i;
     i++;place++;
     }
  return lastitem;
  }

void push_to_destroyed_items(const short *picked_items) {
  int new_count = count_items_total(picked_items);
  if (new_count == 0) return;
  int cur_destroyed = count_items_total(destroyed_items);
  short *nw = NewArr(short, new_count+cur_destroyed+1);
  for (int i = 0; i < new_count; ++i) {
    nw[i] = abs(picked_items[i]);
  }
  for (int i = 0; i < cur_destroyed; ++i) {
    nw[new_count+i] = destroyed_items[i];
  }
  nw[new_count+cur_destroyed] = 0;
  free(destroyed_items);
  destroyed_items = nw;
}

void push_item(int sect,int pos,const short *picked_item)
  {
  int bc;
  int pc;
  int tc;
  short *p;
  char s =  map_sectors[sect].sector_type;


  if (s==S_DIRA || ISTELEPORTSECT(sect) || s == S_SCHODY)
     sect=map_sectors[sect].sector_tag;
  if (sect==0 || s==S_VODA || s == S_LAVA || s == S_SSMRT || s == S_LODKA) {
      push_to_destroyed_items(picked_item);
      return;
  }
  sect=(sect<<2)+pos;
  bc=count_items_total(map_items[sect]);
  pc=count_items_total(picked_item);
  tc=bc+pc;
  if (tc==0) return;
  tc++;
  p=(short *)grealloc(map_items[sect],tc*sizeof(short));
  if (pc) memcpy(p+bc,picked_item,pc*sizeof(short));
  p[tc-1]=0;
  map_items[sect]=p;
  recheck_button(sect>>2,1);
  }

void pop_item(int sect,int pos,int mask,short **picked_item)
  {
  int picked;
  int bc,tc,pc;
  short *s,*t,*b;

  *picked_item=NULL;
  sect=(sect<<2)+pos;
  picked=find_item(map_items[sect],mask);
  if (picked==-1) return;
  s=map_items[sect]+picked;
  pc=count_items_inside(s);
  bc=count_items_total(map_items[sect]);
  tc=bc-pc;
  *picked_item=(short *)getmem((pc+1)*sizeof(short));
  memcpy(*picked_item,s,pc*sizeof(short));(*picked_item)[pc]=0;
  if (tc) b=NewArr(short,tc+1);else b=NULL;
  if (picked) memcpy(b,map_items[sect],picked*sizeof(short));
  t=b+picked;
  s=s+pc;
  while (*s) *t++=*s++;
  if (b!=NULL) b[tc]=0;
  free(map_items[sect]);
  map_items[sect]=b;
  recheck_button(sect>>2,1);
  }

void pick_set_cursor(void)
  {
  if (picked_item==NULL) {
     item_in_cursor=H_MS_DEFAULT;
  } else  {
     item_in_cursor=-(glob_items[*picked_item-1].ikona);
  }
  mouse_set_default(item_in_cursor);
  }




void do_items_specs(void)
        {
        int xa,ya;
        char destroy=0;
        TITEM *p;
        GlobEvent(MAGLOB_ONPICKITEM,viewsector,viewdir);
        if (picked_item==0) return;

        const short *s = picked_item;
        while (*s) {
            ach_event_pick_item(abs(*s)-1);
            ++s;
        }

        p=&glob_items[*picked_item-1];
        switch(p->druh)
           {
           case TYP_RUNA:xa=p->user_value/10;ya=1<<(p->user_value%10);
                        runes[xa]|=ya;
                        destroy=1;
                        play_fx_at(FX_MAGIC);
						bott_disp_rune(p->user_value,*picked_item-1);
                        break;
           case TYP_PENIZE:
                        {
                        //char c[150];
                        money+=p->cena;
                        destroy=1;
                        //sprintf(c,texty[cislovka(p->cena)+131],p->cena);
                        //bott_disp_text(c);
                        play_fx_at(FX_MONEY);
                        break;
                        }
           case TYP_PRACH:
                        destroy=1;
                        bott_disp_text(texty[134]);
                        break;
           case TYP_SVITXT:
                        destroy=1;
                        cur_page=count_pages();
                        cur_page&=~0x1;
                        cur_page++;
                        if (p->popis[0]==0) add_to_book(p->user_value);
                          else
                             {
                             const char *s = build_pathname(2, gpathtable[SR_MAP], p->popis);
                             add_text_to_book(s,p->user_value);
                             }
                        play_fx_at(FX_BOOK);
						if (game_extras & EX_AUTOOPENBOOK) autoopenaction=1;
                        break;
           }
        if (destroy)
           {
           destroy_items(picked_item);
           free(picked_item);
           picked_item=NULL;
           }
        }

static char check_pick(int sect,int id,int idd,int y)
  {
  short c;
  int min=480,d;
  short *place=map_items[(sect<<2)+idd];
  int vzl;

  if (place==NULL) return 0;
  while (*place)
     {
     c=*place;
     if (c>0)
        {
        vzl=glob_items[c-1].vzhled;
        if (vzl) d=get_item_top(0,id>1,possx[id],possy[id],ablock(vzl+face_arr[0]),count_items_visible(place)-1);
        else d=get_item_top(0,id>1,possx[id],possy[id],NULL,count_items_visible(place))-32;
        if (d<min) min=d;
        }
     place++;
     }
  return y>min;
  }

static int get_top_of_next(int sect,int id) //vraci souradnici predmetu na nasledujicim sektoru
  {
  int cnt,idd;

  sect=map_sectors[sect].step_next[viewdir];
  if (sect==0) return 0;
  id=3-id;
  idd=(id+viewdir) & 3;
  sect<<=2;
  cnt=count_items_visible(map_items[sect+idd])-1;
  if (cnt<0) cnt=0;
  return get_item_top(0,1,possx[id],possy[id],NULL,cnt);

  }

char pick_item_(int id,int xa,int ya,int xr,int yr)
  {
  int sect;
  int idd;

  xa,ya,xr,yr;
  if (id>1)
     {
     (sect=map_sectors[viewsector].step_next[viewdir]);
  if ((map_sides[(viewsector<<2)+viewdir].flags & SD_THING_IMPS) && !(map_sides[(viewsector<<2)+viewdir].oblouk & SD_ITPUSH)) return 0;
     }
  else sect=viewsector;
  idd=(id+viewdir)&0x3;
  if (picked_item!=NULL)
     {
      if (map_sectors[sect].sector_type==S_DIRA)
        {
        throw_fly(xa,ya,0);
        letici_veci->speed=0;
        letici_veci->sector=sect;
        letici_veci->xpos=-32;
        letici_veci->velocity=0;
        letici_veci->flags &= ~FLY_NEHMOTNA;
        }
     else
     if (id>1 || ya>=get_top_of_next(sect,id))
        {
		if ((game_extras & EX_BAG_EXTENDED) && (get_control_key_state()) &&
		  (glob_items[*picked_item-1].nosnost>0))
		  {
		  int curinside=count_items_inside(picked_item);
		  int nosnost=(glob_items[*picked_item-1].nosnost);
		  short *batoh=(short *)getmem(nosnost*2+20);
		  short *cur=batoh;
		  memcpy(cur,picked_item,(curinside+1)*2);
		  cur+=curinside;
		  free(picked_item);
		  picked_item=NULL;
		  nosnost-=curinside-1;
		  while (1)
			{
			pop_item(sect,idd,0,&picked_item);
			if (picked_item!=NULL) do_items_specs(); else break;
			if (picked_item!=NULL)
			  {
			  short *p=picked_item;
			  int cnt=count_items_total(picked_item);
			  if (cnt>nosnost)
				{
				push_item(sect,idd,picked_item);
				free(picked_item);
				picked_item=NULL;
				break;
				}
			  while (*p)
				{
				*cur++=-abs(*p++);
				nosnost--;
				}
			  free(picked_item);
			  picked_item=NULL;
			  }
			}
		  *cur=0;
		  picked_item=batoh;
		  pick_set_cursor();
		  return 1;
		  }
        push_item(sect,idd,picked_item);
        free(picked_item);
        picked_item=NULL;
        pick_set_cursor();
        return 1;
        }
     return 0;
     }
  else
     {
     if (id>1 || check_pick(sect,id,idd,ya))
        {
        pop_item(sect,idd,0,&picked_item);
        if (picked_item!=NULL) do_items_specs();
        pick_set_cursor();
        }
     return (picked_item!=NULL);
     }
  }

int celkova_vaha(short *p)
  {
  int suma=0;

  while (*p) suma+=glob_items[abs(*p++)-1].hmotnost;
  return suma;
  }

char put_item_to_inv(THUMAN *p,short *picked_items)
  {
  int i,pos=0;
     int it;

  if (p->inv_size>MAX_INV) p->inv_size=MAX_INV;
  if (picked_items==NULL) return 0;
  if (isdemon(p)) return 0;
  it=*picked_items;
  if (it && glob_items[it-1].umisteni==PL_SIP && !neprezbrojit())
        {
        int u;
        u=glob_items[it-1].user_value;if (!u) u=1;
        if (p->sipy+u<100)
           {
           p->sipy+=u;
           return 1;
           }
        }
  for(i=0;picked_items[i];i++);
  while (i)
     {
     for(;pos<p->inv_size && p->inv[pos];pos++);
     if (pos>=p->inv_size) break;
     i--;
     it=abs(picked_items[i]);
     p->inv[pos]=it;
     ach_event_inv_add(it-1);
     picked_items[i]=0;
     }
  return (i==0);
  }

//-----------------------------------Inventory viewer-------------------

THUMAN *human_selected=postavy;
short vls_min[24]=
  {0,0,0,0,1,1,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0};
short vls_max[]=
  {200,200,200,200,32767,32767,32767,190,199,190,199,99,99,99,99,99,90,90,90,90,99,4};

char bag_click(int id,int xa,int ya,int xr,int yr);
char human_click(int id,int xa,int ya,int xr,int yr);
char inv_swap_desk(int id,int xa,int ya,int xr,int yr);
char exit_inv(int id,int xa,int ya,int xr,int yr);
char ring_place(int id,int xa,int ya,int xr,int yr);
char uloz_sip(int id,int xa,int ya,int xr,int yr);

char info_box_drawed=0;
void *info_box_below=NULL;
void inv_keyboard(EVENT_MSG *msg,void **usr);

T_CLK_MAP clk_inv_view[]=
  {
  {-1,INV_X,INV_Y,INV_X+INV_XS*6,INV_Y+INV_YS*5,bag_click,2,-1},
  {0,236,185,255,204,ring_place,2,-1},
  {1,236,220,255,239,ring_place,2,-1},
  {2,236,255,255,274,ring_place,2,-1},
  {3,236,290,255,309,ring_place,2,-1},
  {0,0,200,29,309,uloz_sip,MS_EVENT_MOUSE_LPRESS,-1},
  {-1,37,34,225,336,human_click,2,-1},
  {-1,45,339,212,358,inv_swap_desk,2,H_MS_DEFAULT},
  {-1,54,378,497,479,start_invetory,2+8,-1},
  {-1,337,0,357,14,go_map,2,H_MS_DEFAULT},
  {-1,87,0,142,14,game_setup,2,H_MS_DEFAULT},
  {-1,30,0,85,14,konec,2,H_MS_DEFAULT},
  {-1,0,0,639,479,exit_inv,8,-1},
  };

#define CLK_INV_VIEW 13

void prepocitat_postavu(THUMAN *human_selected)
  {
  short *sr1,*p,*q,i,j,c;
  int v;

  if (human_selected->inv_size>MAX_INV) human_selected->inv_size=MAX_INV;
  sr1=human_selected->vlastnosti;
  p=human_selected->stare_vls;
  memcpy(sr1,p,sizeof(human_selected->stare_vls));
  for(i=0;i<HUMAN_PLACES+4;i++)
     {
     if (i<HUMAN_PLACES) c=human_selected->wearing[i];else c=human_selected->prsteny[i-HUMAN_PLACES];
     if (c)
        {
        p=sr1;
        q=glob_items[c-1].zmeny;
          for(j=0;j<VLS_MGSIL_L;j++)
          {
            if (glob_items[c-1].druh!=TYP_VRHACI || (j!=VLS_UTOK_L && j!=VLS_UTOK_H))
            {
              if (j>=VLS_OHEN && j<=VLS_MYSL)
                p[0]=(int)floor(((float)p[0]+(float)q[0])/(1.0f+(float)p[0]*(float)q[0]/(90.0*90.0))+0.5);
              else
                (*p)+=(*q);
            }
           p++,q++;
          }
        p=sr1+VLS_DAMAGE;                     //vypocet damage (bez omezeni)
        q=&glob_items[c-1].zmeny[VLS_DAMAGE];
	      *(p++)+=*q++;
        p=sr1+VLS_KOUZLA;                     //aplikace kouzel
        q=&glob_items[c-1].zmeny[VLS_KOUZLA];
        (*p++)|=*q++;
        }
     }
  p=sr1;
  for(j=0;j<VLS_MGSIL_L;j++)
     {
     if (*p<vls_min[j]) *p=vls_min[j];
     if (*p>vls_max[j]) *p=vls_max[j];
     p++;
     }
  if (human_selected->lives>human_selected->vlastnosti[VLS_MAXHIT]) human_selected->lives=human_selected->vlastnosti[VLS_MAXHIT];
  //if (human_selected->mana>human_selected->vlastnosti[VLS_MAXMANA]) human_selected->mana=human_selected->vlastnosti[VLS_MAXMANA];
  if (human_selected->kondice>human_selected->vlastnosti[VLS_KONDIC]) human_selected->kondice=human_selected->vlastnosti[VLS_KONDIC];
  v=MAX_HLAD(human_selected);human_selected->jidlo=MIN(human_selected->jidlo,v);
  v=MAX_ZIZEN(human_selected);human_selected->voda=MIN(human_selected->voda,v);
  }


char exit_inv(int id,int xa,int ya,int xr,int yr)
  {
  inv_view_mode=0;
  if (!battle || battle_mode!=MD_PREZBROJIT || picked_item==NULL) return_game(id,xa,ya,xr,yr);
  return 1;
  }


void definuj_postavy()
  {
  int i,num1,r,inv=0,z;
  char *c,*end;
  const char *tmpptr;
  char cc;
  int tmp;

  tmpptr=ablock(H_POSTAVY_DAT);
  size_t sz = get_handle(H_POSTAVY_DAT)->size;
  c = alloca(sz+1);
  memcpy(c, tmpptr, sz);
  c[sz] = 0;
  end=c + sz;
  for(i=0;c<end && i<20;i++)
     {
     THUMAN *p=&postavy_2[i];
     inv=0;
     memset(p,0,sizeof(*p));r=1;
     p->inv_size=6;
     while (c<end && *c!='\n' && *c!='\r' && r==1)
        {
        r=sscanf(c,"%d",&num1);c=strchr(c,'\n')+1;
        switch(num1)
           {
           case 64:
           case 65:
           case 66:
           case 67:
           case 68:r=sscanf(c,"%d",&z);
                    while(r==1 && z!=-1)
                       {
                       runes[num1-64]|=1<<(z-1);
                       c=strchr(c,'\n')+1;
                       r=sscanf(c,"%d",&z);
                       }
                    break;
           case 128:r=sscanf(c,"%c%14[^\r]",&cc,p->jmeno);r--;break;
           case 129:r=sscanf(c,"%d",&tmp);p->female = (char)tmp;break;
           case 130:r=sscanf(c,"%d",&tmp);p->xicht = (char)tmp;break;
           case 131:r=sscanf(c,"%d",&tmp);p->level = (short)tmp;break;
           case 132:r=sscanf(c,"%d",&tmp);p->exp = tmp;break;
           case 133:r=sscanf(c,"%d",&num1);
                    while(r==1 && num1!=-1)
                       {
                       p->inv[inv++]=num1+1;
                       c=strchr(c,'\n')+1;
                       r=sscanf(c,"%d",&num1);
                       }
                    break;
           case 134:r=sscanf(c,"%hd",&p->wearing[PO_BATOH]);
                    p->inv_size=6+glob_items[p->wearing[PO_BATOH]].nosnost;
                    p->wearing[PO_BATOH]++;
                    break;
           case 135:r=sscanf(c,"%d",&num1);if (r!=1) break;
                    c=strchr(c,'\n')+1;
                    r=sscanf(c,"%hd",&p->wearing[num1]);
                    p->wearing[num1]++;
                    break;
					 case 136:r=sscanf(c,"%d",&num1);if (r!=1) break;
										p->sipy=num1;
										break;
           default:r=sscanf(c,"%hd",&p->stare_vls[num1]);break;
           }
        if (c>=end) break;
        c=strchr(c,'\n')+1;
        }
     if (r!=1)
        {

	    display_error("Error in file POSTAVY.DAT. May be missing a parameter in some definition.");
        exit(0);
        }
     c=strchr(c,'\n')+1;
     prepocitat_postavu(p);
     p->lives=p->vlastnosti[VLS_MAXHIT];
     p->kondice=p->vlastnosti[VLS_KONDIC];
     p->mana=p->vlastnosti[VLS_MAXMANA];
     p->used=1;
     p->groupnum=1;
     p->jidlo=MAX_HLAD(p);
     p->voda=MAX_ZIZEN(p);
     p->bonus=0;
     p->mana_battery=32767;
     }
  }

static void inv_najist(short item)
  {
  TITEM *it;
  int mhlad;

  if (!item) return;
  it=glob_items+item-1;
  if (it->druh!=TYP_JIDLO) return;
  human_selected->jidlo+=it->user_value*HODINA;
  mhlad=MAX_HLAD(human_selected);
  play_sample_at_channel(H_SND_EAT,1,100);
  if (human_selected->jidlo>mhlad) human_selected->jidlo=mhlad;
  if (it->magie>0)
  thing_cast(it->spell,human_selected-postavy,human_selected->sektor,NULL,1);
  bott_draw(1);
  redraw_inventory();
  }

static void inv_napit(short item)
  {
  TITEM *it;
  int mvoda;

  if (!item) return;
  it=glob_items+item-1;
  if (it->druh!=TYP_VODA) return;
  human_selected->voda+=it->user_value*HODINA;
  mvoda=MAX_ZIZEN(human_selected);
  play_sample_at_channel(H_SND_LEKTVAR,1,100);
  if (human_selected->voda>mvoda) human_selected->voda=mvoda;
  if (it->magie>0)
  thing_cast(it->spell,human_selected-postavy,human_selected->sektor,NULL,1);
  bott_draw(1);
  redraw_inventory();
  }

static void inv_quaf(short item)
  {
  TITEM *it;

  if (!item) return;
  it=glob_items+item-1;
  if (it->druh!=TYP_LEKTVAR) return;
  thing_cast(it->spell,human_selected-postavy,human_selected->sektor,NULL,1);
  bott_draw(1);
  play_sample_at_channel(H_SND_LEKTVAR,1,100);
  redraw_inventory();
  }

static void inv_use_spec(short **items)
  {
  short *pp;
  short it;

  pp=*items;
  if (pp==NULL) return;
  it=*pp;
  if (!it) return;
  it--;
  if (it==flute_item)
     {
     unwire_proc();
     wire_proc();
     bott_draw_fletna();
     if (put_item_to_inv(human_selected,pp))
        {
        free(pp);
        *items=NULL;
        }
     poloz_vsechny_predmety();
     pick_set_cursor();
     }
  }

char check_jidlo_voda(THUMAN *p)
  {
  if (game_extras & EX_NOHUNGRY) return 0;
  if (p->used && p->lives && p->inmaphash == current_map_hash)
     {
     p->jidlo--;
     p->voda--;
     if (p->jidlo<0)
       {
       p->jidlo=0;
       return 1;
       }
    if (p->voda<0)
       {
       p->voda=0;
       player_hit(p,1,0);
       return 1;
       }
     }
  return 0;
  }

char check_map_specials(THUMAN *p)
  {
  char c=0;
  if (p->inmaphash != current_map_hash) return c;
  switch(mglob.map_effector)
     {
     case ME_NORMAL:break;
     case ME_SOPKA:if (~p->vlastnosti[VLS_KOUZLA] & SPL_FIRE_RES) player_hit(p,10,0),c=1;break;
     case ME_LEDOV:if (~p->vlastnosti[VLS_KOUZLA] & SPL_ICE_RES) player_hit(p,10,0),c=1;break;
     }
  if (c) bott_draw(0);
  return c;
  }

void umirani_postavy(THUMAN *p) {
  p->kondice = MAX(0, p->kondice - p->vlastnosti[VLS_KONDIC]/4);
}

void pomala_regenerace_postavy(THUMAN *p)
  {
  int mreg,mmax;
  check_map_specials(p);
  if (check_jidlo_voda(p)) return;
  if (p->utek)
     {
     p->utek=0;
     if (p->kondice>0) p->kondice-=1;
     return;
     }
  p->kondice+=(p->vlastnosti[VLS_VPREG]+9)/10;
  if (p->kondice>p->vlastnosti[VLS_KONDIC]) p->kondice=p->vlastnosti[VLS_KONDIC];
  mreg=(p->vlastnosti[VLS_MPREG]+9)/10;
  mmax=p->vlastnosti[VLS_MAXMANA];
  if (p->mana+mreg>mmax) mreg=mmax-p->mana;
  if (mreg>0) p->mana+=mreg;
  }


char sleep_regenerace(THUMAN *p)
  {
  int mreg,mmax;
  if (p->used && (!p->lives) && p->kondice>0) {
    if (p->inmaphash == current_map_hash) {
        p->lives = 1;
    } else {
        p->kondice = 0;
    }
  }
  if (p->used && p->lives && p->inmaphash == current_map_hash)
     {
     if (check_jidlo_voda(p)) return 1;
     p->kondice+=p->vlastnosti[VLS_VPREG];
     p->lives+=p->vlastnosti[VLS_HPREG];
     if (p->kondice>p->vlastnosti[VLS_KONDIC]) p->kondice=p->vlastnosti[VLS_KONDIC];
     if (p->lives>p->vlastnosti[VLS_MAXHIT]) p->lives=p->vlastnosti[VLS_MAXHIT];
     mreg=p->vlastnosti[VLS_MPREG];
     mmax=p->vlastnosti[VLS_MAXMANA];
     if (p->mana+mreg>mmax) mreg=mmax-p->mana;
     if (mreg>0) p->mana+=mreg;
     }
  return 0;
  }


void real_regeneration(THE_TIMER *t)
  {
  int i;
  THUMAN *p;

  for(i=0;i<POCET_POSTAV;i++) {
     p=&postavy[i];
     if (p->used) {
        if (p->lives) {
          pomala_regenerace_postavy(p);
        } else {
          umirani_postavy(p);
        }
     }
  }
  send_message(E_KOUZLO_KOLO);
  sleep_ticks+=MAX_SLEEP/30;
  if (sleep_ticks>MAX_SLEEP) sleep_ticks=MAX_SLEEP;
  tick_tack(1);
  TimerEvents(viewsector,viewdir,game_time);
  SEND_LOG("(GAME) Tick Tack, Game time: %d",game_time);
  GlobEvent(MAGLOB_ONROUND,viewsector,viewdir);
  bott_draw(0);
  }

void init_inventory(void)
  {
  definuj_postavy();
  }

void display_items_in_inv(THUMAN *h)
  {
  int i,x,xr,yr,it;
  short *p;

  put_picture(266,TOP_OFS,ablock(H_IDESKA));
  p=ablock_copy(H_IMRIZ1);
  p[1]=INV_YS*((h->inv_size-1)/6)+58;
  put_picture(INV_X,INV_Y,p);
  xr=INV_X;x=0;
  yr=INV_Y;
  for(i=0;i<h->inv_size;i++)
     {
     if ((it=h->inv[i])!=0)
        {
        int ikn;

        ikn=glob_items[it-1].ikona;
        put_picture(xr,yr,ico_extract(ikn));
        }
     xr+=INV_XS;
     x++;if (x>=6)
        {
        xr=INV_X;yr+=INV_YS;x=0;
        }
     }
  }

void display_rings()
  {
  word pozice[][2]=
     {
     {245,194},{245,229},{245,264},{245,299}
     };
  int i;

  for(i=0;i<4;i++)
     {
     int ikn;

     ikn=human_selected->prsteny[i];
     if (ikn)
        {
        word *w;
        ikn=glob_items[ikn-1].ikona;
        w=(word *)ico_extract(ikn);
        put_picture(pozice[i][0]-w[0]/2,pozice[i][1]-w[1]/2,w);
        }
     }
  }

void *build_items_wearing(THUMAN *h, int32_t *s)
  {
  int i,vzhled,it;
  const word *p;
  word hx,hy;
  word *ob;
  size_t siz;
  char *pp;

  p=ablock(H_CHARS+h-postavy);
  hx=p[0];
  hy=p[1];
  ob=getmem(siz=(PO_XS*PO_YS+PIC_FADE_PAL_SIZE));
  memset(ob,0,siz);
  memcpy(ob,p,PIC_FADE_PAL_SIZE);
  *s = siz;
  ob[0]=PO_XS;
  ob[1]=PO_YS;
  put_picture2picture(p,ob,PO_XSS-(hx/2),PO_YS-hy-20);
  for(i=1;i<HUMAN_PLACES;i++)
     if ((it=h->wearing[i])!=0)
        {
        TITEM *itt;

        itt=&glob_items[it-1];
        vzhled=itt->vzhled;
        if (h->female==1) vzhled+=face_arr[2];else vzhled+=face_arr[1];
        if (i==PO_RUKA_L) place_human_item(ob,itt->polohy[1][0],itt->polohy[1][1],vzhled);
        else place_human_item(ob,itt->polohy[0][0],itt->polohy[0][1],vzhled);
        }
  pp=(char *)ob;
  if (h->vlastnosti[VLS_KOUZLA] & SPL_STONED)
  {
    int i;
    for (i=0;i<(PIC_FADE_PAL_SIZE>>1);++i) if (i>3)
    {
      unsigned short col=ob[i];
      int bw=(GET_R_COLOR(col)+GET_G_COLOR(col)+GET_B_COLOR(col))/3;
      if (bw>255) bw=255;
      ob[i]=RGB(bw,bw,bw);
    }
  }
  else
  {
    pp+=PIC_FADE_PAL_SIZE;
    if (h->vlastnosti[VLS_KOUZLA] & SPL_INVIS)
       for(i=0;i<PO_XS*PO_YS;i++,pp++) if (*pp!=0) *pp=1;
  }
    return ob;
  }


const void *build_items_called(const void *p, int32_t *s, int h)
  {
  return build_items_wearing(&postavy[h-H_POSTAVY], s);
  }

void display_items_wearing(THUMAN *h)
  {
  int it;
  int32_t scr_linelen2 = GetScreenPitch();
  put_picture(4,TOP_OFS,ablock(H_IOBLOUK));
  zneplatnit_block(h-postavy+H_POSTAVY);
  enemy_draw(ablock(h-postavy+H_POSTAVY),GetScreenAdr()+HUMAN_X+HUMAN_Y*scr_linelen2,6,320,HUMAN_Y,640*65536);
  it=h->wearing[PO_BATOH];
  if (it)
        {
        TITEM *itt;
        const word *w;int vzhled;

        itt=&glob_items[it-1];
        vzhled=itt->vzhled;
        if (h->female==1) vzhled+=face_arr[2];else vzhled+=face_arr[1];
        w=ablock(vzhled);
        enemy_draw(w,GetScreenAdr()+itt->polohy[0][0]+HUMAN_X+PO_XSS-w[0]/2+scr_linelen2*(HUMAN_Y-itt->polohy[0][1]-20),6,320,HUMAN_Y,640*65536);
        }
  }


void write_human_big_name(char *c)
  {
  int xs,ys;

  set_font(H_FBOLD,INV_NAME_COL);
  xs=text_width(c)>>1;
  ys=text_height(c)>>1;
  position(INV_NAME_X-xs,INV_NAME_Y-ys);
  outtext(c);
  }



#define pvls(x) (offsetof(struct thuman,vlastnosti[x]))
#define ptpw(x) (offsetof(struct thuman,bonus_zbrani[x]))
static void percent_bar(int x,int y,int xs,int ys,int val,int max,char *popis)
  {
  CTL3D clt;
  char s[25];

  memcpy(&clt,def_border(3,0),sizeof(clt));
  bar32(x,y,x+xs*val/max,ys+y);
  show_textured_button(x-2,y-2,xs+5,ys+5,0,&clt);
  set_aligned_position(x,y-5,0,2,popis);
  outtext(popis);
  sprintf(s,"%d/%d",val/360,max/360);
  set_aligned_position(x+xs/2,y+ys/2,1,1,s);
  outtext(s);
  }

typedef struct t_inv_script
  {
  short col,line;
  char *text;
  short parm1,parm2;
  char lenght;
  char align;
} T_INV_SCRIPT;

#define INFO_AP -1
#define INFO_APF -3
#define INFO_EXP -2
#define LINE_STEP 6
#define COL_STEP 8

static T_INV_SCRIPT script[]=
  {
  {15,12,"%d",pvls(VLS_SILA),0,2+128,2},
  {15,14,"%d",pvls(VLS_SMAGIE),0,2+128,2},
  {15,16,"%d",pvls(VLS_POHYB),0,2+128,2},
  {15,18,"%d",pvls(VLS_OBRAT),0,2+128,2},
  {15,5,"%d/%d",offsetof(THUMAN,lives),pvls(VLS_MAXHIT),2,2},
  {15,7,"%d/%d",offsetof(THUMAN,mana),pvls(VLS_MAXMANA),2,2},
  {15,9,"%d/%d",offsetof(THUMAN,kondice),pvls(VLS_KONDIC),2,2},
  {15,20,"%d",offsetof(THUMAN,bonus),0,2,2},
  {15,25,"%d",ptpw(TPW_MEC),0,2,2},
  {15,27,"%d",ptpw(TPW_SEKERA),0,2,2},
  {15,29,"%d",ptpw(TPW_KLADIVO),0,2,2},
  {15,31,"%d",ptpw(TPW_HUL),0,2,2},
  {15,33,"%d",ptpw(TPW_DYKA),0,2,2},
  {15,35,"%d",ptpw(TPW_STRELNA),0,2,2},
  {15,37,"%d",ptpw(TPW_OST),0,2,2},
  {0,0,NULL,0,31,1,0},
  {0,2,NULL,0,30,1,0},
  {15,0,"%d",offsetof(THUMAN,level),0,2,2},
  {30,2,"%d [%d]",offsetof(THUMAN,exp),INFO_EXP,4,2},
  {0,5,NULL,0,14,1,0},
  {0,7,NULL,0,16,1,0},
  {0,9,NULL,0,15,1,0},
  {0,12,NULL,0,10,128,0},
  {0,14,NULL,0,11,128,0},
  {0,16,NULL,0,12,128,0},
  {0,18,NULL,0,13,128,0},
  {0,20,NULL,0,19,0,0},
  {0,23,NULL,0,90,1,0},
  {0,25,NULL,0,91,1,0},
  {0,27,NULL,0,92,1,0},
  {0,29,NULL,0,93,1,0},
  {0,31,NULL,0,94,1,0},
  {0,33,NULL,0,95,1,0},
  {0,35,NULL,0,96,1,0},
  {0,37,NULL,0,97,1,0},
  {30,5,"%d-%d",pvls(VLS_UTOK_L),pvls(VLS_UTOK_H),2,2},
  {30,7,"%d-%d",pvls(VLS_OBRAN_L),pvls(VLS_OBRAN_H),2,2},
  {30,9,"%d",INFO_AP,0,2,2},
  {17,5,NULL,0,18,1,0},
  {17,7,NULL,0,17,1,0},
  {17,9,NULL,0,20,1,0},
  {30,14,"%d",pvls(VLS_OHEN),0,2,2},
  {30,16,"%d",pvls(VLS_VODA),0,2,2},
  {30,18,"%d",pvls(VLS_ZEME),0,2,2},
  {30,20,"%d",pvls(VLS_VZDUCH),0,2,2},
  {30,22,"%d",pvls(VLS_MYSL),0,2,2},
  {17,12,NULL,0,21,1,0},
  {17,14,NULL,0,22,1,0},
  {17,16,NULL,0,23,1,0},
  {17,18,NULL,0,24,1,0},
  {17,20,NULL,0,25,1,0},
  {17,22,NULL,0,26,1,0},
  };

static int calc_value(int parm,int lenght)
  {
  int32_t l = 0;
  if (parm>=0) l=*(int32_t *)(((char *)human_selected)+parm);
  else
     switch (parm)
        {
        case INFO_EXP:
           l=(human_selected->level<PLAYER_MAX_LEVEL?(level_map[human_selected->level-1]-human_selected->exp):0);
           break;
        case INFO_AP:
           l=human_selected->vlastnosti[VLS_POHYB]/AP_MULTIPLIER;
           break;
       case INFO_APF:
           l=((human_selected->vlastnosti[VLS_POHYB] * 10)/AP_MULTIPLIER) % 10;
           break;
       }
  switch(lenght)
     {
     case 1:l=(int32_t)((signed char)l);break;
     default:
     case 2:l=(int32_t)((short)l);break;
     case 4:l=(int32_t)l;break;
     }
  return l;
  }

void inv_display_vlastnosti()
  {
  unsigned int i;

  put_picture(INV_DESK,TOP_OFS,ablock(H_SVITEK));
  for(i=0;i<sizeof(script)/sizeof(struct t_inv_script);i++)
     {
     struct t_inv_script *scr=script+i;
     int x,y,p1,p2;
     char *s;
     char buffer[80];
     x=INV_DESC_X+scr->col*COL_STEP;y=INV_DESC_Y+scr->line*LINE_STEP;
     s=scr->text;
     p1=scr->parm1;
     p2=scr->parm2;
     p1=calc_value(p1,scr->lenght & 0x7f);
     if (s==NULL) s=texty[scr->parm2];else p2=calc_value(p2,scr->lenght);
     sprintf(buffer,s,p1,p2);
     if (scr->lenght & 0x80 && human_selected->bonus)
        set_font(H_FONT7,scr->text==NULL?INV_LEVEL_COL1:INV_LEVEL_COL2);
     else set_font(H_FONT7,NOSHADOW(0));
     set_aligned_position(x,y,scr->align,0,buffer);
     outtext(buffer);
     }
  set_font(H_FONT7,NOSHADOW(0));
  curcolor=(RGB555(18,17,14));
  percent_bar(INV_DESC_X+140,INV_DESC_Y+170,90,10,human_selected->jidlo,MAX_HLAD(human_selected),texty[69]);
  curcolor=(RGB555(14,17,18));
  percent_bar(INV_DESC_X+140,INV_DESC_Y+210,90,10,human_selected->voda,MAX_ZIZEN(human_selected),texty[70]);
  if (human_selected->bonus)
  for(i=0;i<4;i++) if (calc_value(script[i].parm1,2)<100)
     {
     int x,y;
     x=INV_DESC_X+script[i].col*COL_STEP;y=INV_DESC_Y+script[i].line*LINE_STEP;
     set_font(H_FSYMB,RGB555(31,31,31));
     memcpy(charcolors,butt_plus,sizeof(charcolors));
     position(x+1,y);outtext("+");
     }
  }

typedef struct t_info
  {
  short line,col;
  char *format;
  short parm1,parm2;
  char bonus;
  }T_INFO;


char muze_nosit(short item)
  {
  short *p,i,*q,*z;

  p=human_selected->vlastnosti;
  q=glob_items[item-1].podminky;
  z=glob_items[item-1].zmeny;
  for(i=0;i<4;i++)
     {
     if (*p<*q) return 0;
     if (*p+*z<*q) return 0;
     q++;z++;p++;
     }
  return 1;
  }

void hide_inv_info_box()
  {
  info_box_drawed=0;
  if (info_box_below!=NULL)
     {
     schovej_mysku();
     put_picture(INV_INFO_X-12,INV_INFO_Y-12,info_box_below);
     free(info_box_below);
     info_box_below=NULL;
     ukaz_mysku();
     showview(INV_INFO_X-VEL_RAMEC,INV_INFO_Y-VEL_RAMEC,INV_INFO_XS+2*VEL_RAMEC+2,INV_INFO_YC+2*VEL_RAMEC+2);
     }
  }

void inv_info_box(char *text1,char *text2,char *text3,char asterix)
  {
  int x,y,ys;
  static int last_info_ys=0;
  ys=INV_INFO_YS;
  if (text3!=NULL) ys+=INV_INFO_XP;
  if (info_box_below!=NULL && last_info_ys!=ys)
     {
     put_picture(INV_INFO_X-12,INV_INFO_Y-12,info_box_below);
     free(info_box_below);
     info_box_below=NULL;
     }
  last_info_ys=ys;
  info_box_drawed=1;
  if (info_box_below==NULL)
     {
     info_box_below=getmem((INV_INFO_XS+28)*(ys+28)*2+6);
     get_picture(INV_INFO_X-12,INV_INFO_Y-12,INV_INFO_XS+28,ys+28,info_box_below);
     }
  x=INV_INFO_X;
  y=INV_INFO_Y;
  create_frame(x,y,INV_INFO_XS,ys,1);
  set_font(H_FBOLD,asterix?(NOSHADOW(RGB555(31,0,0))):NOSHADOW(0));
  position(x,y);
  outtext(text1);
  position(x,y+12);
  outtext(text2);
  position(x,y+24);
  if (text3!=NULL) outtext(text3);
  }


static char *get_item_req(char *s,int itn)
  {
  TITEM *it;
  int i;
  char *c=s;

  s[0]=0;
  itn--;if (itn<0) return NULL;
  it=glob_items+itn;
  for(i=0;i<4;i++)
     if (it->podminky[i])
        {
        sprintf(c,texty[200+i],it->podminky[i]);
        c=strchr(c,0);
        if (i!=3)*c++=32;
        }
  *c=0;
  if (s[0]) return s;else return NULL;
  }

void inv_informuj()
  {
  int i;
  char s[80];
  char c[80];
  char found_price = 0;

  if (picked_item==NULL) return;
  i=*picked_item;
  if (cur_shop) {
    for (int j = 0; j < cur_shop->list_size; ++j) {
      if (cur_shop->list[j].item == i-1) {
        int cena = cur_shop->list[j].cena;
        if (cena) {
          sprintf(c,"%s (%d)",glob_items[i-1].jmeno,cena-cur_shop->koef*cena/100);
          found_price = 1;
          break;
        }
      }
    }
}
if (!found_price) {
    strcopy_n(c,glob_items[i-1].jmeno,sizeof(c));
}

  inv_info_box(c,glob_items[i-1].popis,get_item_req(s,i),!muze_nosit(i));
  }

void write_pocet_sipu()
  {
  char s[10];
  if (human_selected->sipy)
     {
     set_font(H_FBOLD,RGB555(31,31,31));
     sprintf(s,"%d",human_selected->sipy);
     set_aligned_position(19,301,1,1,s);
     outtext(s);
     }
  }


static const void *shop_keeper_picture = NULL;


void redraw_inventory()
  {
  update_mysky();
  schovej_mysku();
  curcolor=0;
  bar32(0,16,30,16+360);
  bar32(620,16,640,16+360);
  if (inv_view_mode==0 && ~human_selected->stare_vls[VLS_KOUZLA] & SPL_DEMON && ~human_selected->vlastnosti[VLS_KOUZLA] & SPL_STONED) display_items_in_inv(human_selected);
  else inv_display_vlastnosti();
  display_items_wearing(human_selected);
  write_human_big_name(human_selected->jmeno);
  write_pocet_sipu();
  display_rings();
  other_draw();
  info_box_drawed=0;
  free(info_box_below);
  info_box_below=NULL;
  ms_last_event.event_type=0x1;send_message(E_MOUSE,&ms_last_event);
  ukaz_mysku();
  showview(0,0,0,0);
  }

static void add_vls(int plus,int vls,short *change1,short *change2,float factor)
  {
  int oldv,newv;

  oldv=(int)(vls*factor);
  vls+=plus;
  newv=(int)(vls*factor);
  if (change1!=NULL)
     {
     change1[0]+=newv-oldv;
     if (change2!=NULL) change2[0]+=newv-oldv;
     }
  return;
  }

int advance_vls(int id)
  {
  int i=0;
  short *vls=human_selected->stare_vls;
        switch (id)
           {
           case 0:if (vls[VLS_SILA]<100) //max 100
                    {
                    add_vls(1,vls[VLS_SILA],vls+VLS_MAXHIT,&human_selected->lives,1.5f);
                    add_vls(1,vls[VLS_SILA],vls+VLS_HPREG,NULL,0.08f);
                    vls[VLS_SILA]++;
                    i=1;
                    }
                  break;
           case 1:if (vls[VLS_SMAGIE]<100) //max 100
                    {
                    add_vls(1,vls[VLS_SMAGIE],vls+VLS_MAXMANA,&human_selected->mana,2);
                    add_vls(1,vls[VLS_SMAGIE],vls+VLS_MPREG,NULL,0.1f);
                    vls[VLS_SMAGIE]++;
                    i=1;
                    }
                  break;
           case 2:if (vls[VLS_POHYB]<100) //max 100
                    {
                    add_vls(1,vls[VLS_POHYB],vls+VLS_MAXHIT,&human_selected->lives,0.5);
                    add_vls(1,vls[VLS_POHYB],vls+VLS_KONDIC,&human_selected->kondice,0.25);
                    vls[VLS_POHYB]++;
                    i=1;
                    }
                  break;
           case 3:if (vls[VLS_OBRAT]<100) //max 100
                    {
                    add_vls(1,vls[VLS_OBRAT],vls+VLS_KONDIC,&human_selected->kondice,0.5);
                    add_vls(1,vls[VLS_OBRAT],vls+VLS_VPREG,NULL,0.1f);
                    vls[VLS_OBRAT]++;
                    i=1;
                    }
                  break;
           }
  return i;
  }

static void timed_redraw(THE_TIMER *t)
  {
  t;
  bott_draw(1);
  inv_redraw();
  }

char vls_click(int id,int xa,int ya,int xr,int yr)
  {
  int xs,ys,i;

  if (!human_selected->bonus) return 0;
  set_font(H_FSYMB,0);
  xs=text_width("+");
  ys=text_height("+");
  for(id=0;id<4;id++)
     {
     int xe,ye;
     float mh,mv;
     xr=INV_DESC_X+COL_STEP*script[id].col+1;
     yr=INV_DESC_Y+LINE_STEP*script[id].line;
     xe=xr+xs;ye=yr+ys;
     if (xa>=xr && xa<xe && ya>=yr && ya<ye)
        {
        mh=(float)human_selected->jidlo/MAX_HLAD(human_selected);
        mv=(float)human_selected->voda/MAX_ZIZEN(human_selected);
        i=advance_vls(id);
        if (i>0)
           {
           schovej_mysku();
           set_font(H_FSYMB,0x0);
           memcpy(charcolors,butt_plus,sizeof(butt_plus));
           charcolors[1]=butt_plus[3];
           charcolors[3]=butt_plus[1];
           position(xr,yr);outtext("+");
           ukaz_mysku();
           showview(xr,yr,xs,ys);
           }
        human_selected->bonus-=i;
        prepocitat_postavu(human_selected);
        human_selected->jidlo=(int)(mh*MAX_HLAD(human_selected));
        human_selected->voda=(int)(mv*MAX_ZIZEN(human_selected));
        add_to_timer(-1,6,1,timed_redraw);
        }
     }
  return 1;
  }



void inv_item_info_box(EVENT_MSG *msg,void **data)
  {
  MS_EVENT *ms;
  char podm;
  int pos;
  static int lastpos=-1;
  int xr,yr;

  data;
  if (msg->msg==E_MOUSE)
     {
     ms=get_mouse(msg);
     if (picked_item!=NULL)
        {
        if (ms->y>378 && info_box_drawed) hide_inv_info_box();
        else if (ms->y<=378 && !info_box_drawed)
           {
           schovej_mysku();
           inv_informuj();
           ukaz_mysku();
           showview(INV_INFO_X-VEL_RAMEC,INV_INFO_Y-VEL_RAMEC,INV_INFO_XS+2*VEL_RAMEC+2,INV_INFO_YC+2*VEL_RAMEC+2);
           }
        lastpos=-1;
        }
     else
        {
        if (inv_view_mode) return;
        podm=(ms->x>=clk_inv_view[0].xlu &&
              ms->x<=clk_inv_view[0].xrb &&
              ms->y>=clk_inv_view[0].ylu &&
              ms->y<=clk_inv_view[0].yrb);
        xr=ms->x-clk_inv_view[0].xlu;
        yr=ms->y-clk_inv_view[0].ylu;
        if (podm) pos=(xr/INV_XS)+6*(yr/INV_YS);else pos=-1;
        if (pos>=human_selected->inv_size) podm=0;
        if (podm && (!info_box_drawed || pos!=lastpos))
           {
           int i=human_selected->inv[pos];
           if (i)
              {
              char s[80];
              char c[80];
              schovej_mysku();
              char found_price = 0;
              if (cur_shop) {
                  for (int j = 0; j < cur_shop->list_size; ++j) {
                    if (cur_shop->list[j].item == i-1) {
                      int cena = cur_shop->list[j].cena;
                      if (cena) {
                        sprintf(c,"%s (%d)",glob_items[i-1].jmeno,cena-cur_shop->koef*cena/100);
                        found_price = 1;
                        break;
                      }
                    }
                  }
              }
              if (!found_price) {
                  strcopy_n(c,glob_items[i-1].jmeno,sizeof(c));
              }
              inv_info_box(c,glob_items[i-1].popis,get_item_req(s,i),!muze_nosit(i));
              showview(INV_INFO_X-VEL_RAMEC,INV_INFO_Y-VEL_RAMEC,INV_INFO_XS+2*VEL_RAMEC+2,INV_INFO_YC+2*VEL_RAMEC+2);
              ukaz_mysku();
              }
           else if (info_box_drawed) hide_inv_info_box();
           }
        else if (!podm && info_box_drawed) hide_inv_info_box();
        lastpos=pos;
        }
     }
  }

void unwire_inv_mode(void)
  {
  send_message(E_DONE,E_KEYBOARD,inv_keyboard);  send_message(E_DONE,E_MOUSE,inv_item_info_box);
  build_all_players();
  }

char vejdou_se(int pocet)
  {
  int i=human_selected->inv_size-1;

  while (i>=0) if (!human_selected->inv[i--]) pocet--;
  return pocet>0;
  }

static char uloz_sip_action(char fast_key);
char uloz_sip(int id,int xa,int ya,int xr,int yr)
  {
  id;xa;ya;xr;yr;
  uloz_sip_action(get_control_key_state());
  return 1;
  }

static char uloz_sip_action(char fast_key) {
  if (isdemon(human_selected)) return 0;
  if (neprezbrojit()) return 0;
  if (picked_item!=NULL && picked_item[1]==0 && glob_items[picked_item[0]-1].umisteni==PL_SIP) {
     int pocet=glob_items[picked_item[0]-1].user_value;
     if (pocet==0) pocet=1;
     if (human_selected->sipy+pocet>99) return 1;
     human_selected->sipy+=pocet;
     free(picked_item);
     picked_item=NULL;
  } else {
     if (picked_item==NULL && human_selected->sipy)
        {
        int i;

        if (fast_key) {
            int best_item = -1;
            int max_arrows = 1;
            for (int i = 0; i < item_count; ++i) {
                if (glob_items[i].umisteni == PL_SIP
                        && glob_items[i].user_value > max_arrows
                        && glob_items[i].user_value <= human_selected->sipy) {
                    max_arrows = glob_items[i].user_value;
                    best_item = i;
                }
            }
            if (best_item>=0) {
                picked_item = (short *)getmem(2*sizeof(short));
                picked_item[0] = best_item+1;
                picked_item[1] = 0;
                human_selected->sipy-=max_arrows;
            }
        } else {
            for(i=0;i<item_count;i++) if (glob_items[i].umisteni==PL_SIP && glob_items[i].user_value==0)
               {
               picked_item=(short *)getmem(2*sizeof(short));
               picked_item[0] = i+1;
               picked_item[1] = 0;
               human_selected->sipy--;
               break;
               }
            }
        }
  }
  pick_set_cursor();
  inv_redraw();
  return 1;
  }

static char MakeItemCombinations(short *itm1, short *itm2)
{
  short i1=*itm1-1,i2=*itm2-1;
  int src1;
  int src2;
  int trg1;
  int trg2;
  int cnt;
  char succ=0;

  FILE *table;

  const char *fname = build_pathname(2,gpathtable[SR_MAP],"COMBITEM.DAT");
  table=fopen_icase(fname,"r");
  if (table==NULL) return 0;
  cnt=fscanf(table,"%d %d -> %d %d",&src1,&src2,&trg1,&trg2);
  while(cnt>=3)
  {
    if (src1==i1 && src2==i2)
    {
      if (cnt==3)
      {
        *itm2=trg1+1;
        *itm1=0;
      }
      else
      {
        *itm1=trg1+1;
        *itm2=trg2+1;
      }
      succ=1;
      break;
    }
    if (src1==i2 && src2==i1)
    {
      if (cnt==3)
      {
        *itm2=trg1+1;
        *itm1=0;
      }
      else
      {
        *itm2=trg1+1;
        *itm1=trg2+1;
      }
      succ=1;
      break;
    }
    if (fscanf(table," ;")==-1) break;
    cnt=fscanf(table,"%d %d -> %d %d",&src1,&src2,&trg1,&trg2);
  }
  fclose(table);
  return succ;
}

static char bag_action(int xr, int yr);
char bag_click(int id,int xa,int ya,int xr,int yr)
  {

  if (inv_view_mode || human_selected->stare_vls[VLS_KOUZLA] & SPL_DEMON || human_selected->vlastnosti[VLS_KOUZLA] & SPL_STONED) return vls_click(id,xa,ya,xr,yr);
  xa;ya;id;
  return bag_action(xr, yr);

}

static char bag_action(int xr, int yr) {
  int id=(xr/INV_XS)+6*(yr/INV_YS);
  short p,*pk;
  if (id>=human_selected->inv_size) return 0;
    pk=picked_item;
   if (pk!=NULL)
     {
     if (picked_item[1]!=0 && vejdou_se(count_items_total(picked_item))) return 0;
     if (picked_item[1]!=0 || human_selected->inv[id]==0 || !MakeItemCombinations(picked_item,human_selected->inv+id))
      while (*pk)
        {
        p=human_selected->inv[id];
        int itm = abs(*pk);
        human_selected->inv[id]=itm;
        ach_event_inv_add(itm-1);
        *pk=p;pk++;
        if (*pk) {
            while (human_selected->inv[id]) {
                if ((++id)>=human_selected->inv_size) id=0;
            }
        }
        }
     }
  else
     {
     picked_item=getmem(2*sizeof(short));
     picked_item[0]=human_selected->inv[id];
     picked_item[1]=0;
     human_selected->inv[id]=0;
     }
 if (!picked_item[0])
     {
     free(picked_item);
     picked_item=NULL;
     }
  pick_set_cursor();
  play_sample_at_channel(H_SND_PUTINV,0,100);
  inv_redraw();
  return 1;
  }

char item_pointed(int k,int x,int y,short item,short kind)
  {
  int x1,y1,x2,y2,xs,ys,xsiz,ysiz,i;
  const short *p;
  char *c,cc;

  if (!item) return 0;
  i=glob_items[item-1].vzhled;
  p=ablock(i+face_arr[1+kind]);
  xs=glob_items[item-1].polohy[k][0]+HUMAN_X+PO_XSS;
  ys=HUMAN_Y-glob_items[item-1].polohy[k][1]-20;
  xsiz=p[0]>>1;
  ysiz=p[1];
  x1=xs-xsiz;y1=ys-ysiz;
  x2=xs+xsiz;y2=ys;
  if (x>=x1 && x<=x2 && y>=y1 && y<=y2)
    {
    xs=x-x1;
    ys=y-y1;
    c=(char *)p;c+=PIC_FADE_PAL_SIZE+xs+ys*p[0];
    cc=*c;
    }
  else cc=0;
  return cc!=0;
  }

char inv_swap_desk(int id,int xa,int ya,int xr,int yr)
  {
  id,xa,ya,xr,yr;
  inv_view_mode=!inv_view_mode;
  inv_redraw();
  return 1;
  }

char ring_place(int id,int xa,int ya,int xr,int yr)
  {
  short ring;

  xa,ya,xr,yr;
  if (neprezbrojit()) return 0;
  if (isdemon(human_selected)) return 0;
  if (human_selected->vlastnosti[VLS_KOUZLA] & SPL_STONED) return 0;
  ring=human_selected->prsteny[id];
  if (picked_item==NULL)
     if (ring)
        {
        picked_item=(short *)getmem(2*sizeof(short));
        picked_item[0]=ring;
        picked_item[1]=0;
        human_selected->prsteny[id]=0;
        }
     else return 0;
  else
    if (picked_item[1]==0)
       {
       int i;

       i=picked_item[0]-1;
       if (glob_items[i].umisteni!=PL_PRSTEN) return 1;
       human_selected->prsteny[id]=picked_item[0];
       *picked_item=ring;
       if (!ring)
        {
        free(picked_item);picked_item=0;
        }
       }
  prepocitat_postavu(human_selected);
  zkontroluj_postavu();
  pick_set_cursor();
  inv_redraw();
  return 1;
  }


void vymen_batohy()
  {
  short *old,*c;
  int i;

  c=old=(short *)getmem(sizeof(short)*40);
  memset(old,0,sizeof(short)*40);
  *c++=human_selected->wearing[PO_BATOH];
  for(i=6;i<human_selected->inv_size;i++)
     {
     *c=-human_selected->inv[i];
     if (*c) c++;
     }
  for(i=6;i<MAX_INV;i++) human_selected->inv[i]=0;
  if (picked_item!=NULL)
     {
     int it;

     c=picked_item;
     it=human_selected->wearing[PO_BATOH]=*c++;
     i=6;
     while (*c && i<MAX_INV) {
         int itm = -*c++;
         human_selected->inv[i++]=itm;
         ach_event_inv_add(itm-1);
     }
     human_selected->inv_size=glob_items[it-1].nosnost+6;
     }
  else
     {
     human_selected->inv_size=6;
     human_selected->wearing[PO_BATOH]=0;
     }
  if (old[0]) picked_item=old;
  else
     {
     free(old);
     picked_item=NULL;
     }
  prepocitat_postavu(human_selected);
  zkontroluj_postavu();
  pick_set_cursor();
  play_sample_at_channel(H_SND_WEAR,1,100);
  inv_redraw();
  }

void remove_item(THUMAN *p,int what)
  {
  if (p->wearing[what])
     {
     int i;
    short z[2];

     z[0]=p->wearing[what];
     z[1]=0;
     if (glob_items[z[0]-1].flags & ITF_NOREMOVE) destroy_items(z);
     else
        {
        for(i=0;i<p->inv_size && p->inv[i];i++);
        if (i!=p->inv_size) p->inv[i]=p->wearing[what];
        else push_item(p->sektor,p->direction,z);
        }
     p->wearing[what]=0;
     }
  }

int calculate_weight(THUMAN *p)
{
  int suma=0;
  int i;
  for (i=0;i<p->inv_size;i++) if (p->inv[i])
    suma+=glob_items[p->inv[i]-1].hmotnost;
  suma=suma*3/2;
  for (i=0;i<HUMAN_PLACES;i++) if (p->wearing[i])
    suma+=glob_items[p->wearing[i]-1].hmotnost;
  for (i=0;i<HUMAN_RINGS;i++) if (p->prsteny[i])
    suma+=glob_items[p->prsteny[i]-1].hmotnost;
  return suma;
}


int weigth_defect(THUMAN *p)
{
  int wh,wf;
  wh=calculate_weight(p);
  if (wh>p->vlastnosti[VLS_SILA]*20)
    wf=(wh-p->vlastnosti[VLS_SILA]*20)/200;
  else
    wf=0;
  return wf;
}

static char check_double_wield()  {
    short i1 = human_selected->wearing[PO_RUKA_L];
    short i2 = human_selected->wearing[PO_RUKA_R];
    if (!i1 || !i2) return 0;
    const TITEM *it1 = glob_items+i1-1;
    const TITEM *it2 = glob_items+i2-1;
    if (it1->druh!=TYP_UTOC || it2->druh!=TYP_UTOC ) return 0;
    for (int i = VLS_SILA; i <=VLS_OBRAT; ++i) {
        if ((it1->podminky[i] + it2->podminky[i] > human_selected->vlastnosti[i])
            && (MAX(it1->podminky[i], it2->podminky[i]) > human_selected->vlastnosti[VLS_OBRAT])) {
            return 1;
        }
    }
    return 0;
}

void zkontroluj_postavu()
  {
  short *p,i;

  p=human_selected->wearing;
  for(i=0;i<HUMAN_PLACES;i++,p++)
     if (*p && !muze_nosit(p[0])) remove_item(human_selected,i);
  if (check_double_wield())
     remove_item(human_selected,PO_RUKA_L);
  prepocitat_postavu(human_selected);
  }




char human_click(int id,int xa,int ya,int xr,int yr)
  {
  short itsave=0;
  short place=-1;
  short um=0;


  xr;yr;id;
  if ((battle && ((battle_mode!=MD_PREZBROJIT) || (select_player!=human_selected-postavy))) || (human_selected->vlastnosti[VLS_KOUZLA] & SPL_STONED)) return 0;
  if (isdemon(human_selected)) return 0;
  if (picked_item!=NULL)
   if (muze_nosit(*picked_item))
     if (glob_items[(*picked_item)-1].umisteni==PL_BATOH)
        {
        vymen_batohy();
        return 0;
        }
     else if (picked_item[1]!=0) return 0;
     else
       {
       um=place=glob_items[picked_item[0]-1].umisteni;
       if (!place)
         {
         switch (glob_items[picked_item[0]-1].druh)
           {
           case TYP_LEKTVAR:inv_quaf(*picked_item);destroy_items(picked_item);free(picked_item);picked_item=NULL;pick_set_cursor();break;
           case TYP_JIDLO:inv_najist(*picked_item);destroy_items(picked_item);free(picked_item);picked_item=NULL;pick_set_cursor();break;
           case TYP_VODA:inv_napit(*picked_item);destroy_items(picked_item);free(picked_item);picked_item=NULL;pick_set_cursor();break;
           case TYP_SPECIALNI:inv_use_spec(&picked_item);break;
           }
         inv_redraw();
         return 1;
         }
       if (place==PL_RUKA)
           if (xr<94) place=PO_RUKA_R;else place=PO_RUKA_L;
       else if (place==PL_OBOUR) place=PO_RUKA_R;
       else place--;
       if (place>=HUMAN_PLACES) return 0;
       itsave=human_selected->wearing[place];
       }
   else return 0;
  else
     {
     int i=HUMAN_PLACES-1;
     while (i>=0)
        {
        if (item_pointed(i==PO_RUKA_L,xa,ya,human_selected->wearing[i],human_selected->female)) break;
        i--;
        }
     if (i<0) return 0;
     if (i==PO_BATOH) vymen_batohy();
     else
        {
        picked_item=getmem(2*sizeof(short));
        picked_item[0]=human_selected->wearing[i];
        picked_item[1]=0;
        human_selected->wearing[i]=0;
        }
     }
  if (um)
     {
     switch (um)
        {
        case PL_KUTNA:remove_item(human_selected,PO_HLAVA);
                      remove_item(human_selected,PO_TELO_D);
                      remove_item(human_selected,PO_TELO_H);
                      break;
        case PL_OBOUR:remove_item(human_selected,PO_RUKA_L);
                      break;
        case PL_TELO_H:
        case PL_TELO_D:
        case PL_HLAVA:remove_item(human_selected,PO_KUTNA);
                      break;
        case PL_RUKA :if (place==PO_RUKA_L)
                      if (human_selected->wearing[PO_RUKA_R] && glob_items[human_selected->wearing[PO_RUKA_R]-1].umisteni==PL_OBOUR)
                         remove_item(human_selected,PO_RUKA_R);
                      break;
        }
     human_selected->wearing[place]=*picked_item;
     ach_event_inv_add(*picked_item-1);
     if (itsave) *picked_item=itsave;
     else
        {
        free(picked_item);picked_item=NULL;
        }
     }
  if (picked_item!=NULL && glob_items[*picked_item-1].flags & ITF_NOREMOVE)
      {
      destroy_items(picked_item);
      free(picked_item);
      picked_item=NULL;
      }
  play_sample_at_channel(H_SND_WEAR,1,100);
  prepocitat_postavu(human_selected);
  if (check_double_wield()) {
      if (place == PO_RUKA_L) remove_item(human_selected, PO_RUKA_R);
      else remove_item(human_selected, PO_RUKA_L);
      prepocitat_postavu(human_selected);
  }
  zkontroluj_postavu();
  pick_set_cursor();
  inv_redraw();
  return 1;
  }


static void fast_inv_action(void) {
    int x = ms_last_event.x;
    int y = ms_last_event.y;
    const T_CLK_MAP *item = find_in_click_map_entry(x, y, clk_inv_view, CLK_INV_VIEW, MS_EVENT_MOUSE_LPRESS);
    if (item && item->proc == &uloz_sip) {
        uloz_sip_action(1);
    }
}

void inv_keyboard(EVENT_MSG *msg,void **usr)
  {
  char c;

  usr;
  if (msg->msg==E_KEYBOARD)
     {
     c=quit_request_as_escape(va_arg(msg->data, int))>>8;
     switch (c)
        {
        default:break;
        case 0x17: //i
        case 1:unwire_inv_mode();
               wire_proc();
               inv_view_mode=0;
               break;
        case 57:
            fast_inv_action();
            break;
        }
     }
  }


void wire_inv_mode(THUMAN *select)
  {
  mute_all_tracks(0);
  send_message(E_ADD,E_KEYBOARD,inv_keyboard);
  change_click_map(clk_inv_view,CLK_INV_VIEW);
  send_message(E_ADD,E_MOUSE,inv_item_info_box);
  human_selected=select;
  if (human_selected->bonus!=0 && picked_item==NULL) inv_view_mode=1;
  cur_mode=MD_INV;
  build_all_players();
  redraw_inventory();
  unwire_proc=unwire_inv_mode;
  inv_redraw=redraw_inventory;
  }

static LETICI_VEC *fly_map=NULL;
static int fly_map_size=0;  //velikost mapy
static int fly_count; //vyuziti mapy

void draw_fly_items(int celx,int cely,int sector,int side)
  {
  LETICI_VEC *p;
  int xpos=0,ypos=0;
  const short *pic;
  short picnum;char turn=0,smr=0;
  TITEM *it;
  int i;

  p=fly_map;
  if (map_coord[sector].flags & 2)
     {
     i=fly_count;
     while (i--)
       if (p->sector==sector && (p->items!=NULL || p->item!=0))
          {
          switch(smr=(p->smer-side)&0x3)
             {
             case 0: xpos=p->ypos;ypos=p->xpos;turn=(celx*128+xpos)<0;break;
             case 1: xpos=p->xpos;ypos=-p->ypos;turn=1;break;
             case 2: xpos=-p->ypos;ypos=-p->xpos;turn=(celx*128+xpos)<0;break;
             case 3: xpos=-p->xpos;ypos=p->ypos;turn=0;break;
             }
          xpos+=64;
          ypos+=64;
          if (p->items==NULL) it=glob_items+p->item-1;else it=&glob_items[*(p->items)-1];
          if (p->flags & FLY_DESTROY_SEQ) {
              smr=3;
          }
          else if (smr==3) smr=1;
          picnum=it->v_letu[(smr<<2)+p->anim_pos];
          if (!picnum)
            if (it->vzhled)
              picnum=it->vzhled+face_arr[0];
            else
              picnum=0;
          else picnum+=face_arr[3];
          if (picnum)
           {
           pic=ablock(picnum);
           draw_placed_texture(pic,celx,cely,xpos,ypos,p->zpos,turn);
           }
          p++;
          }
       else p++;
     }
  }

void build_fly_map()
  {
  LETICI_VEC *p,*p2;
  static int counter=0;

  fly_count=0;
  for(p=letici_veci;p!=NULL;p=p->next) if (!(p->flags & (FLY_BURNT|FLY_UNUSED))) fly_count++;
  if (fly_count>fly_map_size || !counter)
     {
     free(fly_map);fly_map=NewArr(LETICI_VEC,fly_count);
     if (!counter) {
       SEND_LOG("(FLY) Fly_map was reduced - capacity: %d flies in game / was: %d",fly_count,fly_map_size);
     } else {
       SEND_LOG("(FLY) Fly_map was expanded - capacity: %d flies in game / was: %d",fly_count,fly_map_size);
     }
     counter=1000;
     fly_map_size=fly_count;
     }
  else counter--;
  for(p=letici_veci,p2=fly_map;p!=NULL;p=p->next) if (!(p->flags & (FLY_BURNT|FLY_UNUSED)))
     {
     map_coord[p->sector].flags|=2;
     p->anim_pos++;p->anim_pos&=3;
     memcpy(p2,p,sizeof(LETICI_VEC));
     if (p->flags & FLY_DESTROY_SEQ && p->anim_pos==3)
        {
        stop_fly(p,1);
        }
     p2++;
     }
  else
     p->flags|=FLY_UNUSED;
  }

void destroy_fly_map()
  {
  int i;
  LETICI_VEC *f;

  for(i=0,f=fly_map;i<fly_count;i++,f++) map_coord[f->sector].flags &=~MC_FLY;
  fly_count=0;
  }



void add_fly(LETICI_VEC *p)
  {

  p->next=letici_veci;
  letici_veci=p;
  }


LETICI_VEC *throw_fly(int x,int y, char rovne)
  {
  LETICI_VEC *p;
  int m;

  p=create_fly();
  p->zpos=128-y*128/360;
  p->ypos=(x>320)?32:-32;
  p->sector=viewsector;
  p->smer=viewdir;
  p->items=picked_item;
  p->counter=0;
  p->hit_bonus=0;
  p->damage=0;
  m=abs(celkova_vaha(picked_item));
  if (mglob.map_effector==ME_PVODA) m+=50;
  p->flags=(rovne?FLY_NEHMOTNA:0) | (glob_items[*picked_item-1].flags & ITF_DESTROY?FLY_DESTROY:0);
  if (m) p->speed=1000/(m)+1;else p->speed=24;
  if (p->speed>56) p->speed=56;
  p->xpos=p->speed+1;
  p->velocity=-(360-y)/72;
  if (rovne) p->velocity=0;
  if (select_player==-1 || !battle) p->owner=postavy-human_selected+1;
  else p->owner=select_player+1;
  p->hit_bonus=0;
  add_fly(p);
  picked_item=NULL;
  if (!battle) pick_set_cursor();
  return p;
  }

void build_all_players()
  {
  int i;

  for(i=0;i<POCET_POSTAV;i++) zneplatnit_block(i+H_POSTAVY);
  }
//-------------------------- SHOP SYSTEM ---------------------------
#define CLK_SHOP 10

static short cur_owner=0; //1 hrac, -1 obchodnik, 0 Nikdo
char shop_bag_click(int id,int xa,int ya,int xr,int yr);
char shop_keeper_click(int id, int xa, int ya,int xr,int yr);
char shop_block_click(int id, int xa, int ya,int xr,int yr);
char shop_change_player(int id, int xa, int ya,int xr,int yr);
char _exit_shop(int id, int xa, int ya,int xr,int yr);
void unwire_shop(void);
void wire_shop(void);



static int top_item;
static short shp_item_map[IMAP_SIZE];
static short shp_item_pos[IMAP_SIZE];
static int shop_sector;

T_CLK_MAP clk_shop[]=
  {
  {-1,54,378,497,479,shop_change_player,2+8,-1},
  {-1,INV_X,INV_Y,INV_X+INV_XS*6,INV_Y+INV_YS*5,shop_bag_click,MS_EVENT_MOUSE_LPRESS,-1},
  {1,2+BUYBOX_X,39+BUYBOX_Y,22+BUYBOX_X,76+BUYBOX_Y,shop_block_click,2,H_MS_DEFAULT},
  {2,246+BUYBOX_X,39+BUYBOX_Y,266+BUYBOX_X,76+BUYBOX_Y,shop_block_click,2,H_MS_DEFAULT},
  {-1,BUYBOX_X+SHP_ICPLCX,17,BUYBOX_X+SHP_ICPLCX+4*SHP_ICSIZX,BUYBOX_Y+SHP_ICPLCY+2*SHP_ICSIZY,shop_keeper_click,MS_EVENT_MOUSE_LPRESS,-1},
  {-1,0,17,BUYBOX_X+SHP_ICPLCX,BUYBOX_Y,shop_keeper_click,2,-1},
  {-1,337,0,357,14,go_map,2,H_MS_DEFAULT},
  {-1,87,0,142,14,game_setup,2,H_MS_DEFAULT},
  {-1,30,0,85,14,konec,2,H_MS_DEFAULT},
  {-1,0,0,639,479,_exit_shop,8,-1},
  };

static void shop_mouse_event(EVENT_MSG *msg,void **unused)
  {
  unused;

  if (msg->msg==E_MOUSE)
     {
     MS_EVENT *ms;
     int x,y;
     char cc=1;
     static int last_pos=-1;
     EVENT_MSG msgc = clone_message(msg);

     ms=get_mouse(msg);
     x=ms->x-(BUYBOX_X+SHP_ICPLCX);
     y=ms->y-(BUYBOX_Y+SHP_ICPLCY);
     if (picked_item==NULL && x>0 && y>0 && x<(4*SHP_ICSIZX) && y<(2*SHP_ICSIZY))
        {
        int i,j;
        x/=SHP_ICSIZX;
        y/=SHP_ICSIZY;
        i=4*y+x;
        if (i<8 && (j=shp_item_map[i])!=0 && i!=last_pos)
           {
           char c[80];
           char s[80];
           int cena=cur_shop->list[shp_item_pos[i]].cena;

           j--;
           schovej_mysku();
           sprintf(c,"%s (%d)",glob_items[j].jmeno,cena+cur_shop->koef*cena/100);
           inv_info_box(c,glob_items[j].popis,get_item_req(s,j+1),!muze_nosit(j+1));
           ukaz_mysku();
           showview(INV_INFO_X-VEL_RAMEC,INV_INFO_Y-VEL_RAMEC,INV_INFO_XS+2*VEL_RAMEC+2,INV_INFO_YC+2*VEL_RAMEC+2);
           last_pos=i;
           cc=0;
           }
        else cc=i!=last_pos;
        }
        if (cc)
        {
        inv_item_info_box(&msgc,unused);
        last_pos=-1;
        }
        destroy_message(&msgc);
     }

  }

static __inline void copy_data(const char **src, void *target, int size) {
    memcpy(target, *src, size);
    (*src)+=size;
}

static const char * load_TSHOP(const char *binary, TSHOP *target) {
    copy_data(&binary, target->keeper, 16);
    copy_data(&binary, target->picture, 13);
    copy_data(&binary, &target->koef, 4);
    copy_data(&binary, &target->products, 4);
    copy_data(&binary, &target->shop_id, 4);
    copy_data(&binary, &target->list_size, 4);
    copy_data(&binary, &target->spec_max, 2);
    copy_data(&binary, &target->list, 4);
    return binary;
}

static const char * load_TPRODUCT(const char *binary, TPRODUCT *target) {
    copy_data(&binary, &target->item, 2);
    copy_data(&binary, &target->cena, 4);
    copy_data(&binary, &target->trade_flags, 2);
    copy_data(&binary, &target->pocet, 4);
    copy_data(&binary, &target->max_pocet, 4);
    return binary;
}

static void rebuild_shops(const void *shop_ptr)
  {
  const char *c=(const char *)shop_ptr;
  int i;



  SEND_LOG("(SHOP) Rebuilding shops....");
  max_shops = *(const int32_t *)c;
  c+=4;
  const char *d = c;
  size_t reqsize  = 0;
  size_t products = 0;
  for(i=0;i<max_shops;i++) {
      TSHOP s;
      d = load_TSHOP(d, &s);
      reqsize += sizeof(TSHOP)+sizeof(TSHOP *); //shop and its state + pointer to it
      for (int j = 0; j < s.products; ++j) {
          TPRODUCT p;
          d = load_TPRODUCT(d, &p);
          reqsize += sizeof(TPRODUCT) + sizeof(int32_t);    //each product one state
          ++products;
      }
  }
  SEND_LOG("(SHOP) Loaded total %d shops, %lu products, %lu bytes total memory", max_shops, products, reqsize);
  void *newhacek = getmem(reqsize);
  shop_list = (TSHOP **)newhacek;
  TSHOP *shop_iter = (TSHOP *)(shop_list+max_shops);
  TPRODUCT *prod_iter = (TPRODUCT *)(shop_iter+max_shops);
  shop_all_state.first_product = prod_iter;
  shop_all_state.count_states = products;
  shop_all_state.first_state = (TSHOP_PRODUCT_STATE *)(prod_iter+products);
  TSHOP_PRODUCT_STATE *state_iter = shop_all_state.first_state;

  TSTRINGTABLE *stbl = lang_load("shops.dat");

  for(i=0;i<max_shops;i++) {
      shop_list[i] = shop_iter;
      c = load_TSHOP(c, shop_iter);
      if (stbl) {
        const char *n = stringtable_find(stbl,shop_iter->shop_id,NULL);
        if (n) {
          strcopy_n(shop_iter->keeper,n,sizeof(shop_iter->keeper));
        }
      }
      shop_iter->list = prod_iter;
      for (int j = 0; j < shop_iter->products; ++j) {
          c = load_TPRODUCT(c, prod_iter);
          state_iter->count = prod_iter->pocet;
          state_iter->previous_price = 0;
          ++prod_iter;
          ++state_iter;
      }
      ++shop_iter;
      SEND_LOG("(SHOP) Shop found: '%s', products %d",shop_list[i]->keeper,shop_list[i]->products);
  }
  stringtable_free(stbl);
  free(shop_hacek);
  shop_hacek = newhacek;
  }

void load_shops(void)
  {

  if (!test_file_exist(SR_MAP,SHOP_NAME))
     {
     shop_hacek=NULL;
     shop_list=NULL;
     return;
     }
  int32_t shop_hacek_size=0; //toto je jeho delka
  const void *sh=afile(SHOP_NAME,SR_MAP,&shop_hacek_size);
  rebuild_shops(sh);
  ablock_free(sh);
  }

static uint16_t *get_product_count(const TPRODUCT *p) {
    int32_t index = p - shop_all_state.first_product;
    assert(index >= 0 && index < (int32_t)shop_all_state.count_states);
    return &shop_all_state.first_state[index].count;
}

static uint16_t *get_last_haggle_price(const TPRODUCT *p) {
    int32_t index = p - shop_all_state.first_product;
    assert(index >= 0 && index < (int32_t)shop_all_state.count_states);
    return &shop_all_state.first_state[index].previous_price;
}

static void rebuild_keepers_items()
  {
  int i;
  const TPRODUCT *p;
  int remain;
  char c;

  memset(shp_item_map,0,sizeof(shp_item_map));
  memset(shp_item_pos,0,sizeof(shp_item_map));
  do
     {
     remain=cur_shop->list_size-top_item;
     p=cur_shop->list+top_item;
     for(i=0;i<8 && remain>0;remain--,p++)
        if (p->trade_flags & SHP_SELL && *get_product_count(p))
           {
           shp_item_pos[i]=p-cur_shop->list;
           shp_item_map[i++]=p->item+1;
           }
     c=(i<8 && top_item);
     if (c) top_item--;
     }
  while (c);
  }

static int make_offer(int i)
  {
  int j;
  i--;
  for(j=0;j<cur_shop->list_size;j++)
     {
     const TPRODUCT *p=cur_shop->list+j;
     if (p->item==i && p->trade_flags & SHP_BUY) return p->cena-(p->cena*cur_shop->koef/100);
     }
  return 0;
  }

static int get_sell_price(int i) //cislo predmetu
  {
  int j;
  i--;
  for(j=0;j<cur_shop->list_size;j++)
     {
     const TPRODUCT *p=cur_shop->list+j;
     if (p->item==i && p->trade_flags & SHP_SELL && *get_product_count(p)>0) return p->cena+(p->cena*cur_shop->koef/100);
     }
  return 0;
  }

static const TPRODUCT *find_sell_product(int i) //cislo predmetu
  {
  int j;
  i--;
  for(j=0;j<cur_shop->list_size;j++)
     {
     const TPRODUCT *p=cur_shop->list+j;
     if (p->item==i) return p;
     }
  return NULL;
  }


static void sell_item(int i)
  {
  int j;
  i--;
  for(j=0;j<cur_shop->list_size;j++)
     {
     const TPRODUCT *p=cur_shop->list+j;
     if (p->item==i)
        {
         (*get_product_count(p))--;
        return;
        }
     }
  }

static void buy_item(int i)
  {
  int j;
  i--;
  for(j=0;j<cur_shop->list_size;j++)
     {
     const TPRODUCT *p=cur_shop->list+j;
     if (p->item==i)
        {
         (*get_product_count(p))++;
        return;
        }
     }
  }

static void display_keepers_items()
  {
  int x,y,i;
  put_picture(BUYBOX_X,BUYBOX_Y,ablock(H_SHOP_PIC));
  i=0;
  for(y=0;y<2*SHP_ICSIZY;y+=SHP_ICSIZY)
     for(x=0;x<4*SHP_ICSIZX;x+=SHP_ICSIZX)
       if (shp_item_map[i])
           {
           int ikn;

           ikn=glob_items[shp_item_map[i++]-1].ikona;
           put_picture(BUYBOX_X+SHP_ICPLCX+x,BUYBOX_Y+SHP_ICPLCY+y,ico_extract(ikn));
           }
        else
           i++;
  set_font(H_FBOLD,INV_NAME_COL);
  set_aligned_position(135+BUYBOX_X,17+BUYBOX_Y,1,1,cur_shop->keeper);
  outtext(cur_shop->keeper);
  }

static const char *shop_keeper_bubble=NULL;

static void show_buble(int x, int y, int xs, const char *text) {
    set_font(H_FTINY, NOSHADOW(0));
    int newxs;
    int newys;
    char *buffer = (char *)alloca(strlen(text)+3);
    zalamovani(text, buffer, xs-10, &newxs, &newys);
    int lxs = newxs+10;
    int lys = newys+10;
    y-=newys+10;
    x+=(xs-lxs)/2;
     draw_rounded_rectangle(x,y,lxs, lys, 8,RGB888(0,0,0),RGB888(255,255,255));
    while (*buffer) {
        position(x+5,y+5);outtext(buffer);
        y+= text_height(buffer);
        buffer = strchr(buffer,0)+1;
    }
}

static void redraw_shop()
  {
  update_mysky();
  schovej_mysku();
  curcolor=0;
  display_items_in_inv(human_selected);
  display_keepers_items();
  //write_shopkeeper_name(cur_shop->keeper);
  other_draw();
  info_box_drawed=0;
  if (info_box_below!=NULL) free(info_box_below);
  info_box_below=NULL;
  if (shop_keeper_picture) put_picture(5,SCREEN_OFFLINE,shop_keeper_picture);
  ms_last_event.event_type=0x1;send_message(E_MOUSE,&ms_last_event);
  if (shop_keeper_bubble) {
      show_buble(5,SCREEN_OFFLINE+((word *)shop_keeper_picture)[1],
                 ((word *)shop_keeper_picture)[0], shop_keeper_bubble);
  }
  shop_keeper_bubble=NULL;
  ukaz_mysku();
  showview(0,0,0,0);
  }



static void block_next()
  {
  const TPRODUCT *p=cur_shop->list+top_item;
  int remain=cur_shop->list_size-top_item;
  int i,j;

  if (remain<8) return;
  j=top_item;
  for(i=0;i<8 && remain>0;j++,remain--,p++) if (*get_product_count(p) && p->trade_flags & SHP_SELL) i++;
  if (i!=8 || !remain) return;
  top_item=j;
  }

static void block_back()
  {
  const TPRODUCT *p=cur_shop->list+top_item;
  int i,j;


  if (top_item<8) top_item=0;
  else
     {
     j=top_item;
     for(i=0;i<8 && j>0;j--,p--) if (*get_product_count(p) && p->trade_flags & SHP_SELL) i++;
     if (i==8) top_item=j; else top_item=0;
     }
  return;
  }

static void redraw_keepers_items()
  {
    const word *w;
        schovej_mysku();
        w=ablock(H_SHOP_PIC);
        display_keepers_items();
        ukaz_mysku();
        showview(BUYBOX_X,BUYBOX_Y,w[0],w[1]);
  }



static char shop_keeper_action(int xr, int yr, char fast_trade);
char shop_keeper_click(int id, int xa, int ya, int xr, int yr) {
    id;
    xr;
    yr;
    return shop_keeper_action(xa, ya, get_control_key_state());
}
static char shop_keeper_action(int xa, int ya, char fast_trade) {
    int xr = (xa - BUYBOX_X - SHP_ICPLCX);
    int yr = (ya - BUYBOX_Y - SHP_ICPLCY);
    if (picked_item == NULL) {
        int i, j;
        if (xr < 0 || yr < 0)
            return 0;
        xr /= SHP_ICSIZX;
        yr /= SHP_ICSIZY;
        i = yr * 4 + xr;
        if (i < 8 && i >= 0 && (j = shp_item_map[i]) != 0) {
            picked_item = NewArr(short, 2);
            picked_item[0] = shp_item_map[i];
            picked_item[1] = 0;
            shp_item_map[i] = 0;
            cur_owner = -1;
            schovej_mysku();
            pick_set_cursor();
            redraw_keepers_items();
            ukaz_mysku();
            update_mysky();
            if (fast_trade && get_sell_price(*picked_item) <= money) {
                play_sample_at_channel(H_SND_OBCHOD, 1, 100);
                money -= get_sell_price(*picked_item);
                sell_item(*picked_item);
                if (put_item_to_inv(human_selected, picked_item)) {
                    picked_item = NULL;
                    pick_set_cursor();
                }
                rebuild_keepers_items();
                cur_owner = picked_item != NULL;
                redraw_shop();
            }
            return 1;
        }
    } else if (cur_owner == -1) {
        free(picked_item);
        picked_item = NULL;
        rebuild_keepers_items();
        schovej_mysku();
        pick_set_cursor();
        redraw_keepers_items();
        ukaz_mysku();
        update_mysky();
        cur_owner = 0;
        return 1;
    }
    if (cur_owner != -1 && picked_item != NULL) {
        int price, z;
        char c[200];
        mouse_set_cursor(H_MS_DEFAULT);
        if (picked_item[1] != 0) {
            message(1, 0, 0, "", texty[100], texty[80]);
            wire_shop();
        } else {
            price = make_offer(z = picked_item[0]);
            if (!price) {
                sprintf(c, texty[103], glob_items[z - 1].jmeno);
                message(1, 0, 1, "", c, texty[80]);
                wire_shop();
            } else {
                int p;
                const TPRODUCT *pp;

                pp = find_sell_product(z);
                sprintf(c, texty[102], price);
                p = message(3, 0, 2, texty[118], c, texty[77], texty[230],
                        texty[78]);
                if (p == 2) {
                    price = -1;
                }
                if (p == 1) {
                    redraw_shop();
                    THAGGLERESULT hr = smlouvat_dlg(price, pp->cena,
                            *get_product_count(pp), *get_last_haggle_price(pp), money, 0);
                    if (hr.canceled) {
                        price = -1;
                    } else if (hr.message) {
                        price = -1;
                        shop_keeper_bubble = hr.message;
                        *get_last_haggle_price(pp) = hr.hprice;
                    } else {
                        price = hr.hprice;
                    }
                }
                if (price >= 0) {
                    *get_last_haggle_price(pp) = 0;
                    play_sample_at_channel(H_SND_OBCHOD, 1, 100);
                    buy_item(z);
                    free(picked_item);
                    picked_item = NULL;
                    money += price;
                    rebuild_keepers_items();
                }
                wire_shop();
            }
        }
        pick_set_cursor();
        update_mysky();
        return 1;
    }
    return 0;
}


static char shop_bag_action(int xr,int yr, char fast_trade);
char shop_bag_click(int id,int xa,int ya,int xr,int yr) {
    id;xa;ya;
    return shop_bag_action(xr, yr, get_control_key_state());
}
static char shop_bag_action(int xr,int yr, char fast_trade)
  {
  char s[200],p;
  int price,z;
  const TPRODUCT *pp;
  if (cur_owner>-1)
     {
     char id=bag_action(xr,yr);
     cur_owner=picked_item!=NULL;
	 if (picked_item!=NULL && picked_item[1]==0 && fast_trade)
	   {
	   short z;
       price=make_offer(z=picked_item[0]);
	   if (price)
		 {
		 play_sample_at_channel(H_SND_OBCHOD,1,100);
		 buy_item(z);
		 free(picked_item);picked_item=NULL;
		 money+=price;
		 rebuild_keepers_items();
		 pick_set_cursor();
		 redraw_shop();
		 }
	   }
     return id;
     }
  if (picked_item==NULL) return 0;
  z=picked_item[0];
  price=get_sell_price(z);
  pp=find_sell_product(z);
  if (pp==NULL) return 0;
  mouse_set_cursor(H_MS_DEFAULT);
  if (!price) return 0;
    if (price > money) {
        p = message(2, 0, 1, "", texty[104], texty[230], texty[78]) + 1;
    } else {
        sprintf(s, texty[101], price);
        p = message(3, 0, 2, texty[118], s, texty[77], texty[230], texty[78]);
    }
    if (p == 1) {
        redraw_shop();
        THAGGLERESULT hr = smlouvat_dlg(price, pp->cena, *get_product_count(pp),
                *get_last_haggle_price(pp), money, 1);
        if (hr.canceled) {
            price = -1;
        } else if (hr.message) {
            price = -1;
            shop_keeper_bubble = hr.message;
            *get_last_haggle_price(pp) = hr.hprice;
        } else {
            price = hr.hprice;
        }
    } else if (p == 2) {
        price = -1;
    }
    if (price >= 0) {
        play_sample_at_channel(H_SND_OBCHOD, 1, 100);
        money -= price;
        sell_item(z);
        rebuild_keepers_items();
        bag_action(xr, yr);
        cur_owner = picked_item != NULL;
    } else {
        shop_keeper_click(0, 0, 0, 0, 0);
    }
    wire_shop();
    return 1;
}

char shop_block_click(int id, int xa, int ya,int xr,int yr)
  {
  xa,ya,xr,yr;
  if (id==1) block_back();else block_next();
  rebuild_keepers_items();
  redraw_keepers_items();
  return 1;
  }

static void fast_trade_click() {
    int x = ms_last_event.x;
    int y = ms_last_event.y;
    const T_CLK_MAP *item = find_in_click_map_entry(x, y, clk_shop, CLK_SHOP, MS_EVENT_MOUSE_LPRESS);
    if (item) {
        if (item->proc == &shop_bag_click) {
            shop_bag_action(x-item->xlu, y-item->ylu, 1);
        } else if (item->proc == &shop_keeper_click) {
            shop_keeper_action(x, y, 1);
        }
    }
}

static void shop_keyboard_proc(EVENT_MSG *msg, void **_) {
    if (msg->msg == E_KEYBOARD) {
        int c = quit_request_as_escape(va_arg(msg->data,int));
        switch(c>>8) {
            case 1: _exit_shop(0,0,0,0,0);break;
            case 57: fast_trade_click();break;
            case 32:
            case 'M':shop_block_click(2,0,0,0,0);break;
            case 30:
            case 'K':shop_block_click(1,0,0,0,0);break;
            default:break;
        }
    }
}


static int old_inv_view_mode;

void unwire_shop(void)
  {
  send_message(E_DONE,E_MOUSE,shop_mouse_event);
  send_message(E_DONE,E_KEYBOARD, shop_keyboard_proc);
  norefresh=0;
  wire_proc=wire_shop;
  inv_view_mode=old_inv_view_mode;
  }

void wire_shop(void)
  {
  int32_t size;
  static TSHOP *last_shop=NULL;
  mute_all_tracks(0);
  old_inv_view_mode=inv_view_mode;
  inv_view_mode=0;
  inv_redraw=redraw_shop;
  schovej_mysku();
  if (last_shop!=cur_shop)
     {
     ablock_free(shop_keeper_picture);
     shop_keeper_picture=afile(cur_shop->picture,SR_DIALOGS,&size);
     last_shop=cur_shop;
     }
  send_message(E_ADD,E_MOUSE,shop_mouse_event);
  send_message(E_ADD,E_KEYBOARD, shop_keyboard_proc);
  unwire_proc=unwire_shop;
  change_click_map(clk_shop,CLK_SHOP);
  if (shop_sector==viewsector) redraw_shop();else _exit_shop(0,0,0,0,0);
  ukaz_mysku();
  update_mysky();
  }

void enter_shop(int shopid)
  {
  int i;

  SEND_LOG("(SHOP) Entering shop...");
  for(i=0;i<POCET_POSTAV;i++) if (isdemon(postavy+i)) unaffect_demon(i);
  for(i=0;i<POCET_POSTAV && postavy[i].sektor!=viewsector;i++);
  if (i==POCET_POSTAV) return; //nesmysl, nemelo by nikdy nastat.
  if (battle) return;
  select_player=i;
  inv_view_mode=0;
  human_selected=postavy+i;
  top_item=0;cur_owner=(picked_item!=NULL);
  for(i=0;i<max_shops;i++) if (shop_list[i]->shop_id==shopid) break;
  if (i==max_shops) return;
  unwire_proc();
  cur_shop=shop_list[i];
  curcolor=0;
  bar32(0,0,639,479);
  rebuild_keepers_items();
  bott_draw(1);
  shop_sector=viewsector;
  wire_shop();
  cancel_pass=1;
  norefresh=1;
  cur_mode=MD_SHOP;
  }

  word *xs;

char shop_change_player(int id, int xa, int ya,int xr,int yr)
  {
    const word *xs;
  int i;

  id;xa;ya;yr;
  xs=ablock(H_OKNO);
  i=xr/xs[0];
  if (i<POCET_POSTAV)
     {
     THUMAN *p;
     i=group_sort[i];
     p=&postavy[i];

     if (p->used && p->sektor==viewsector && p->inmaphash == current_map_hash) {
        if (ms_last_event.event_type & 0x2)
           {
           int j=select_player;
           select_player=i;
           human_selected=p;
           if (i==j && cur_owner>-1)
              {
              unwire_proc();
              wire_inv_mode(p);
              }
           else
              {
              bott_draw(1);
              redraw_shop();
              }
           }
        else if (picked_item!=NULL && cur_owner>-1)
           {
           if (put_item_to_inv(p,picked_item))
			 {
  			 free(picked_item);picked_item=NULL;
			 pick_set_cursor();
			 redraw_shop();
			 }
           }
        else if (picked_item==NULL) _exit_shop(id,xa,ya,xr,yr);

     }
     return 1;
     }
  return 0;
  }

char _exit_shop(int id, int xa, int ya,int xr,int yr)
  {
  xr,yr,xa,ya,id;
  SEND_LOG("(SHOP) Exiting shop...");
  if (cur_owner==-1)
     {
     free(picked_item);
     picked_item=NULL;
     pick_set_cursor();
     }
  unwire_proc();
  wire_main_functs();
  return 1;
  }


static void reroll_shop(TSHOP *p)
  {
  int i,j,r;
  int poc_spec=0;
  const TPRODUCT *pr;

  SEND_LOG("(SHOP) Shops reroll: '%s' ",p->keeper);
  pr=p->list;
  for(i=0;i<p->list_size;i++,pr++)
     {
      uint16_t *count = get_product_count(pr);
     if (pr->trade_flags & SHP_AUTOADD) (*count)++;
     if ((pr->trade_flags & SHP_SELL) == 0) *count = 0;
     if (*count > pr->max_pocet) *count = pr->max_pocet;
     if (pr->trade_flags & SHP_SPECIAL)
        {
        poc_spec++;
        *count = 0;
        }
     }
  pr=p->list;
  for(i=0;i<p->spec_max;i++)
     {
     int i=0;
     r=rnd(poc_spec)+1;
     for(j=0;i<r;j++) if (pr[j].trade_flags & SHP_SPECIAL) i++;
     j--;
     const TPRODUCT *sel = pr+j;
     int maxp = sel->max_pocet;
     if (maxp) {
         *get_product_count(pr+j)=rnd(maxp)+1;
     } else {
         *get_product_count(pr+j) = 0;
     }
     }
  }

void reroll_all_shops()
  {
  int i;
  for(i=0;i<max_shops;i++) reroll_shop(shop_list[i]);
  }

char save_shops()
  {

  SEND_LOG("(SHOP) Saving shops...");
  if (shop_all_state.count_states == 0) {
      temp_storage_delete(_SHOP_ST);
      return 0;
  }
  temp_storage_store(_SHOP_ST, shop_all_state.first_state, shop_all_state.count_states*sizeof(*shop_all_state.first_state));
  return 0;
  }


char load_saved_shops()
  {
  SEND_LOG("(SHOP) Loading saved shops...");
  int32_t sz = temp_storage_find(_SHOP_ST);
  int32_t needsz = shop_all_state.count_states*(sizeof(*shop_all_state.first_state));
  if (sz != needsz) return 0;
  temp_storage_retrieve(_SHOP_ST, shop_all_state.first_state, sz);
  return 0;
  }
