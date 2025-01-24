#define WINCOLOR RGB555(24,24,24)
#define LABELCOLOR RGB555(0,0,15)

#define OK_BUTT 10000
#define CANCEL_BUTT 10001
#define YES_BUTT 10002
#define NO_BUTT 10003

#define SID ".SID"
#define CEI ".CEI"
#define FLR ".FLR"
#define OBL ".OBL"
#define TXT ".TXT"
#define TEXT_FILE "..\\popisy.txt"
#define XITEM_FILE "ITEMS.DAT"
#define XDLG_SCRIPT "dialogy.scr"
#define XWEAPONS_SCRIPT "weapons.scr"

#define SAMPLE_PATH sample_path
#define XMUSIC_PATH "..\\music\\"

#define XSHOP_NAME "shops.dat"
#define ITEMS_DAT ITEM_FILE
#define XMOB_FILE "enemy.dat"
#define XMOB_SOUND "SOUND.DAT"

#define ITEM_FILE mapFiles.items_dat
#define DLG_SCRIPT mapFiles.dialogy_scr
#define WEAPONS_SCRIPT mapFiles.weapons_scr


#define SHOP_NAME mapFiles.shops_dat
#define MOB_FILE mapFiles.enemy
#define MOB_SOUND mapFiles.enemy_sound

typedef struct _MapFilesTag
{
  char *items_script;
  char *items_pics;
  char *items_dat;
  char *dialogy_scr;
  char *weapons_scr;
  char *shops_dat;
  char *enemy;
  char *enemy_sound;
}SMapFiles;

extern SMapFiles mapFiles;

#define concat(c,a,b) strcpy((c=alloca(strlen(a)+strlen(b)+1)),a),strcat(c,b)

extern char *sample_path;
extern word *icones,*vga_font;
extern word icone_color[7];
extern char **actions;
extern char **steny2;
extern char **zivly;
extern char **sector_types;
extern char **act_types;
extern char **typy_zbrani;
extern char **typy_veci;
extern char **umisteni_veci;
extern char **side_flgs;
extern word menu_win;
extern int sektor_win;
extern draw_win;
extern m_zoom;
extern long xmap_offs,ymap_offs,cur_layer;
extern char sekceid[8];
extern char filename[MAX_PATH];
extern int test_mode;
extern int multiaction_win;
extern int enemy_win;
extern long wiz_tool_numb;
extern int item_win;
extern int vzor_win;
extern int selected_place;
extern char **config_file;
extern char *script_name;

#define MSB_OK "Ok",NULL
#define MSB_ANONE "Ano","Ne",NULL
#define MSB_ANONEZRUSIT "Ano","Ne","Zru¨it",NULL
#define MSB_OKZRUSIT "Ok","Zru¨it",NULL
#define SCROLLBARCOL RGB(0,20,0)

void graph_init(char windowed);
long def_window(word xs,word ys,char *name);
void def_dialoge(word x,word y,word xs, word ys, char *name);
void zoomin();
void zoomout();
char *pripona(char *fname,char *prip);
void browser(const char *pathname,char *filename);
void call_testmap(int);
void editor_veci();
void pcxviewer();

void shop_train_edit();
void save_items();
void load_items();
void save_all_shops();
void load_all_shops();
void re_build_shop_list(void *,void *, int);
void mapy_init(void);
void init_multiactions();
void update_multiactions();
void *save_macros();
void load_macros(void *data);
void play_wav(va_list args);
void save_sound_map();
void load_sound_map();
void load_background();
void mob_test_sound();
void check_unused_sounds();
int add_mob_sound(char *wav);
void mob_sound_copy();
extern TSTR_LIST enemy_sound;
void edit_vyklenek(int idnum);
void create_isort_list(TSTR_LIST *ls_sorts,int filter);
void item_sound_call(void *it);


void remove_items_from_sector(int sector);
void remove_mobs_from_sector(int sector);

char ask_password(char *pass,char text);
char check_data_password(void);
char *set_data_password(char *);

char **read_ddl_dir(char *filter);
void read_ddl_list_wav(char ***list);


