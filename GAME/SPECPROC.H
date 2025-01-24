

#define SMPR_WALK  1
#define SMPR_ATTACK 2
#define SMPR_KNOCK 3 //povoleni, nebo zakazani KNOCK (zakazani = return 1)

#define SIPR_USAGE 1
#define SIPR_PICK 2
#define SIPR_DROP 3


typedef char (*t_mob_proc)(int event_type,TMOB *);
typedef char (*t_item_proc)(int event_type,short *ptr,THUMAN *p);
typedef char (*t_map_proc)(int sector,int side,int value,int event);

#define MOB_PROC(name) static char name(int event,TMOB *m)
#define ITEM_PROC(name) static char name(int event,short *ptr,THUMAN *p)
#define MAP_PROC(name) static char name(int sector,int side,int value,int event)

//specproc pro nestvury vraci char informaci
// 1 - nestvura ma jiz zadanou akci a program se zaridi podle pozadavku specproc
// 0 - specproc ignorovala udalost a nestvura se ma chovat stejne jako bez specproc

//specproc do mapy
// 1 - specproc uspesna
// 0 - specproc neuspesna
// value - je cislo predavane specproc


char call_map_event(int event_number,int sector,int side,int value,int event);
char call_item_event(int event_number,int event_type,short *ptr,THUMAN *p);
char call_mob_event(int event_number,int event_type,TMOB *m);


//funkce z enemy.c
char mob_check_next_sector(int sect,int dir,char alone,char passable);
                                //alone = MOB_BIG, passable= 1 je-li pruchozi
extern char mob_go_x[];
extern char mob_go_y[];
