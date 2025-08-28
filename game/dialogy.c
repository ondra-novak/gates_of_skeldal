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
#include "lang.h"
#include "ach_events.h"

typedef struct t_paragraph
  {
  unsigned num:15;
  unsigned alt:15;
  unsigned visited:1;
  unsigned first:1;
  int32_t position;
  }T_PARAGRAPH;

#define STR_BUFF_SIZ 4096
#define SAVE_POSTS 20
#define P_STRING 1
#define P_SHORT 2
#define P_VAR 3

#define MAX_VOLEB 10

#define VOLBY_X 85
#define VOLBY_Y 398
#define VOLBY_XS 450
#define VOLBY_YS 10

#define TEXT_X 17
#define TEXT_Y 270
#define TEXT_XS 606
#define TEXT_YS 94
#define TEXT_STEP 11

#define OPER_EQ 32
#define OPER_BIG 35
#define OPER_LOW 33
#define OPER_BIGEQ 36
#define OPER_LOWEQ 34
#define OPER_NOEQ 37

#define PIC_X 17
#define PIC_Y (17+SCREEN_OFFLINE)

#define DESC_COLOR1 (RGB555(28,28,21))

static short varibles[20];

static char sn_nums[SAVE_POSTS];
static char sn_nams[SAVE_POSTS][32];
static char sn_rods[SAVE_POSTS];

static word *back_pic;
static char back_pic_enable=0;

static char showed=0;
static char *pc;
static char *descript=NULL;
static char *string_buffer=NULL;
static char iff;

static char _flag_map[32];

static int local_pgf=0;

static char story_on=0;

static char pocet_voleb=0;
static char vyb_volba=0;
static short vol_n[MAX_VOLEB];
//static char *vol_s[MAX_VOLEB];
static short save_jump;

static TSTR_LIST history=NULL;
static int his_line=0;
static int end_text_line=0;
static int last_his_line=0;

static int starting_shop=-1;

static char halt_flag=0;

static int dialog_mob=0;

static char code_page=1;

char trace_dialogs=0;	

static char case_click(int id,int xa,int ya,int xr,int yr);
static char ask_who_proc(int id,int xa,int ya,int xr,int yr);

static TSTRINGTABLE *dialogy_strtable = NULL;

void wire_dialog_drw(void);

static void (*old_wire_proc)(void) = NULL;


#define CLK_DIALOG 5
static T_CLK_MAP clk_dialog[CLK_DIALOG]=
  {
  {0,TEXT_X,TEXT_Y,TEXT_X+TEXT_XS,TEXT_Y+TEXT_YS,case_click,3,H_MS_DEFAULT},
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

static void error(char *text)
  {
  wzprintf("%.125s paragraph %d\r\nLocal_pgf=%d / DIALOG : %d / SENTENCE : %d\r\n",text,last_pgf+local_pgf,local_pgf,local_pgf/128,last_pgf);
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

static T_PARAGRAPH *find_paragraph(int num)
  {
  int *pp;
  int pocet,i;
  T_PARAGRAPH *z;

  num+=local_pgf;
  pp=(int *)ablock_copy(H_DIALOGY_DAT);
  pocet=*pp;pp+=2;
  z=(T_PARAGRAPH *)pp;
  for(i=0;i<pocet;i++,z++) if (z->num==(unsigned)num) return z;
  {
  char s[80];

  sprintf(s,"Paragraph %d doesn't exists! Called from",num);
  error(s);
  return (T_PARAGRAPH *)pp;
  }
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

static void goto_paragraph(int prgf)
  {

  T_PARAGRAPH *z;

  ach_event_dialog_paragraph(prgf+local_pgf);

  do
     {
     z=find_paragraph(prgf);
     if (trace_dialogs) wzprintf("Dialog goto_paragraph %d (visited=%d)\n",prgf+local_pgf, z->visited);
     if (z->visited) z->first=1;
     if (z->alt==z->num || !z->visited)
        {
        pc=((char *)ablock_copy(H_DIALOGY_DAT))+*((const int *)ablock(H_DIALOGY_DAT))*sizeof(T_PARAGRAPH)+8+z->position;
        last_pgf=prgf;
        z->visited=1;
        return;
        }
     prgf=z->alt-local_pgf;
     do_events();
     }
  while (1);
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
           case 'n':strcpy(target,sn_nams[0]);target+=strlen(sn_nams[0]);break;
           default: num=0;while (isdigit(*source)) num=10*num+*source++-'0';
                    if (*source=='l')
                       {
                       sn_nums[0]=sn_nums[num];
                       strcopy_n(sn_nams[0],sn_nams[num],sizeof(sn_nams[0]));
                       sn_rods[0]=sn_rods[num];
                       }
                    break;
           }
           source++;
        }
     else if (*source=='[')
        {
        source++;
        num=sn_rods[0];
        while(num>0)
           {
           source=strchr(source,',');
           num--;
           if (source==NULL)
              {
			  char buff[256];

			  sprintf(buff,"%s\r\nChybny rod nebo maly pocet tvaru od jednoho slova",orgn);
              error(buff);
              exit(-1);
              }
           source++;
           }
        while (*source!=',' && *source!=']' && *source!=0) *target++=*source++;
        if (*source!=']')
           {
           source=strchr(source,']');
           if (source==NULL)
              {
			  char buff[256];

			  sprintf(buff,"%s\r\nOcekava se ]",orgn);
              error(buff);
              exit(-1);
              }
           }
        source++;
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
    const char *start = (const char *)ablock(H_DIALOGY_DAT);
  char *c,i;
  if (*pc==P_STRING)
     {
      int ofs = pc - start+1;
     pc++;
     const char *txt = stringtable_find(dialogy_strtable,ofs, pc);
     c=conv_text(txt);
     do
        {
        pc+=strlen(pc)+1;
        ofs = pc - start;
        if ((i=zjisti_typ())==P_STRING)
           {
            const char *txt = stringtable_find(dialogy_strtable,ofs, pc);
           pc++;
           c=transfer_text(txt,c);
           }
        }
     while(i==P_STRING);
     return string_buffer;
     }
  if (zjisti_typ()==P_SHORT)
     {
     short i;
     pc++;
     i = (uint8_t)pc[0] + 256*pc[1];
     pc+=2;
     if (i<=0) c=conv_text(texty[abs(i)]);else c=conv_text(level_texts[i]);
     return string_buffer;
     }
  error("O�ek�v� se �et�zec nebo index do tabulky �et�zc�");
  exit(0);
  return NULL;
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
     return varibles[p];
     }
  error("O�ek�v� se ��slo");
  exit(0);
  return 0;
  }

static void show_desc()
  {
  char *c=descript;
  int y;

  showed=0;
  show_dialog_picture();
  if (c==NULL) return;
  y=34;
  set_font(H_FBOLD,DESC_COLOR1);
  while (*c)
     {
     position(382,y);
     outtext(c);y+=text_height(c);
     c=strchr(c,0)+1;
     }
  }

static void add_desc(char *c)
  {
  int xs,ys;
  if (story_on) write_story_text(c);
  if (descript!=NULL) free(descript);
  descript=(char *)getmem(strlen(c)+2);
  set_font(H_FBOLD,RGB555(31,31,31));
  zalamovani(c,descript,225,&xs,&ys);
  }

static void show_emote(char *c)
  {
  int xs,ys;
  char *a;

  if (story_on) write_story_text(c);
  a=alloca(strlen(c)+2);
  set_font(H_FBOLD,RGB555(31,31,31));
  zalamovani(c,a,TEXT_XS,&xs,&ys);
  while (*a)
     {
     char z[100]="M";
     strcat(z,a);
     end_text_line=str_add(&history,z)+1;
     a=strchr(a,0)+1;
     }
  }


static void echo(char *c)
  {
  int xs,ys;
  char *a;

  if (story_on) write_story_text(c);
  a=alloca(strlen(c)+2);
  set_font(H_FBOLD,RGB555(0,30,0));
  zalamovani(c,a,TEXT_XS,&xs,&ys);
  while (*a)
     {
     char z[100]="E";
     strcat(z,a);
     end_text_line=str_add(&history,z)+1;
     a=strchr(a,0)+1;
     }
  }

#define TEXT_UNSELECT *((word *)ablock(H_DIALOG)+3+254)
#define TEXT_SELECT *((word *)ablock(H_DIALOG)+3+255)

static void redraw_text()
  {
  int y=TEXT_Y;
  int ys=TEXT_YS;
  int ls_cn,i;


  put_textured_bar(ablock(H_DIALOG),TEXT_X,TEXT_Y,TEXT_XS,TEXT_YS,TEXT_X,TEXT_Y-SCREEN_OFFLINE);
  //create_frame(TEXT_X,TEXT_Y,TEXT_XS,TEXT_YS,1);
  ls_cn=str_count(history);
  if (ls_cn<=his_line) return;

  for (i=his_line;i<ls_cn;i++)
     if (history[i]!=NULL)
        {
        char *c=&history[i][0];
        if (*c=='E') set_font(H_FBOLD,NOSHADOW(0));
        else if(*c=='M') set_font(H_FBOLD,NOSHADOW(0)+TEXT_UNSELECT);
        else if(*c-48==vyb_volba) set_font(H_FBOLD,NOSHADOW(0)+TEXT_SELECT);
        else set_font(H_FBOLD,NOSHADOW(0)+TEXT_UNSELECT);
        c++;
        position(TEXT_X,y);outtext(c);
        y+=TEXT_STEP;
        ys-=TEXT_STEP;
        if (ys<TEXT_STEP) break;
        }
  }

static int get_last_his_line()
  {
  int i=0,cf;

  cf=str_count(history);
  while(i<cf && history[i]!=NULL) i++;
  return i;
  }

static void draw_all()
  {
  const void *c;
  show_desc();
  if (back_pic_enable) c=back_pic;else c=ablock(H_DIALOG_PIC);
  other_draw();
  if (c!=NULL) put_picture(PIC_X,PIC_Y,c);
  redraw_text();
  }

static void lecho(char *c)
  {
  write_story_text(c);
  }

static void save_name(int pos)
  {
  sn_nums[pos]=sn_nums[0];
  strcopy_n(sn_nams[pos],sn_nams[0],sizeof(sn_nams[pos]));
  sn_rods[pos]=sn_rods[0];
  }

static void load_name(int pos)
  {
  sn_nums[0]=sn_nums[pos];
  strcopy_n(sn_nams[0],sn_nams[pos],sizeof(sn_nams[0]));
  sn_rods[0]=sn_rods[pos];
  }

static void nahodne(int vls,int omz,char check)
  {
  char chk[POCET_POSTAV];
  int i,l,m;

  memset(chk,0,sizeof(chk));
  if (!check) for(i=0;i<SAVE_POSTS;i++) if (sn_nums[i]<POCET_POSTAV) chk[(uint8_t)sn_nums[i]]|=1;
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor!=viewsector || !postavy[i].lives || !postavy[i].used) chk[i]|=2;
  m=0;l=-1;
  for(i=0;i<POCET_POSTAV;i++)
     if (postavy[i].vlastnosti[vls]>=omz && chk[i]==0)
        {
        if (l>=0) chk[l]|=4;
        l=-2;iff=0;
        }
     else if  (l!=-2 && chk[i]==0 && postavy[i].vlastnosti[vls]>=m)
           {
           if (l>=0) chk[l]|=4;
           l=i;iff=1;
           m=postavy[i].vlastnosti[vls];
           }
        else chk[i]|=4;
  m=0;
  for(i=0;i<POCET_POSTAV;i++) if (!chk[i])m++;
  if (m==0 && !check)
     {
     for(i=0;i<POCET_POSTAV;i++) chk[i]&=~1;
     for(i=0;i<POCET_POSTAV;i++) if (!chk[i])m++;
     iff=1;
     if (m==0) return;
     }
  l=rnd(m)+1;
  for(i=0;l>0;i++) if (!chk[i]) l--;
  i--;
  sn_nums[0]=i;
  strcpy(sn_nams[0],postavy[i].jmeno);
  sn_rods[0]=postavy[i].female;
  }

static void pc_xicht(int xichtid)
{
  int i;
  for (i=0;i<POCET_POSTAV;i++)
    if (postavy[i].used && postavy[i].xicht==xichtid)
    {
      sn_nums[0]=i;
      strcpy(sn_nams[0],postavy[i].jmeno);
      sn_rods[0]=postavy[i].female;
      iff=1;
      return;
    }
iff=0;
return;
}

static char visited(int prgf)
  {
  T_PARAGRAPH *z;

  z=find_paragraph(prgf);
  return z->visited;
  }

static void set_nvisited(int prgf)
  {
  T_PARAGRAPH *z;

  z=find_paragraph(prgf);
  z->visited=0;
  z->first=0;
  }


void q_flag(int flag)
  {
  iff=_flag_map[flag>>3] & (1<<(flag & 0x7));
  }

static void set_flag(int flag)
  {
  q_flag(flag);
  _flag_map[flag>>3]|=(1<<(flag & 0x7));
  }

static void reset_flag(int flag)
  {
  q_flag(flag);
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
      redraw_text();
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
  vol_n[(uint8_t)pocet_voleb]=num;
  if (pocet_voleb>MAX_VOLEB) {error("POZOR! Je priliz mnoho voleb");pocet_voleb=MAX_VOLEB;}
  a=alloca(strlen(text)+2);
  set_font(H_FBOLD,RGB555(0,30,0));
  zalamovani(text,a,TEXT_XS,&xs,&ys);
  while (*a)
     {
     char z[100];
     z[0]=pocet_voleb+48;
     z[1]=0;
     strcat(z,a);
     str_add(&history,z);
     a=strchr(a,0)+1;
     }
  pocet_voleb++;
  }

static void remove_all_cases()
  {
  int cf,i;
  pocet_voleb=0;
  cf=str_count(history);
    for (i = end_text_line; i < cf; i++) {
        if (history[i] != NULL) {
            if (history[i][0] - 48 != vyb_volba) {
                str_replace(&history, i, NULL);
            } else {
                history[i][0] = 'M';
            }
        } else {
            break;
        }
    }
  str_delfreelines(&history);
  vyb_volba=0;
  }

static char case_click(int id,int xa,int ya,int xr,int yr)
  {
  int cf;

  xa,ya,xr,yr;

  if (pocet_voleb>1)
     {
     id=yr/TEXT_STEP;
     cf=str_count(history);
     id+=his_line;
     if (id>=cf) return 0;
     if (history[id]==NULL) return 0;
     id=history[id][0];
     if (id=='E' || id=='M') return 0;
     id-=48;
  if (id!=vyb_volba)
     {
     vyb_volba=id;
     schovej_mysku();
     redraw_text();
     ukaz_mysku();
     showview(TEXT_X,TEXT_Y,TEXT_XS,TEXT_YS);
     }
  if (ms_last_event.event_type & 0x2)
     {
     dialog_cont();
     }
     }
  else
  if (ms_last_event.event_type & 0x2)
     {
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
  _flag_map[0]|=0x1;
  stop_anim();
  if (dialog_mob>-1)
     {
     mobs[dialog_mob].dialog_flags=_flag_map[0];
     mobs[dialog_mob].stay_strategy=_flag_map[1];
     }
  unwire_proc();
  aunlock(H_DIALOGY_DAT);
  ukaz_mysku();
  free(descript);descript=NULL;
  free(string_buffer);string_buffer=NULL;
  remove_all_cases();
  release_list(history);
  history=0;
  free(back_pic);
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
  if (strcmp(c,"SCREEN")) def_handle(H_DIALOG_PIC,c,hi_8bit_correct,SR_DIALOGS),back_pic_enable=0;
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
  int selected_player=sn_nums[0];
  if (selected_player<0 || selected_player>=POCET_POSTAV) return 1;
  memcpy(postavy+selected_player,postavy+selected_player+1,sizeof(*postavy)*(POCET_POSTAV-selected_player));
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
  dead_players=0;
  schovej_mysku();
  mouse_set_default(H_MS_DEFAULT);
  if (selected_player==-1) return 1;
  strcpy(sn_nams[0],postavy[selected_player].jmeno);
  sn_rods[0]=postavy[selected_player].female;
  sn_nums[0]=selected_player;
  change_click_map(NULL,0);
  return 0;
  }

extern word weapon_skill[];

static void pract(int h,int vls,int how,int max)
  {
   iff=0;
   if (vls>=100)
     {
     vls-=100;
     if (postavy[h].bonus_zbrani[vls]>=max) iff=1;
     else
        {
        postavy[h].bonus_zbrani[vls]+=how;
        if (postavy[h].bonus_zbrani[vls]>max) postavy[h].bonus_zbrani[vls]=max,iff=1;
        postavy[h].weapon_expy[vls]=weapon_skill[postavy[h].bonus_zbrani[vls]];
        }
     }
   else
     {
     if (postavy[h].vlastnosti[vls]>=max) iff=1;
     else
        {
        postavy[h].stare_vls[vls]+=how;
        prepocitat_postavu(postavy+h);
        if (postavy[h].vlastnosti[vls]>max)
           {
           postavy[h].stare_vls[vls]-=postavy[h].vlastnosti[vls]-max;
           postavy[h].vlastnosti[vls]=max;
           iff=1;
           }
        }
     }
  }

static void pract_to(int h, int vls,int how)
  {
   iff=0;
   if (vls>=100)
     {
     vls-=100;
     if (postavy[h].bonus_zbrani[vls]<how)
        {
        postavy[h].bonus_zbrani[vls]=how;
        postavy[h].weapon_expy[vls]=weapon_skill[how];
        }
     else iff=1;
     }
   else
     {
     if (postavy[h].vlastnosti[vls]<how)
        postavy[h].stare_vls[vls]+=how-postavy[h].vlastnosti[vls];else iff=1;
     prepocitat_postavu(postavy+h);
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
     default:error("Chybn� operator porovn�v�n�");
     }
  return 0;
  }

static char test_vls(int h,int vls,int oper,int num)
  {
  int val;
   if (vls>=100)
     {
     vls-=100;
     val=postavy[h].bonus_zbrani[vls];
     }
  else
     val=postavy[h].stare_vls[vls];
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

static void cast_spell(int spell)
  {
  int cil=1+sn_nums[0];

  add_spell(spell,cil,cil,1);
  }

static void free_dialog_stringtable(void) {
    stringtable_free(dialogy_strtable);
}

void do_dialog()
  {
  int i,p1,p2,p3;
  char *c;

  if (!dialogy_strtable) {
      dialogy_strtable = lang_load("dialogs.csv");
      if (dialogy_strtable) {
          atexit(free_dialog_stringtable);
      }
  }

  do
     {
  i=Get_short();p3=0;
  switch(i)
     {
     case 128:add_desc(Get_string());break;
     case 129:show_emote(Get_string());break;
     case 130:save_name(Get_short());break;
     case 131:iff=!iff;break;
     case 132:load_name(Get_short());break;
     case 133:nahodne(0,0,Get_short());break;
     case 134:p1=Get_short();p2=Get_short();nahodne(VLS_SMAGIE,p1,p2);break;
     case 135:p1=Get_short();p2=Get_short();nahodne(VLS_SILA,p1,p2);break;
     case 136:p1=Get_short();p2=Get_short();nahodne(VLS_OBRAT,p1,p2);break;
     case 137:c=Get_string();p1=Get_short();strcopy_n(sn_nams[0],c,32);sn_rods[0]=p1;break;
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
			  if (game_extras & EX_AUTOOPENBOOK) autoopenaction=1;
              break;
     case 150:set_nvisited(Get_short());break;
     case 151:iff=rnd(100)<=(unsigned int)Get_short();break;
     case 152:iff=q_item(Get_short(),viewsector)!=NULL;break;
     case 153:create_item(Get_short());break;
     case 154:destroy_item(Get_short());break;
     case 155:money+=Get_short();break;
     case 156:p1=Get_short();if (p1>money) iff=1;else money-=p1;break;
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
              if (dlg_ask_who()) if (p1) goto_paragraph(p1);
                                   else iff=1;
                               else iff=0;
              break;
     case 176:p1=Get_short();p2=Get_short();pract_to(sn_nums[0],p1,p2);break;
     case 177:p1=Get_short();p2=Get_short();p3=Get_short();iff=test_vls(sn_nums[0],p1,p2,p3);break;
     case 178:p1=Get_short();runes[p1/10]|=1<<(p1%10);break;
     case 179:p1=Get_short();iff=((runes[p1/10] & (1<<(p1%10)))!=0);break;
     case 180:p1=Get_short();iff=(money>=p1);break;
     case 181:p1=Get_short();p2=Get_short();p3=Get_short();pract(sn_nums[0],p1,p2,p3);break;
     case 182:p1=Get_short();p2=Get_short();dark_screen(p1,p2);break;
     case 183:spat(Get_short());break;
     case 184:p1=Get_short();iff=najist_postavy(p1);break;
     case 185:iff=isall();break;
		 case 186:enable_glmap=Get_short();break;
     case 187:p1=Get_short();p2=Get_short();iff=atsector(p1,p2);break;
     case 188:p1=Get_short();cast_spell(p1);break;
     case 190:spell_sound(Get_string());break;
     case 191:p1=Get_short();p2=Get_short();iff=test_volby_select(p1,p2);break;
     case 192:p1=Get_short();p2=Get_short();varibles[p1]=p2;break;
     case 193:p1=Get_short();p2=Get_short();varibles[p1]+=p2;break;
     case 194:p1=Get_short();p2=Get_short();p3=Get_short();iff=oper_balance(varibles[p1],p3,p2);break;
     case 195:p2=find_pgnum(pc);p1=Get_short();varibles[p1]=p2;break;
     case 196:p1=Get_short();varibles[p1]=iff;break;
     case 197:p1=Get_short();add_case(varibles[p1],Get_string());break;
     case 198:p1=Get_short();p2=Get_short();c=Get_string();if (iff==p1) add_case(varibles[p2],c);break;
     case 199:goto_paragraph(varibles[Get_short()]);break;
     case 200:iff=drop_character();break;
     case 201:pc_xicht(Get_short());break;
     case 518:set_flag(Get_short());break;
     case 519:reset_flag(Get_short());break;
     case 255:exit_dialog();return;
     default:
        {
        char s[80];
        sprintf(s,"Nezn�m� instrukce: %d",i);
        error(s);
        }
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

void call_dialog(int entr,int mob)
  {
  int i;
//  void (*old_wire_proc)()=wire_proc;
  curcolor=0;
  create_back_pic();
  bar32(0,SCREEN_OFFLINE,639,SCREEN_OFFLINE+359);
  SEND_LOG("(DIALOGS) Starting dialog...");
  for(i=0;i<POCET_POSTAV;i++) if (isdemon(postavy+i)) unaffect_demon(i);
  mute_all_tracks(0);
  dialog_mob=mob;
  if (mob>-1)
     {
     _flag_map[0]=mobs[mob].dialog_flags;
     _flag_map[1]=mobs[mob].stay_strategy;
     }
  local_pgf=0;
  poloz_vsechny_predmety();
  norefresh=1;
  history=create_list(256);
  his_line=0;
  memset(sn_nums,0xff,sizeof(sn_nums));
  goto_paragraph(entr);
  schovej_mysku();
  alock(H_DIALOGY_DAT);
  selected_player=-1;
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
  int pgf_pocet;
  int *p,i;
  size_t siz;
  char *c,res=0;
  T_PARAGRAPH *q;

  SEND_LOG("(DIALOGS)(SAVELOAD) Loading dialogs info...");
  p=ablock_copy(H_DIALOGY_DAT);
  temp_storage_read(&pgf_pocet,1*4,f);
  siz=(pgf_pocet+3)/4;
  if (pgf_pocet!=*p)
     {
     SEND_LOG("(ERROR) Dialogs has different sizes %d!=%d (can be skipped)",pgf_pocet,*p);
     temp_storage_skip(f,siz+sizeof(_flag_map));
     return 0;
     }
  if (siz)
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
  
