
#include <libs/types.h>
#include "engine1.h"
#include "temp_storage.h"
#include <platform/config.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <libs/memman.h>

#define POCET_POSTAV 6
#define HODINA 360

#define GAMESPEED 5
#define GAMESPEED_FASTBATTLE 2


#define MAX_FILESYSTEM_PATH 256

#define A_SIDEMAP 0x8001
#define A_SECTMAP 0x8002
#define A_STRMAIN 0x8003 //stena main
#define A_STRLEFT 0x8004 //stena leva
#define A_STRRIGHT 0x8005 //stena prava
#define A_STRCEIL 0x8006 //strop
#define A_STRFLOOR 0x8007 //podlaha
#define A_STRARC 0x8008 //oblouk
#define A_MAPINFO 0x8009
#define A_MAPGLOB 0x800A //globalni vlastnosti mapy
#define A_STRARC2 0x800B //oblouk pravy
#define A_MAPITEM 0x800C //pozice vsech predmetu;
#define A_MAPMACR 0x800D //makra
#define A_MAPMOBS 0x800F
#define A_MAPVYK  0x8010 //vyklenky
#define A_MOBS    0x8011 //Mobs
#define A_MOBSND  0x8012 //Mob sounds
#define A_PASSW   0x8013
#define A_MAPEND 0x8000

#define E_KROK   34   //udalost jenz se zavola pri kazdem kroku postav(y)
#define E_AUTOMAP_REDRAW 35
#define E_MENU_SELECT 36
#define E_CLOSE_MAP 37
#define E_CLOSE_GEN 38
#define E_HACKER 39

//side flags

#define SD_AUTOMAP     0x1
#define SD_PLAY_IMPS   0x2
#define SD_MONST_IMPS  0x4
#define SD_THING_IMPS  0x8
#define SD_SOUND_IMPS  0x10
#define SD_ALARM       0x20
#define SD_PASS_ACTION 0x40
#define SD_TRANSPARENT 0x80
#define SD_PRIM_ANIM   0x100
#define SD_PRIM_VIS    0x200
#define SD_PRIM_GAB    0x400
#define SD_PRIM_FORV   0x800
#define SD_SEC_ANIM    0x1000
#define SD_SEC_VIS     0x2000
#define SD_SEC_GAB     0x4000
#define SD_SEC_FORV    0x8000
#define SD_LEFT_ARC    0x10000
#define SD_RIGHT_ARC   0x20000
#define SD_DOUBLE_SIDE 0x40000
#define SD_SPEC        0x80000
#define SD_COPY_ACTION 0x100000
#define SD_SEND_ACTION 0x200000
#define SD_APPLY_2ND   0x400000
#define SD_AUTOANIM    0x800000
#define SD_SECRET      0x20000000
#define SD_TRUESEE     0x40000000
#define SD_INVIS       0x80000000

//extended side flags (oblouky)
#define SD_POSITION    0x60
#define SD_RECESS      0x10     //vyklenek
#define SD_ITPUSH      0x80     //vec lze zkrs tuto stenu polozit na dalsi sektor


//extra side flags  (direction)
#define SD_SHIFTUP     0x80     //presune sekundarni stenu dopredu
#define SD_UNUSED2     0x40     //nevyuzite (rezervovane) vlajky
#define SD_UNUSED3     0x20
#define SD_UNUSED4     0x10
#define SD_UNUSED5     0x08
#define SD_RESERVED    0x04     //rezervovano pro pripadnou expanzi SD_DIRECTION
#define SD_DIRECTION   0x03     //smer akce.

#define get_string(t) texty[t]
static __inline unsigned int rnd(unsigned int max) {
    if (max == 0) return 0;
    return rand() % max;
}


static __inline int rangrnd(int a, int b) {return rnd(b-a+1)+a;}

#define font_color(tabl) memcpy(charcolors,tabl,sizeof(charcolors));
#define tick_tack(num) game_time+=num;
#define TEXTY "POPISY.TXT"
#define ITEM_FILE "ITEMS.DAT"

#define XICHT_NAME "XICHT%02x.PCX"
#define CHAR_NAME "CHAR%02x.PCX"

#define STORY_BOOK "_STORY.TMP"

#define TX_LOAD 0

#define LODKA_POS (SCREEN_OFFLINE+301)*GetBuffer2ndPitch()+GetBuffer2nd()
#define LODKA_SIZ 640*60


#define VEL_RAMEC 12 //velikost zakladni jednotky ramecku (ctverec)
#define COL_RAMEC RGB555(31,31,26) //((31*32+31)*32+26)

#undef RGB
#define RGB(r,g,b) RGB888(r,g,b)
#define GET_R_COLOR(col) ((col & 0xF800)>>8)
#define GET_G_COLOR(col) ((col & 0x07E0)>>3)
#define GET_B_COLOR(col) ((col & 0x001F)<<3)

#define NOSHADOW(x) ((x)|BGSWITCHBIT)


#define PICTURES ".." PATH_SEPARATOR "OBRAZKY" PATH_SEPARATOR
#define PIC_FADE_PAL_SIZE (10*512+6)

#define E_REFRESH  256 //udalost refresh scene
#define E_KOUZLO_KOLO 257 //funkce kouzel kazde jedno kolo
#define E_KOUZLO_ANM 258 //funkce kouzel kazdy frame

//Registracni konstranty
#define H_DESK 0
#define H_TOPBAR 1
#define H_OKNO 2
#define H_KOMPAS 3
#define H_SIPKY_S 4
#define H_SIPKY_SV 8
#define H_SIPKY_SZ 9
#define H_SIPKY_V 5
#define H_SIPKY_Z 7
#define H_SIPKY_J 6
#define H_BACKMAP 10
#define H_IOBLOUK 11
#define H_IDESKA 12
#define H_IMRIZ1 13
#define H_RAMECEK 14
#define H_ENEMY 15
#define H_FBOLD 16
#define H_FSYMB 17
#define H_FLITT 18
#define H_FLITT5 19
#define H_FONT6 20
#define H_FONT7 21
#define H_FTINY  22
#define H_FKNIHA  23
#define H_KOUZLA     24
#define H_POWERBAR   25
#define H_POWERLED   26
#define H_LODKA      27
#define H_BATTLE_BAR 33
#define H_BATTLE_MASK 34
#define H_MZASAH1 35
#define H_MZASAH2 36
#define H_MZASAH3 37
#define H_PZASAH  38
#define H_BATTLE_CLICK 39
#define H_SIPKY_END 40
#define H_LEBKA 41
#define H_KOSTRA 42
#define H_KNIHA  43
#define H_NS_LOGO 44
#define H_WINTXTR 45
#define H_SAVELOAD 46
#define H_SVITEK 47
#define H_LOADTXTR 48
#define H_DIALOGY_DAT 49
#define H_DIALOG     50
#define H_MS_DEFAULT 51
#define H_MS_SOUBOJ  52
#define H_MS_WHO     53
#define H_MS_LIST    54
#define H_MS_ZARE    55
#define H_POSTAVY    60
#define H_BOTTBAR    66
#define H_RUNEHOLE   68
#define H_RUNEMASK   69
#define H_CHARS      70
#define H_CHARS_MAX  6
#define H_POSTAVY_DAT 76
#define H_SOUND_DAT  77
#define H_SND_SWHIT1   78
#define H_SND_SWHIT2   79
#define H_SND_SWMISS1   80
#define H_SND_SWMISS2   81
#define H_SND_SIP1 82
#define H_SND_SIP2 83
#define H_SND_KNIHA  84
#define H_SND_OBCHOD  85
#define H_SND_LEKTVAR 86
#define H_SND_TELEPIN 87
#define H_SND_TELEPOUT 88
#define H_SND_HEK1M   89
#define H_SND_HEK2M   90
#define H_SND_HEK1F   91
#define H_SND_HEK2F   92
#define H_SND_EAT     93
#define H_SND_WEAR    94
#define H_SND_PUTINV  95
#define H_RUNEBAR1    100
#define H_RUNEBAR2    101
#define H_RUNEBAR3    102
#define H_RUNEBAR4    103
#define H_RUNEBAR5    104
#define H_SPELLDEF    105
#define H_BUTBIG      106
#define H_BUTSMALL    107
#define H_TELEPORT    108
#define H_XICHTY     112
#define H_XICHTY_MAX 6
#define H_DIALOG_PIC 124
#define H_SHOP_PIC 125
#define H_SPELL_ANM 127
#define H_SPELL_WAV 128
#define H_TELEP_PCX 129
#define H_TELEP_CNT 14
#define H_FX 143
#define H_KILL 144
#define H_KILL_MAX 10
#define H_CHECKBOX 154
#define H_SETUPBAR 155
#define H_SOUPAK   156
#define H_SETUPOK  157
#define H_POSTUP   158
#define H_LODKA0   159
#define H_LODKA1   160
#define H_LODKA2   161
#define H_LODKA3   162
#define H_LODKA4   163
#define H_LODKA5   164
#define H_LODKA6   165
#define H_LODKA7   166
#define H_FLETNA   167
#define H_FLETNA_BAR 168
#define H_FLETNA_MASK 169
#define H_SND_SEVER 170
#define H_SND_VYCHOD 171
#define H_SND_JIH   172
#define H_SND_ZAPAD 173
#define H_SND_RAND1 174
#define H_SND_RAND2 175
#define H_SND_RAND3 176
#define H_SND_RAND4 177
#define H_FBIG      178
#define H_CHARGEN 179
#define H_CHARGENB 180
#define H_CHARGENM 181
#define H_BGR_BUFF 182
#define H_KREVMIN 183
#define H_KREVMID 184
#define H_KREVMAX 185
#define H_ARMAGED 186
#define H_GLOBMAP 200
#define H_ARMA_CNT 13
#define H_FIRST_FREE 225
#define H_MENUS_FREE 32768

#define MAX_HLAD(x) (((x)->vlastnosti[VLS_MAXHIT]/2+2*24)*HODINA)
#define MAX_ZIZEN(x) (((x)->vlastnosti[VLS_MAXHIT]/3+24)*HODINA)


#define concat(c,s1,s2) c=strcat(strcpy((char *)alloca(strlen(s1)+strlen(s2)+1),s1),s2)
#define local_strdup(c) strcpy((char *)alloca(strlen(c)+1), c)
#define concat2(s1,s2) strcat(strcpy((char *)alloca(strlen(s1)+strlen(s2)+1),s1),s2)
#define concat3(s1,s2,s3) strcat(strcat(strcpy((char *)alloca(strlen(s1)+strlen(s2)+strlen(s3)+1),s1),s2),s3)
#define concat4(s1,s2,s3,s4) strcat(strcat(strcat(strcpy((char *)alloca(strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+1),s1),s2),s3),s4)
#define countof(array) (sizeof(array)/sizeof(array[0]))

#define AP_MULTIPLIER 15
#define get_ap(vls) (((vls[VLS_POHYB])>0 && (vls[VLS_POHYB])<AP_MULTIPLIER)?1:(vls[VLS_POHYB])/AP_MULTIPLIER)

#define SAVE_NAME_SIZE 32

static inline int mgochrana(int x) {
    if (x > 100) return 100;
    return 100-x;
}
//#define mgochrana(x) (100-(x))
//#define mgochrana(x) (1000/(10+(x)))

//typy sektoru

#define ISTELEPORT(c) ((c)==S_TELEPORT || ((c)>=S_USERTELEPORT && (c)<=S_USERTELEPORT_END))
#define ISTELEPORTSECT(sect) ISTELEPORT(map_sectors[sect].sector_type)

#undef S_NORMAL

#define S_NORMAL 1
#define S_SCHODY 2
#define S_SMER 5
#define S_VODA 9
#define S_SLOUP 10
#define S_DIRA 11
#define S_TELEPORT 12
#define S_LAVA 4
#define S_LODKA 3
#define S_TLAC_OFF 13
#define S_TLAC_ON 14
#define S_FLT_SMER 15
#define S_LEAVE 19
#define S_VIR   20
#define S_SSMRT 21
#define S_ACID  22
#define S_USERTELEPORT 50
#define S_USERTELEPORT_END 58

//sector flags

#define MC_AUTOMAP 0x1       //automapovano
#define MC_FLY     0x2       //fly na sektoru
#define MC_PLAYER  0x4       //hrac na sektoru
#define MC_SHADING 0x100     //druhe stinovani  (do tmy)
#define MC_DEAD_PLR 0x8      //mrtvy hrac na sektoru
#define MC_DPLAYER (MC_PLAYER | MC_DEAD_PLR)   //nejaky hrac na sektoru
#define MC_SPECTXTR 0x10     //specialni textura na sektoru (viz spectxtr)
#define MC_SAFEPLACE 0x20    //bezpecne misto (pod vodou muzou dychat);
#define MC_MARKED   0x80     //oznaceny sektor (pro volne pouziti)
#define MC_DISCLOSED 0x40    //odhaleno kouzlem automapping
#define MC_SHOW_STAIRS 0x200 //obrazek schodu
#define MC_STAIRS_DOWN 0x400 //obrazek schodu dolu (nebo sipka)
#define MC_NOAUTOMAP   0x800 //nelze najit automapingem
#define MC_NOSUMMON    0x1000 //nelze privolat hrace


typedef enum skeldal_folders_tag {
    SR_DATA,
    SR_GRAFIKA,
    SR_ZVUKY,
    SR_FONT,
    SR_MAP,
    SR_MUSIC,
    SR_TEMP, /* = unused group */
    SR_BGRAFIKA ,
    SR_ITEMS,
    SR_ENEMIES,
    SR_VIDEO,
    SR_DIALOGS,
    SR_SAVES,
    SR_WORK,
    SR_LANG,


SR_COUNT} SKELDAL_FOLDERS_TAG;
#if 0 //adresare
#define SR_DATA 0
#define SR_GRAFIKA 1
#define SR_ZVUKY 2
#define SR_FONT 3
#define SR_MAP 4
#define SR_MUSIC 5
#define SR_BGRAFIKA 7
#define SR_ITEMS 8
#define SR_ENEMIES 9
#define SR_VIDEO 10
#define SR_DIALOGS 11
#define SR_SAVES 12
#define SR_WORK 13
#define SR_CD 14
#define SR_MAP2 15
#define SR_ORGMUSIC 16
#define SR_COUNT 17
#endif

//globalni cisla k casovacum
#define TM_BACK_MUSIC 1
#define TM_SCENE 2
#define TM_FAST_TIMER 3
#define TM_SOUND_RANDOMIZER 4
#define TM_BOTT_MESSAGE 5
#define TM_FLY 6
#define TM_CLEAR_ZASAHY 7
#define TM_REGEN 8
#define TM_WAITER 9
#define TM_SCENE2 10
#define TM_HACKER 11
#define TM_FX 12
#define TM_CHECKBOX 13
#define TM_FLETNA 14
#define TM_DELAIER 15
#define TM_VZPLANUTI 16

//umisteni predmetu

#define PL_NIKAM 0
#define PL_BATOH 1
#define PL_TELO_H 2
#define PL_TELO_D 3
#define PL_HLAVA 4
#define PL_NOHY 5
#define PL_KUTNA 6
#define PL_KRK 7
#define PL_RUKA 8
#define PL_OBOUR 9
#define PL_PRSTEN 10
#define PL_SIP 11

//hracovi_pozice
#define PO_BATOH 0
#define PO_TELO_H 1
#define PO_TELO_D 2
#define PO_HLAVA 3
#define PO_NOHY 4
#define PO_KUTNA 5
#define PO_KRK 6
#define PO_RUKA_L 7
#define PO_RUKA_R 8


//hracovi vlastnosti

#define VLS_MAX     24
#define VLS_SILA    0
#define VLS_SMAGIE  1
#define VLS_POHYB   2
#define VLS_OBRAT   3
#define VLS_MAXHIT  4
#define VLS_KONDIC  5
#define VLS_MAXMANA 6
#define VLS_OBRAN_L 7
#define VLS_OBRAN_H 8
#define VLS_UTOK_L  9
#define VLS_UTOK_H  10
#define VLS_OHEN    11
#define VLS_VODA    12
#define VLS_ZEME    13
#define VLS_VZDUCH  14
#define VLS_MYSL    15
#define VLS_HPREG   16
#define VLS_MPREG   17
#define VLS_VPREG   18
#define VLS_MGSIL_L 19
#define VLS_MGSIL_H 20
#define VLS_MGZIVEL 21
#define VLS_DAMAGE  22
#define VLS_KOUZLA  23


//rezimy interakce

#define MD_GAME 0
#define MD_MAP 1
#define MD_INV 2
#define MD_SETUP 3
#define MD_INBATTLE 4
#define MD_PRESUN 5
#define MD_PREZBROJIT 6
#define MD_UTEK 7
#define MD_KZ_VYBER 8
#define MD_END_GAME 9
#define MD_SHOP 10
#define MD_ANOTHER_MAP 11

//typy map
#define ME_NORMAL 0 //normal map
#define ME_SOPKA  1 //sopka
#define ME_LEDOV  2 //ledove jeskyne
#define ME_PVODA  3 //pod vodou
#define ME_MESTO  4 //nejde sleep (ve meste)

#define CASE_KEY_1_6 case 2:\
                     case 3:\
                     case 4:\
                     case 5:\
                     case 6:\
                     case 7

typedef struct tdregisters
  {
  int h_num;
  char name[13];
  ABLOCK_DECODEPROC proc;
  char path;
  }TDREGISTERS;

typedef struct tstena
  {
  char prim,sec,oblouk,side_tag;
  unsigned short sector_tag;
  char xsec,ysec;
  uint32_t flags;
  char prim_anim,sec_anim,lclip,action;
  }TSTENA;

typedef struct tsector
  {
  char floor,ceil;
  char flags,sector_type; //sector_type = 0 - sector not used;
  char action,side_tag;
  unsigned short step_next[4];
  unsigned short sector_tag;
  }TSECTOR;

typedef struct tvyklenek
  {
  short sector,dir,xpos,ypos,xs,ys;
  short items[9];
  short reserved;
  }TVYKLENEK;



typedef TSTENA TSIDEDEF[][4];
typedef TSECTOR TSECTORDEF[];

typedef struct tmap_edit_info
  {
  short x,y,layer,flags;
  }TMAP_EDIT_INFO;

typedef TMAP_EDIT_INFO TMAP_EDIT[];

typedef
   struct mapglobal
   {
   char back_fnames[4][13];
   int fade_r,fade_g,fade_b;
   int start_sector;
   int direction;
   char mapname[30];
   char map_effector;
   char local_monsters;
   char map_autofadefc;
   }MAPGLOBAL;

typedef struct the_timer
  {
  int zero;
  int id;
  int counter,count_max,calls;
  void (*proc)(struct the_timer *);
  int32_t userdata[4];
  struct the_timer *next;
  char zavora;
  }THE_TIMER;

typedef struct d_action
  {
  unsigned short action,sector,side,flags,nocopy,delay;
  struct d_action *next;
  }D_ACTION;

extern word color_topbar[7];


extern MAPGLOBAL mglob;            //globalni informace o levelu
extern TSTENA *map_sides;          //popisy jednotlivych sten (map_sides[(sector<<2)+dir])
extern TSECTOR *map_sectors;       //popisy sektoru map_sectors[sector]
extern TVYKLENEK *map_vyk;         //mapa vyklenku
extern word vyk_max;               //pocet vyklenku v mape
extern char *flag_map;             //mapa prenasenych flagu
extern TMAP_EDIT_INFO *map_coord;  //mapa souradnic a flagu
extern int viewsector;             //aktualni sektor vyhledu
extern int viewdir;                //aktualni smer vyhledu
extern THE_TIMER timer_tree;       //strom casovych udalosti
extern D_ACTION *d_action;         //spojovy seznam zpozdenych akci
extern const char *gpathtable[SR_COUNT];          //tabulka adresaru
extern char level_preload;         //informace o preloadingu
extern char **texty;               //globalni tabulka textu
extern char **level_texts;         //lokalni tabulka textu
extern char *level_fname;          //jmeno levelu
extern int mapsize;                //pocet sektoru v mape
extern int hl_ptr;                 //ukazatel na konec staticke tabulky registraci
extern int end_ptr;                //ukazatel na uplny konec tabulky registraci
extern short **map_items;          //ukazatel na mapu predmetu
extern short *destroyed_items;     //list of destroyed items to be returned to the game
extern int default_ms_cursor;      //cislo zakladniho mysiho kurzoru
extern void (*unwire_proc)(void);      //procedura zajistujici odpojeni prave ukoncovane interakce
extern void (*wire_proc)(void);        //procedura zajistujici pripojeni drive ukoncene interakce
extern char cur_mode;              //cislo aktualni interakce
extern word minimap[VIEW3D_Z+1][VIEW3D_X*2+1]; //minimalizovana mapa s informacemi pro sestaveni vyhledu
extern char norefresh;             //vypina refresh obrazovky
extern char cancel_pass;           //okamzite zrusi plynuly prechod
extern char reverse_draw ;         //kresba odpredu dozadu
extern uint8_t gamespeed;             //rychlost hry
extern uint8_t gamespeedbattle;	   //akcelerace rychlosti pro bitvy
extern int num_ofsets[];           //tabulka offsetu pro steny
extern int back_color;             //cislo barvy pozadi
extern uint8_t cur_group;             //cislo aktualni skupiny
extern uint8_t group_select;               //1 = prave byla sestavena nova skupina
extern unsigned int barvy_skupin[POCET_POSTAV+1]; //cisla barev skupin
extern char battle;          //jednicka znaci ze bezi bitva
extern char battle_mode;          //rezim bitvy 0=programovani
extern char neco_v_pohybu;          //jednicka znaci ze se nektere potvory jeste hejbou
extern short select_player;         //vybrana postava nebo -1
extern char group_sort[POCET_POSTAV]; //pretrideni skupin
extern char global_anim_counter;
extern char one_buffer;            //1 zapina pouziti pouze jednoho bufferu pro render
extern char save_map;     //1 oznamuje ze pri opusteni levelu je nutne ulozit stav mapy
extern int32_t money;           //stav konta hracu
extern int32_t level_map[];        //tabulka urovni
extern char true_seeing;            //1 oznamuje ze bezi kouzlo true_seeing
extern char set_halucination;
extern int hal_sector;    //cislo sektoru a smeru pri halucinaci
extern int hal_dir;
extern char side_touched;    //promena se nastavuje na 1 pri kazdem uspesnem dotyku steny
extern const char *texty_knihy;     //jmeno souboru s textamy knihy
extern int cur_page;         //cislo stranky v knize;
extern int32_t game_time;              //hraci cas
extern char autoattack;
extern char enable_sort;
extern char last_send_action;      //naposled vyslana akce
extern char see_monster;    //jednicka pokud hraci vidi nestvuru
extern char lodka;
extern char lodka_battle_draw;  //jedncka pokud se zobrazuje programovani (pro lodku)
extern char anim_mirror;     //je li 1 pak animace kouzel a zbrani jsou zrcadlove otocene
extern char insleep;         //je li 1 pak bezi sleep
extern char pass_zavora;     //je-li 1 pak bezi passing (hraci zrovna jdou)
extern short moving_player;   //cislo presouvaneho hrace
extern int bgr_distance; //vzdalenost pozadi od pohledu
extern int bgr_handle;  //cislo handle k obrazku pozadi;
extern char enable_glmap; //povoluje globalni mapu;
extern int charmin;
extern int charmax;
extern uint32_t current_map_hash;

extern int autoopenaction;
extern int autoopendata;

extern char doNotLoadMapState;

extern char log_combat;

//builder - skeldal

void game_keyboard(EVENT_MSG *msg,void **usr);
void calc_animations(void);
int load_map(const char *filename);
void other_draw(void);
void refresh_scene(THE_TIMER *t);
const void *pcx_fade_decomp(const void *p, int32_t *s, int h);
const void *pcx_15bit_decomp(const void *p, int32_t *s, int h);
const void *pcx_15bit_autofade(const void *p, int32_t *s, int h);
const void *pcx_15bit_backgrnd(const void *p, int32_t *s, int h);
const void *pcx_8bit_decomp(const void *p, int32_t *s, int h);
const void *hi_8bit_correct(const void *p, int32_t *s, int h);
const void *pcx_8bit_nopal(const void *p, int32_t *s, int h);
const void *set_background(const void *p, int32_t *s, int h);
const void *wav_load(const void *p, int32_t *s, int h);
const void *load_mob_legacy_format_direct(const void *p, int32_t *s, int h);
const void *load_mob_legacy_format(const void *p, int32_t *s, int h);
const void *load_spells_legacy_format(const void *p, int32_t *s, int h);
void wire_main_functs(void);
void ukaz_kompas(char mode);
void timming(EVENT_MSG *msg,void **data);
void do_timer(void);
void hold_timer(int id,char hld);
typedef void (*TIMER_PROC)(THE_TIMER *t);
THE_TIMER *add_to_timer(int id,int delay,int maxcall,TIMER_PROC proc);
void delete_from_timer(int id);
THE_TIMER *find_timer(int id);
void objekty_mimo(THE_TIMER *t);
void mouse_set_cursor(int cursor);
#define FONT_TSHADOW 0x10000
#define FONT_TSHADOW_GRAY 0x30000
void set_font(int font,int c1,...);
void bott_draw(char);
const void *bott_draw_proc(const void *p, int32_t *s, int h);
void mouse_set_default(int cursor);
void create_frame(int x,int y,int xs,int ys,char clear);
void save_dump(const uint16_t *screen_addr,
        unsigned int width,
        unsigned int height,
        unsigned int linelen);
void bott_disp_text(const char *);
void bott_text_forever(void);
char chod_s_postavama(char sekupit);
void hide_ms_at(int line); //schova mysku ktera je nad line
int cislovka(int i);
void wire_kniha(void);
void purge_temps(char z); //z=1 vymaze i swapsoubor
void destroy_player_map(void); //je nutne volat pred presunem postav
void build_player_map(void); //je nutne volat po presunem postav
int postavy_propadnout(int sector);
void postavy_teleport_effect(int sector,int dir,int postava,char eff);
void reg_grafiku_postav(void);
void play_movie_seq(const char *s,int y);
void check_postavy_teleport(void);  //je-li viewsector=teleport pak presune postavy



//builder
#define MAIN_NUM 0
#define LEFT_NUM 1
#define RIGHT_NUM 2
#define CEIL_NUM 3
#define FLOOR_NUM 4
#define OBL_NUM 5
#define OBL2_NUM 6
#define BACK_NUM 7

#define MSG_DELAY (50*10)


extern char bott_display; //cislo udava co je zobrazeno v dolni casti
extern int vmode;
extern char runes[5];


typedef struct spectxtr
  {
  word sector;
  word handle;
  char count;
  char pos;
  signed char xpos;
  char repeat;
  }SPECTXTR;

#define MAX_SPECTXTRS 64
typedef SPECTXTR SPECTXT_ARR[MAX_SPECTXTRS];

extern SPECTXT_ARR spectxtr;

void add_spectxtr(word sector,word fhandle,word count,word repeat,integer xpos);
void calc_spectxtrs(void);
void init_spectxtrs(void);
void play_fx(int x,int y);
void draw_fx(void);
void play_fx_at(int where);
void draw_blood(char mode,int mob_dostal,int mob_dostal_pocet);
		//kresli krev. mode=1 nasaveni, mode=0 kresleni, pri mode=0 se parametry ignoruji

#define FX_MAGIC 0
#define FX_BOOK 1
#define FX_MONEY 2
#define FX_MAP 3
#define FX_SAVE 4




#define BOTT_NORMAL 0
#define BOTT_TEXT 1
#define BOTT_FLETNA 2
#define BOTT_RUNA 3


void step_zoom(char smer);
void turn_zoom(int smer);
void a_touch(int sector,int dir);
int do_action(int action_numb,int sector,int direct,int flags,int nosend);
void delay_action(int action_numb,int sector,int direct,int flags,int nosend,int delay);
int32_t load_section(FILE *f,void **section, int *sct_type,int32_t *sect_size);
int prepare_graphics(int *ofs,char *names,int32_t size,ABLOCK_DECODEPROC decomp,int class);
void show_automap(char full);
void draw_medium_map(void);
void anim_sipky(int h,int mode);
void redraw_scene(void);
void calc_game(void);
void do_delay_actions(void);
void real_krok(EVENT_MSG *msg,void **data);
void sort_groups(void);
void recheck_button(int sector,char auto_action);
void start_dialog(int entr,int mob);
void show_money(void);
void chveni(int i);
void render_scene(int,int);
void bott_draw_fletna(void);
void bott_disp_rune(char rune, int item);

void display_ver(int x,int y,int ax,int ay);
void check_players_place(char mode);


void add_leaving_place(int sector);
int set_leaving_place(void);
int get_leaving_place(char *level_name);


//click_map

typedef struct t_clk_map
  {
  int id,xlu,ylu,xrb,yrb;
  char (*proc)(int id,int xa,int ya,int xr,int yr);
  char mask;
  int cursor;
  }T_CLK_MAP;

#define CLK_MAIN_VIEW 18
#define MS_GAME_WIN 256
extern T_CLK_MAP clk_main_view[];        //clickovaci mapa pro hlavni vyhled

void change_click_map(T_CLK_MAP *map,int mapsize);
void ms_clicker(EVENT_MSG *msg,void **usr);
const T_CLK_MAP * find_in_click_map_entry(int x, int y,T_CLK_MAP *pt,int pocet,int evtype);
void restore_click_map(void *map,int mapsize);
void save_click_map(void **map,int *mapsize);
void set_game_click_map(void);
void change_global_click_map(T_CLK_MAP *map,int mapsize);
char empty_clk(int id,int xa,int ya,int xr,int yr); //tato udalost slouzi ke zruseni nekterych mist v globalni mape
void disable_click_map(void);

char start_invetory(int id,int xa,int ya,int xr,int yr);
char go_map(int id,int xa,int ya,int xr,int yr);
char konec(int id,int xa,int ya,int xr,int yr);
char return_game(int id,int xa,int ya,int xr,int yr);
char clk_step(int id,int xa,int ya,int xr,int yr);
char clk_touch(int id,int xa,int ya,int xr,int yr);
char go_book(int id,int xa,int ya,int xr,int yr);
char clk_saveload(int id,int xa,int ya,int xr,int yr);
char clk_sleep(int id,int xa,int ya,int xr,int yr);
char spell_casting(int id,int xa,int ya,int xr,int yr);


//inventory viewer - items

#define IT_ICONE_SIZE (2+2+2+2*256+55*45)
#define MAX_INV 30
#define HUMAN_PLACES 9
#define HUMAN_RINGS 4

extern int item_count; //pocet predmetu ve hre
extern int it_count_orgn; //puvodni pocet predmetu ve hre (pri loadmap)
extern short water_breath;       //vec pro dychani pod vodou
extern short flute_item;

void load_items(void);
void load_item_map(void *p,int32_t s);
void draw_placed_items_normal(int celx,int cely,int sect,int side);

#define SPL_INVIS 0x1           //hrac je neviditelny
#define SPL_OKO 0x2             //hrac ma kouzlo oko za oko
#define SPL_TVAR 0x4            //hrac ma kouzlo nastav tvar
#define SPL_DRAIN 0x8           //hrac kazdym utokem drainuje nepritele
#define SPL_MANASHIELD 0x10     //hrac je chranen stitem z many
#define SPL_SANC 0x20           //resistance to fyzical attack
#define SPL_HSANC 0x40          //resistance to all attack
#define SPL_BLIND 0x80          //hrac je slepy
#define SPL_REGEN 0x100         //hrac ma regeneraci pri boji
#define SPL_ICE_RES 0x200       //hrac je chranen proti ledu
#define SPL_FIRE_RES 0x400      //hrac je chranen proti ohni
#define SPL_KNOCK   0x800       //knock target back
#define SPL_FEAR    0x1000      //hrac nebo nestvura jsou posedle kouzlem "strach"
#define SPL_STONED  0x2000      //hrac nebo nestvura jsou zkamenele
#define SPL_LEVITATION 0x4000   //hrac nebo nestvura levituji, takze se na ne neuplatnuji nektere efekty sektoru
#define SPL_DEMON 0x8000        //hrac je zmenen v demona


#define AC_ATTACK   1
#define AC_MOVE     2
#define AC_CANCEL   3
#define AC_RUN      4
#define AC_ARMOR    5
#define AC_STAND    6
#define AC_MAGIC    7
#define AC_START    8
#define AC_THROW    9

#define TYP_NESPEC 0
#define TYP_UTOC 1
#define TYP_VRHACI 2
#define TYP_STRELNA 3
#define TYP_ZBROJ 4
#define TYP_SVITEK 5
#define TYP_LEKTVAR 6
#define TYP_MECH 7
#define TYP_VODA 7
#define TYP_JIDLO 8
#define TYP_SPECIALNI 9
#define TYP_RUNA 10
#define TYP_PENIZE 11
#define TYP_SVITXT 12
#define TYP_PRACH 13
#define TYP_OTHER 14

#define ITF_DUPLIC 0x8000 //Predmet je duplikaci jineho predmetu
#define ITF_FREE 0x4000 //Predmet byl duplikaci, ted je jeho pozice volna
#define ITF_DESTROY 0x1
#define ITF_NOREMOVE 0x2
#define IT_FACES 5

#define TPW_MEC 0
#define TPW_SEKERA 1
#define TPW_KLADIVO 2
#define TPW_HUL 3
#define TPW_DYKA 4
#define TPW_STRELNA 5
#define TPW_OST 6
#define TPW_MAX 7

#define TSP_WATER_BREATH 1   //cislo specialni veci pro dychani pod vodou
#define TSP_FLUTE 2

#define MAX_SLEEP 4320

typedef struct titem
  {
  char jmeno[32];   //32        Jmeno predmetu
  char popis[32];   //64
  short zmeny[24];  //112       Tabulka, jakych zmen ma na hracovy vlastnosti
  short podminky[4];//120       Tabulka, jake vlastnosti musi mit hrac k pouziti predmetu
  short hmotnost,nosnost,druh; //126  druh = Typ predmetu
  short umisteni;              //128  Kam se predmet umisti?
  word flags;                  //130  ruzne vlajky
  short spell,magie,sound_handle;//136  specialni kouzla / rukojet zvuku
  short use_event;             //140  specialni udalost
  unsigned short ikona,vzhled; //144  ikony a vzhled
  short user_value;            //146  uzivatelska hodnota
  short keynum;                //148 cislo klice
  short polohy[2][2];          //156 souradnice poloh pro zobrazeni v inv
  uint8_t typ_zbrane;              //160 Typ zbrane
  char unused;
  short sound;                  //cislo zvuku
  short v_letu[16];             //192
  unsigned short cena;
  unsigned short cena_high;
  char weapon_attack;           //relativni handle k souboru s animaci utok
  char hitpos;                  //pozice zasahu animace
  char shiftup;
  char byteres;
  short rezerva[12];           //224 rezervovane
  }TITEM;

#define PLAYER_MAX_LEVEL 40


typedef struct hum_action
  {
  short action,data1,data2,data3;
  }HUM_ACTION;



typedef struct thuman
  {
  char used;                    //1 kdyz je pozice pouzita
  char spell;                   //1 kdyz postava ma na sobe aspon 1 kouzlo.
  uint8_t groupnum;                //cislo skupiny 0-6
  int8_t xicht;                   //cislo obliceje 0-5
  uint8_t direction;               //smer otoceni
  uint8_t downpoints;           //pokud mrtva postava ma nejake body, tak ji lze ozivit lecenim
  short sektor;                 //sektor postaveni
  short vlastnosti[VLS_MAX];    //mapa aktualnich vlastnosti po korekcich
  short bonus_zbrani[TPW_MAX];  //bonusy za zbrane
  short lives;                  //pocet zraneni
  short mana;                   //mnozstvi many
  short kondice;                //kondice postavy
  short actions;                //aktualni pocet AP
  short mana_battery;           //udaj po nabyti nakouzlene many
  short stare_vls[VLS_MAX];     //mapa vlastnosti pred korekcemi
  short wearing[HUMAN_PLACES];  //nosene predmety
  short prsteny[HUMAN_RINGS];   //nosene prsteny
  short sipy;                   //pocet sipu v toulci
  short inv_size;               //velikost inventare 6-30
  short inv[MAX_INV];           //inventar
  short level;                  //uroven
  short weapon_expy[TPW_MAX];    //zkusenosti za zbrane
  int32_t exp;                     //zkusenost
  char female;                  //1 kdyz zena
  char utek;                    //hodnota udavajici pocet kroku pri uteku
  HUM_ACTION *zvolene_akce;     //ukazatel na tabulku zvolenych akci
  HUM_ACTION *provadena_akce;   //ukazatel na aktualni akci
  char programovano;            //pocet programovanych akci
  char jmeno[15];               //jmeno
  short zasah;                   //posledni zasah postavy ???
  short dostal;                  //cislo ktere se ukazuje na obrazku s postavou jako zasah
  short bonus;                  //bonus pro rozdeleni vlastnosti
  int jidlo;   //max 25000      //pocet kol o hladu zbyvajicich
  int voda;    //max 20000      //pocet kol o zizny zbyvajicich
  struct thuman *demon_save;    //ukazatel na postavu ulozenou behem kouzla demon
  uint32_t inmaphash;                //hash nazvu mapy, kde se postavy nachazi
  }THUMAN;


extern TITEM *glob_items;             //tabulka predmetu
extern int ikon_libs;
extern short *picked_item;            //retezec sebranych predmetu
extern int item_in_cursor;
extern THUMAN postavy[POCET_POSTAV];  //postavy
extern THUMAN postavy_2[];  //postavy
extern THUMAN *human_selected;        //vybrana postava v invetorari
extern int sleep_ticks;
extern int face_arr[IT_FACES];
char pick_item_(int id,int xa,int ya,int xr,int yr);
void wire_inv_mode(THUMAN *select);
void init_inventory(void);
void init_items(void);
void push_item(int sect,int pos,const short *picked_item);
void pop_item(int sect,int pos,int mask,short **picked_item);
int count_items_inside(const short *place);
int count_items_total(const short *place);
char put_item_to_inv(THUMAN *p,short *picked_items); //funkce vklada predmet(y) do batohu postavy
void pick_set_cursor(void);         //nastavuje kurzor podle vlozeneho predmetu;
void calc_fly(THE_TIMER *t);
void zmen_skupinu(THUMAN *p);
void add_to_group(int num);
void group_all(void);
const void *build_items_called(const void *p, int32_t *s, int h);
void real_regeneration(THE_TIMER *t); //regenerace postav behem hry v realu (pouze kondice a mana)
char sleep_regenerace(THUMAN *p);  //regenerace postav behem spani
char check_jidlo_voda(THUMAN *p);
void prepocitat_postavu(THUMAN *human_selected);
void sleep_players(va_list args); //Pozor !!! TASK
void item_sound_event(int item,int sector);
short create_item_money(int obnos);  //vytvori predmet penize s urcitym obnosem
char check_map_specials(THUMAN *p);
void destroy_items(short *items);  //nici predmety v mysi
void do_items_specs(void); //vola specialni akci predmetu v mysi
short duplic_item(short item); //duplikuje vec a vraci id cislo klonu
int advance_vls(int id);
short create_unique_item(const TITEM *it);//vytvari jedinecnou vec pro hru (jako duplikat niceho :-))
int calculate_weight(THUMAN *p);
int weigth_defect(THUMAN *p);



#define SHP_SELL 0x1      //objekt lze prodat
#define SHP_BUY 0x2       //objekt lze koupit
#define SHP_AUTOADD 0x4   //objekt pribyva casem
#define SHP_SPECIAL 0x8   //objekt se objevi jen obcas
#define SHP_TYPE 0x80     //objekt je popis typu
#define SHP_NOEDIT 0x40   //objekt se nenahrava do editoru.

typedef struct tproduct
  {
  short item;          //cislo predmetu ktere nabizi
  int cena;           //cena za jeden
  short trade_flags;    //vlajky
  int pocet;           //pocet predmetu na sklade
  int max_pocet;
  }TPRODUCT;

typedef struct tshop
  {
  char keeper[16];
  char picture[13];
  int koef;
  int products;
  int shop_id;
  int list_size;
  short spec_max;     //maximalni pocet specialnich predmetu
  const TPRODUCT *list;
  }TSHOP;


typedef struct tshop_product_state_tag {
    uint16_t count;
    uint16_t previous_price;
} TSHOP_PRODUCT_STATE;

typedef struct tshop_all_state {
    const TPRODUCT *first_product;
    TSHOP_PRODUCT_STATE *first_state;
    size_t count_states;
} TSHOP_ALL_STATE;

void enter_shop(int shopid);
void load_shops(void);
void reroll_all_shops(void);
char save_shops(void);
char load_saved_shops(void);

//macros

#define MAX_ACTIONS 40
#define MA_GEN 0
#define MA_SOUND 1
#define MA_TEXTG 2
#define MA_TEXTL 3
#define MA_SENDA 4
#define MA_FIREB 5
#define MA_DESTI 6
#define MA_LOADL 7
#define MA_DROPI 8
#define MA_DIALG 9
#define MA_SSHOP 10
#define MA_CLOCK 11
#define MA_CACTN 12
#define MA_LOCK  13
#define MA_SWAPS 14
#define MA_WOUND 15
#define MA_IFJMP 16
#define MA_CALLS 17
#define MA_HAVIT 18
#define MA_STORY 19
#define MA_IFACT 20
#define MA_SNDEX 21
#define MA_MOVEG 22
#define MA_PLAYA 23
#define MA_CREAT 24
#define MA_ISFLG 25
#define MA_CHFLG 26
#define MA_CUNIQ 27
#define MA_MONEY 28
#define MA_GUNIQ 29
#define MA_PICKI 30
#define MA_WBOOK 31
#define MA_RANDJ 32
#define MA_ENDGM 33
#define MA_GOMOB 34
#define MA_SHRMA 35
#define MA_MUSIC 36
#define MA_GLOBE 37  //global events
#define MA_IFSEC 38	 //if sector num
#define MA_IFSTP 39  //if sector type

#define MAGLOB_LEAVEMAP 0 // v urcitou nastavenou hodinu a minutu dene
#define MAGLOB_STARTSLEEP 1 // postavy maji jit spat.
#define MAGLOB_ENDSLEEP 2 // postavy se probouzi
#define MAGLOB_CLICKSAVE 3  // p�ed otev�en�m SAVE dialogu
#define MAGLOB_AFTERSAVE 4  // po ulo�en� hry
#define MAGLOB_BEFOREMAGIC 5 // p�ed vyvol�n�m kouzla
#define MAGLOB_AFTERMAGIC 6 //po vyvol�n� kouzla
#define MAGLOB_BEFOREMAPOPEN 7 //p�ed otev�en�m mapy
#define MAGLOB_AFTERMAPOPEN 8 //po uzav�en� mapy
#define MAGLOB_BEFOREBATTLE 9 //p�ed spu�t�n�m souboje
#define MAGLOB_AFTERBATTLE 10 //po ukon�en� souboje
#define MAGLOB_BEFOREBOOK 11 //pred otev�en�m knihy
#define MAGLOB_AFTERBOOK 12 //po uzav�en� knihy
#define MAGLOB_ONROUND 13 //p�i ka�d�m kole nebo po 10s
#define MAGLOB_ONDEADMAN 14 //p�i umrt� mu�e
#define MAGLOB_ONDEADWOMAN 15 //p�i umrt� �eny
#define MAGLOB_ONDEADALL 16 //p�i umrt� v�ech postav
#define MAGLOB_ONHITMAN 17 //p�i z�sahu mu�e
#define MAGLOB_ONHITWOMAN 18 //p�i z�sahu �eny
#define MAGLOB_ONNEWRUNE 19 //p�i nalezen� nov� runy
#define MAGLOB_ONPICKITEM 20 //p�i sebr�n� p�edm�tu (pro speci�ln� p�edm�ty)
#define MAGLOB_ONSTEP 21 //p�i kroku (p�ed animaci)
#define MAGLOB_ONTURN 22 //p�i oto�en� (p�ed animaci)
#define MAGLOB_ALARM 23 //p�i spu�t�n� alarmu
#define MAGLOB_ONFIREMAGIC 24
#define MAGLOB_ONWATERMAGIC 25
#define MAGLOB_ONGROUNDMAGIC 26
#define MAGLOB_ONAIRMAGIC 27
#define MAGLOB_ONMINDMAGIC 28
#define MAGLOB_ONSPELLID1 29  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONSPELLID2 30  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONSPELLID3 31  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONSPELLID4 32  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONSPELLID5 33  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONSPELLID6 34  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONSPELLID7 35  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONSPELLID8 36  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONSPELLID9 37  //p�i jednom konr�tn�m kouzle
#define MAGLOB_ONTIMER1 38  //cas - pocet hernich sekund od nastaveni
#define MAGLOB_ONTIMER2 39  //cas - pocet hernich sekund od nastaveni
#define MAGLOB_ONTIMER3 40  //cas - pocet hernich sekund od nastaveni
#define MAGLOB_ONTIMER4 41  //cas - pocet hernich sekund od nastaveni
#define MAGLOB_ONFLUTE1 42  //zahrani urcite melodie
#define MAGLOB_ONFLUTE2 43  //zahrani urcite melodie
#define MAGLOB_ONFLUTE3 44  //zahrani urcite melodie
#define MAGLOB_ONFLUTE4 45  //zahrani urcite melodie
#define MAGLOB_ONFLUTE5 46  //zahrani urcite melodie
#define MAGLOB_ONFLUTE6 47  //zahrani urcite melodie
#define MAGLOB_ONFLUTE7 48  //zahrani urcite melodie
#define MAGLOB_ONFLUTE8 49  //zahrani urcite melodie

#define MAGLOB_NEXTID 50 //mus� b�t posledn�




#define MC_PASSSUC 0x1
#define MC_PASSFAIL 0x2
#define MC_TOUCHSUC 0x4
#define MC_TOUCHFAIL 0x8
#define MC_LOCKINFO 0x10
#define MC_EXIT 0x20
#define MC_INCOMING 0x40
#define MC_STARTLEV 0x80
#define MC_CLOSEDOOR 0x100
#define MC_ANIM  0x200
#define MC_ANIM2 0x400
#define MC_SUCC_DONE 0x800
#define MC_SPEC_SUCC 0x1000
#define MC_OPENDOOR 0x2000
#define MC_VYKEVENT 0x4000
#define MC_WALLATTACK 0x8000

typedef struct tma_gen
 {
 unsigned action : 6;
 unsigned cancel : 1;
 unsigned once   : 1;
 unsigned flags  : 16;
 }TMA_GEN;

typedef struct tma_sound
  {
    uint8_t action,flags,eflags; //3
    uint8_t bit16;
  uint8_t volume;             //5
  uint8_t soundid;            //6
  unsigned short freq;     //8
  int32_t start_loop,end_loop,offset;//20
  char filename[12];       //32
  }TMA_SOUND;


typedef struct tma_text
  {
    uint8_t action,flags,eflags,pflags; //4
  int32_t textindex;                    //8
  }TMA_TEXT;

typedef struct tma_send_action
  {
    uint8_t action,flags,eflags,change_bits; //4
  unsigned short sector,side,s_action;       //10
  char delay;                                //11
  }TMA_SEND_ACTION;

typedef struct tma_fireball
  {
    uint8_t action,flags,eflags;            //3 + padding = 4
    short xpos,ypos,zpos,speed,item;        //14
  }TMA_FIREBALL;

typedef struct tma_loadlev
  {
    uint8_t action,flags,eflags;            //3+padding
  short start_pos;                          //6
  char dir;                                 //7
  char name[13];
  }TMA_LOADLEV;



typedef struct tma_dropitm
  {
    uint8_t action,flags,eflags;            //3+padding
  short item;
  }TMA_DROPITM;

typedef struct tma_codelock
  {
    uint8_t action,flags,eflags;
  char znak;
  char string[8];
  char codenum;
  }TMA_CODELOCK;

typedef struct tma_cancelaction
  {
    uint8_t action,flags,eflags,pflags;     //4
  short sector,dir;
  }TMA_ACTN;

typedef struct tma_swapsectors
  {
    uint8_t action,flags,eflags,pflags;     //4
  short sector1,sector2;
  }TMA_SWAPS;

typedef struct tma_wound
  {
    uint8_t action,flags,eflags,pflags;     //4
  short minor,major;
  }TMA_WOUND;



typedef struct tma_lock
  {
    uint8_t action,flags,eflags;            //3+padding
  short key_id;
  short thieflevel;
  }TMA_LOCK;

typedef struct tma_two_parms
  {
    uint8_t action,flags,eflags;            //3+padding
  short parm1,parm2;
  }TMA_TWOP;

typedef struct tma_create_unique
  {
    uint8_t action,flags,eflags;            //3+padding
  TITEM item;
  }TMA_UNIQUE;

typedef struct tma_globe
  {
  uint8_t action,flags,eflags,event; //event - MAGLOB_XXXX
  unsigned short sector;	  //sektor of action target, when event occured
  unsigned char side;		  //side of action target, when event occured
  unsigned char cancel;		  //1 - cancel event
  uint32_t param;		  //event depend param - zero is default
  }TMA_GLOBE;


typedef struct tma_ifsec
{
  char action,flags,eflags;
  unsigned char side;		  //side of action target, when event occured
  unsigned short sector;	  //sektor of action target, when event occured
  short line;		  //jump line
  char invert;				  //invert condition
}TMA_IFSEC;



extern TMA_LOADLEV loadlevel;

typedef union tmulti_action
  {
  struct tma_gen general;
  struct tma_sound sound;
  struct tma_text text;
  struct tma_send_action send_a;
  struct tma_fireball fireball;         //p
  struct tma_loadlev loadlev;           //p
  struct tma_dropitm dropi;             //p
  struct tma_codelock clock;
  struct tma_cancelaction cactn;
  struct tma_lock lock;                 //p
  struct tma_swapsectors swaps;
  struct tma_wound wound;
  struct tma_two_parms twop;            //p
  struct tma_create_unique uniq;        //p
  struct tma_globe globe;
  struct tma_ifsec ifsec;
  }TMULTI_ACTION;

typedef struct tmulti_action_record_t {
    const TMULTI_ACTION *action_list;
    size_t count;
} TMULTI_ACTION_RECORD;

typedef struct tmulti_action_states_t {
    uint8_t *states;
    size_t count;
} TMULTI_ACTION_STATE;

extern TMULTI_ACTION_RECORD *macros;                  //tabulka maker
extern TMULTI_ACTION_STATE  macro_state_block;
extern void *macro_block;          //alokovany blok maker (pri unloadu free!)
extern int macro_block_size;       //velikost bloku;

void load_macros(int size,void *data);
void call_macro(int side,int flags);
void call_macro_ex(int side,int flags, int runatsect);
char get_player_triggered(int p);  //zjistuje zda hrac s cislem p byl makrem zasazen;
char save_load_trigger(short load); //uklada/obnovuje trigger vlajky. -1 uklada, jinak hodnota ulozeneho triggeru
char save_codelocks(TMPFILE_WR *fsta); //uklada do savegame nastaveni kodovych zamku (128 bytu);
char load_codelocks(TMPFILE_RD *fsta); //obnovuje ze savegame nastaveni kodovych zamku (128 bytu);

typedef struct letici_vec
  {
  struct letici_vec *next;
  int sector,smer;
  int xpos,ypos,zpos;
  short item;
  short *items;
  int counter;
  char anim_pos;
  char flags;
  short owner;
  int speed;
  int velocity;
  short hit_bonus;
  short damage;
  short lives;
 }LETICI_VEC;


extern LETICI_VEC *letici_veci;    //spojovy seznam leticich veci
extern LETICI_VEC *letici_veci2;   //spojovy seznam leticich veci aktualni pred zahajenim renderace
#define FLY_NEHMOTNA 1          //leti rovne
#define FLY_DESTROY 2           //znici se
#define FLY_DESTROY_SEQ 4       //bezi nicici sekvence
#define FLY_NEDULEZITA 8        //nema vliv na souboj
#define FLY_IN_FLY 16           //Nastaveno, pokud fly ma vliv na okoli (v letu)
#define FLY_BURNT  32           //vyhorela - bude uvolnena
#define FLY_UNUSED  64          //nevyuzita - muze byt uvolnena
#define TFLY LETICI_VEC
LETICI_VEC *throw_fly(int x,int y, char rovne);

/* postup pri niceni fly

Pri triggeru zniceni
 1) Je Destroy? - > nastav DESTROY_SEQ
 2) Po zniceni -> nastav BURNT
 3) Pokud je pri zpracovani  BURNT -> nastav UNUSED
 4) Pokud je unused -> vymaz ji

Tento slozity postup je zaveden, z duvodu viceulohoveho zpracovani, aby mel
system jistotu ze vsechny pripadne kopie fly jsou znicene a neukazuji
 na neplatne objekty

 Zkratka - pokud je fly znicena, musi ji system potvrdit jeste vlajkou UNUSED, aby
 ji mohl uvolnit
 */

LETICI_VEC *create_fly(void); //vytvari fly - optimalizuje tim ze hleda nevyuzity fly nosice, a teprve pokud neuspeje alokuje novy prostor.
void draw_fly_items(int celx,int cely,int sector,int side);
void add_fly(LETICI_VEC *p);
void build_fly_map(void);
void destroy_fly_map(void);
void destroy_all_fly();
void stop_fly(LETICI_VEC *p,char zvuk);
void herni_cas(char *s);

typedef char **TSTR_LIST;

//gamesaver
void leave_current_map(void);
int save_map_state(void); //uklada stav mapy pro savegame (neuklada aktualni pozici);
int load_map_state(void); //obnovuje stav mapy; nutno volat po zavolani load_map;
void restore_current_map(void); //pouze obnovuje ulozeny stav aktualni mapy
uint32_t fnv1a_hash(const char *str);
int load_game(const char *fname);
int save_game(long game_time,char *gamename, char is_autosave);
void save_map_description(TMPFILE_WR *f);
void load_map_description(TMPFILE_RD *f);
void free_map_description();
TSTR_LIST swap_map_description(TSTR_LIST new_list);
void wire_save_load(char save);
void do_save_dialog();
char ask_save_dialog(char *name_buffer, size_t name_size, char allow_remove);
void do_autosave();
#define autosave() if (autosave_enabled) do_autosave();
extern char autosave_enabled;
extern char autosave_on_enter;
int load_map_automap(char *mapfile);
 /* ^^^ Tato funkce zmeni mapu, bez zmeny grafiky a stavu cele hry.
    Jeji vyuziti je pro zobrazeni automapingu jineho levelu nez aktualniho
    Pred navratem do hry je treba udelat load_map_automap(level_fname);!!!*/

//setup
char q_runsetup(char *);
void user_setup(void);
void setup_dialoge(void);
char game_setup(int id,int xa,int ya,int xr,int yr);
void GamePause(void);
typedef struct ctl3d CTL3D;
void show_textured_button(int x,int y,int xs,int ys,int texture,CTL3D *border3d);

//sounder & music

extern short sample_volume;           //hlastitost samplu
extern char **sound_table;

void init_tracks(void);
void recalc_volumes(int sector,int side);
void play_effekt(int x,int y,int xd,int yd,int sector, int side,const TMA_SOUND *p);
void create_playlist(char *playlist);
const char *get_next_music_from_playlist(void);
const char * end_of_song_callback(void *ctx);
void purge_playlist(void);
void play_sample_at_sector(int sample,int listener,int source,int track, char loop);
void play_sample_at_channel(int sample,int channel,int vol);
void stop_track(int track);
char test_playing(int track);
void stop_track_free(int track);
void mute_all_tracks(char all);
void kill_all_sounds(void);
void create_sound_table(char *template,int32_t size);
void create_sound_table_old(void);
void start_play_flute(char );
void stop_play_flute(void);
void pc_speak_play_sample(char *sample,int size,char step,int freq);
char enable_sound(char enbl);
void wire_main_functs(void);
void unwire_main_functs(void);

//enemy
#define MOBS_INV 16
#define MOB_POSIT 0
#define MOB_ATTACK 1
#define MOB_TOHIT 2

#define MAX_MOBS 255


#define MOB_ATTACKING 0x2
#define MOB_TO_HIT    0x4
#define MOB_DEATH     0x5
#define MOB_START 1

#define MOB_WALK 0x1
#define MOB_WATCH 0x2
#define MOB_LISTEN 0x4
#define MOB_BIG 0x8
#define MOB_GUARD 0x10
#define MOB_PICK 0x20
#define MOB_PICKING 0x40
#define MOB_ROGUE 0x80
//vlajky
#define MOB_IN_BATTLE 0x1
#define MOB_PASSABLE 0x2
#define MOB_SENSE 0x4
#define MOB_MOBILE 0x8
#define MOB_RELOAD 0x10
#define MOB_CASTING 0x20
#define MOB_SAMPLE_LOOP 0x40
#define MOB_LIVE 0x80    //potvora zije

//vlajky2
#define MOB_F2_DONT_SAVE 0x1

typedef struct tmob
  {
  char name[30];           //jmeno moba
  short casting;
  short adjusting[6*16];     //volba stredu pro animace
  unsigned short sector,dir;        //pozice
  char locx,locy;         //presna pozice
  char headx,heady;        //pozice kam mob miri
  short anim_counter;        //citac animaci
  short vlastnosti[24];     //zakladni vlastnosti potvory
  short inv[MOBS_INV];      //batoh potvory
  short lives;              //pocet zivotu potvory
  short cislo_vzoru;         //informace urcujici ze ktereho vzoru byl mob vytvoren
  short speed;             //rychlost pohybu
  short dohled;            //kam dohl�dne
  short dosah;             //okam�ik za��tku souboje
  char stay_strategy;      //chovani moba ve statickem modu (nepronasleduje)
  char walk_data;           //cislo potrebne pro pohyb moba v bludisti
  unsigned short bonus;              //bonus za zabiti
  char flee_num;             //pravdepodobnost uteku
  char anim_counts[6];     //pocet animacnich policek pro kazdy pohyb
  char mobs_name[7];       //zaklad jmena souboru pro moba
  char vlajky2;             //vlajky pro summon z konzoli (BIT0 - neukladat)
  uint8_t id;               //mobile id (template index);
  int32_t experience;          //zkusenost
  char vlajky;             //BIT0 - 1 v boji
  char anim_phase;            //cinnost kterou mob dela
  short csektor;            //Cilovy sektor
  short home_pos;          //domaci pozice
  short next;              //Cislo dalsiho moba, ktery stoji na jeho pozici
  char actions;            //pocet akci ktere muze potvora provest v kole
  char hit_pos;           //animacni pozice, kdy potvora zasahne
  unsigned short sounds[4];          //zvuky z listu
  signed char palette;           // pocet pouzitelnych palet / cislo palety
  char mode;              //akce potvory
  short dialog;           //cislo dialogu, -1 kdyz neni;
  char dialog_flags;      //vlajky mapovane do dialogu;
  unsigned short money;             //penize
  unsigned short specproc;          //specproc
  unsigned short dostal;             //pocet zivotu, ktere mu byly ubrany poslednim zasahem
  char user_data;         //data uzivatelem definovane - treba pro spec.
  }TMOB;


extern TMOB mobs[MAX_MOBS];
extern char *mob_map;

void draw_mob(int num,int curdir,int celx,int cely,char shiftup);
void calc_mobs(void);
void najdi_cestu(word start,word konec,int flag,word **cesta, int aimbig);
void sirit_zvuk(word start);
void check_all_mobs(void);
void build_all_players(void);
void init_mobs(void);
void refresh_mob_map(void);
char akce_moba_zac(TMOB *m);
void mob_animuj(void); //animuje prave bojovou akci potvor na sektoru
void sleep_enemy(char regen);
int vyber_potvoru(int sect,int dir,int *z);
void mob_hit(TMOB *mm,int dostal);
int mob_alter(int sect);
void check_all_mobs_battle(void); //kontroluje zda je nekdo v battle
void manashield_check(short *vls,short *lives,short *mana,int dostal);
char track_mob(int sect,int dir);//trackuje pritomnost potvory v urcitem smeru
void stop_all_mobs(void);
int utok_na_sektor(THUMAN *p,TMOB *m,int chaos,int bonus, int ruka);
int vyber_potvoru(int sect,int dir,int *chaos); //vybere potvoru ze sektoru a smeru. Vraci take pocet potvor v promenne *chaos
void load_enemies(short *data,int size,int *grptr,const TMOB *template,int32_t tsize);
char mob_test_na_bitvu(TMOB *p);  //nastavi p->vlajky|MOB_INBATTLE pokud potvora muze vstoupit do bitvy;
void send_mob_to(int m,word *path);
void save_enemy_paths(TMPFILE_WR *f);
int load_enemy_paths(TMPFILE_RD *f);
void regen_all_mobs(void);


//souboje
extern int mob_dostal;       // ikona pro zasah moba
extern int mob_dostal_pocet; // ciselna hodnota toho kolik mob dostal;
extern short vybrana_zbran; //zbran_kterou utoci select_player;
extern char mute_hit_sound; //nastavit, pokud je nutne zabranit zvuku vzdechy, automaticky se resetuje
extern int hromadny_utek; //1 postavy utikaji hromadne.

extern char running_battle;

#define MAX_WEAPON_SKILL 10

void zacni_souboj(TMOB *p,int delka,short sector);
char q_zacit_souboj(TMOB *p,int d,short sector);
void stop_mob(TMOB *p);
void start_battle(void);
int vypocet_zasahu(const short *utocnik,const short *obrance, int chaos,int  zbran,int bonusplus, char enable_finesse_rule);
void rozhodni_o_smeru(TMOB *p);
void krok_moba(TMOB *p);
void pomala_regenerace_postavy(THUMAN *p);
void umirani_postavy(THUMAN *p);
char zasah_veci(int sector,TFLY *fl);
void vymaz_zasahy(THE_TIMER *q);
char check_end_game(void);
void wire_end_game(void);
void auto_group(void);
void wire_fly_casting(int i);
void konec_kola(void);
void send_experience(TMOB *p,int dostal);
void send_weapon_skill(int druh);
void check_player_new_level(THUMAN *p);
void poloz_vsechny_predmety(void);
char player_check_death(THUMAN *p, char afterround); //pokud je battle, nezabije postavu, ale az po skonceni kola (afterround==1)
char player_hit(THUMAN *p,int zraneni,char manashield);
void enforce_start_battle(void);
void pozdrz_akci(void);
void uprav_podle_kondice(THUMAN *p,int *chaos); //upravi parametr chaos podle kondice pro obranu
THUMAN *isplayer(int sector,THUMAN *h,char death);
  /* Vraci nasledujiciho hrace na sektoru. Pokud je h==NULL vraci prvniho.
     pokud je vysledek NULL neni dalsiho hrace
     Death=1 pocita i mrtvoly
  */
int trace_path(int sector,int dir); //zjistuje zda je mozne strilet
void wzprintf(const char *text,...) __attribute__((format(printf, 1, 2)));


int numplayers(int sector,char death);
  /* Vraci pocet hracu na sektoru, death=1 pocita i mrtvoly */
TMOB *ismonster(int sector,TMOB *m);
  /* Vraci dalsi nestvuru na sektoru, pokud je m==NULL vraci prvni.
     Pokud je vysledek NULL neni nestvura na sektoru*/

void correct_level(void);



//kouzla
extern char running_anm;
extern short teleport_target;
extern char hlubina_level;
extern word *anim_render_buffer;
extern char spell_cast; //0=neni rezim vyberu kouzla;

#define isdemon(p) ((p)->stare_vls[VLS_KOUZLA] & SPL_DEMON)


void kouzla_init(void);
void test_play(int handle);
void cast(int num,THUMAN *p,int owner,char backfire);
int add_spell(int num,int cil,int owner,char noanim);
void klicovani_anm(void *target,void *source,char mirror);
//#pragma aux klicovani_anm parm [edi][esi][eax] modify [ecx edx ebx]
int get_spell_color(THUMAN *p,int num);
int get_spell_info(int num, char *txt, size_t sz);
int get_spell_mana(int num);
int get_spell_um(int num);
char ask_who(int num);
void display_spell_in_icone(int handle,int xicht);
void reinit_kouzla_full(void);
char get_rune_enable(THUMAN *p,int strnum);
void remove_all_mob_spells(void);
int save_spells(TMPFILE_WR *f);
int load_spells(TMPFILE_RD *f);
char get_spell_track(int num);
void mob_cast(int num,TMOB *m,int mob_num);
void thing_cast(int num,int postava,int sector,TMOB *victim,char noanim);//vyvolavaji veci
void area_cast(int num,int sector,int owner,char noanim);
int select_teleport_target(char nolimit);
char get_spell_teleport(int num);
void spell_throw(int cil,int what); //to je procedura ktera umoznuje potvoram strilet
void play_big_mgif_animation(int block);
void unaffect_demon(int cil); //ukonci demona pri jeho smrti
char *get_rune_name(int strnum);
void spell_sound(char *name);
void spell_teleport(int cil,int owner, int teleport_target);


//interface
#define WINCOLOR RGB555(24,24,24) // 11000 11000 0 11000
#define BAR_COLOR RGB555(15,13,11)
#define SETUP_COL1 RGB555(20,31,20)
#define SETUP_COL2 (RGB555(31,31,16) | FONT_TSHADOW)
#define S_WINPOS_X 100
#define S_WINPOS_Y 100
#define S_WINPOS_XS 320
#define S_WINPOS_YS 300

typedef struct objrec OBJREC;
typedef struct enc_file
  {
  FILE *f;
  char *to_delete;
  }ENCFILE;

void add_window(int x,int y,int xs,int ys,int texture,int border,int txtx,int txty);
int message(int butts,char def,char canc,char *keys,...);
void type_text(EVENT_MSG *msg,void **data); //event procedura (parms: X,Y,TEXT,MAX_SPACE,MAX_CHARS);
void type_text_v2(va_list args);//char *text_buffer,int x,int y,int max_size,int max_chars,int font,int color,void (*exit_proc)(char));
void zalamovani(const char *source,char *target,int maxxs,int *xs,int *ys);
const void *col_load(const void *data, int32_t *size, int h);
void open_story_file(void);
void write_story_text(char *text);
void close_story_file(void);
char labyrinth_find_path(word start,word konec,int flag,char (*proc)(word, void *),word **cesta, void *ctx);
  //tato procedura je obecne hledani cesty. Start - startovni cislo sektoru
                                          //Konec - cilove cislo sektoru
                                          //flag - je podminkovy flag pro nepruchozi steny
                                          //proc - je procedura volana pro kazdy sektor
                                          //cesta - je ukazatel na ukazatel na vyslednou cestu
                 //pokud je cesta=NULL pak vraci pouze zda cesta existuje ci nikoliv
void radio_butts_gr(OBJREC *o);
void start_check(void); //testuje stav pocitace a rozhodne zda lze program spustit
void check_number_1phase(char *exename); //check serial number! Task!//
void animate_checkbox(int first_id,int last_id,int step);
void fletna_pridej_notu(char note);
void check_fletna(THE_TIMER *t);
char fletna_get_buffer_pos(void);
char fletna_can_add_timer();
void check_global_fletna(THE_TIMER *t);
void fletna_glob_add_note(uint8_t note);


TMPFILE_RD *enc_open(const char *filename); //dekoduje a otevira TXT soubor (ENC)
void enc_close(TMPFILE_RD *fil);
int load_string_list_ex(char ***list,const char *filename);

typedef struct {
    int hprice;
    const char *message;
    char canceled;
} THAGGLERESULT;

int smlouvat_nakup(int cena,int ponuka,int posledni,int puvod,int pocet);
int smlouvat_prodej(int cena,int ponuka,int posledni,int puvod,int pocet);
THAGGLERESULT smlouvat_dlg(int cena,int puvod,int pocet,int posledni, int money,char mode);

void disable_intro(void);
//void show_jrc_logo(char *filename);


//dialogy
void call_dialog(int entr,int mob);
char save_dialog_info(TMPFILE_WR *f);
char load_dialog_info(TMPFILE_RD *f);
short *q_item_one(int i,int itnum); //test zda postava i ma vec itnum
short *q_item(int itnum,int sector); //test zda-li aspon jeden na sectoru ma vec itnum
void change_flag(int flag,char mode); //meni vlajku = 0 - reset, 1 - set, 2 - neg
char test_flag(int flag); //vraci stav vlajky;
char dialog_is_paragraph(int id); //vraci zda-li je id dialogem
char dialog_set_notvisited(int pgf); //nastavi ze stranka nebyla navstivena
extern char trace_dialogs;


//generator
char enter_generator(void);

//kniha
#define add_to_book(odst) add_text_to_book(texty_knihy,odst)
void add_text_to_book(const char *filename,int odst);
void write_book(int page);
int count_pages(void);
void save_book(void);
void load_book(void);
void prekodovat(char *c);
char clk_enter(int id,int xa,int ya,int xr,int yr);

//menu
int enter_menu(char open); //task!
void titles(va_list args); //task!
void run_titles(void );
void effect_show(void); //effektni zobrazeni
void konec_hry(void);


//globmap
void wire_global_map(void);
void wire_automap_file(char *mapfile);
char set_select_mode(char mode);

void macro_load_another_map(const TMA_LOADLEV *);

void PodporaStitu(THUMAN *h, short *vlastnosti);

typedef struct _tag_globalEventDef
{
  unsigned short sector;	  //sektor of action target, when event occured
  unsigned char side;		  //side of action target, when event occured
  unsigned char cancel;		  //
  int32_t param;		  //event depend param - zero is default
}SGlobalEventDef;

extern SGlobalEventDef GlobEventList[MAGLOB_NEXTID];

static __inline char GlobEvent(int event, int sector, int side)
{
  if (GlobEventList[event].sector || GlobEventList[event].side)
	call_macro_ex(sector*4+side,MC_INCOMING,GlobEventList[event].sector*4+GlobEventList[event].side);
  return !GlobEventList[event].cancel;
}


static __inline char GlobEvents(int firstevid, int lastevid, int sector, int side, int32_t param)
{
  int i;
  for (i=firstevid;i<=lastevid;i++) if (GlobEventList[i].param==param)
  {
	return GlobEvent(i,sector,side);
  }
  return 1;
}


static __inline char TimerEvents(int sector, int side, int32_t time)
{
  int i;
  for (i=MAGLOB_ONTIMER1;i<=MAGLOB_ONTIMER4;i++) if (GlobEventList[i].param && GlobEventList[i].param<=time)
  {
    GlobEventList[i].param=0;
	return GlobEvent(i,sector,side);
  }
  return 1;
}

static __inline int quit_request_as_escape(int c) {
    if (c == E_QUIT_GAME_KEY) return 0x011B;else return c;
}

void draw_console_window(void);
void console_show(char show);
char console_is_visible(void);

char *map_hash_to_string_impl(char *c, uint32_t h, int sz);
#define map_hash_to_string(h) (map_hash_to_string_impl((char *)alloca(31), h, 30))
const char *find_map_from_hash_impl(char *c, uint32_t h, int sz);
#define find_map_from_hash(h) (find_map_from_hash_impl((char *)alloca(31), h, 30))
char can_select_player(THUMAN *p, char select_dead, char select_far);
char generate_string_tables(const char *path);

char *change_extension_support(char *buffer, const char *filename,char *new_extension);
#define set_file_extension(filename, extension) change_extension_support((char *)alloca(strlen(filename)+strlen(extension)+1), (filename), (extension))
void load_enemy_to_map(int i, int sector, int dir, const TMOB *t);

void hide_boat();

//extras
#include "extras.h"
