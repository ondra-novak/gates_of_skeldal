#include <platform/platform.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include <libs/types.h>
#include <libs/bgraph.h>
#include <libs/event.h>
#include <libs/devices.h>
#include <libs/bmouse.h>
#include <libs/memman.h>
#include <platform/sound.h>
#include <libs/strlite.h>
#include <libs/gui.h>
#include <libs/basicobj.h>
#include <time.h>
#include <stdarg.h>
#include "globals.h"

void kamenik2windows(const char *src, int size, char *trg);

#define BREAK

static void wzprintf(const char *text,...)
  {
    va_list lst;
    va_start(lst, text);
    vprintf(text, lst);
  }

static void wzputs(const char *text)
  {
  wzprintf(text);
  wzprintf("\r\n");
  }

static void wzcls()
  {

  }



static int wzscanf(const char *prompt, const char *format,...)
  {
    va_list lst;
    va_start(lst, format);
    return vscanf(format, lst);
  }
char *side_flags[]=
  {
  "AUTOMAP",
  "!PLR",
  "!MONST",
  "!THING",
  "!SOUND",
  "ALARM",
  "PASS",
  "TRANSPARENT",
  "P_ANIM",
  "P_VIS",
  "P_PING",
  "P_FORW",
  "S_ANIM",
  "S_VIS",
  "S_PING",
  "S_FORW",
  "LEFT_A",
  "RIGHT_A",
  "ALT_SIDES",
  "SPEC",
  "COPY",
  "SEND",
  "APLY2ND",
  "AUTOANIM",
  "",
  "",
  "",
  "",
  "",
  "SECRET",
  "TRUESEE",
  "INVIS"
  };

char *obl_flags[]=
  {
  "RECESS",
  "UP_SIDE",
  "DOWN_SIDE",
  "ITPUSH"
  };

char *mc_flags[]=
  {
  "MAPPED",
  "FLY_OBJECT",
  "PLAYER",
  "DEAD_PLAYER",
  "SPECTXTR",
  "SAFEPLACE",
  "UNUSED",
  "MARKED!",
  "SHADED",
  "STAIRS",
  "DOWN",
  "!AUTOMAP",
  "!SUMMON"
  };

void mman_scan(int )
  {
  }

void show_flags(int number,char **flags,char nums)
  {
  int i=0;
  while (nums--)
     {
     if (number & 1) wzprintf("%s ",flags[i]);
     i++;
     number>>=1;
     }
  }

void spell_group_invis()
  {
  int i;
  char ok=1;

  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==viewsector && !(postavy[i].stare_vls[VLS_KOUZLA] & SPL_INVIS))
        {
        postavy[i].stare_vls[VLS_KOUZLA]|=SPL_INVIS;
        ok=0;
        }
  if (ok)
  for(i=0;i<POCET_POSTAV;i++) if (postavy[i].sektor==viewsector)
        {
        postavy[i].stare_vls[VLS_KOUZLA]&=~SPL_INVIS;
        }
  for(i=0;i<POCET_POSTAV;i++) prepocitat_postavu(postavy+i);
  build_all_players();
  }



static void advence_player(int player,int level,char auto_advance)
  {
  THUMAN *h;
  float mh,mv;

  if (level<2) return;
  h=postavy+player;
  mh=(float)human_selected->jidlo/MAX_HLAD(human_selected);
  mv=(float)human_selected->voda/MAX_ZIZEN(human_selected);
  human_selected=h;
  h->exp=level_map[level-2];
  check_player_new_level(h);
  if (auto_advance)
     {
     int vlssuma=h->vlastnosti[VLS_SILA]+
                 h->vlastnosti[VLS_OBRAT]+
                 h->vlastnosti[VLS_POHYB]+
                 h->vlastnosti[VLS_SMAGIE];
     int b,i;

     for(i=0;i<4;i++)
        {
        b=h->vlastnosti[i]*h->bonus/vlssuma;
        h->bonus-=b;vlssuma-=h->vlastnosti[i];
        while (b--) advance_vls(i);
        }
     prepocitat_postavu(human_selected);
     }
  human_selected->jidlo=(int)(mh*MAX_HLAD(human_selected));
  human_selected->voda=(int)(mv*MAX_ZIZEN(human_selected));
  wzprintf("%s ziskal%s uroven cislo %d\r\n",h->jmeno,h->female?"a":"",level);
  }


extern char folow_mode;
extern char folow_mob;
void macro_drop_item();

static char take_money()
  {
  int i;
  if (!wzscanf("Kolik: (0 - zrusit):","%d",&i)) return 0;
  money+=i;
  if (i)
     {
     SEND_LOG("(WIZARD) Take Money %d, total %d",i,money);
     }
  return (i!=0);
  }

#define ALL "ALL"
static char purge_map()
  {
  char buffer[200];
  char *c;

  STOP();
/*  struct find_t rc;
  int rs;

  concat(c,pathtable[SR_TEMP],"*.TMP");
  rs=_dos_findfirst(c,_A_NORMAL,&rc);
  while (rs==0)
     {
     if (rc.name[0]!='~') wzputs(rc.name);
     rs=_dos_findnext(&rc);
     }
  _dos_findclose(&rc);*/
  wzprintf("\r\n Zadej jmeno tempu (all - vse):");gets(buffer);
  if (buffer[0]==0) return 0;
  strupper(buffer);
  concat(c,pathtable[SR_TEMP],buffer);
  if (strcmp(buffer,ALL) && check_file_exists_ex(c))
     {
     wzputs("Soubor nenalezen!");
     return 0;
     }
  SEND_LOG("(WIZARD) Purge Map: '%s'",buffer,0);
  if (!strcmp(buffer,ALL)) purge_temps(0);
  else remove(c);
  return 1;
  }

static char heal_meditate(void)
  {
  int a,b,i;
  THUMAN *p;

  if (!wzscanf("Obnovit postavu c: (0 - vsechny, -1 - zrusit):","%d",&b)) return 0;
  if (b==-1) return 0;
  if (b) a=b-1;else a=0,b=POCET_POSTAV;
  p=postavy+a;
  for(i=a;i<b;i++,p++) if (p->used && p->lives)
     {
     p->lives=p->vlastnosti[VLS_MAXHIT];
     p->mana=p->vlastnosti[VLS_MAXMANA];
     p->kondice=p->vlastnosti[VLS_KONDIC];
     p->jidlo=MAX_HLAD(p);
     p->voda=MAX_ZIZEN(p);
     SEND_LOG("(WIZARD) Restoring character '%s'",p->jmeno,0);
     bott_draw(1);
     }
  return 1;
  }

static char raise_death(void)
  {
  int b;
  THUMAN *p;
  char *c,*d;

  if (!wzscanf("Obzivit postavu c: (0 a -1 - zrusit):","%d",&b)) return 0;
  b--;
  if (b<0) return 0;
  p=postavy+b;
  p->lives=p->vlastnosti[VLS_MAXHIT];
  p->mana=p->vlastnosti[VLS_MAXMANA];
  p->kondice=p->vlastnosti[VLS_KONDIC];
  c="(WIZARD) '%s' has been returned to game by gods power!";d=strchr(c,'\'');
  wzprintf(d,p->jmeno);putchar('\r\n');
  bott_draw(1);
  return 0;
  }

/*
  static char raise_killed_monster(HWND hDlg)
  {
    HWND listdlg=PrepareListWindow(hDlg);
    HWND list=GetDlgItem(listdlg,IDC_LIST);
    char buff[256];
    int i;
    int res;

    for (i=0;i<MAX_MOBS;i++) if (~mobs[i].vlajky & MOB_LIVE && mobs[i].cislo_vzoru!=0)
    {
      int p;
      _snprintf(buff,sizeof(buff),"%4d. %s (sector: %d home %d)",i,mobs[i].name,mobs[i].sector,mobs[i].home_pos);
      kamenik2windows(buff,strlen(buff),buff);
      p=ListBox_AddString(list,buff);
      ListBox_SetItemData(list,p,i);
    }
    res=PumpDialogMessages(listdlg);
    while (res==IDOK)
    {
      int cnt;
      for (i=0,cnt=ListBox_GetCount(list);i<cnt;i++) if (ListBox_GetSel(list,i))
      {
        int idx=ListBox_GetItemData(list,i);
        mobs[idx].vlajky|=MOB_LIVE;
        mobs[idx].lives=mobs[idx].vlastnosti[VLS_MAXHIT];
        wzprintf("%s znovu povstal(a)\r\n",mobs[idx].name);
        SEND_LOG("(WIZARD) '%s' has been raised",mobs[idx].name,0);
      }
      res=PumpDialogMessages(listdlg);
    }
    CloseListWindow(listdlg);
    return 1;
  }
*/
void unaffect();
extern char immortality;
extern char nohassle;

static char set_immortality()
  {
  immortality=!immortality;
  SEND_LOG("(WIZARD) Immortality has been turned %s.",immortality?"on":"off",0);
  wzprintf("Nesmrtelnost byla %s.\r\n",immortality?"zapnuta":"vypnuta");
  return 0;
  }

static char set_nohassle()
  {
  nohassle=!nohassle;
  SEND_LOG("(WIZARD) Nohassle has been turned %s.",nohassle?"on":"off",0);
  wzprintf("Nevycititelnost byla %s.\r\n",nohassle?"zapnuta":"vypnuta");
  return 0;
  }


static char advance_weapon()
  {
  int p,i;
  char buff[128];
  THUMAN *h;
  if (!wzscanf("Cislo postavy: (0 = Zpet)","%d",&p)) return 0;
  if (p==0) return 0;
  h=postavy+p-1;
  do
     {
     int bonus, value;
     for(i=0;i<TPW_MAX;i++) wzprintf("%d. %-15s: %2d Exp %5d\r\n",i+1,texty[91+i],h->bonus_zbrani[i],h->weapon_expy[i]);
     if (!wzscanf("<Zbran> <Hodnota>","%[^\n]",buff)) return 0;
     if (buff[0]==0) return 0;
     if (sscanf(buff,"%d %d",&bonus,&value)!=2) wzputs("Huh?!");
     else
        {
        bonus--;
        if (bonus<0 || bonus>=TPW_MAX) wzputs("Spatna zbran");
        else
           if (value<0 || value>=10) wzputs("Spatna hodnota");
              else
                 h->bonus_zbrani[bonus]=value;
        }
     }
  while(1);
  }

static reload_mobs()
  {
  extern char reset_mobiles;
  reset_mobiles=1;
  strcopy_n(loadlevel.name,level_fname,sizeof(loadlevel.name));
  loadlevel.start_pos=viewsector;
  loadlevel.name[12]=0;
  loadlevel.dir=viewdir;
  send_message(E_CLOSE_MAP);
  }

static char display_game_status(void)
  {
  short *v;
  THUMAN *p;
  TSTENA *s;
  TSECTOR *ss;
  register i,cn,astr;

  wzcls();
  SEND_LOG("(WIZARD) Starting wizard window at Sect %d Side %d",viewsector,viewdir);
  wzprintf("Sektor: %5d  Smer: %d  Skupina %d \r\n",viewsector,viewdir,cur_group);
  for(i=0,p=postavy;i<POCET_POSTAV;i++,p++)
     if (p->used)
        wzprintf("%d.%-14s (%d) Sek:%5d Smr:%d HPReg:%d MPReg:%d VPReg:%d %04X%s\r\n",i+1,p->jmeno,p->groupnum,p->sektor,p->direction,p->vlastnosti[VLS_HPREG],
              p->vlastnosti[VLS_MPREG], p->vlastnosti[VLS_VPREG], p->vlastnosti[VLS_KOUZLA], p->lives?"":"(smrt)");
     else
        wzprintf("%d. (nepouzito)\r\n",i);
  wzputs("");
  wzprintf("Predmet(y) v mysi: ");
  v=picked_item;
  if (v==NULL) wzprintf("<zadne>");else while(*v) wzprintf("%d ",abs(*v++)-1);
  wzputs("\r\n");
  for(i=0,cn=0,astr=0;i<MAX_MOBS;i++)
     {
     if (mobs[i].vlajky & MOB_LIVE) cn++;
     if (mobs[i].vlajky & MOB_MOBILE) astr++;
     }
  wzprintf("Celkem potvor ve hre:  %5d (+%d) astral mobiles\r\n"
         "Celkem predmetu ve hre:%5d\r\n"
         " .. z toho klonu:      %5d\r\n",cn-astr,astr,item_count,item_count-it_count_orgn);

  wzputs("");
  ss=map_sectors+viewsector;
  s=map_sides+viewsector*4+viewdir;
  wzprintf("Sector: (%d) Podlaha %d Strop %d Cil akce %d Smer akce %d Akce %d\r\n",
         ss->sector_type, ss->floor,ss->ceil,ss->sector_tag,ss->side_tag,ss->action);
  wzprintf("        Vychody: Sev %d Vych %d Jih %d Z�p %d\r\n",ss->step_next[0],ss->step_next[1],ss->step_next[2],ss->step_next[3]);
  wzprintf("        Vlajky: %02X %02X ",ss->flags,map_coord[viewsector].flags);show_flags(map_coord[viewsector].flags,mc_flags,12);
  wzputs("\r\n");
  wzprintf("Stena: Prim %d Sec %d Obl %d Anim_prim %d/%d Anim_sec %d/%d\r\n",
         s->prim,s->sec,s->oblouk & 0xf,s->prim_anim>>4,s->prim_anim & 0xf,s->sec_anim>>4,s->sec_anim & 0xf);
  wzprintf("       Cil akce %d Smer akce %d Akce %d\r\n",s->action,s->sector_tag,s->side_tag & 0x3);
  wzprintf("       Multiakce: %s\r\n",macros[viewsector*4+viewdir]==NULL?"<zadna>":"Existuje");
  wzprintf("       Vlajky: %04X %02X %02X ",s->flags,s->oblouk>>4,s->side_tag>>2);
  wzputs("");
  show_flags(s->flags,side_flags,32);
  show_flags(s->oblouk>>4,obl_flags,4);
  return 0;
  }

void wizard_kbd(EVENT_MSG *msg,void **usr)
  {
  char c;

  if (map_with_password && debug_enabled!=266859) return;
  usr;
  if (msg->msg==E_KEYBOARD)
     {
     int c=va_arg(msg->data,int)>>8;
     msg->msg=-1;
     switch (c)
        {
        case 'C':
        case '<':log_combat=!log_combat;break;
        case '=':show_lives=!show_lives;break;
        case '>':if (mman_action!=NULL) mman_action=NULL;else mman_action=mman_scan;break;
        case '@':set_immortality();set_nohassle();break;
        case 'A':bott_draw_fletna();break;
        case 'B':wire_global_map();break;
        case '?':cur_group=10;break;/*folow_mode=!folow_mode;
                 if (folow_mode) folow_mob=mob_map[map_sectors[viewsector].step_next[viewdir]]-1;
                 else for(c=0;c<POCET_POSTAV;c++) if (postavy[c].groupnum==cur_group) viewsector=postavy[c].sektor;
                 if (folow_mob==255) folow_mode=0;
                 */
                 break;

        default:
              msg->msg=E_KEYBOARD;break;

        }
     }
  return;
  }

c

void install_wizard()
  {
  send_message(E_ADD,E_KEYBOARD,wizard_kbd);
  }



