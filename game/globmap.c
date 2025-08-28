#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>


#include <libs/types.h>
#include <libs/event.h>
#include <libs/memman.h>
#include <ctype.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/bgraph.h>
#include <platform/sound.h>
#include <libs/strlite.h>
#include "engine1.h"
#include <libs/pcx.h>
#include "globals.h"

#include <string.h>
#define GLOBMAP "GLOBMAP.DAT"

#define ODDELOVACE ";:=,\n\r{}"
#define OD_COMMAND oddelovace[0]
#define OD_CRIT    oddelovace[1]
#define OD_SET     oddelovace[2]
#define OD_COMMA   oddelovace[3]
#define OD_NEWLINE oddelovace[4]
#define OD_CR      oddelovace[5]
#define OD_IN      oddelovace[6]
#define OD_OUT     oddelovace[7]
#define OD_COMMENT '`'

#define INDEXU   256

typedef struct index_def
   {
   char mapname[13];
   char *text;
   char defined;
   }INDEX_DEF;

static INDEX_DEF *index_tab=NULL;
static uint8_t last_index=1;

static int usemap;

static TMPFILE_RD *glbm;
static char oddelovace[]=ODDELOVACE;
static int last_oddelovac;
static int linecounter=0;
static int enter_sector=0;
static char *symbolmap[]=
  {
  "A !",
  "B &&",
  "D BREAK",
  "O CURMAP",  // krit
  "E DRAW",
//  "F ESCAPE",
  "G FLG",    // krit
  "P IFDEF",  // krit
  "I INDX",
  "I INDEX",
  "K MAP",
  "Q SEKTOR", // krit
  "Q SECTOR", // krit
  "L TEXT",
  "M UNDEF",
  "N USEMAP",
  "C ||",
  };

#define OP_NOT 'A'
#define OP_AND 'B'
#define OP_OR  'C'
#define OP_INDX 'I'
#define OP_DRAW  'E'
#define OP_TEXT 'L'
#define OP_MAP  'K'
#define OP_UNDEF 'M'
#define OP_FLG 'G'
#define OP_USEMAP 'N'
#define OP_BREAK 'D'
//#define OP_ESCAPE 'F'
#define OP_CURMAP 'O'
#define OP_ISDEF 'P'
#define OP_SEKTOR 'Q'

#define ODD last_oddelovac

static char cti_int_num(int *readed)
  {
  //return !fscanf(glbm,"%d",readed);
    return temp_storage_scanf(glbm, "%d", readed)<1;
  }

static int find_symbol(char **symbolmap,int list_len,int offset,char *symbol)
  {
  int start_pos;
  char chr=0;

  start_pos=0;
  while (start_pos<list_len && *symbol)
     {
     char *ss=symbolmap[start_pos];
     if (ss[offset]==*symbol)
        {
        chr=*symbol;
        symbol++;offset++;continue;
        }
     if (ss[offset]>*symbol) return -1;
     if (chr && ss[offset-1]!=chr) return -1;
     start_pos++;
     }
  if (*symbol) return -1;
  return start_pos;
  }

static char get_symbol(char *symb)
  {
  int i;

  i=find_symbol(symbolmap,sizeof(symbolmap)/sizeof(char *),2,symb);
  if (i==-1) return 0;else return symbolmap[i][0];
  }

static int cti_oddelovac(void)//cte prvni oddelovac - preskakuje mezery
  {
  int c;
  do
     {
     c=temp_storage_getc(glbm);
     if (c==OD_COMMENT) while((c=temp_storage_getc(glbm))!='\n');
     if (c=='\n') linecounter++;
     if (strchr(oddelovace,c)!=NULL || c==EOF) return c;
     }
  while (c==' ' || c=='\x9');
  temp_storage_ungetc(glbm);
  return 0;
  }

static int cti_retezec(int znaku,char *text,char mezera,char upcase)
  {
  int c;

  znaku--;
  if (mezera)
     {
     while((c=temp_storage_getc(glbm))==32 || c==9);
     }
  else c=temp_storage_getc(glbm);
  while (strchr(oddelovace,c)==NULL && ((c!=32 && c!=9) || !mezera) && c!=EOF)
     {
     if (upcase) c=toupper(c);
     if (znaku)
        {
        *text++=c;
        znaku--;
        }
     c=temp_storage_getc(glbm);
     }
  if (c!=32 && c!=9) temp_storage_ungetc(glbm);
  *text=0;
  return 0;
  }

static void error(const char *text)
  {
  char popis[300];

  sprintf(popis,"Chyba v souboru "GLOBMAP" na radce %d.\r\n%s",linecounter,text);
  SEND_LOG("(ERROR) %s : %s",popis,text);

  display_error(popis);
  exit(0);
  }

static void ex_error(char znak)
  {
  char ex_text[100];

  sprintf(ex_text,"Ocekava se znak '%c'",znak);
  error(ex_text);
  }

static void ready_index_tab(void)
  {
  index_tab=NewArr(INDEX_DEF,INDEXU);
  memset(index_tab,0,INDEXU*sizeof(INDEX_DEF));
  }

static void purge_index_tab(void)
  {
  if (index_tab)
    {
    int i;
    for(i=0;i<255;i++) if (index_tab[i].text!=NULL) free(index_tab[i].text);
    free(index_tab);
    index_tab=NULL;
    }
  }

static char test_kriterii(void)
  {
  char last_op=0;
  char not_op=0;
  char text[128];
  char hodn = 0;
  char vysl=0;
  char symb,*c;

  while (ODD==0 || ODD==OD_NEWLINE)
     {
     cti_retezec(100,text,1,1);
     c=text;if (*c=='!') not_op=1,c++;
     symb=get_symbol(c);
     switch (symb)
        {
        case OP_AND:last_op=1;break;
        case OP_NOT:not_op=!not_op;break;
        case OP_OR:last_op=0;break;
        case OP_SEKTOR:
                      {
                      int c;
                      if (cti_int_num(&c)) error("O�ek�v� se ��slo");
                      hodn=c==enter_sector;
                      }
                      break;
        case OP_CURMAP:cti_retezec(100,text,1,1);
                       hodn=!strcmp(level_fname,text);
                       break;
        case OP_ISDEF:{
                       int c;
                       if (cti_int_num(&c)) error("O�ek�v� se ��slo");
                       hodn=index_tab[c].defined;
                      }
                      break;
        case OP_FLG:
                {
                int flag_num;

                if (cti_int_num(&flag_num)) error("Za FLG mus� b�t ��slo!");
                if (flag_num>255) error("��slo vlajky (FLG) mus� b�t v rozsahu 0-255!");
                hodn=(test_flag(flag_num)!=0);
                }
                break;
        default:
                {
                    hodn=temp_storage_find(concat2(text,".map"))>=0;
/*                char c[200];
                sprintf(c,"%s.TMP",text);
                hodn=!check_file_exists(c);*/
                }
              break;
        }
     hodn^=not_op;not_op=0;
     if (last_op) vysl&=hodn;else vysl|=hodn;
     ODD=cti_oddelovac();
     }
  return vysl;
  }

static char proved_prikaz()
  {
  char prikaz[20];
  char text[128];
  int c;
  char op;

  ODD=cti_oddelovac();
  while (ODD==OD_NEWLINE || ODD==OD_CR) ODD=cti_oddelovac(); //preskoc prazdne radky
  if (ODD==OD_IN) return 0;  //cti znak {
  do
     {
     while (ODD==OD_NEWLINE || ODD==OD_CR || ODD==OD_COMMAND) ODD=cti_oddelovac();
     if (ODD!=0) error("O�ek�v� se jm�no definice (p��klad: INDX=)");
     cti_retezec(20,prikaz,1,1);
     op=get_symbol(prikaz);
     if (op==OP_BREAK) return 1;
     ODD=cti_oddelovac();
     if (ODD!=OD_SET) ex_error(OD_SET);
     switch(op)
        {
        case OP_INDX:
                if (cti_int_num(&c)) error("INDX=?");
                if (c<0 || c>255) error("INDX=<0,255>");
                index_tab[last_index=c].defined=1;
                break;
        case OP_TEXT:
                cti_retezec(128,text,0,0);
                index_tab[last_index].text=NewArr(char,strlen(text)+1);
                strcpy(index_tab[last_index].text,text);
                break;
        case OP_MAP:
                cti_retezec(13,index_tab[last_index].mapname,1,1);
                break;
        case OP_DRAW:
                {
                char file[20];
                int xp,yp;
                int h;

                cti_retezec(20,file,1,1);
                ODD=cti_oddelovac();if (ODD!=OD_COMMA)ex_error(OD_COMMA);
                if (cti_int_num(&xp)) error("O�ek�v� se ��slo xp");
                ODD=cti_oddelovac();if (ODD!=OD_COMMA)ex_error(OD_COMMA);
                if (cti_int_num(&yp)) error("O�ek�v� se ��slo yp");
                h=find_handle(file,pcx_8bit_decomp);
                if (h==-1) def_handle(h=end_ptr++,file,pcx_8bit_decomp,SR_DIALOGS);
                put_picture(xp,yp+SCREEN_OFFLINE,ablock(h));
                }
                break;
        case OP_UNDEF:
                if (cti_int_num(&c)) error("UNDEF=?");
                if (c<0 || c>255) error("UNDEF=<0,255>");
                index_tab[c].defined=0;
                break;
        case OP_USEMAP:
                {
                char file[20];
                cti_retezec(20,file,1,1);
                usemap=find_handle(file,pcx_8bit_nopal);
                if (usemap==-1) def_handle(usemap=end_ptr++,file,pcx_8bit_nopal,SR_DIALOGS);
                }
                break;
/*        case OP_ESCAPE:
                {
                cti_retezec(20,prikaz,1,1);
                temp_storage_close_rd(glbm);
                const char *s = build_pathname(2, gpathtable[SR_MAP], prikaz);
                if ((glbm=fopen_icase(s,"r"))==NULL) error(s);
                return 0;
                }*/
        default:error(prikaz);
        }
     ODD=cti_oddelovac();
     if (ODD!=OD_COMMAND && ODD!=OD_NEWLINE && ODD!=OD_CR && ODD!=EOF) ex_error(OD_COMMAND);
     }
  while (ODD!=OD_NEWLINE && ODD !=OD_CR && ODD!=EOF);
  return 0;
  }

static void preskoc_prikaz(void)
  {
  char ending=0;
  int uroven=0;
  char last;
  char text;

  do
     {
     last=ODD;
     ODD=cti_oddelovac();
     switch (ODD)
        {
        case 0:cti_retezec(1,&text,0,0);ending=1;break;
        case '\r':continue;
        case '\n':if (ending && uroven==0) return;break;
        case EOF: if (uroven!=0)ex_error(OD_OUT);return;break;
        case '{': if (last==OD_CRIT || last==OD_NEWLINE) uroven++;break;
        case '}': if (last==OD_NEWLINE) uroven--; if (uroven<0) ex_error(OD_IN);
                  else if (uroven==0)return;
        break;
        }
     if (ODD!=0) ending=0;
     }
  while(1);
  }

static void do_script(void)
  {

  char vysledek;

  const char *script = (const char *)ablock(H_GLOBMAP);
  glbm=temp_storage_from_string(script);
/*

	const char *s=build_pathname(2,gpathtable[SR_MAP], GLOBMAP);
  linecounter=0;
  glbm=fopen_icase(s,"r");
  if (glbm==NULL) error("Chyb� uveden� soubor...");
  */
  ODD=cti_oddelovac();
  do
    {
    if (ODD==0) //existuji kriteria
       {
       vysledek=test_kriterii();
       }
     else vysledek=1;
    if (ODD==OD_CRIT)  //oddelovac kriterii
       {
       char c=0;
       if (vysledek)c=proved_prikaz();else preskoc_prikaz();
       if (c) break;
       }
    ODD=cti_oddelovac();
    }
  while(ODD!=EOF);
  temp_storage_close_rd(glbm);
  }


/*
static char flp_validate2(word sector)
  {
  TMOB *m;
  char c;

  if (mob_map[sector])
     {
     m=mobs+mob_map[sector]-1;
     if (m->vlajky & ~MOB_PASSABLE) return 0;
     if (!m->next)
        if (mobs[m->next-1].vlajky & ~MOB_PASSABLE) return 0;
     }
  c=map_sectors[sector].sector_type;
  if (c==S_DIRA || ISTELEPORT(c) || c==S_LAVA || c==S_VODA ) return 0;
  return 1;
  }
*/
static char flp_validate(word sector, void *ctx)
  {
  TMOB *m;
  int *found_place =  (int *)ctx;
  char c;

  if (*found_place) return 0;
  if (mob_map[sector])
     {
     m=mobs+mob_map[sector]-1;
     if (m->vlajky & ~MOB_PASSABLE) return 0;
     if (m->next)
        if (mobs[m->next-1].vlajky & ~MOB_PASSABLE) return 0;
     }
  c=map_sectors[sector].sector_type;
  if (~map_coord[sector].flags & 1) return 0;
  if (c==S_DIRA || ISTELEPORT(c) || c==S_LAVA || c==S_VODA ) return 0;
  if (c==S_LEAVE && !*found_place) *found_place=sector;
  return 1;
  }


static int find_leave_place(int sector)
  {
  int found_place=0;
  if (map_sectors[sector].sector_type==S_LEAVE) return sector;
  labyrinth_find_path(sector,65535,(SD_PLAY_IMPS | SD_SECRET),flp_validate,NULL, &found_place);
  return found_place;
  }



static int select_mode = 0;


static char load_index_map(int index)
  {
  TMA_LOADLEV x;
  int lv;


  if (select_mode)
     {
     char *a;

     a=alloca(strlen(index_tab[index].mapname)+1);strcpy(a,index_tab[index].mapname);
     wire_automap_file(a);
     return 1;
     }
  if (!strcmp(index_tab[last_index].mapname,level_fname)) return 0;
  group_all();
  lv=find_leave_place(viewsector);
  if (lv<1)
     {
     bott_disp_text(texty[121]);
     return 0;
     }
/*  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++)
    if (h->used && h->lives)
      if (h->sektor!=lv && !labyrinth_find_path(h->sektor,lv,(SD_PLAY_IMPS | SD_SECRET),flp_validate2,NULL))
        {
        char c[20];
        bott_disp_text(int2ascii(i,c,10));
        return 0;
        }
   */
  if (!GlobEvent(MAGLOB_LEAVEMAP,viewsector,viewdir)) return 0;
  viewsector=lv;
  strcopy_n(x.name,index_tab[index].mapname,sizeof(x.name));
  x.start_pos=0;
  x.dir=0;
  macro_load_another_map(&x);
  return 0;
  }

static char *fly_text;
static int fly_x,fly_y,fly_xs,fly_ys;
static void *fly_background;

void global_map_point(EVENT_MSG *msg,void **_)
  {
  MS_EVENT *ms;

  if(msg->msg == E_INIT)
     {
     fly_background=NULL;last_index=0;fly_text=NULL;
     fly_x=0;fly_y=0;fly_xs=4;fly_ys=4;
     }
  if(msg->msg == E_MOUSE)
     {
     int x,y,i,xs,ys;
     const char *ptr;
     ms=get_mouse(msg);
     if (ms->event_type & 0x1)
        {
        x=ms->x;y=ms->y-SCREEN_OFFLINE;
        if (y<0 || y>359) return;
        else
           {
           alock(usemap);
           ptr=ablock(usemap);
           ptr+=640*y+x+6;
           i=*ptr;
           }
        y+=60;
        schovej_mysku();
        if (fly_background!=NULL)put_picture(fly_x,fly_y,fly_background);
        set_font(H_FTINY,RGB555(31,31,0));
        xs=fly_xs;ys=fly_ys;
        if (i!=last_index)
           {
           free(fly_background);
           last_index=i;
           if (index_tab[i].defined)
              {
              fly_text=index_tab[i].text;
              mouse_set_default(H_MS_ZARE);
              set_ms_finger(5,5);
              }
           else
              {
              fly_text=NULL;
              mouse_set_default(H_MS_DEFAULT);
              }

           if (fly_text!=NULL)
              {
              xs=text_width(fly_text)+4;
              ys=text_height(fly_text)+4;
              fly_background=NewArr(word,xs*ys*2+6);
              }
           else
              {
              fly_text=NULL;xs=4;ys=4;
              fly_background=NULL;
              }
           }
        if (fly_text!=NULL)
           {
           if ((x+xs)>639) x=639-xs;
           get_picture(x,y,xs,ys,fly_background);
           trans_bar(x,y,xs,ys,0);
           position(x+2,y+2);outtext(fly_text);
           }
        send_message(E_MOUSE,ms);
        ukaz_mysku();
        showview(fly_x,fly_y,fly_xs+1,fly_ys);
        showview(fly_x=x,fly_y=y,(fly_xs=xs)+1,fly_ys=ys);
        aunlock(usemap);
        }
     if (ms->event_type & 0x2 && ms->y>SCREEN_OFFLINE && ms->y<378)
        {
            if (last_index && index_tab[last_index].defined) {
                if (load_index_map(last_index)) {
                    return;
                }
            } else {
                return;
            }
        unwire_proc();
        wire_proc();
		 		msg->msg=-1;
        }
		 if (ms->event_type & 0x8)
				{
				unwire_proc();
				wire_proc();
		 		msg->msg=-1;
				}
     }
  if (msg->msg == E_DONE)
     {
     free(fly_background);
     }
  }

void unwire_global_map(void)
  {
  purge_index_tab();
  send_message(E_DONE,E_MOUSE,global_map_point);
  set_select_mode(0);
  pick_set_cursor();
  }


void wire_global_map(void)
  {
  unwire_proc();
  schovej_mysku();
  ready_index_tab();
  do_script();
  ukaz_mysku();
  showview(0,0,0,0);
  send_message(E_ADD,E_MOUSE,global_map_point);
  unwire_proc=unwire_global_map;
  change_click_map(NULL,0);
  }

static void (*old_wire_save)(void);
static void empty_unwire(void)
  {

  }

struct _tag_map_save_state{
    TSTENA *_map_sides;
    TSECTOR *_map_sectors;
    TMAP_EDIT_INFO *_map_coord;
    TSTR_LIST _map_desc;
    int _map_size;
    int _viewsector;
    int _viewdir;
    uint32_t _hash;
} MAP_SAVE_STATE;

static struct _tag_map_save_state save_state = {NULL,NULL,NULL,NULL,0,0,0,0};

static void save_current_map() {
    if (save_state._map_coord) {
        display_error("Already saved map state");
        abort();
    }
    save_state._map_sides = map_sides;
    save_state._map_coord = map_coord;
    save_state._map_sectors = map_sectors;
    save_state._map_size = mapsize;
    save_state._map_desc = swap_map_description(NULL);
    save_state._viewsector = viewsector;
    save_state._viewdir = viewdir;
    save_state._hash = current_map_hash;
    map_sides = NULL;
    map_coord = NULL;
    map_sectors = NULL;
    mapsize = 0;
}

static void restore_saved_map() {
    if (save_state._map_coord) {
        free(map_sides);
        free(map_sectors);
        free(map_coord);
        free_map_description();
        mapsize =save_state._map_size;
        map_sides = save_state._map_sides;
        map_sectors = save_state._map_sectors;
        map_coord = save_state._map_coord;
        viewsector = save_state._viewsector;
        viewdir = save_state._viewdir;
        current_map_hash = save_state._hash;
        swap_map_description(save_state._map_desc);
        save_state._map_sides = NULL;
        save_state._map_coord = NULL;
        save_state._map_sectors = NULL;
        save_state._map_desc = NULL;
    }
}

static void unwire_automap_file(void)
  {
    restore_saved_map();
  wire_proc=old_wire_save;
  build_player_map();
  bott_draw(0);
  wire_proc();
  }

void wire_automap_file(char *mapfile)
  {
  int c;
  if ((c=get_leaving_place(mapfile))==0) return;
  old_wire_save=wire_proc;

  save_current_map();

  viewsector=c;
  unwire_proc();
  unwire_proc=empty_unwire;
  wire_proc=unwire_automap_file;
  load_map_automap(mapfile);
  current_map_hash = fnv1a_hash(mapfile);
  cur_mode=MD_ANOTHER_MAP;
  build_player_map();
  show_automap(1);
  }

char set_select_mode(char mode)
  {
  char last=select_mode;
  select_mode=mode;
  return last;
  }

void cestovat()
  {

  }


