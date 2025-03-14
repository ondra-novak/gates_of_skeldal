#ifndef __ENGINE1_H
#define __ENGINE1_H
#define VIEW_SIZE_X 640
#define VIEW_SIZE_Y 360
#define TAB_SIZE_X 640
#define TAB_SIZE_Y 600
#define MIDDLE_X 320
#define MIDDLE_Y 112
#define TXT_SIZE_Y 320
#define TXT_SIZE_X_3D 74
#define TXT_SIZE_X 500
#define VIEW3D_X 4
#define VIEW3D_Z 5
#define START_X1 357
#define START_Y1 305
#define START_X2 357
#define START_Y2 -150
#define FACTOR_3D 3.33
#define ZOOM_PHASES 9
#define SCREEN_OFFLINE (17)
#define SCREEN_OFFSET (GetScreenPitch()*SCREEN_OFFLINE)
#define C_YMAP_SIZE 90
#define F_YMAP_SIZE 199
#define CF_XMAP_SIZE 7

#define SHADE_STEPS 5
#define SHADE_PAL (SHADE_STEPS*512*2)


void general_engine_init(void);
void draw_floor_ceil(int celx,int cely,char f_c,const void *txtr);
void show_cel2(int celx,int cely,const void *stena,int xofs,int yofs,char rev, int alpha);
  //zobrazi primou stenu ktera lezi pred nebo napravo od pozorovatele
void show_cel(int celx,int cely,const void *stena,int xofs,int yofs,char rev, int alpha);
  void turn_left(void);
void turn_right(void);
void zooming_backward(const word *background);
void zooming_forward(const word *background);
void OutBuffer2nd(void);
void CopyBuffer2nd(void);
void report_mode(int);
void draw_item(int celx,int cely,int posx,int posy,const short *pic,int index);
void draw_item2(int celx,int cely,int xpos,int ypos,const void *texture,int index);
//void textmode_effekt(void);
//#pragma aux textmode_effekt modify[eax ebx ecx edx edi];


void clear_buff(word *background,word backcolor,int lines);

typedef struct zoominfo
  {
     void *startptr;
     const void *texture;
     int32_t texture_line,line_len;
     int32_t *xtable;
     short *ytable;
     word *palette;
     word ycount;
     word xmax;
     const void *texture_end;
  }ZOOMINFO;


typedef struct t_info_y
  {
  int32_t drawline; //ukazatel na radku na ktere bude stena zacinat
  word vert_size; //konecna velikost steny, pokud ma pocatecni velikost TXT_SIZE_Y
  word vert_total; //maximalni velikost textury aby jeste nepresahla obrazovku
  short zoom_table[TAB_SIZE_Y];  //tabulka pro zoomovaci rutiny
  }T_INFO_Y;

typedef struct t_info_x_3d
  {
  char used;  // 1 pokud je tato strana videt
  integer xpos;      //bod od leveho okraje
  word txtoffset; //posunuti x vuci texture
  word point_total; //rozdil mezi levym prednim a levym zadnim okrajem postranni steny (v adresach)
  int32_t zoom_table[VIEW_SIZE_X]; //zoomovaci tabulka pro osu x pro postranni steny
  }T_INFO_X_3D;

typedef struct t_info_x
  {
  char used;  // 1 pokud je tato strana videt
  integer xpos;  //bod od leveho okraje
  integer xpos2; //totez ale pro pravou stranu
  word txtoffset; //posunuti x vuci texture
  word max_x; //pocet viditelnych bodu z textury
  word point_total;  //celkovy pocet adres mezi levym a pravym okrajem
  int32_t zoom_table[VIEW_SIZE_X]; //zoomovaci tabulka pro osu x pro kolme steny
  }T_INFO_X;

typedef struct t_floor_map
  {
  int32_t lineofs,linesize,counter,txtrofs;
  }T_FLOOR_MAP;

typedef struct all_view
  {
  T_INFO_Y y_table[VIEW3D_Z+1];
  T_INFO_X_3D z_table[VIEW3D_X][VIEW3D_Z];
  T_INFO_X x_table[VIEW3D_X][VIEW3D_Z+1];
  T_FLOOR_MAP f_table[CF_XMAP_SIZE][F_YMAP_SIZE];
  T_FLOOR_MAP c_table[CF_XMAP_SIZE][C_YMAP_SIZE];

  }ALL_VIEW;

typedef struct t_point
  {
  int x,y;
  }T_POINT;

typedef T_POINT t_points[VIEW3D_X+1][2][VIEW3D_Z+1];
extern word *background;
extern t_points viewport_geometry;
extern word *buffer_2nd;
extern char show_names;
extern char show_lives;
extern char secnd_shade;
extern char show_mob_info;

typedef short palette_t[256];

typedef struct drw_enemy_struct
  {
  const void *txtr;
  int celx,cely,posx,posy,adjust,shiftup,num;
  char mirror;
  char stoned;
  const palette_t *palette;
  const char *more_info;
  }DRW_ENEMY;




void enemy_draw(const void *src,void *trg,int shade,int scale,int maxspace,int clip);
//#pragma aux enemy_draw parm[ESI][EDI][EBX][EDX][EAX][ECX]
void enemy_draw_transp(const void *src,void *trg,  const void *shade,int scale,int maxspace,int clip);
//#pragma aux enemy_draw_transp parm[ESI][EDI][EBX][EDX][EAX][ECX]
void enemy_draw_mirror_transp(const void *src,void *trg,  const void *shade,int scale,int maxspace,int clip);
//#pragma aux enemy_draw_mirror_transp parm[ESI][EDI][EBX][EDX][EAX][ECX]
void enemy_draw_mirror(const void *src,void *trg,int shade,int scale,int maxspace,int clip);
//#pragma aux enemy_draw_mirror parm[ESI][EDI][EBX][EDX][EAX][ECX]
//clip je v poradi vpravo - vlevo (HiLo)

void draw_enemy(DRW_ENEMY *drw);
void draw_player(const short *txtr,int celx,int cely,int posx,int posy,int adjust,  const char *name);

void set_lclip_rclip(int celx,int cely,int lc,int rc);
void draw_spectxtr(const short *txtr,int celx,int cely,int xpos);

int turn_speed(int turnspeed); //oba je nutne volat na zacatku
int zoom_speed(int zoomspeed);

void scroll_and_copy(const void *pic,void *slide, void *scr, int size,int shift, void *lineinfo);
//#pragma aux scroll_and_copy parm[esi][ebx][edi][ecx][edx][eax]

void set_backgrnd_mode(int mode);

int get_item_top(int celx,int cely,int posx,int posy,const word *txtr,int index);
 //vraci nejnizsi souradnici y predmetu leziciho na zemi v celx, cely na pozici posx,posy;

#endif
