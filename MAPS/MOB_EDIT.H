#define MOB_DIR mob_dir
#define MOB_VER 0x100


extern char *mob_dir;
void enemy_window();
void init_mob_list();
extern int enemy_win;
extern short mob_map[MAPSIZE];
void select_enemy(int at_sector);
void save_mobs();
void load_mobs();
void *save_mob_map(long *size);
void load_mob_map(void *data,int size);
void pcx_view(OBJREC *o);
void load_mobs_to_map(void *p,long size);
void *save_mobs_to_map(long *size);
void load_sound_dat(void *p,long siz);
int save_sound_dat(FILE *f,int id);

