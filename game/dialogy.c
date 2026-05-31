#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>


#include <libs/types.h>
#include <libs/event.h>
#include <ctype.h>
#include <libs/memman.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/bgraph.h>
#include <platform/sound.h>
#include <libs/strlite.h>
#include <libs/mgifmem.h>
#include "engine1.h"
#include <libs/pcx.h>
#include "globals.h"
#include <stdarg.h>
#include <string.h>
#include "ach_events.h"
#include "libs/vector.h"
#include "platform/timer.h"

typedef struct t_paragraph
  {
  unsigned num:15;
  unsigned alt:15;
  unsigned visited:1;
  unsigned first:1;
  int32_t position;
  }T_PARAGRAPH;

#define STR_BUFF_SIZ 4096
#define SAVE_SPKRS 20
#define P_STRING 1
#define P_SHORT 2
#define P_VAR 3
#define P_POP 4

#define MAX_VOLEB 10

typedef struct {
    int32_t txt_window_x;
    int32_t txt_window_y;
    int32_t txt_window_xs;
    int32_t txt_window_ys;
    int32_t txt_window_line_height;

    int32_t txt_desc_width;
    int32_t txt_desc_x;
    int32_t txt_desc_y;

    int32_t desc_color;
    int32_t text_color;
    int32_t choice_color;
    int32_t sel_choice_color;
    
    int32_t pic_x;
    int32_t pic_y;
    int32_t icon_padding;
    int32_t icon_height;
    int32_t icon_size; //shrink factor

    int32_t desc_font;
    int32_t text_font;
    int32_t draw_order;

} TDIALOGY_LAYOUT;

#define DRAW_ORDER_UI_FIRST 0
#define DRAW_ORDER_PIC_FIRST 1

static TDIALOGY_LAYOUT dlg_layout = {
    17,270,606,
    94,11, 
    225, 382, 34,
    RGB555(28,28,21),NOSHADOW(0),42115,
    49252,17,17,25, 25,3, H_FBOLD, H_FBOLD,
    DRAW_ORDER_UI_FIRST };

#define TEXT_X dlg_layout.txt_window_x
#define TEXT_Y dlg_layout.txt_window_y
#define TEXT_XS dlg_layout.txt_window_xs
#define TEXT_YS dlg_layout.txt_window_ys
#define TEXT_STEP dlg_layout.txt_window_line_height
#define DESC_COLOR1 dlg_layout.desc_color

#define OPER_EQ 32
#define OPER_BIG 35
#define OPER_LOW 33
#define OPER_BIGEQ 36
#define OPER_LOWEQ 34
#define OPER_NOEQ 37

#define PIC_X dlg_layout.pic_x
#define PIC_Y (dlg_layout.pic_y+SCREEN_OFFLINE)

#define LAYOUT_FILE "DIALOGY.LAY"

#define MAX_VARIABLES 100
static short variables[100];

static THUMAN *speakers[SAVE_SPKRS];

static word *back_pic;
static char back_pic_enable=0;

static char showed=0;
static char *pc;
static char *descript=NULL;
static size_t descript_len = 0;
static char *string_buffer=NULL;
static char iff;

static char _flag_map[32];
static char _monster_flag_map[2];

static int local_pgf=0;


static char pocet_voleb=0;
static char vyb_volba=0;
static short vol_n[MAX_VOLEB];

static short save_jump;

#define MAX_DIALOG_LINE 200
#define DLG_LINE_IMAGE_SPACE 20

enum LineType {
    lt_emote,
    lt_echo,
    lt_choice
};

typedef struct {
    char line[MAX_DIALOG_LINE];
    enum LineType type;
    int8_t id;
    uint8_t xofs;
    short height;
    void *face;
} TDLG_TEXT_LINE;

static Vector dlg_text;
//static TSTR_LIST history=NULL;
static int his_line=0;
static int end_text_line=0;
static int last_his_line=0;

static int starting_shop=-1;
static int held_item=-1;

static char halt_flag=0;

static int dialog_mob=0;

static char code_page=1;

char trace_dialogs=0;

static char case_click(int id,int xa,int ya,int xr,int yr);
static char ask_who_proc(int id,int xa,int ya,int xr,int yr);


void wire_dialog_drw(void);

static void (*old_wire_proc)(void) = NULL;


#define CLK_DIALOG 5
static T_CLK_MAP clk_dialog[CLK_DIALOG]=
  {
  {0,0,0,0,0,case_click,3,H_MS_DEFAULT},
  {-1,30,0,85,14,konec,2,H_MS_DEFAULT},
  {-1,87,0,142,14,game_setup,2,H_MS_DEFAULT},
  {0,207,0,265,14,clk_saveload,2,H_MS_DEFAULT},
  {0,0,0,639,479,empty_clk,0xff,H_MS_DEFAULT},
  };

#define CLK_DLG_WHO 3
static T_CLK_MAP clk_dlg_who[CLK_DLG_WHO]=
  {
  {1,54,378,497,479,ask_who_proc,2,-1},
  {2,0,0,639,479,ask_who_proc,8,-1},
  {-1,0,0,639,479,empty_clk,0xff,-1},
  };


static int glob_y;

static int last_pgf;

static const word *paleta;
static int32_t loc_anim_render_buffer;
static short task_num=-1;

void small_anm_buff(void *target,const void *buff,const void *paleta);
//#pragma aux small_anm_buff parm[edi][esi][ebx] modify [eax ecx]
void small_anm_delta(void *target,const void *buff,const void *paleta);
//#pragma aux small_anm_delta parm[edi][esi][ebx] modify [eax ecx]

static void animace_kouzla(MGIF_HEADER_T *_,int act,const void *data,int csize)
  {
  word *p=GetScreenAdr()+loc_anim_render_buffer;
  switch (act)
     {
     case MGIF_LZW:
     case MGIF_COPY:small_anm_buff(p,data,paleta);break;
     case MGIF_DELTA:small_anm_delta(p,data,paleta);break;
     case MGIF_PAL:paleta=data;break;
     }
  }


static void dialog_anim(va_list args)
//#pragma aux dialog_anim parm []
  {
  char *block=va_arg(args,char *);
  int speed=va_arg(args,int);
  int rep=va_arg(args,int);

  const void *anm;
  void *aptr;
  char hid;
  int spdc=0,cntr=rep,tm = 0,tm2 = 0;

  int32_t scr_linelen2 = GetScreenPitch();
  loc_anim_render_buffer=PIC_Y*scr_linelen2+PIC_X;
  mgif_install_proc(animace_kouzla);
  const char *ch = build_pathname(2,gpathtable[SR_DIALOGS], block);
  free(block);
  size_t loadsize;
  aptr=load_file(ch, &loadsize);
  do
     {
     anm=open_mgif(aptr);
     char f = anm != NULL;
     while (f && task_quitmsg())
       {
       task_sleep();
       if (!spdc)
          {
          if (ms_last_event.x<=PIC_X+320 && ms_last_event.y<=PIC_Y+180)
             {
             hid=1;schovej_mysku();
             }
          else hid=0;
          f=mgif_play(anm);
          spdc=speed;
          if (hid) ukaz_mysku();
          showview(PIC_X,PIC_Y,320,180);
          }
       tm2=get_timer_value();
       if (tm!=tm2)
        {
        spdc--;tm=tm2;
        }
       }
     rep--;
     close_mgif(anm);
     }
  while (!cntr && rep && !task_quitmsg());
  free(aptr);
  }

#define MAX_STACK_SIZE 256
static int64_t script_stack[MAX_STACK_SIZE];
static int script_stack_pos = MAX_STACK_SIZE;

static void stk_push(int64_t value) {
    if (script_stack_pos == 0) {
        display_error("script stack overflow"); exit(1);
    }
    script_stack[--script_stack_pos] = value;
}
static int64_t stk_pop() {
    if (script_stack_pos >= MAX_STACK_SIZE) {
        display_error("script stack underflow"); exit(1);
    }
    return script_stack[script_stack_pos++];
}

static void stk_clear() {
    script_stack_pos = MAX_STACK_SIZE;
}


static void stop_anim()
  {
  if (task_num!=-1) term_task(task_num);
  }

static void run_anim(char *name,int speed,int rep)
  {
  char *bl;
  stop_anim();
  bl=getmem(strlen(name)+1);strcpy(bl,name);
  task_num=add_task(8196,dialog_anim,bl,speed,rep);
  }


static void show_dialog_picture()
  {
  if (!showed)
     {
     put_picture(0,SCREEN_OFFLINE,ablock(H_DIALOG));
     showed=1;
     glob_y=250;
     }
  }

static void *small_xicht(int xicht_handle) {
    const void *xicht = ablock(xicht_handle);
    int w = PICTURE_WIDTH(xicht);
    int h = PICTURE_HEIGHT(xicht)/4;
    int isz = dlg_layout.icon_size;
    int w2 = w/isz;
    int h2 = h/isz;
    uint16_t *buf;
    uint16_t *buf2;
    void *p1 = picture_create(w,h,&buf);
    memset(buf,0,w*h*2)    ;
    put_picture_ex(0, 0, xicht, buf, w, h);    

    void *p2 = picture_create(w2, h2, &buf2);
    for (int y = 0; y < h2; ++y) {
        int yp = y * isz;
        int ym = MIN(yp+isz, h);
        for (int x = 0; x< w2; ++x) {
            int xp = x * isz;
            int xm = MIN(xp+isz, w);
            int cnt = 0;
            int r  = 0,g = 0, b = 0;
            for (int y1 = yp; y1 < ym; ++y1)  {
                for (int x1 = xp; x1 < xm; ++x1) {
                    short px = buf[y1 * w + x1];
                    r += (px >> 10) & 0x1F;
                    g += (px >> 5) & 0x1F;
                    b += px & 0x1F;
                    ++cnt;
                }
            }
            buf2[y * w2 + x] = RGB555(r/cnt, g/cnt, b/cnt);
        }
    }
    free(p1);
    return p2;
}

static T_PARAGRAPH *find_paragraph(int num)
  {
  int *pp;
  int pocet,i;
  T_PARAGRAPH *z;

  num+=local_pgf;
  pp=(int *)ablock_copy(H_DIALOGY_DAT);
  pocet=*pp;pp+=2;
  z=(T_PARAGRAPH *)pp;
  for(i=0;i<pocet;i++,z++) {
      if (z->num==(unsigned)num) return z;
  }
  return NULL;
  }

static int find_pgnum(char *pc)
  {
  T_PARAGRAPH *z;
  int *pp;
  int lastnum=-1;
  int pocet;
  int pcc,i;

  pp=(int *)ablock_copy(H_DIALOGY_DAT);
  pocet=*pp;pp+=2;
  pcc=pc-(char *)pp-8-sizeof(T_PARAGRAPH)*pocet;
  z=(T_PARAGRAPH *)pp;
  for(i=0;i<pocet;i++,z++) if (z->position>pcc) break;else lastnum=z->num;
  return lastnum-local_pgf;
  }



static void dlg_error(const char *pattern, ...);

static void goto_paragraph(int prgf)
  {

  T_PARAGRAPH *z;

  ach_event_dialog_paragraph(prgf+local_pgf);

  do
     {
     z=find_paragraph(prgf);
     if (z == NULL) {
        dlg_error("Can't find paragraph %d", prgf);
        return;
     }
     if (trace_dialogs) wzprintf("Dialog goto_paragraph %d (visited=%d)\n",prgf+local_pgf, z->visited);
     if (z->visited) z->first=1;
     if (z->alt==z->num || !z->visited)
        {
        void *dlg = ablock_copy(H_DIALOGY_DAT);
        pc=(char *)dlg+*((const int32_t *)dlg)*sizeof(T_PARAGRAPH)+8+z->position;
        last_pgf=prgf;
        z->visited=1;
        return;
        }
     prgf=z->alt-local_pgf;
     do_events();
     }
  while (1);
  }

static THUMAN *getSafeSpeaker(int idx) {
    static THUMAN err;
    if (idx < SAVE_SPKRS && speakers[idx]) {
        return speakers[idx];
    } else {
        err.sektor = 0;
        strcpy(err.jmeno,"?Error?");
        return &err;
    }
}

static char *transfer_text(const char *source,char *target)
  {
  const char *orgn=source;
  char *ot=target;
  int num;
  while (*source)
     {
     if (*source=='%')
        {
        source++;
        switch(*source)
           {
           case '[':*target++='[';break;
           case ']':*target++=']';break;
           case 'a':*target++='\'';break;
           case 'p':
           case '%':*target++='%';break;
           case 'n': strcpy(target, getSafeSpeaker(0)->jmeno);
                     target += strlen(target);
                     break;
           default: num=0;while (isdigit(*source)) num=10*num+*source++-'0';
                    if (*source=='l') {
                       if (num < SAVE_SPKRS) {
                          speakers[0] = speakers[num];
                       }
                    }
                    break;
           }
           source++;
        }
     else if (*source=='[')
        {
        source++;
        num=getSafeSpeaker(0)->female;
        while(num>0)
           {
           source=strchr(source,',');
           num--;
           if (source==NULL)
              {
              dlg_error("Invalid gender number (%d) or invalid count of variants (%s): %s",num, orgn);
              strcpy(target, "");
              return target;
              }
           source++;
           }
        while (*source!=',' && *source!=']' && *source!=0) *target++=*source++;
        if (*source) {
           char *x =strchr(source,']');
           if (x==NULL) x = strchr(source,0);
           else x++;
           source = x;
           }
        }
    
     else *target++=*source++;
     }
  *target=0;
  if (code_page==2)
     prekodovat(ot);
  return target;
  }

static char *conv_text(const char *source)
  {
  if (string_buffer==NULL) string_buffer=getmem(STR_BUFF_SIZ);
  return transfer_text(source,string_buffer);
  }

static char zjisti_typ()
  {
  return *pc;
  }

static char *Get_string()
  {
  char *c,i;
  if (*pc==P_STRING)
     {
     pc++;
     const char *txt = pc;
     c=conv_text(txt);
     do
        {
        pc+=strlen(pc)+1;
        if ((i=zjisti_typ())==P_STRING)
           {
            const char *txt = pc;
           pc++;
           c=transfer_text(txt,c);
           }
        }
     while(i==P_STRING);
     return string_buffer;
     }
  short idx = 0;
  short typ = zjisti_typ();
  if (typ == P_SHORT||typ == P_VAR) {
     pc++;
     idx = (uint8_t)pc[0] + 256*pc[1];
     pc+=2;
     if (typ == P_VAR) {
        if (idx < 0 || idx >= MAX_VARIABLES) {
          idx = -1;
        } else {
          idx = variables[idx];
        }
     }
     if (idx < 0 || str_count(texty)< idx || texty[idx] == NULL) {
          strcpy(string_buffer, "<NULL>");
          return string_buffer;
     }
     return conv_text(texty[idx]);
  }
  dlg_error("Expected string (%d), found (%d)", P_STRING, zjisti_typ());
  return "";
  }

static short Get_short()
  {
  short p;
  if (*pc==P_SHORT)
     {
     pc++;
     p = (uint8_t)pc[0] + 256*pc[1];
     pc+=2;
     return p;
     }
  if (*pc==P_VAR)
     {
     pc++;
     p = (uint8_t)pc[0] + 256*pc[1];
     pc+=2;
     return variables[p];
     }
  if (*pc == P_POP) {
    return stk_pop();
  }
  dlg_error("Expected numeric argument, found type: %d", *pc);
  return 0;
  }

static void show_desc()
  {
  char *c=descript;
  int y;

  showed=0;
  if (c==NULL) return;
  y=dlg_layout.txt_desc_y;
  char *end = descript+descript_len;
  set_font(dlg_layout.desc_font,DESC_COLOR1);
  while (c < end)
     {
     position(dlg_layout.txt_desc_x,y);
     outtext(c);y+=text_height(*c?c:" ");
     c=strchr(c,0)+1;
     }
  }

static void echo(const char *c);
static void draw_all();

static void set_desc(const char *c)
  {
  int xs,ys;
  if (descript!=NULL) free(descript);
  descript_len = strlen(c);
  descript=(char *)getmem(descript_len+2);
  set_font(dlg_layout.text_font,DESC_COLOR1);
  zalamovani(c,descript,dlg_layout.txt_desc_width,&xs,&ys);
  descript[descript_len+1] =0;
  }

static void add_desc(const char *c)
  {
    set_desc(c);
    echo(c);
    echo("");
    last_his_line = his_line = vector_size(&dlg_text);
  }

struct SlowDrawState {
    int next;
    int f;
    int pos;
    const char *text;
    
};


static void unwire_slow_desc();
static void unwire_slow_desc_force();
static void slow_desc_draw(EVENT_MSG *msg, void **user_data) {
        switch (msg->msg) {
            case E_INIT: {
                const char *text = va_arg(msg->data, const char *);
                char *textcpy;
                struct SlowDrawState *st = (struct SlowDrawState *)make_string_array(sizeof(struct SlowDrawState), &text,1,&textcpy);
                st->text = textcpy;
                st->next = get_timer_value();
                st->pos = 0;
                st->f = 1;
                *user_data = st;                            
            }break;
            case E_TIMER: {
                struct SlowDrawState *st = (struct SlowDrawState *)*user_data;
                int t = get_timer_value();
                if (t < st->next) return;
                int len = st->pos++;                
                int maxlen = strlen(st->text);
                if (len > maxlen) {
                    unwire_slow_desc();
                } else if (len > 0) {
                    set_desc(st->text);
                    char c = descript[len-1];
                    char cc = len > 1?descript[len-2]:' ';
                    if (c == '"') c = cc;
                    if (descript[len] == '"') c = ' ';
                    if (c == '.' || c == '?' || c == '!' ) {
                        st->next += 20;  
                    } else {
                        st->next += 2;
                    }
                    descript[len] = 0;
                    descript_len = len;
                    draw_all();
                    if (st->f) {
                        showview(0,0,0,0);
                        st->f = 0;
                    } else {
                        showview(dlg_layout.txt_desc_x, dlg_layout.txt_desc_y, dlg_layout.txt_desc_width, 360-dlg_layout.txt_desc_y);
                    }
                }
            }

        }
}

static void slow_desc_draw_interrupt(EVENT_MSG *msg) {
    if (msg->msg == E_KEYBOARD) unwire_slow_desc();
    else if (msg->msg == E_MOUSE) {
        const MS_EVENT *ev = va_arg(msg->data, const MS_EVENT *);
        if (ev->event_type & MS_EVENT_MOUSE_LPRESS) unwire_slow_desc();
    }
}

static void unwire_slow_desc() {
    send_message(E_DONE,E_TIMER, slow_desc_draw);
    send_message(E_DONE,E_KEYBOARD,slow_desc_draw_interrupt);
    send_message(E_DONE,E_MOUSE,slow_desc_draw_interrupt);
    exit_wait = 1;
}
static void unwire_slow_desc_force() {
    unwire_slow_desc();
    static char exit_code[] = {P_SHORT, 255, 0};
    pc = exit_code;
}

static void add_desc_slow(const char *c) {
    unwire_proc();
    unwire_proc = unwire_slow_desc_force;
    send_message(E_ADD,E_TIMER, slow_desc_draw, c);
    send_message(E_ADD,E_KEYBOARD,slow_desc_draw_interrupt);
    send_message(E_ADD,E_MOUSE,slow_desc_draw_interrupt);
    escape();
    add_desc(c);
    wire_proc();
}


static void unwire_delay_proc();
static void dlg_delay_proc(EVENT_MSG *msg, void **userptr) {
    if (msg->msg == E_INIT) {
        unsigned int till = va_arg(msg->data, unsigned int);
        uint32_t *end = New(uint32_t);
        *end = get_timer_value() + till;
        *userptr = end;
    } else if (msg->msg == E_TIMER) {
        uint32_t cur = get_timer_value();
        uint32_t *end = (uint32_t *)(*userptr);
        if (cur >= *end)  {
            unwire_delay_proc();
        }
    }
}
static void dlg_delay_interrupt(EVENT_MSG *msg) {
    if (msg->msg == E_KEYBOARD) unwire_delay_proc();
    else if (msg->msg == E_MOUSE) {
        const MS_EVENT *ev = va_arg(msg->data, const MS_EVENT *);
        if (ev->event_type & MS_EVENT_MOUSE_LPRESS) unwire_delay_proc();
    }
}

static void unwire_delay_proc() {
    send_message(E_DONE,E_TIMER, dlg_delay_proc);
    send_message(E_DONE,E_KEYBOARD,dlg_delay_interrupt);
    send_message(E_DONE,E_MOUSE,dlg_delay_interrupt);
    exit_wait = 1;
}


static void dlg_delay(unsigned int delay) {
    unwire_proc();
    unwire_proc = unwire_delay_proc;
    send_message(E_ADD,E_TIMER, dlg_delay_proc, delay);
    send_message(E_ADD,E_KEYBOARD,dlg_delay_interrupt);
    send_message(E_ADD,E_MOUSE,dlg_delay_interrupt);
    escape();
    wire_proc();

}

static void show_emote(char *c)
  {
  int xs,ys;
  char *a;

  a=alloca(strlen(c)+2);
  set_font(dlg_layout.text_font,RGB555(31,31,31));
  zalamovani(c,a,TEXT_XS,&xs,&ys);
  while (*a)
     {
        TDLG_TEXT_LINE dlt ={0};
        strcopy_n(dlt.line, a, sizeof(dlt.line));
        dlt.type = lt_emote;
        dlt.height = TEXT_STEP;
        a=strchr(a,0)+1;
        end_text_line = vector_size(&dlg_text);
        vector_push_back(&dlg_text, &dlt);
     }
  }


static void echo(const char *c)
  {
  int xs,ys;
  char *a;

  a=alloca(strlen(c)+2);
  set_font(dlg_layout.text_font,RGB555(0,30,0));
  zalamovani(c,a,TEXT_XS,&xs,&ys);
  char *end = a + strlen(c)+1;
  while (a < end)
     {
        TDLG_TEXT_LINE dlt = {0};
        strcopy_n(dlt.line, a, sizeof(dlt.line));
        dlt.type = lt_echo;
        dlt.height = TEXT_STEP;
        a=strchr(a,0)+1;
        end_text_line = vector_size(&dlg_text);
        vector_push_back(&dlg_text, &dlt);
     }
  }

#define TEXT_UNSELECT dlg_layout.choice_color
#define TEXT_SELECT dlg_layout.sel_choice_color

static void redraw_text()
  {
  int y=TEXT_Y;
  int ys=TEXT_YS;
  int ls_cn,i;


  ls_cn=vector_size(&dlg_text);
  if (ls_cn<=his_line) return;

  for (i=his_line;i<ls_cn;i++)
    {
        const TDLG_TEXT_LINE *ln = (const TDLG_TEXT_LINE *)vector_get(&dlg_text, i);
        int x = TEXT_X + ln->xofs;
        if (ln->face) {
            put_picture(TEXT_X, y, ln->face);
        }
        switch (ln->type) {
            default:
            case lt_echo:set_font(dlg_layout.text_font,dlg_layout.text_color);;break;
            case lt_emote:set_font(dlg_layout.text_font,TEXT_UNSELECT);break;
            case lt_choice:
                if (ln->id == vyb_volba) set_font(dlg_layout.text_font,TEXT_SELECT);
                else set_font(dlg_layout.text_font,TEXT_UNSELECT);
                break;        
        }
        
        position(x,y);outtext(ln->line);
        y+=ln->height;
        ys-=ln->height;
        if (ys<TEXT_STEP) break;
    }
  }

static int get_last_his_line()
  {
  

  return vector_size(&dlg_text);
  }

static void draw_all()
  {
  const void *c;
  if (back_pic_enable) c=back_pic;else c=ablock(H_DIALOG_PIC);
  other_draw();
  if (c!=NULL && dlg_layout.draw_order == DRAW_ORDER_PIC_FIRST) put_picture(PIC_X,PIC_Y,c);
  show_dialog_picture();
  if (c!=NULL && dlg_layout.draw_order == DRAW_ORDER_UI_FIRST) put_picture(PIC_X,PIC_Y,c);
  show_desc();
  redraw_text();
  }

static void lecho(char *c)
  {
  write_story_text(c);
  }

static void save_name(int pos)
  {
    if (pos < SAVE_SPKRS) {
      speakers[pos] = speakers[0];
    }
  }

static void load_name(int pos)
  {
    if (pos < SAVE_SPKRS) {
      speakers[0] = speakers[pos];
    }
  }


static void select_speaker(int vls,int omz, int slot) {
      if (slot >= SAVE_SPKRS) return;
      int stats[POCET_POSTAV] = {0};
      for (int i = 0; i < SAVE_SPKRS; ++i) {
          if (i != slot) {
              const THUMAN *spk = speakers[i];
              if (spk != NULL && spk->lives && spk->used) {
                int id = spk - postavy;
                if (id >=0 && id < POCET_POSTAV) {
                    ++stats[id];
                }
              }
          }
      }
      int m = SAVE_SPKRS;
      for (int i = 0; i < POCET_POSTAV; ++i) {
        if (postavy[i].used && postavy[i].lives) m = MIN(stats[i], m);
      }
      THUMAN *candidates[POCET_POSTAV];
      int ccount = 0;
      for (int i = 0; i < POCET_POSTAV; ++i) {
          if (stats[i] == m && postavy[i].used && postavy[i].lives) candidates[ccount++] = &postavy[i];
      }
      if (ccount == 0) {
          speakers[slot] = NULL;
          return;
      }
      THUMAN *selected[POCET_POSTAV];
      int scount = 0;
      for (int i = 0; i < ccount; ++i) {
          if (candidates[i]->vlastnosti[vls] >= omz) {
              selected[scount++] = candidates[i];
          }
      }
      if (scount == 0) {
          m = 0;
          for (int i = 0; i < ccount; ++i) {
             m = MAX(candidates[i]->vlastnosti[vls], m);
          }
          for (int i = 0; i < ccount; ++i) {
              if (candidates[i]->vlastnosti[vls] == m) {
                  selected[scount++] = candidates[i];
              }
          }
      }

      int roll = rnd(scount);
      speakers[slot] = selected[roll];
  }

static void select_speaker_by_face(int face, int slot)
{
  int i;
  if (slot >= SAVE_SPKRS) return;
  for (i=0;i<POCET_POSTAV;i++) {
      if (postavy[i].used && postavy[i].xicht==face)  {
          speakers[slot] = &postavy[i];
          iff=1;
          return;
      }
  }
  iff=0;
  speakers[slot] = NULL;
}

static void select_speaker_by_slot(int char_slot, int speaker_slot)
{
  if (speaker_slot >= SAVE_SPKRS) return;
  if (char_slot >= POCET_POSTAV) return;
  if (postavy[char_slot].used) {
      speakers[speaker_slot] = &postavy[char_slot];
  } else {
      speakers[speaker_slot] = NULL;
  }
}


static char visited(int prgf)
  {
  T_PARAGRAPH *z;

  z=find_paragraph(prgf);
  if (z == NULL) return 0;
  return z->visited;
  }

static void set_nvisited(int prgf)
  {
  T_PARAGRAPH *z;

  z=find_paragraph(prgf);
  if (z == NULL) return;
  z->visited=0;
  z->first=0;
  }


static void q_flag(int flag)
  {
  if (flag < 16) iff=_monster_flag_map[flag>>3] & (1<<(flag & 0x7));
  else iff=_flag_map[flag>>3] & (1<<(flag & 0x7));
  }

static void set_flag(int flag)
  {
  if (flag < 16) _monster_flag_map[flag>>3]|=(1<<(flag & 0x7));
  else _flag_map[flag>>3]|=(1<<(flag & 0x7));
  }

static void reset_flag(int flag)
  {
  if (flag < 16) _monster_flag_map[flag>>3]&=~(1<<(flag & 0x7));
  else _flag_map[flag>>3]&=~(1<<(flag & 0x7));
  }

static void q_fact(int flag)
  {
  iff=_flag_map[flag>>3] & (1<<(flag & 0x7));
  }

static void set_fact(int flag)
  {
  _flag_map[flag>>3]|=(1<<(flag & 0x7));
  }

static void reset_fact(int flag)
  {
  _flag_map[flag>>3]&=~(1<<(flag & 0x7));
  }


void change_flag(int flag,char mode)
  {
  if (mode==0) reset_flag(flag);
  else if (mode==1) set_flag(flag);
  else _flag_map[flag>>3]^=(1<<(flag & 0x7));
  }

char test_flag(int flag)
  {
  return (_flag_map[flag>>3] & (1<<(flag & 0x7)))!=0;
  }

static void first_visited(int prgf)
  {
  T_PARAGRAPH *z;

  z=find_paragraph(prgf);
  if (z == NULL) return;
  iff=!z->first;
  }



void do_dialog();
static void remove_all_cases();

static void dialog_cont()
     {
     save_jump=vol_n[(uint8_t)vyb_volba];
     remove_all_cases();
     echo(" ");
     wire_proc = old_wire_proc;
     his_line=get_last_his_line();
     if (halt_flag) goto_paragraph(save_jump);
     schovej_mysku();
     do_dialog();
     }


static void key_check(EVENT_MSG *msg,void **unused)
  {
//  char d;

  unused;
  if (msg->msg==E_KEYBOARD)
     {
     int c = va_arg(msg->data, int) >> 8;
     char redraw = 0;
     switch (c) {
      case 1:konec(0,0,0,0,0);break;
      case 17:
      case 'H':if (vyb_volba==0) his_line-=(his_line>0);
               else vyb_volba--;
               redraw = 1;
               break;
      case 31:
      case 'P':if (his_line<last_his_line) his_line++;
               else if (vyb_volba+1<pocet_voleb) vyb_volba++;
               redraw = 1;
               break;
      case 28:
      case 57:dialog_cont();break;
      case 61:clk_saveload(0, 0, 0, 0, 0);break;
      case 59:game_setup(0,0,0,0,0);break;
     }
     if (redraw) {
      schovej_mysku();
      draw_all();
      ukaz_mysku();
      showview(TEXT_X,TEXT_Y,TEXT_XS,TEXT_YS);
     }
/*

     d=c>>8;
     if (c & 0xFF)
        {
        switch(d)
           {
            case 17:
           case 'H':if (vyb_volba==0) his_line-=(his_line>0);else vyb_volba--;break;
           case 31:
           case 'P':if (his_line<last_his_line) his_line++;else if (vyb_volba+1<pocet_voleb) vyb_volba++;break;
           }
        }
     else if (c==13||c==32)
        {
        dialog_cont();
        msg->msg=-1;
        }
     else if (c==27) {
      dlg_konec(0,0,0,0,0);

     }*/
     }
  }

void wire_dialog();
void wire_dialog_drw(void)
  {
  schovej_mysku();
  wire_dialog();
  draw_all();
  ukaz_mysku();
  effect_show();
  }
void unwire_dialog(void)
  {
  send_message(E_DONE,E_KEYBOARD,key_check);
  disable_click_map();

  }

void wire_dialog()
  {
  send_message(E_ADD,E_KEYBOARD,key_check);
  change_click_map(clk_dialog,CLK_DIALOG);
  unwire_proc=unwire_dialog;
  showview(0,0,0,0);
  last_his_line=his_line;
  }


short *q_item_one(int i,int itnum)
     {
     int j;
     THUMAN *p=&postavy[i];
     for(j=0;j<p->inv_size;j++)
        if (p->inv[j]==itnum) return &p->inv[j];
     for(j=0;j<HUMAN_PLACES;j++)
        if (p->wearing[j]==itnum) return &p->wearing[j];
     for(j=0;j<HUMAN_RINGS;j++)
        if (p->prsteny[j]==itnum) return &p->wearing[j];
     return NULL;
     }

short *q_item(int itnum,int sector)
  {
  int i;
  short *p;

  itnum++;
  for(i=0;i<POCET_POSTAV;i++)
     if (postavy[i].sektor==sector)
       if ((p=q_item_one(i,itnum))!=NULL)return p;
  return NULL;
  }

void destroy_item(int itnum)
  {
  short *q=q_item(itnum,viewsector);
  if (q!=NULL) *q=0;
  }

void create_item(int itnum)
  {
  //short *p;

  poloz_vsechny_predmety();
  picked_item=(short *)getmem(4);
  picked_item[0]=itnum+1;
  picked_item[1]=0;
  pick_set_cursor();
  }

static void add_case(int num,char *text)
  {
  char *a;
  int xs,ys;
  if (pocet_voleb>=MAX_VOLEB) {
      dlg_error("Too many choices. Limit is %d", MAX_VOLEB);
      return;
  }
  vol_n[(uint8_t)pocet_voleb]=num;
  a=alloca(strlen(text)+2);
  set_font(dlg_layout.text_font,RGB555(0,30,0));
  zalamovani(text,a,TEXT_XS,&xs,&ys);
  while (*a)
     {     
     TDLG_TEXT_LINE ln ={};
     ln.type = lt_choice;
     ln.id = pocet_voleb;
     ln.height = TEXT_STEP;
     strcopy_n(ln.line,a,sizeof(ln.line));
     vector_push_back(&dlg_text, &ln);
     a=strchr(a,0)+1;
     }
  pocet_voleb++;
  }

static void add_case_speaker(int num,int speaker, char *text)
  {
  char *a;
  int xs,ys;
  if (pocet_voleb>=MAX_VOLEB) {
      dlg_error("Too many choices. Limit is %d", MAX_VOLEB);
      return;
  }
  vol_n[(uint8_t)pocet_voleb]=num;
  a=alloca(strlen(text)+2);
  set_font(dlg_layout.text_font,RGB555(0,30,0));
  THUMAN *h = speaker?speakers[speaker]:NULL;;
  int xxs = TEXT_XS;
  int xofs = 0;
  void *xcht = NULL;;
  xofs += dlg_layout.icon_padding;
  xxs -= xofs;  
  if (h) {
    unsigned long idx = h - postavy;
    if (idx < POCET_POSTAV && h->used) {
        xcht = small_xicht(H_XICHTY+idx);
    }
  }
  zalamovani(text,a,xxs,&xs,&ys);
  TDLG_TEXT_LINE lines[10] = {0};
  int lnidx = 0;
  while (*a && lnidx < 10)
     {
        TDLG_TEXT_LINE *ln = &lines[lnidx];
        lnidx++;
        ln->type = lt_choice;
        ln->id = pocet_voleb;
        ln->xofs = xofs;
        ln->height = TEXT_STEP;
        strcopy_n(ln->line,a, sizeof(ln->line));
        a=strchr(a,0)+1;        
     }
  TDLG_TEXT_LINE *tmp = NULL;
  {
    int xh = dlg_layout.icon_height+1;
    if (lnidx * TEXT_STEP >= xh) {
        lines[0].face = xcht;
    } else {
        tmp = &lines[lnidx];
        tmp->type = lt_choice;
        tmp->id = pocet_voleb;
        tmp->height = (xh - lnidx * TEXT_STEP )/2;     
        tmp->face = xcht;
        tmp[1].height = xh - lnidx * TEXT_STEP - tmp->height;
        tmp[1].id = pocet_voleb;
        tmp[1].type = lt_choice;
        vector_push_back(&dlg_text, tmp);
    }
 }
 for (int i = 0; i < lnidx; ++i) {
    
        vector_push_back(&dlg_text, &lines[i]);
 }
 if (tmp) {
     vector_push_back(&dlg_text, &tmp[1]);
 }
    

  pocet_voleb++;
  }

static int remove_choice_callback(const void *item, void *context) {
    (void)context;
    TDLG_TEXT_LINE *x = (TDLG_TEXT_LINE *)item;
    return x->type == lt_choice && x->id != vyb_volba;
}

static void remove_all_cases()
  {
    size_t s = linear_remove_if((TDLG_TEXT_LINE *)vector_data(&dlg_text)+end_text_line,
                    vector_size(&dlg_text) - end_text_line, dlg_text.element_size,remove_choice_callback,NULL)
                    + end_text_line;
    vector_resize(&dlg_text, s,NULL);   
    for (size_t i = 0; i < s; ++i)     {
        TDLG_TEXT_LINE *l = (TDLG_TEXT_LINE *)vector_get(&dlg_text, i);
        l->id = -1;
    }
    vyb_volba=0;
    pocet_voleb = 0;
  }

  static char case_click(int id, int xa, int ya, int xr, int yr) {
    

    xa, ya, xr, yr;

    if (pocet_voleb > 1) {      
      const TDLG_TEXT_LINE *list = (const TDLG_TEXT_LINE *)vector_data(&dlg_text);
      size_t count = vector_size(&dlg_text);
      int yp = 0;
      size_t f = count;
      for (size_t i = his_line; i< count; ++i) {
        int yn = yp + list[i].height;        
        if (yr >= yp && yr < yn) {
            f = i;
        }
        yp = yn;
      }
      if (f >= count) return 0;
      if (list[f].type != lt_choice) return 0;
      id = list[f].id;
      if (id != vyb_volba) {
        vyb_volba = id;
        schovej_mysku();
        redraw_text();
        ukaz_mysku();
        showview(TEXT_X, TEXT_Y, TEXT_XS, TEXT_YS);
      }
      if (ms_last_event.event_type & 0x2) {
        dialog_cont();
      }
    } else if (ms_last_event.event_type & 0x2) {
      dialog_cont();
    }
    return 1;
  }

void dialog_select(char halt)
  {
  showed=0;
  unwire_proc();
  draw_all();
  ukaz_mysku();
  old_wire_proc = wire_proc;
  wire_dialog();
  wire_proc = wire_dialog_drw;
  halt_flag=halt && (pocet_voleb>0);
  }

void dialog_select_jump()
  {
  goto_paragraph(save_jump);
  }

static void exit_dialog()
  {
  //TODO why? _flag_map[0]|=0x1;
  stop_anim();
  if (dialog_mob>-1)
     {
     mobs[dialog_mob].dialog_flags=_monster_flag_map[0];
     mobs[dialog_mob].stay_strategy=_monster_flag_map[1];
     }
  unwire_proc();
  aunlock(H_DIALOGY_DAT);
  ukaz_mysku();
  free(descript);descript=NULL;
  free(string_buffer);string_buffer=NULL;
  remove_all_cases();
  vector_destroy(&dlg_text);
  free(back_pic); back_pic = NULL;
  undef_handle(H_DIALOG_PIC);
  if (starting_shop!=-1 && !battle)
     {
     enter_shop(starting_shop);
     ukaz_mysku();
     update_mysky();
     cancel_pass=0;
     }
  else
     {
     wire_proc();
     norefresh=0;
     }
  starting_shop=-1;
  SEND_LOG("(DIALOGS) Exiting dialog...");
  }


static void picture(char *c)
  {
  undef_handle(H_DIALOG_PIC);
  if (strcmp(c,"SCREEN")) def_handle(H_DIALOG_PIC,c,0,SR_DIALOGS),back_pic_enable=0;
  else back_pic_enable=1;
  }



static void dlg_start_battle()
  {
  if (dialog_mob!=-1)
     {
     mobs[dialog_mob].vlajky|=MOB_IN_BATTLE;
     }
  battle=1;
  }

static void teleport_group(short sector,short dir)
  {
  int i;
  THUMAN *h=postavy;

  destroy_player_map();
  for(h=postavy,i=0;i<POCET_POSTAV;i++,h++)
     if (h->used && h->groupnum==cur_group)
        {
        recheck_button(h->sektor,1);
        recheck_button(sector,1);
        h->sektor=sector;
        h->direction=dir;
        }
  viewsector=sector;
  viewdir=dir;
  build_player_map();
  }

extern THUMAN postavy_2[];

char join_character(int i)
  {
  THUMAN *h;
  THUMAN *s=postavy_2+i;
  int j;

  SEND_LOG("(DIALOGS) Joining character '%s'",s->jmeno);
  for(j=0,h=postavy;j<POCET_POSTAV;j++,h++) if (!h->used)
     {
     memcpy(h,s,sizeof(THUMAN));
     h->sektor=viewsector;
     h->direction=viewdir;
     h->groupnum=cur_group;
     h->inmaphash=current_map_hash;
     if (j == POCET_POSTAV-1) ach_event_full_party();
     reg_grafiku_postav();
     bott_draw(1);
     return 0;
     }
  SEND_LOG("(DIALOGS) Join failed - no room for new character");
  return 1;
  }


static int selected_player;

char drop_character()
{
  THUMAN *h = speakers[0];
  if (h == NULL) return 1;
  int selected_player = h - postavy;
  if (selected_player<0 || selected_player>=POCET_POSTAV) return 1;
  memcpy(postavy+selected_player,postavy+selected_player+1,sizeof(*postavy)*(POCET_POSTAV-selected_player));
  for (int i = 0; i < SAVE_SPKRS; ++i) {
      THUMAN *h = speakers[i];
      if (h) {
          int idx = h - postavy;
          if (idx >selected_player) {
            speakers[i] = &postavy[idx-1];
          }
      }
  }
  postavy[POCET_POSTAV-1].used=0;
  reg_grafiku_postav();
  bott_draw(1);
  return 0;
}


static char dead_players=0;

 char is_player_near(int sector, THUMAN *p) {
   if (mob_map[sector]) return 0;
    if (sector == p->sektor) return 1;
    for (int i = 0; i  < 4; ++i) {
        int s = map_sectors[sector].step_next[i];
        if (mob_map[s]) continue;
        if (s == p->sektor && (map_sides[sector * 4 + i].flags & SD_PLAY_IMPS) == 0) return 1;
    }
    return 0;
}

char can_select_player(THUMAN *p, char select_dead, char select_far) {
    if (select_dead) {
        if (p->used && p->lives == 0 && (
                p->sektor == 0 || p->sektor == viewsector)) return 1;
    } else {
        int side = (viewsector << 2) + viewdir;
    if (p->used && !(p->lives == 0 && p->kondice == 0) && (select_far ||
            (p->sektor == viewsector
                    || ((map_sides[side].flags & SD_PLAY_IMPS) == 0
                            && is_player_near(map_sectors[viewsector].step_next[viewdir], p)

                    ))))
            return 1;
    }
    return 0;
}

static char ask_who_proc(int id,int xa,int ya,int xr,int yr)
  {
  {
  THUMAN *p;
  int i;
  const word *xs;

  if (id==2)
     {
     selected_player=-1;
     exit_wait=1;
     return 1;
     }
  xs=ablock(H_OKNO);
  i=xr/xs[0];yr;xa;ya;id;
  if (i<POCET_POSTAV)
     {
     i=group_sort[i];
     p=&postavy[i];
     if (can_select_player(p,dead_players,0))
           {
           selected_player=i;
           exit_wait=1;
           }
     }
  return 1;
  }

  }


static int dlg_ask_who()
  {
  draw_all();
  mouse_set_default(H_MS_WHO);
  ukaz_mysku();
  showview(0,0,0,0);
  *otevri_zavoru=1;
  change_click_map(clk_dlg_who,CLK_DLG_WHO);
  escape();
  his_line=get_last_his_line();
  dead_players=0;
  schovej_mysku();
  mouse_set_default(H_MS_DEFAULT);
  if (selected_player==-1) return 1;
  speakers[0] = &postavy[selected_player];
  change_click_map(NULL,0);
  return 0;
  }

extern word weapon_skill[];

static void pract(THUMAN *h,int vls,int how,int max)
  {
   iff=0;
   if (!h) return;
   if (vls>=100)
     {
     vls-=100;
     if (h->bonus_zbrani[vls]>=max) iff=1;
     else
        {
        h->bonus_zbrani[vls]+=how;
        if (h->bonus_zbrani[vls]>max) h->bonus_zbrani[vls]=max,iff=1;
        h->weapon_expy[vls]=weapon_skill[h->bonus_zbrani[vls]];
        }
     }
   else
     {
     if (h->vlastnosti[vls]>=max) iff=1;
     else
        {
        h->stare_vls[vls]+=how;
        prepocitat_postavu(h);
        if (h->vlastnosti[vls]>max)
           {
           h->stare_vls[vls]-=h->vlastnosti[vls]-max;
           h->vlastnosti[vls]=max;
           iff=1;
           }
        }
     }
  }

static void pract_to(THUMAN *h, int vls,int how)
  {
   iff=0;
   if (!h) return;
   if (vls>=100)
     {
     vls-=100;
     if (h->bonus_zbrani[vls]<how)
        {
        h->bonus_zbrani[vls]=how;
        h->weapon_expy[vls]=weapon_skill[how];
        }
     else iff=1;
     }
   else
     {
     if (h->vlastnosti[vls]<how)
        h->stare_vls[vls]+=how-h->vlastnosti[vls];else iff=1;
     prepocitat_postavu(h);
     }
  }


static char oper_balance(int val1,int val2,int oper)
  {
  switch (oper)
     {
     case OPER_EQ:return val1==val2;
     case OPER_BIG:return val1>val2;
     case OPER_LOW:return val1<val2;
     case OPER_BIGEQ:return val1>=val2;
     case OPER_LOWEQ:return val1<=val2;
     case OPER_NOEQ:return val1!=val2;
     default:dlg_error("Invalid relation argument %d", oper);
     }
  return 0;
  }

static char test_vls(THUMAN *h,int vls,int oper,int num)
  {
  int val;
  if (h == NULL) return 0;
   if (vls>=100)
     {
     vls-=100;
     val=h->bonus_zbrani[vls];
     }
   else
     val=h->stare_vls[vls];
  return oper_balance(val,num,oper);
  }

static char atsector(int oper,int sector)
  {
  return oper_balance(viewsector,sector,oper);
  }


static void dark_screen(int time,int gtime)
  {
  int z,i;
  THUMAN *h;
  i=get_timer_value()+time*50;
  curcolor=0;
  bar32(0,17,639,377);
  showview(0,0,0,0);
  while (get_timer_value()<i) do_events();
  game_time+=gtime*HODINA;
  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && h->lives && h->inmaphash == current_map_hash)
     {
     z=h->vlastnosti[VLS_HPREG]*gtime;z+=h->lives;
     if (z>h->vlastnosti[VLS_MAXHIT]) {z=h->vlastnosti[VLS_MAXHIT];h->lives=z;}
     z=h->vlastnosti[VLS_MPREG]*gtime;z+=h->mana;
     if (z>h->vlastnosti[VLS_MAXMANA]) {z=h->vlastnosti[VLS_MAXMANA];h->mana=z;}
     z=h->vlastnosti[VLS_VPREG]*gtime;z+=h->kondice;
     if (z>h->vlastnosti[VLS_KONDIC]) {z=h->vlastnosti[VLS_KONDIC];h->kondice=z;}
     }
  bott_draw(0);
  }

static char najist_postavy(int cena)
  {
  int i,s=0;
  THUMAN *h=postavy;

  for(i=0;i<POCET_POSTAV;i++,h++) if (h->used && h->sektor==viewsector && h->lives) s=s+cena;
  if (s>money) return 1;
  money-=s;
  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->used && h->sektor==viewsector && h->lives)
     {
     h->jidlo=MAX_HLAD(h);
     h->voda=MAX_ZIZEN(h);
     }
  return 0;
  }

static char isall()
  {
  THUMAN *h=postavy;
  int i;

  for(i=0,h=postavy;i<POCET_POSTAV;i++,h++) if (h->sektor!=viewsector && h->used && h->lives && h->inmaphash == current_map_hash) return 0;
  return 1;
  }

static void spat(int hodin)
  {
  sleep_ticks=hodin*HODINA;add_task(16384,sleep_players);
  insleep=1;
  while (insleep) do_events();
  }

static char test_volby_select(int balance,int value)
  {
  return oper_balance(pocet_voleb,value,balance);
  }

static void cast_spell_human(int spell)
  {
    if (speakers[0] == NULL) return;
    int cil = speakers[0] - postavy;
    if (cil < 0 || cil >= POCET_POSTAV) return;

    thing_cast(spell, cil, viewsector, NULL, 0);
  }

static void cast_spell_enemy(int spell)
  {
    if (dialog_mob > -1) {
      int cil=-dialog_mob-1;
      add_spell(spell,cil,cil,1);
    }
  }



static short count_slots() {
    short s = 0;
    for (int i = 0; i < POCET_POSTAV; ++i) {
        if (postavy[i].used) s++;
    }
    return s;
}

static short count_present(int sector) {
    short s = 0;
    for (int i = 0; i < POCET_POSTAV; ++i) {
        if (postavy[i].used && postavy[i].sektor == sector) s++;
    }
    return s;
}

static void teleport_char(const char *level, int sector, int dir) {
    THUMAN *p = speakers[0];
    if (!p) return;
    uint32_t h = fnv1a_hash(level);
    p->inmaphash = h;
    p->sektor = h == current_map_hash?sector:-sector;
    p->direction = dir;
    bott_draw(0);
    build_player_map();
}

static void do_replace_monster(size_t monster_index, size_t monster_id) {
  if ( monster_index >= MAX_MOBS ||  monster_id >= mob_templates_count) return;
    TMOB *m = &mobs[monster_index];
    load_enemy_to_map(monster_index, m->sector, m->dir, &mob_templates[monster_id]);
}

static void replace_monster(short n) {
    if (dialog_mob>-1) do_replace_monster(dialog_mob, n);
    refresh_mob_map();
}

static void replace_monsters(short m, short n) {
    for (size_t i = 0; i < MAX_MOBS; ++i) {
      if (mobs[i].cislo_vzoru == m && (mobs[i].vlajky & MOB_LIVE)) {
        do_replace_monster(i, n);
      }
    }
    refresh_mob_map();
}

static inline float pow2(float s) {
    return s*s;
}
static void replace_monsters_r(short m, short n, short s, short r) {
    int x = map_coord[s].x;
    int y = map_coord[s].y;
    int l = map_coord[s].layer;
    float rad = pow2(r);

    for (size_t i = 0; i < MAX_MOBS; ++i) {
      if (mobs[i].cislo_vzoru == m && (mobs[i].vlajky & MOB_LIVE)) {
        size_t sect = mobs[i].sector;
        int mx = map_coord[sect].x;
        int my = map_coord[sect].y;
        int ml = map_coord[sect].layer;
        if (ml == l) {
            float dist = sqrt(pow2(mx-x)+pow2(my-y));
            if (dist <= rad) {
              do_replace_monster(i, n);
            }
        }
        do_replace_monster(i, n);
      }
    }
}

static char get_lever(unsigned short sector, unsigned short dir) {
    if (sector >= mapsize) return 0;
    if (dir >= 4) return 0;
    const TSTENA *sect = &map_sides[sector*4+dir];
    if ((sect->flags & SD_SEC_ANIM) == 0 && (sect->flags & SD_SEC_VIS) != 0) {
        return (sect->flags & SD_SEC_FORV) != 0;
    }
    return (sect->flags & SD_PRIM_FORV) != 0;
}

static void load_level(const char *levl, unsigned short sector, unsigned short dir) {
    TMA_LOADLEV ld;
    strcopy_n(ld.name, levl, sizeof(ld.name));
    ld.start_pos = sector;
    ld.dir = dir;
    battle = 0;
    macro_load_another_map(&ld);
}

static void send_monsters(int from, int to) {
    for (int i = 0; i < MAX_MOBS; ++i) {
        if (mobs[i].sector == from) {
            send_mob_to_sector(i, to);
        }
    }
}
static void teleport_enemies(int from, int to, int dir) {
    for (int i = 0; i < MAX_MOBS; ++i) {
        if (mobs[i].sector == from) {
            mobs[i].sector = to;
            mobs[i].dir = dir;
        }
    }
    refresh_mob_map();
}
static void kill_current_enemy() {
    if (dialog_mob >= 0) {
        mobs[dialog_mob].kill_dialog = 0;
        mobs[dialog_mob].lives = 0;
        mob_check_death(dialog_mob);
    }
}


static void dlg_formated_print(const char *text, int args);

static void play_music_playlist(const char *playlist) {
    create_playlist(playlist);
    change_music(get_next_music_from_playlist());
}

void do_dialog()
  {
  int i,p1,p2,p3;
  int64_t arg1, arg2;
  char *c;

  stk_clear();


  do
     {
  i=Get_short();p3=0;
  switch(i)
     {
     case 1: stk_push(Get_short());break;
     case 2: variables[Get_short()] = stk_pop();break;
     case 3: stk_pop();break;
     case 4: arg1 = stk_pop(); stk_push(arg1); stk_push(arg1); break;
     case 5: arg1 = stk_pop(); arg2=stk_pop(); stk_push(arg2%arg1);break;
     case 6: arg1 = stk_pop(); arg2=stk_pop(); stk_push(arg2+arg1);break;
     case 7: arg1 = stk_pop(); arg2=stk_pop(); stk_push(arg2-arg1);break;
     case 8: arg1 = stk_pop(); arg2=stk_pop(); stk_push(arg2*arg1);break;
     case 9: arg1 = stk_pop(); arg2=stk_pop(); stk_push(arg2/arg1);break;
     case 10: arg1 = stk_pop(); arg2=stk_pop(); iff = (arg2 &&arg1);break;
     case 11: arg1 = stk_pop(); arg2=stk_pop(); iff = (arg2 ||arg1);break;
     case 12: arg1 = stk_pop(); arg2=stk_pop(); iff = (arg2 == arg1);break;
     case 13: arg1 = stk_pop(); arg2=stk_pop(); iff = (arg2 != arg1);break;
     case 14: arg1 = stk_pop(); arg2=stk_pop(); iff = (arg2 < arg1);break;
     case 15: arg1 = stk_pop(); arg2=stk_pop(); iff = (arg2 > arg1);break;
     case 16: arg1 = stk_pop(); arg2=stk_pop(); iff = (arg2 <= arg1);break;
     case 17: arg1 = stk_pop(); arg2=stk_pop(); iff = (arg2 >= arg1);break;
     case 18: stk_push(-stk_pop());break;
     case 19: stk_push(!stk_pop());break;
     case 20: stk_push(iff?1:0);break;
     case 21: iff = stk_pop() != 0;break;
     case 22: p1 = Get_short(); p2=Get_short(); p3=Get_short(); select_speaker(p1, p2, p3);break;
     case 23: stk_push(getSafeSpeaker(0)->vlastnosti[Get_short()]);break;
     case 24: stk_push(getSafeSpeaker(0)->wearing[Get_short()]-1);break;
     case 25: stk_push(getSafeSpeaker(0)->bonus_zbrani[Get_short()]);break;
     case 26: stk_push(getSafeSpeaker(0)->female);break;
     case 27: stk_push(count_slots());break;
     case 28: stk_push(count_present(viewsector));break;
     case 29: p1 = Get_short(); p2=Get_short(); select_speaker_by_face(p1, p2);break;
     case 30: q_fact(Get_short());break;
     case 31: set_fact(Get_short());break;
     case 32: reset_fact(Get_short());break;
     case 33: c = Get_string(); p1 = Get_short(); p2 = Get_short(); teleport_char(c, p1, p2); break;
     case 34: stk_push(getSafeSpeaker(0)->xicht);break;
     case 35: stk_push(getSafeSpeaker(0)->sektor);break;
     case 36: change_music(Get_string());break;
     case 37: replace_monster(Get_short());break;
     case 38: p1 = Get_short(); p2 = Get_short(); replace_monsters(p1,p2);break;
     case 39: p1 = Get_short(); p2 = Get_short(); p3 = Get_short(); replace_monsters_r(p1,p2,p3,Get_short());break;
     case 40: cast_spell_enemy(Get_short());break;
     case 41: iff = getSafeSpeaker(0)->sektor == viewsector;break;
     case 42: stk_push(money);break;
     case 43: iff = dialog_mob != -1;break;
     case 44: iff = battle;break;
     case 45: p1 = Get_short(); p2 = Get_short(); iff=get_lever(p1,p2);break;
     case 46: c = Get_string(); p1 = Get_short(); p2 = Get_short(); load_level(c,p1,p2);break;
     case 47: stk_push(viewsector);break;
     case 48: stk_push(viewdir);break;
     case 49: stk_push(rnd(10000));break;
     case 50: stk_push(held_item);break;
     case 51: iff = pocet_voleb == 0;break;
     case 52: kill_current_enemy();break;
     case 53: p1 = Get_short(); p2 = Get_short(); select_speaker_by_slot(p1, p2);break;
     case 54: c = Get_string();dlg_formated_print(c, Get_short()); break;
     case 55: iff = speakers[0]?has_spell_group(speakers[0] - postavy,Get_short()):0;break;
     case 56: if (speakers[0]) end_spell_group(speakers[0] - postavy, Get_short());break;
     case 57: stk_push(game_time);break;
     case 58: autosave();break;
     case 59: play_music_playlist(Get_string());break;
     case 60: dlg_delay(Get_short());break;
     case 61: iff = getSafeSpeaker(0)->lives > 0;break;
     case 128:add_desc(Get_string());break;
     case 129:show_emote(Get_string());break;
     case 130:save_name(Get_short());break;
     case 131:iff=!iff;break;
     case 132:load_name(Get_short());break;
     case 133:Get_short() /*ignore check*/ ; select_speaker(0,0,0);break;
     case 134:p1=Get_short();p2=Get_short();select_speaker(VLS_SMAGIE,p1,0);break;
     case 135:p1=Get_short();p2=Get_short();select_speaker(VLS_SILA,p1,0);break;
     case 136:p1=Get_short();p2=Get_short();select_speaker(VLS_OBRAT,p1,p2);break;
     case 138:iff=Get_short();break;
     case 139:goto_paragraph(Get_short());break;
     case 140:p1=Get_short();if (iff) goto_paragraph(p1);break;
     case 141:p1=Get_short();if (!iff) goto_paragraph(p1);break;
     case 142:p1=Get_short();add_case(p1,Get_string());break;
     case 143:p1=Get_short();p2=Get_short();c=Get_string();if (iff==p1) add_case(p2,c);break;
     case 144:dialog_select(1);return;
     case 145:iff=visited(Get_short());break;
     case 146:p1=Get_short();c=Get_string();iff=visited(p1);if (iff==0) add_case(p1,c);break;
     case 147:picture(Get_string());break;
     case 148:echo(Get_string());break;
     case 149:cur_page=count_pages();
              cur_page&=~0x1;
              cur_page++;
              add_to_book(Get_short());
              play_fx_at(FX_BOOK);
              break;
     case 150:set_nvisited(Get_short());break;
     case 151:iff=rnd(100)<=(unsigned int)Get_short();break;
     case 152:iff=q_item(Get_short(),viewsector)!=NULL;break;
     case 153:create_item(Get_short());break;
     case 154:destroy_item(Get_short());break;
     case 155:money+=Get_short();break;
     case 156:p1=Get_short();if (p1>=money) iff=1;else money-=p1;break;
     case 157:dlg_start_battle();break;
     case 158:p1=Get_short();p2=Get_short();delay_action(0,p1,p2,0,0,0);break;
     case 160:p1=Get_short();p2=Get_short();teleport_group(p1,p2);break;
     case 161:c=Get_string();p1=Get_short();p2=Get_short();run_anim(c,p1,p2);break;
     case 162:lecho(Get_string());break;
     case 163:q_flag(Get_short());break;
     case 164:dialog_select(0);return;
     case 165:dialog_select_jump();break;
     case 166:first_visited(Get_short());break;
     case 167:local_pgf=Get_short();break;
     case 168:starting_shop=Get_short();break;
     case 169:p1=Get_short();if (!iff) pc+=p1;break;
     case 170:p1=Get_short();if (iff) pc+=p1;break;
     case 171:p1=Get_short();pc+=p1;break;
     case 172:code_page=Get_short();break;
     case 173:break; //ALT_SENTENCE
     case 174:iff=join_character(Get_short());break;
     case 189:dead_players=1;break;
     case 175:echo(Get_string()); p1=Get_short();
              if (dlg_ask_who()) {
                    if (p1) goto_paragraph(p1);
                    else iff=1;
              } else iff=0;
              break;
     case 176:p1=Get_short();p2=Get_short();pract_to(speakers[0],p1,p2);break;
     case 177:p1=Get_short();p2=Get_short();p3=Get_short();iff=test_vls(speakers[0],p1,p2,p3);break;
     case 178:p1=Get_short();runes[p1/10]|=1<<(p1%10);break;
     case 179:p1=Get_short();iff=((runes[p1/10] & (1<<(p1%10)))!=0);break;
     case 180:p1=Get_short();iff=(money>=p1);break;
     case 181:p1=Get_short();p2=Get_short();p3=Get_short();pract(speakers[0],p1,p2,p3);break;
     case 182:p1=Get_short();p2=Get_short();dark_screen(p1,p2);break;
     case 183:spat(Get_short());break;
     case 184:p1=Get_short();iff=najist_postavy(p1);break;
     case 185:iff=isall();break;
	 case 186:enable_glmap=Get_short();break;
     case 187:p1=Get_short();p2=Get_short();iff=atsector(p1,p2);break;
     case 188:p1=Get_short();cast_spell_human(p1);break;
     case 190:spell_sound(Get_string());break;
     case 191:p1=Get_short();p2=Get_short();iff=test_volby_select(p1,p2);break;
     case 192:p1=Get_short();p2=Get_short();variables[p1]=p2;break;
     case 193:p1=Get_short();p2=Get_short();variables[p1]+=p2;break;
     case 194:p1=Get_short();p2=Get_short();p3=Get_short();iff=oper_balance(variables[p1],p3,p2);break;
     case 195:p2=find_pgnum(pc);p1=Get_short();variables[p1]=p2;break;
     case 196:p1=Get_short();variables[p1]=iff;break;
     case 197:p1=Get_short();add_case(variables[p1],Get_string());break;
     case 198:p1=Get_short();p2=Get_short();c=Get_string();if (iff==p1) add_case(variables[p2],c);break;
     case 199:goto_paragraph(variables[Get_short()]);break;
     case 200:iff=drop_character();break;
     case 201:select_speaker_by_face(Get_short(),0);break;
     case 202:p1=Get_short();runes[p1/10]&=~(1<<(p1%10));break;
     case 203:p1=Get_short();p2=Get_short();send_monsters(p1,p2);break;
     case 204:if (dialog_mob>=0) send_mob_to_sector(dialog_mob, Get_short());break;
     case 205:p1=Get_short();p2=Get_short();p3=Get_short();teleport_enemies(p1,p2,p3);break;
     case 206:p1=Get_short();p2=Get_short();if (dialog_mob>=0) {
            mobs[dialog_mob].sector = p1;
            mobs[dialog_mob].dir = p2;
            refresh_mob_map();
            };break;
     case 207:p1=Get_short();p2=Get_short();add_case_speaker(p1, p2, Get_string());break;
     case 208:bott_disp_text(Get_string());break;
     case 209:add_desc_slow(Get_string());break;
     case 210:autoopenaction=1;break;
     case 211:add_text_to_book_direct(Get_string());
              play_fx_at(FX_BOOK);
              break;             
     case 518:set_flag(Get_short());break;
     case 519:reset_flag(Get_short());break;
     case 255:exit_dialog();return;
     default:
          dlg_error("Unknown dialog instruction %d", i);
        break;
     }
    }
  while(1);
  }

static void create_back_pic()
  {
  int skpx=4,skpy=5,xp,yp;
  word *p,*s=GetScreenAdr()+SCREEN_OFFSET,*s2;
  int32_t scr_linelen2 = GetScreenPitch();

  schovej_mysku();
  p=back_pic=NewArr(word,3+340*200);
  *p++=340;
  *p++=200;
  *p++=A_16BIT;
  for(yp=0;yp<200;yp++)
    {
    s2=s;
    for(xp=0;xp<340;xp++)
      {
      *p++=*s2++;
      if (!skpx) skpx=8;else s2++,skpx--;
      }
    s+=scr_linelen2;
    if (!skpy) skpy=4;else s+=scr_linelen2,skpy--;
    }
  ukaz_mysku();
  }

void line_destructor(void *x) {
    TDLG_TEXT_LINE *item = (TDLG_TEXT_LINE *)x;
    free(item->face);
}

static void load_custom_layout() {
    if (test_file_exist(0,LAYOUT_FILE)) {
        int32_t sz;
        const void *ptr = afile(LAYOUT_FILE, 0, &sz);
        memcpy(&dlg_layout,ptr, MIN(sizeof(dlg_layout),(size_t)sz));
        ablock_free(ptr);
    }
}

void dialog_set_speaker(THUMAN *h) {
    speakers[0] = h;
}

void call_dialog(int entr,int mob)
  {
  int i;


  curcolor=0;
  end_text_line = 0;
  vyb_volba=0;
  pocet_voleb = 0;

  bott_draw(1);
  load_custom_layout();
  create_back_pic();
  bar32(0,SCREEN_OFFLINE,639,SCREEN_OFFLINE+359);
  SEND_LOG("(DIALOGS) Starting dialog...");
  for(i=0;i<POCET_POSTAV;i++) if (isdemon(postavy+i)) unaffect_demon(i);
  mute_all_tracks(0);
  dialog_mob=mob;
  if (mob>-1)
     {
     _monster_flag_map[0]=mobs[mob].dialog_flags;
     _monster_flag_map[1]=mobs[mob].stay_strategy;
     }
  local_pgf=0;
  if (picked_item) held_item=*picked_item-1; else held_item=-1;
  poloz_vsechny_predmety();
  norefresh=1;
  vector_init(&dlg_text, sizeof(TDLG_TEXT_LINE), line_destructor);
  his_line=0;
  memset(speakers,0,sizeof(speakers));
  goto_paragraph(entr);
  schovej_mysku();
  alock(H_DIALOGY_DAT);
  selected_player=-1;
  clk_dialog[0].xlu = TEXT_X;
  clk_dialog[0].ylu = TEXT_Y;
  clk_dialog[0].xrb = TEXT_X+TEXT_XS;
  clk_dialog[0].yrb = TEXT_Y+TEXT_YS;
  do_dialog();
  }

char save_dialog_info(TMPFILE_WR *f)
  {
  int pgf_pocet;
  int *p,i;
  size_t siz;
  char *c,res=0;
  T_PARAGRAPH *q;

  SEND_LOG("(DIALOGS)(SAVELOAD) Saving dialogs info...");
  int32_t varcnt = -MAX_VARIABLES;
  temp_storage_write(&varcnt, sizeof(varcnt), f);
  temp_storage_write(variables, sizeof(variables), f);
  p=ablock_copy(H_DIALOGY_DAT);
  pgf_pocet=*p;
  temp_storage_write(&pgf_pocet,1*4,f);
  siz=(pgf_pocet+3)/4;
  if (siz)
     {
     c=getmem(siz);
     memset(c,0,siz);
     p=ablock_copy(H_DIALOGY_DAT);
     q=(T_PARAGRAPH *)(p+2);
     for(i=0;i<pgf_pocet;i++)
       {
       int j=(i & 3)<<1;
       c[i>>2]|=(q[i].visited<<j) | (q[i].first<<(j+1));
       }
     temp_storage_write(c,1*siz,f);
     free(c);
     }
  temp_storage_write(_flag_map,1*sizeof(_flag_map),f);
  SEND_LOG("(DIALOGS)(SAVELOAD) Done...");
  return res;
  }

char load_dialog_info(TMPFILE_RD *f)
  {
  int32_t pgf_pocet;
  int *p,i;
  size_t siz;
  char *c,res=0;
  T_PARAGRAPH *q;

  SEND_LOG("(DIALOGS)(SAVELOAD) Loading dialogs info...");
  p=ablock_copy(H_DIALOGY_DAT);
  temp_storage_read(&pgf_pocet,1*4,f);
  if (pgf_pocet < 0) {
    SEND_LOG("(ERROR) Different variable count");
    if (pgf_pocet != -MAX_VARIABLES) {
        temp_storage_skip(f,-pgf_pocet * sizeof(short));
    } else {
        temp_storage_read(variables, sizeof(variables), f);
        temp_storage_read(&pgf_pocet,1*4,f);
    }
  }
  siz=(pgf_pocet+3)/4;
  if (pgf_pocet!=*p)
     {
     SEND_LOG("(ERROR) Dialogs has different sizes %d!=%d (can be skipped)",pgf_pocet,*p);
     temp_storage_skip(f,siz);     
     }
  else if (siz)
     {
     c=getmem(siz);
     res|=(temp_storage_read(c,1*siz,f)!=siz);
     p=ablock_copy(H_DIALOGY_DAT);
     q=(T_PARAGRAPH *)(p+2);
     for(i=0;i<pgf_pocet;i++)
       {
       int j=(i & 3)<<1;
       q[i].visited=(c[i>>2]>>j);
       q[i].first=(c[i>>2]>>(j+1));
       }
     free(c);
     }
  res|=(temp_storage_read(_flag_map,1*sizeof(_flag_map),f)!=sizeof(_flag_map));
  SEND_LOG("(DIALOGS)(SAVELOAD) Done...");
  return res;
  }

  char dialog_is_paragraph(int id) {
    const  int *pp=(const int *)ablock(H_DIALOGY_DAT);
    int pocet=*pp;
    pp+=2;
    const T_PARAGRAPH *z=(const T_PARAGRAPH *)pp;
    for(int i=0;i<pocet;i++,z++) if (z->num==(unsigned)id) return 1;
    return 0;
  }

  char dialog_set_notvisited(int pgf) {
    local_pgf = 0;
    if (!dialog_is_paragraph(pgf)) return 0;
    set_nvisited(pgf);
    return 1;
  }

  static void dlg_error(const char *pattern, ...) {
    va_list args;
    va_start(args,pattern);
    char buff[1024];
    vsnprintf(buff, sizeof(buff), pattern, args);
    va_end(args);

    char *c, *d = buff;
    echo("DIALOG ERROR:");
    c = strchr(buff, '\n');
    while (c) {
        *c = 0;
        echo(d);
        d = c+1;
        c = strchr(d, '\n');
    }
    echo(d);



    static char exit_buff[] = {P_SHORT,131,0,P_SHORT,131,0,P_SHORT,131,0,P_SHORT,131,0,P_SHORT,131,0,P_SHORT,164,0,P_SHORT,255,0};
    pc = exit_buff;
}

void dlg_formated_print(const char *text, int args) {

    int need_buffer = strlen(text) + 4 * args + 1; // -32767 = 6, {} = 2 : 6-2=4
    char *buff = (char *)alloca(need_buffer);
    char *c = buff;
    const char *s = text;
    while (*s) {
        if (s[0] == '{' && s[1] == '}' ) {
           if (args) {
               --args;
               short v = stk_pop();
               sprintf(c, "%d", v);
               c = strchr(c, 0);
           }
           ++s;
        } else {
           *c++ = *s;
        }
        ++s;
    }
    *c = 0;
    echo(buff);
    while (args) {
        stk_pop();
        --args;
    }
}

