#include <stdarg.h>

#ifndef SKELDAL_LIB_GUI
#define SKELDAL_LIB_GUI

#define E_MS_CLICK 50
#define E_MS_MOVE 51
#define E_GET_FOCUS 52
#define E_LOST_FOCUS 53
#define E_KEY_PRESS 54
#define E_CHANGE 55
#define E_CURSOR_TICK 56
#define E_REDRAW 57 //redraw desktop
#define E_GUI 58    //direct enter to gui system
#define E_CONTROL 59 //User defined feature, enables direct controling desktop objects

#define CURSOR_SPEED 5;
#define get_title(title) (char *)*(int32_t *)(title);
#define DESK_TOP_COLOR RGB555(0,15,15);
#define MINSIZX 60
#define MINSIZY 40


#define SCR_WIDTH_X DxGetResX()
#define SCR_WIDTH_Y DxGetResY()


typedef struct ctl3d
  {
  word light,shadow,bsize,ctldef;
  }CTL3D;

typedef word FC_TABLE[7];
typedef FC_TABLE FC_PALETTE[16];

//Run_routs:
/*   0 - INIT
     1 - DRAW
     2 - EVENT
     3 - DONE

     0 - WHEN EVENT
     1 - AFTER GOT FOCUS
     2 - BEFORE LOST FOCUS
     3 - WHEN VALUE CHANGED / WHEN AKTIVATE BUTTON
*/

//objects
/*   prototypy jednotlivych funkci
     INIT(OBJREC *object,VOID *initparms);
     DRAW(int x1,int y1,int x2,int y2,OBJREC *object);
     EVENT(EVENT_MSG *msg, OBJREC *object);
     DONE(OBJREC *object);
*/

typedef struct objrec OBJREC;

typedef void (*RUN_ROUTS[4])(OBJREC *, va_list);

typedef struct objrec
  {
  short x,y,xs,ys;
  CTL3D border3d;
  word color;
  word id;
  char align,autoresizex,autoresizey;
  char enabled;
  short locx,locy;
  int32_t datasize;
  void *data;
  FC_TABLE f_color;
  const word *font;
  void *userptr;
  void (*call_init)(struct objrec *, va_list);
  void (*call_draw)(int , int, int, int, struct objrec *);
  void (*call_event)(EVENT_MSG *msg, struct objrec *);
  void (*call_done)(struct objrec *);
  void (*on_event)(EVENT_MSG *msg, struct objrec *);
  void (*on_enter)(void);
  void (*on_exit)(void);
  void (*on_change)(void);
  char draw_error;       //1 znamena ze objekt zpusobil chybu a nebude vykreslovan
  struct objrec *next;
  }OBJREC;

// align urcuje vzhledem ke ktermu rohu je vypocet souradnic vztazen
/* align = 0  levy horni roh
   align = 1  pravy horni roh
   align = 2  pravy dolni roh
   align = 3  levy dolni roh

   autoresize=1 znamena, ze object zmeni svou velikost tak aby jeho
   pravy dolni roh sledoval pravy dolni roh okna a levy horni roh sledoval
   levy horni roh okna
*/

typedef struct tidlist
  {
  struct tidlist *next;
  OBJREC *obj;
  }TIDLIST;


typedef struct window
  {
  short x,y,xs,ys;
  CTL3D border3d;
  word color;
  OBJREC *objects;
  int32_t id;
  char modal,minimized,popup;
  word minsizx,minsizy;
  char *window_name;
  void (*draw_event)(struct window *);
  struct window *next;
  TIDLIST *idlist;
  }WINDOW;

extern WINDOW *desktop,*waktual;
extern OBJREC *o_aktual,*o_end,*o_start;
extern CTL3D noneborder;
extern FC_TABLE f_default;
extern word desktop_y_size;
//extern char change_flag;
extern const word *default_font;
extern void *gui_background;




void draw_border(integer x,integer y,integer xs,integer ys,CTL3D *btype);
WINDOW *create_window(int x,int y, int xs, int ys, word color, CTL3D *okraj);
int32_t desktop_add_window(WINDOW *w);
void select_window(int32_t id);
WINDOW *find_window(int32_t id);
void redraw_object(OBJREC *o);
void redraw_window(void);
void define(int id,int x,int y,int xs,int ys,char align,void (*initproc)(OBJREC *),...);
CTL3D *border(word light,word shadow, word bsize, word btype);
void property(CTL3D *ctl,const word *font,FC_TABLE *fcolor,word color);
FC_TABLE *flat_color(word color);
void aktivate_window(MS_EVENT *ms);
void redraw_desktop(void);
void close_window(WINDOW *w);
void close_current(void);
void check_window(WINDOW *w);
void install_gui(void);
void uninstall_gui(void);
void on_control_change(void (*proc)(void));
void on_control_enter(void (*proc)(void));
void on_control_exit(void (*proc)(void));
void on_control_event(void (*proc)(EVENT_MSG *, struct objrec *));
void terminate_gui(void);
void set_change(void);
void set_value(int win_id,int obj_id,void *value);
void set_value_bin(int win_id,int obj_id,void *value, size_t value_size);
void set_default(const char *value);
void set_default_bin(const void *value, size_t value_size);
void c_set_value(int win_id,int obj_id,int cnst);
void c_default(int cnst);
int f_get_value(int win_id,int obj_id);
void get_value(int win_id,int obj_id,void *buff);
void cancel_event(void);
OBJREC *find_object(WINDOW *w,int id);
void set_window_modal(void);
void set_enable(int win_id,int obj_id,int condition);
void run_background(void (*p)(void));
void disable_bar(int x,int y,int xs,int ys,word color);
void movesize_win(WINDOW *w, int newx,int newy, int newxs, int newys);
void goto_control(int obj_id);

#endif









