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


#define MAPSIZE 100000
#define ITEM_SORTS 2000
#define MIN_DEPTH -20
#define MAX_DEPTH 20
#define M_ZOOM (1<<m_zoom)
#define TOTAL_MOBS 256

#define XITEMS_SCRIPT "items.scr"
#define XITEMS_PICS "items.pic"

#define ITEMS_SCRIPT mapFiles.items_script
#define ITEMS_PICS mapFiles.items_pics


typedef struct tstena
  {
  char prim,sec,oblouk,side_tag;
  unsigned short sector_tag;
  char xsec,ysec;
  unsigned long flags;
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

extern TVYKLENEK vyklenky[256];
extern char side_chscr[];

typedef struct tmap
  {
  TSTENA sidedef[MAPSIZE][4];
  TSECTOR sectordef[MAPSIZE];
  }TMAP;

typedef struct tmap_edit_info
  {
  short x,y,layer,flags;
  }TMAP_EDIT_INFO;

typedef TMAP_EDIT_INFO TMAP_EDIT[MAPSIZE];

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
   char mappassw[54];
   }MAPGLOBAL;



#define U_HLAVA     0x1
#define U_KRK       0x2
#define U_TELO      0x4
#define U_RUCE      0x8
#define U_LOKTE     0x10
#define U_PAS       0x20
#define U_NOHY      0x40
#define U_CHODIDLA  0x80
#define U_RAMENA    0x100
#define U_ZADA      0x200
#define U_RESERVED  0x80000000

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
#define VLS_DAMAGE   22
#define VLS_KOUZLA  23



#define ZVL_OHEN    1
#define ZVL_VODA    2
#define ZVL_ZEME    3
#define ZVL_VZDUCH  4
#define ZVL_MYSL    5


#define IT_ARMOR    1
#define IT_WEAPON   2
#define IT_SCROLL   3
#define IT_SHOOTING 4
#define IT_THROW    5
#define IT_LEKTVAR  6
#define IT_JIDLO    7
#define IT_BATOH    8
#define IT_SPECIAL  9
#define IT_OTHER    0
#define IT_MAPLOC   256
#define IT_MONSTER  257


extern TSTR_LIST ls_sorts;
typedef short TVLASTNOST[2];

typedef struct titem
  {
  char jmeno[32];   //32        Jmeno predmetu
  char popis[32];   //64
  short zmeny[24];  //112       Tabulka, jakych zmen ma na hracovy vlastnosti
  short podminky[4];//120       Tabulka, jake vlastnosti musi mit hrac k pouziti predmetu
  short hmotnost,nosnost,druh; //126  druh = Typ predmetu
  short umisteni;              //128  Kam se predmet umisti?
  short flags;                 //130  ruzne vlajky
  short spell,magie,silaspell; //136  specialni kouzla
  short use_event;             //140  specialni udalost
  unsigned short ikona,vzhled; //144  ikony a vzhled
  short user_value;            //146  uzivatelska hodnota
  short keynum;                //148 cislo klice
  short polohy[2][2];          //157 souradnice poloh pro zobrazeni v inv
  char  typ_zbrane;            //160 typ zbrane
  char  fly_flags;             //158 fly_flags
  word sound;                //    Rezevovano
  short v_letu[16];            //192 cisla obrazku vyjadrujici letici predmet
  int cena;
  char animace;
  char hitpos;
  char shiftup;              //posunuti ve vyklenku
  char rez;
  short rezerva[12];           //224 rezervovane
  }TITEM;


typedef struct snd_stamp
  {
  unsigned short sector;      //2
  char side,name[12],flags;   //16
  long strsmp,strloop,endloop; //28
  unsigned short freq;    //30
  unsigned short volume;  //32
  }TSMP_STAMP;

extern PTRMAP *smplist;

#define SMP_START 0x1
#define SMP_MUSIC 0x2
#define SMP_ANIM  0x4

extern TMAP mapa;
extern TMAP_EDIT minfo;
extern long map_win;
extern long tool_bar;
extern int maplen;
extern char tool_sel;
extern MAPGLOBAL mglob;

extern word icone_color[7];

extern TITEM item_list[ITEM_SORTS];
extern short *item_place[MAPSIZE*4];
extern char chka[];

extern short *item_place[MAPSIZE*4];
extern short max_items;

void create_map_win(int xp,...);
void unselect_map(void);
void set_defaults(void);
int add_sector(int x,int y,int layer,int source_num);
void wire_sector(int sector);


void init_item_system(void);
void ikris(int,int,int,int,int);
void select_item(int itpos);
void change_tools();


char save_item_map(FILE *f,int id);
void load_item_map(void *data,int size);

#define MAX_ACTIONS 40
#define MA_GEN 0
#define MA_SOUND 1
#define MA_TEXTG 2
#define MA_TEXTL 3
#define MA_SENDA 4
#define MA_FIREB 5
#define MA_DESTR 6
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
#define MA_TELEP 22
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
#define MA_THEND 33
#define MA_GOMOB 34
#define MA_SHRMA 35
#define MA_MUSIC 36
#define MA_GLOBE 37  //global events
#define MA_IFSEC 38	 //if sector num
#define MA_IFSTP 39  //if sector type

#define MAGLOB_LEAVEMAP 0 // v urcitou nastavenou hodinu a minutu dene
#define MAGLOB_STARTSLEEP 1 // postavy maji jit spat.
#define MAGLOB_ENDSLEEP 2 // postavy se probouzi
#define MAGLOB_CLICKSAVE 3  // pøed otevøením SAVE dialogu
#define MAGLOB_AFTERSAVE 4  // po uložení hry
#define MAGLOB_BEFOREMAGIC 5 // pøed vyvoláním kouzla
#define MAGLOB_AFTERMAGIC 6 //po vyvolání kouzla
#define MAGLOB_BEFOREMAPOPEN 7 //pøed otevøením mapy
#define MAGLOB_AFTERMAPOPEN 8 //po uzavøení mapy
#define MAGLOB_BEFOREBATTLE 9 //pøed spuštìním souboje
#define MAGLOB_AFTERBATTLE 10 //po ukonèení souboje
#define MAGLOB_BEFOREBOOK 11 //pred otevøením knihy
#define MAGLOB_AFTERBOOK 12 //po uzavøení knihy
#define MAGLOB_ONROUND 13 //pøi každém kole nebo po 10s
#define MAGLOB_ONDEADMAN 14 //pøi umrtí muže
#define MAGLOB_ONDEADWOMAN 15 //pøi umrtí ženy
#define MAGLOB_ONDEADALL 16 //pøi umrtí všech postav
#define MAGLOB_ONHITMAN 17 //pøi zásahu muže
#define MAGLOB_ONHITWOMAN 18 //pøi zásahu ženy
#define MAGLOB_ONNEWRUNE 19 //pøi nalezení nové runy
#define MAGLOB_ONPICKITEM 20 //pøi sebrání pøedmìtu (pro speciální pøedmìty)
#define MAGLOB_ONSTEP 21 //pøi kroku (pøed animaci)
#define MAGLOB_ONTURN 22 //pøi otoèení (pøed animaci)
#define MAGLOB_ALARM 23 //pøi spuštìní alarmu
#define MAGLOB_ONFIREMAGIC 24
#define MAGLOB_ONWATERMAGIC 25
#define MAGLOB_ONGROUNDMAGIC 26
#define MAGLOB_ONAIRMAGIC 27
#define MAGLOB_ONMINDMAGIC 28
#define MAGLOB_ONSPELLID1 29  //pøi jednom konrétním kouzle
#define MAGLOB_ONSPELLID2 30  //pøi jednom konrétním kouzle
#define MAGLOB_ONSPELLID3 31  //pøi jednom konrétním kouzle
#define MAGLOB_ONSPELLID4 32  //pøi jednom konrétním kouzle
#define MAGLOB_ONSPELLID5 33  //pøi jednom konrétním kouzle
#define MAGLOB_ONSPELLID6 34  //pøi jednom konrétním kouzle
#define MAGLOB_ONSPELLID7 35  //pøi jednom konrétním kouzle
#define MAGLOB_ONSPELLID8 36  //pøi jednom konrétním kouzle
#define MAGLOB_ONSPELLID9 37  //pøi jednom konrétním kouzle
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

#define MAGLOB_NEXTID 50 //musí být poslední







typedef struct tma_gen
  {
 unsigned action : 6;
 unsigned cancel : 1;
 unsigned once   : 1;
 unsigned ps:1;
 unsigned pf:1;
 unsigned ts:1;
 unsigned tf:1;
 unsigned li:1;
 unsigned ul:1;
 unsigned ia:1;
 unsigned sp:1;
 unsigned as:1;
 unsigned am:1;
 unsigned a2:1;
 unsigned pa:1;
 unsigned us:1;     //uspesne spec proc
 unsigned od:1;     //otevreni dveri
 unsigned vy:1;
 }TMA_GEN;

typedef struct tma_sound
  {
  char action,flags,eflags; //3
  char bit16;             //4
  char volume;             //5
  char soundid;            //6
  unsigned short freq;     //8
  long start_loop,end_loop,offset;//20
  char filename[12];       //32
  }TMA_SOUND;


typedef struct tma_text
  {
  char action,flags,eflags,pflags;
  long textindex;
  }TMA_TEXT;

typedef struct tma_cactn
  {
  char action,flags,eflags,pflags;
  short sector,dir;
  }TMA_CACTN;

typedef struct tma_swapsectors
  {
  char action,flags,eflags,pflags;
  short sector1,sector2;
  }TMA_SWAPSC;

typedef struct tma_wounds
  {
  char action,flags,eflags,pflags;
  short minor,major;
  }TMA_WOUND;


typedef struct tma_two_parms
  {
  char action,flags,eflags;
  short parm1,parm2;
  }TMA_TWOP;




typedef struct tma_send_action
  {
  char action,flags,eflags,change_bits;
  unsigned short sector,side,s_action;
  char delay;
  }TMA_SEND_ACTION;

typedef struct tma_fireball
  {
  char action,flags,eflags;
  short xpos,ypos,zpos,speed,item;
  }TMA_FIREBALL;

typedef struct tma_loadlev
  {
  char action,flags,eflags;
  short start_pos;
  char dir;
  char name[13];
  }TMA_LOADLEV;

typedef struct tma_dropitm
  {
  char action,flags,eflags;
  short item;
  }TMA_DROPITM;


typedef struct tma_codelock
  {
  char action,flags,eflags;
  char znak;
  char string[8];
  char codenum;
  }TMA_CODELOCK;

typedef struct tma_lock
  {
  char action,flags,eflags;
  short key_id;
  short thieflevel;
  }TMA_LOCK;


typedef struct tma_uniq
  {
  char action,flags,eflags;
  TITEM item;
  }TMA_UNIQ;

typedef struct tma_globe
  { 
  char action,flags,eflags,event; //event - MAGLOB_XXXX
  unsigned short sector;	  //sektor of action target, when event occured
  unsigned char side;		  //side of action target, when event occured
  unsigned char cancel;		  //1 - cancel event
  unsigned long param;		  //event depend param - zero is default
  }TMA_GLOBE;


typedef struct tma_ifsec
{
  char action,flags,eflags;
  unsigned char side;		  //side of action target, when event occured
  unsigned short sector;	  //sektor of action target, when event occured
  short line;		  //jump line
  char invert;				  //invert condition
}TMA_IFSEC;



typedef union tmulti_action
  {
  struct tma_gen general;
  struct tma_sound sound;
  struct tma_text text;
  struct tma_send_action send_a;
  struct tma_fireball fireball;
  struct tma_loadlev loadlev;
  struct tma_dropitm dropi;
  struct tma_codelock clock;
  struct tma_cactn cactn;
  struct tma_lock lock;
  struct tma_swapsectors swaps;
  struct tma_wounds wound;
  struct tma_two_parms twop;
  struct tma_uniq uniq;
  struct tma_globe globe;
  struct tma_ifsec ifsec;
  }TMULTI_ACTION;

extern TMULTI_ACTION **multi_actions[MAPSIZE*4];



//enemies

#define MOBS_INV 16

#define MBS_GUARD       0  // stoji na miste a strazi
#define MBS_WAITING     1  // stoji na miste a automaticky utoci
#define MBS_WALKING     2  // chodi po mistnosti a nevsima si niceho
#define MBS_LISTENING   3  // stoji a posloucha. Pri zvuku se sebere a jde tim smerem
#define MBS_ANGRY       4  // chodi a zautoci na prvniho nepritele
#define MBS_ALIEN       5  // nahodne chodi po bludisti a chova se zakernicky

#define MIQ_STAY        0  // nepronasleduje sve nepratele
#define MIQ_PURSUE      1  // pokud nekoho zahledne jde za nim
#define MIQ_RUN         2  // stane se alienem
#define MIQ_FLEE        3  // potvora utika od sveho protivnika


#define MANI_FORWARD     0
#define MANI_LEFT        1
#define MANI_BACKWARD    2
#define MANI_RIGHT       3
#define MANI_COMBAT      4
#define MANI_TOHIT       5

#define MOB_SOUNDS       4

typedef struct tmob
  {
  char name[30];           //jmeno moba
  short casting;           //cislo kouzla
  short adjusting[16*6];     //volba stredu pro animace
  word sector,dir;        //pozice
  char locx,locy;         //presna pozice
  char headx,heady;        //pozice kam mob miri
  word anim_counter;        //citac animaci
  short vlastnosti[24];     //zakladni vlastnosti potvory
  short inv[MOBS_INV];      //batoh potvory
  short lives;              //pocet zivotu potvory
  short cislo_vzoru;         //informace urcujici ze ktereho vzoru byl mob vytvoren
  short speed;             //rychlost pohybu
  short dohled;            //kam dohl‚dne
  short dosah;             //okam‘ik za‡ tku souboje
  char stay_strategy;      //chovani moba ve statickem modu (nepronasleduje)
  char walk_data;           //cislo potrebne pro pohyb moba v bludisti
  word bonus;              //bonus za zabiti
  char flee_num;             //pravdepodobnost uteku
  char anim_counts[6];     //pocet animacnich policek pro kazdy pohyb
  char mobs_name[7];       //zaklad jmena souboru pro moba
  long experience;          //zkusenost
  char vlajky;             //BIT0 - 1 v boji
  char anim_phase;            //??
  short csektor;            //Cilovy sektor
  short home_pos;          //domaci pozice
  short next;              //Cislo dalsiho moba, ktery stoji na jeho pozici
  char actions;            //pocet akci ktere muze potvora provest v kole
  char hit_pos;           //animacni pozice kdy mob zasahne
  word sounds[MOB_SOUNDS];         //4 zvukove udalosti
  signed char paletts_count;
  char mode;
  short dialog;
  char dialog_reserved;
  word money;            //obnos ktery ma u sebe
  word specproc;         //cislo udalosti pro akci potvory
  char reserved[3];       //rezervovana data
  }TMOB;


extern TMOB moblist[TOTAL_MOBS];


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
  int products;       //celkova velikost listu, v behu ma jiny vyznam - cislo bloku obrazku
  int shop_id;
  int list_size;
  short spec_max;     //maximalni pocet specialnich predmetu
  TPRODUCT *list;
  }TSHOP;

#define TRN_EXPENSIVE 0x1 //0 - vypocet basic+koef*lev, 1 - vypocet basic+koef*2^level;
#define TRN_CONTROL 0x2 //1 - ridi max_lev podle contr_vls. Hodnota se vynasobi koef/100 a pokud je mensi nez level vlastnosti pak nedojde k nabidnuti trainingu.

typedef struct tsession
  {
  short vls;
  int basic_price;
  int koef;
  char flags;
  char max_lev;
  short contr_vls;
  short vls_koef;
  }TSESSION;

typedef struct ttraining
  {
  char trainer[16];
  char picture[12];
  int list_count;
  TSESSION *list;
  }TTRAINING;

extern TSHOP *shop_list;
extern int max_shops;
void item_edit(TITEM *it);

void calc_changes_mem(void *orgn,void *new, void *maskreg, char *scr);
void move_changes(void *source,void *target, void *chmem, long size);

