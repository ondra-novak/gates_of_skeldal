#include <libs/bgraph.h>
#include <libs/event.h>
#include "globals.h"

#include <ctype.h>
#define console_max_characters  120
#define console_max_lines  16


/****/


static void wzprintf(const char *text,...);
static void wzputs(const char *text);

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


#if 0
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
  strupr(buffer);
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
  strncpy(loadlevel.name,level_fname,12);
  loadlevel.start_pos=viewsector;
  loadlevel.name[12]=0;
  loadlevel.dir=viewdir;
  send_message(E_CLOSE_MAP);
  }

#endif


static char display_game_status(void)
  {
  short *v;
  THUMAN *p;
  TSTENA *s;
  TSECTOR *ss;
 int i,cn,astr;

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
  wzprintf("       Multiakce: %s\r\n",macros[viewsector*4+viewdir].action_list==NULL?"<zadna>":"Existuje");
  wzprintf("       Vlajky: %04X %02X %02X ",s->flags,s->oblouk>>4,s->side_tag>>2);
  wzputs("");
  show_flags(s->flags,side_flags,32);
  show_flags(s->oblouk>>4,obl_flags,4);
  return 0;
  }

/*****/

void unaffect();
extern char immortality;
extern char nohassle;
extern char pass_all_mobs;


static char console_input_line[console_max_characters+1]  = "";
static char *console_output_lines[console_max_lines] = {};

static const int console_x = 0;
static const int console_y = 20;
static const int console_width = 640;
static const int console_height = 160;
static const int console_padding = 3;
static int console_blink = 0;
static char console_visible = 0;


void draw_console_window() {
    if (!console_visible) return;
    trans_bar(console_x, console_y, console_width, console_height, 0);

    set_font(H_FLITT5, RGB888(255,255,128));
    int y = console_y+console_height-text_height("X")-console_padding;
    position(console_x+console_padding, y);
    outtext("$ ");
    outtext(console_input_line);
    if ((console_blink>>1) & 1) {
        outtext("_");
    }
    ++console_blink;

    set_font(H_FLITT5, RGB888(255,255,255));


    y-=3*text_height("X")/2;

    for (int i = 0; i < console_max_lines;++i) {
        position(console_x+console_padding,y);
        if (console_output_lines[i]) outtext(console_output_lines[i]);
        y-=text_height("X");
        if (y < console_y+console_padding) break;
    }
}

char console_is_visible() {
    return console_visible;
}

static void console_add_line(const char *line) {
    free(console_output_lines[console_max_lines-1]);
    memmove(console_output_lines+1,console_output_lines, (console_max_lines-1)*sizeof(char *));
    console_output_lines[0] = strdup(line);
}

typedef struct {
    char *cmd_buffer;
    const char *command;
    const char *args;
} PARSED_COMMAND;

static PARSED_COMMAND parse_command(const char *cmd) {
    PARSED_COMMAND ret;
    ret.command = 0;
    ret.args = 0;
    ret.cmd_buffer = strdup(cmd);
    char *c =ret.cmd_buffer;
    while (*c && isspace(*c)) ++c;
    if (!*c) return ret;
    ret.command = c;
    ret.args = NULL;
    char *sep = strchr(c, ' ');
    if (sep!= NULL) {
        *sep = 0;
        ++sep;
        while (*sep && isspace(*sep)) ++sep;
        if (*sep) {
            ret.args = sep;
            char *end = strrchr(sep,0);
            end--;
            while (end > sep && isspace(*end)) end--;
            end++;
            *end = 0;
        }

    }
    return ret;
}

extern int ghost_walls;
extern int nofloors;

static int process_on_off_command(const char *cmd, char on) {
    if (stricmp(cmd, "inner-eye") == 0) {
        show_debug = on;
        return 1;
    }
    if (stricmp(cmd, "hunter-instinct") == 0) {
        show_lives = on;
        return 1;
    }
    if (stricmp(cmd, "no-hassle") == 0) {
        nohassle=on;
        return 1;
    }
    if (stricmp(cmd, "iron-skin") == 0) {
        immortality=on;
        return 1;
    }
    if (stricmp(cmd, "spirit-wander") == 0) {
        cur_group=on?10:postavy[0].groupnum;
        return 1;
    }
    if (stricmp(cmd, "ghost-walls") == 0) {
        ghost_walls = on;
        return 1;
    }
    if (stricmp(cmd, "true-seeing") == 0) {
        true_seeing = on;
        return 1;
    }
    if (stricmp(cmd, "walking-in-air") == 0) {
        nofloors = on;
        return 1;
    }
    if (stricmp(cmd, "enemy-insight") == 0) {
        show_mob_info = on;
        return 1;
    }
    if (stricmp(cmd, "enemy-walk-diagonal") == 0) {
        game_extras = on?(game_extras | EX_WALKDIAGONAL):(game_extras & ~EX_WALKDIAGONAL);
        return 1;
    }
    if (stricmp(cmd, "ghost-form") == 0) {
        pass_all_mobs = on;
        return 1;
    }
    return 0;
}

static int process_actions(const char *command) {
    if (stricmp(command, "flute") == 0) {
        bott_draw_fletna();
        return 1;
    }
    if (stricmp(command, "global-map") == 0) {
        wire_global_map();
        return 1;
    }
    if (stricmp(command, "status") == 0) {
        display_game_status();
        return 1;
    }
    if (stricmp(command, "offlers-blessing") == 0) {
        money=150000;
        play_fx_at(FX_MONEY);
        return 1;
    }
    if (stricmp(command, "i-require-gold") == 0) {
        money+=1;
        play_fx_at(FX_MONEY);
        return 1;
    }
    if (stricmp(command, "to-the-moon") == 0) {
        money+=100000;
        play_fx_at(FX_MONEY);
        return 1;
    }
    if (stricmp(command, "echo-location") == 0) {
        for (int i = 1; i < mapsize; ++i) {
            if (map_coord[i].flags & MC_NOAUTOMAP) continue;
            map_coord[i].flags |= MC_AUTOMAP;
        }
        play_fx_at(FX_MAP);
        return 1;
    }
    return 0;
}

static void wiz_find_item(const char *name) {
    for (int i = 0; i <item_count; ++i) {
        if (imatch(glob_items[i].jmeno, name)
                || imatch(glob_items[i].popis, name)) {
            wzprintf("i%d %s - %s\n",i, glob_items[i].jmeno, glob_items[i].popis);
        }
    }
}

static void wiz_find_monster(const char *name) {

    alock(H_ENEMY);
    const TMOB *mobs =(TMOB *)ablock(H_ENEMY);
    size_t cnt = get_handle_size(H_ENEMY)/sizeof(TMOB);


    for (size_t i = 0; i <cnt; ++i) {
        if (imatch(mobs[i].name, name)) {
           wzprintf("m%d %s\n", i, mobs[i].name);
        }
    }
}

static int process_with_params(const char *cmd, const char *args) {
    if (stricmp(cmd, "locate") == 0) {
        if (args[0] == 0) return 0;
        wiz_find_item(args);
        wiz_find_monster(args);
        console_add_line("");
        return 1;
    }
    if (stricmp(cmd, "say") == 0) {
        console_add_line(args);
        return 1;
    }
    if (stricmp(cmd, "speed") == 0) {
        long v = strtol(args, NULL, 10);
        if (v > 0) timerspeed_val = v;
        return v > 0;
    }
    return 0;
}

static int process_command(PARSED_COMMAND cmd) {
    if (cmd.command == NULL) {
        console_add_line("");
        return 0;
    }
    int onoff = -1;
    if (cmd.args) {
        if (stricmp(cmd.args, "on") == 0) onoff = 1;
        else if (stricmp(cmd.args, "off") == 0) onoff = 0;
    } else {
        return process_actions(cmd.command);
    }
    if (onoff != -1) {
        return process_on_off_command(cmd.command, onoff);
    } else
        return process_with_params(cmd.command, cmd.args);
    return 0;
}

static void console_keyboard(EVENT_MSG *msg, void **_) {
    if (msg->msg == E_KEYBOARD) {
        int c = va_arg(msg->data, int) & 0xFF;
        if (c == E_QUIT_GAME_KEY) return;
        if (c) {
            int len = strlen(console_input_line);
            if (c == 0x1B) {
                console_show(0);
                 msg->msg = -1;
                 return;
            }
            if (c == '\b') {
                if (len) {
                    console_input_line[len-1] = 0;
                }
                msg->msg = -1;
            } else if (c == '\r') {
                PARSED_COMMAND cmd = parse_command(console_input_line);
                char ok = process_command(cmd);
                if (ok) {
                    console_add_line(console_input_line);
                    console_input_line[0] = 0;
                }
                free(cmd.cmd_buffer);
                msg->msg = -1;
            } else if (c >=32 && len < console_max_characters) {
                console_input_line[len] = c;
                console_input_line[len+1] = 0;
                msg->msg = -1;
            }
        }
    }
}


static void wire_console(void) {
    send_message(E_ADD,E_KEYBOARD, console_keyboard);
}


static void unwire_console(void) {
    send_message(E_DONE,E_KEYBOARD, console_keyboard);
}

void console_show(char show) {
    if (console_visible != show) {
        console_visible = show;
        if (show) wire_console();
        else unwire_console();
    }

}

static void wzprintf(const char *text,...)
  {
    char buff[console_max_characters+1];
    static char wzprint_line[console_max_characters+1];
    va_list lst;
    va_start(lst, text);
    vsnprintf(buff,console_max_characters, text, lst);
    char *c = buff;
    char *d = strchr(c, '\n');
    while (d != NULL) {
        *d = 0;
        strncat(wzprint_line,c, console_max_characters);
        console_add_line(wzprint_line);
        c = d+1;
        d = strchr(c, '\n');
        wzprint_line[0] = 0;
    }
    strncat(wzprint_line, c, console_max_characters);

  }


static void wzputs(const char *text)
  {
    console_add_line(text);
  }





