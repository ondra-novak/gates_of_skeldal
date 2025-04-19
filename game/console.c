#include <libs/bgraph.h>
#include <libs/event.h>
#include <platform/achievements.h>
#include "globals.h"
#include "../platform/platform.h"

#include <ctype.h>
#define console_max_characters  120
#define console_max_lines  300

void macro_drop_item(int sector,int smer,short item);
/****/


void wzprintf(const char *text,...) __attribute__((format(printf, 1, 2)));
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




static char display_game_status(void)
  {
  short *v;
  THUMAN *p;
  TSTENA *s;
  TSECTOR *ss;
 int i,cn,astr;

  wzprintf("Sector: %5d  Dir: %d  Group %d \n",viewsector,viewdir,cur_group);
  for(i=0,p=postavy;i<POCET_POSTAV;i++,p++) {
     if (p->used) {
        wzprintf("%d.%-14s (%d) Sek:%5d Smr:%d HPReg:%d MPReg:%d VPReg:%d %04X%s\n",i+1,p->jmeno,p->groupnum,p->sektor,p->direction,p->vlastnosti[VLS_HPREG],
              p->vlastnosti[VLS_MPREG], p->vlastnosti[VLS_VPREG], p->vlastnosti[VLS_KOUZLA], p->lives?"":"(dead)");
      }
  }
  wzprintf("Held items: ");
  v=picked_item;
  if (v==NULL) wzprintf("<none>\n");else while(*v) wzprintf("%d \n",abs(*v++)-1);
  for(i=0,cn=0,astr=0;i<MAX_MOBS;i++)
     {
     if (mobs[i].vlajky & MOB_LIVE) cn++;
     if (mobs[i].vlajky & MOB_MOBILE) astr++;
     }
  wzprintf("Total monsters:  %5d (+%d astral)\n"
           "Total items: %5d (+%d clones)\n",cn-astr,astr,item_count,item_count-it_count_orgn);

  ss=map_sectors+viewsector;
  s=map_sides+viewsector*4+viewdir;
  wzprintf("Sector: (%d) Floor %d Ceil %d Action target %d Action side %d Action ID %d\r\n",
         ss->sector_type, ss->floor,ss->ceil,ss->sector_tag,ss->side_tag,ss->action);
  wzprintf("        Exits: North %d East %d South %d West %d\n",ss->step_next[0],ss->step_next[1],ss->step_next[2],ss->step_next[3]);
  wzprintf("        FLAGS: %02X %02X ",ss->flags,map_coord[viewsector].flags);
  show_flags(map_coord[viewsector].flags,mc_flags,12);
  wzprintf("\nSide: Prim %d Sec %d Arc %d Anim_prim %d/%d Anim_sec %d/%d\n",
         s->prim,s->sec,s->oblouk & 0xf,s->prim_anim>>4,s->prim_anim & 0xf,s->sec_anim>>4,s->sec_anim & 0xf);
  wzprintf("       Action target %d Action side %d Action %d\n",s->action,s->sector_tag,s->side_tag & 0x3);
  wzprintf("       Multiaction: %s\n",macros[viewsector*4+viewdir].action_list==NULL?"No":"Yes");
  wzprintf("       Flags: %04X %02X %02X\n",s->flags,s->oblouk>>4,s->side_tag>>2);
  show_flags(s->flags,side_flags,32);
  show_flags(s->oblouk>>4,obl_flags,4);
  wzprintf("\n");
  return 0;
  }

/*****/

void unaffect();
extern char immortality;
extern char nohassle;
extern char dead_food;
extern char pass_all_mobs;


static char console_input_line[console_max_characters+1]  = "";
static char *console_output_lines[console_max_lines] = {};
static int console_top_line = 0;
static const char *console_command = NULL;
static int copied_text = 0;

static const int console_x = 0;
static const int console_y = SCREEN_OFFLINE;
static const int console_width = 640;
static const int console_height = 165;
static const int console_padding = 3;
static int console_blink = 0;
static char console_visible = 0;
#define CONSOLE_FONT H_FLITT5

void draw_console_window() {
    if (!console_visible) return;
    trans_bar(console_x, console_y, console_width, console_height, 0);

    set_font(CONSOLE_FONT, RGB555(31,31,16));
    int y = console_y+console_height-text_height("X")-console_padding;
    position(console_x+console_padding, y);
    outtext("$ ");
    outtext(console_input_line);
    if ((console_blink>>1) & 1) {
        outtext("_");
    }
    ++console_blink;

    set_font(CONSOLE_FONT, RGB555(31,31,31));


    y-=3*text_height("X")/2;

    for (int i = 0; i < console_max_lines;++i) {
        int p = i + console_top_line;
        if (p>=console_max_lines) break;
        position(console_x+console_padding,y);
        if (console_output_lines[p]) outtext(console_output_lines[p]);
        y-=text_height("X");
        if (y < console_y+console_padding) break;
    }
    if (copied_text > 0) {
        const char *c = "Copied";
        set_aligned_position(console_x+console_width, console_y, 2, 0, c);
        outtext(c);
        --copied_text;
    } else if (get_control_key_state()) {
        const char *c = "Press CTRL+C to copy content";
        set_aligned_position(console_x+console_width, console_y, 2, 0, c);
        outtext(c);
    }
}

char console_is_visible() {
    return console_visible;
}

static void console_add_line(const char *line);

static void flush_console_command() {
    if (console_command) {
        const char *cmd = concat2("> ", console_command);
        console_command = NULL;
        console_add_line(cmd);        
    }
}

static void console_add_line_s(const char *line, size_t sz) {
    flush_console_command();
    free(console_output_lines[console_max_lines-1]);
    memmove(console_output_lines+1,console_output_lines, (console_max_lines-1)*sizeof(char *));
    console_output_lines[0] = malloc(sz+1);
    memcpy(console_output_lines[0], line, sz);
    console_output_lines[0][sz] = 0;
}

static void console_add_line(const char *line) {
    console_add_line_s(line, strlen(line));
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

static int add_file_to_console(const char *name, LIST_FILE_TYPE _, size_t __, void *ctx) {
    int *cnt = (void *)ctx;
    char buff[20] = "";
    for (int i = 0; i < 19; ++i) buff[i] = ' ';
    buff[19] = 0;
    int l = strlen(name);
    if (l > 3 && istrcmp(name+l-4,".MAP") == 0) {
        if (l>19) l = 19;
        memcpy(buff, name, l);
        wzprintf("%s", buff);
        if (++(*cnt) == 6) {
            wzprintf("\n");
            *cnt = 0;
        }
    }
    return 0;

}

static int process_on_off_command(const char *cmd, char on) {
    if (istrcmp(cmd, "inner-eye") == 0) {
        log_combat = on;
        return 1;
    }
    if (istrcmp(cmd, "hunter-instinct") == 0) {
        show_lives = on;
        return 1;
    }
    if (istrcmp(cmd, "no-hassle") == 0) {
        nohassle=on;
        return 1;
    }
    if (istrcmp(cmd, "iron-skin") == 0) {
        immortality=on;
        return 1;
    }
    if (istrcmp(cmd, "spirit-wander") == 0) {
        cur_group=on?10:postavy[0].groupnum;
        return 1;
    }
    if (istrcmp(cmd, "ghost-walls") == 0) {
        ghost_walls = on;
        return 1;
    }
    if (istrcmp(cmd, "true-seeing") == 0) {
        true_seeing = on;
        return 1;
    }
    if (istrcmp(cmd, "walking-in-air") == 0) {
        nofloors = on;
        return 1;
    }
    if (istrcmp(cmd, "enemy-insight") == 0) {
        show_mob_info = on;
        return 1;
    }
    if (istrcmp(cmd, "enemy-walk-diagonal") == 0) {
        game_extras = on?(game_extras | EX_WALKDIAGONAL):(game_extras & ~EX_WALKDIAGONAL);
        return 1;
    }
    if (istrcmp(cmd, "ghost-form") == 0) {
        pass_all_mobs = on;
        return 1;
    }
    if (istrcmp(cmd, "dead-food") == 0) {
        dead_food = on;
        return 1;
    }
    if (istrcmp(cmd, "trace-dialogs") == 0) {
        trace_dialogs = on;
        return 1;
    }
    return 0;
}

void postavy_teleport_effect(int sector,int dir,int postava,char effect);

static int command_ls_callback(const char*name, void* ctx) {
    wzprintf("%s\n", name);
    return 0;
}

static int process_actions(const char *command) {
    if (istrcmp(command, "flute") == 0) {
        bott_draw_fletna();
        return 1;
    }
    if (istrcmp(command, "global-map") == 0) {
        wire_global_map();
        return 1;
    }
    if (istrcmp(command, "status") == 0) {
        display_game_status();
        return 1;
    }
    if (istrcmp(command, "offlers-blessing") == 0) {
        money=150000;
        play_fx_at(FX_MONEY);
        return 1;
    }
    if (istrcmp(command, "i-require-gold") == 0) {
        money+=1;
        play_fx_at(FX_MONEY);
        return 1;
    }
    if (istrcmp(command, "to-the-moon") == 0) {
        money+=100000;
        play_fx_at(FX_MONEY);
        return 1;
    }
    if (istrcmp(command, "echo-location") == 0) {
        for (int i = 1; i < mapsize; ++i) {
            map_coord[i].flags |= MC_AUTOMAP;
        }
        play_fx_at(FX_MAP);
        return 1;
    }
    if (istrcmp(command, "rise-and-shine") == 0) {
        int r = 0;
        for (int i = 0; i < POCET_POSTAV; ++i) {
            THUMAN *p = postavy+i;
            if (p->used && p->inmaphash == current_map_hash) {
                p->lives=p->vlastnosti[VLS_MAXHIT];
                p->mana=p->vlastnosti[VLS_MAXMANA];
                p->kondice=p->vlastnosti[VLS_KONDIC];
                p->sektor = viewsector;
                r = 1;
            }
        }
        bott_draw(0);
        return r;

    }
    if (istrcmp(command, "save") == 0) {
        do_autosave();
        return 1;
    }
    if (istrcmp(command, "ascent") == 0) {
        int lev = postavy[0].level;
        for (int i = 0; i < POCET_POSTAV; ++i) {
            THUMAN *p = postavy+i;
            if (p->used) lev = MAX(lev,p->level);
        }
        for (int i = 0; i < POCET_POSTAV; ++i) {
            THUMAN *p = postavy+i;
            if (p->used) {
                p->exp = level_map[lev-1];
                check_player_new_level(p);
            }
        }
        return 1;
    }
    if (istrcmp(command, "by-the-power-of-grayskull") == 0) {
        memset(runes,0xFF, sizeof(runes));
        play_fx_at(FX_MAGIC);
        return 1;
    }
    if (istrcmp(command, "beam-me-up") == 0) {
        for (int i = 1; i < mapsize; ++i) {
            map_coord[i].flags |= MC_AUTOMAP;
        }
        play_fx_at(FX_MAP);
        int kdo = 0;
        THUMAN *h = postavy;
        for(int i=0;i<POCET_POSTAV;i++,h++) {
           if (postavy[i].used && postavy[i].sektor == viewsector) kdo|=1<<i;
        }
        int sektor = select_teleport_target(1);
        if (sektor) {
            postavy_teleport_effect(sektor, viewdir, kdo, 1);
            return 1;
        } else {
            unwire_proc();
            wire_proc();
        }
        return 0;

    }
    if (istrcmp(command, "dispel-magic") == 0) {
         unaffect();
         return 1;
    }
    if (istrcmp(command, "echoes-of-the-past") == 0) {
        const char *lname = local_strdup(level_fname);
        leave_current_map();
        temp_storage_delete(lname);
        load_map(lname);
        return 1;
    }
    if (istrcmp(command, "steam") == 0) {
        if (is_steam_available()) {
            char *c = get_steam_status();;
            wzputs(c);
            free(c);
        } else {
            wzputs("N/A");
        }
        return 1;
    }
    if (istrcmp(command,"help") == 0)  {
        wzputs("Help has left the chat. Try the forums, brave wanderer. Set your inner-eye on");
        return 1;
    }
    if (istrcmp(command, "ls") == 0)  {
        temp_storage_list(command_ls_callback,NULL);
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
           wzprintf("m%d %s\n", (int)i, mobs[i].name);
        }
    }
}




static int process_with_params(const char *cmd, const char *args) {
    if (istrcmp(cmd, "locate") == 0) {
        if (args[0] == 0) return 0;
        wiz_find_item(args);
        wiz_find_monster(args);
        console_add_line("");
        return 1;
    }
    if (istrcmp(cmd, "summon") == 0) {
        if (args[0] == 'i') {
            char *end;
            unsigned long  id = strtoul(args+1, &end, 10);
            if (*end == 0) {
                if (id < (unsigned long)item_count) {
                    macro_drop_item(viewsector,viewdir,id);
                    return 1;
                }
            }
        } if (args[0] == 'm') {
            char *end;
            unsigned long  id = strtoul(args+1, &end, 10);
            if (*end == 0) {
                ablock(H_ENEMY);
                size_t cnt = get_handle_size(H_ENEMY)/sizeof(TMOB);
                if (id < cnt) {
                    int choosen_id = -1;
                    for (int i = 0; i < MAX_MOBS; ++i) {
                        if (mobs[i].vlajky & MOB_LIVE) continue;
                        choosen_id = i;
                    }
                    if (choosen_id >= 0) {
                        const TMOB *t =(TMOB *)ablock(H_ENEMY);
                        int sect = map_sectors[viewsector].step_next[viewdir];
                        if (sect) {
                            load_enemy_to_map(choosen_id, sect, (viewdir+2) & 3, t+id);
                            return 1;
                        }
                    }
                }
            }
            return 0;
        }

        return 0;
    }
    if (istrcmp(cmd, "say") == 0) {
        console_add_line(args);
        return 1;
    }
    if (istrcmp(cmd, "speed") == 0) {
        long v = strtol(args, NULL, 10);
        if (v > 0) timerspeed_val = v;
        return v > 0;
    }
    if (istrcmp(cmd, "portal-jump") == 0) {
        if (check_file_exists(build_pathname(2,gpathtable[SR_MAP], args))) {
            TMA_LOADLEV lev;
            strcopy_n(lev.name,args,sizeof(lev.name));
            lev.start_pos = 0;
            macro_load_another_map(&lev);
            return 1;
        }
        return 0;
    }

    if (istrcmp(cmd, "unachieve") == 0) {
        return !clear_achievement(args);
    }
    if (istrcmp(cmd, "talk") == 0) {
        if (args[0] == 0) return 0;
        int id;
        char pgf = 0;
        if (args[0] == '/') {
            pgf = 1;
            args++;
        }
        id = atoi(args);
        if (!pgf) id = id * 128;
        if (dialog_is_paragraph(id)) {
            call_dialog(id, -1);
            return 1;
        }
    }
    if (istrcmp(cmd, "unvisit") == 0) {
        if (args[0] == 0) return 0;
        int id = atoi(args);
        return dialog_set_notvisited(id);        
    }

    if (istrcmp(cmd, "cat") == 0) {
        int32_t sz =temp_storage_find(args);
        if (sz < 0) return 0;
        char *data = (char *)malloc(sz+1);
        temp_storage_retrieve(args,data,sz);
        data[sz] = 0;
        char *data2 = (char *)malloc(sz+10);
        int xs, ys;
        zalamovani(data, data2,600, &xs, &ys);
        char *iter = data2;
        while (iter < data2+sz) {
            wzputs(iter);
            iter = iter + strlen(iter)+1;
        }
        free(data);
        free(data2);
        return 1;
    }

    if (istrcmp(cmd, "hex") == 0) {
        int32_t sz =temp_storage_find(args);
        if (sz < 0) return 0;
        uint8_t *data = (uint8_t *)malloc(sz);
        temp_storage_retrieve(args,data,sz);
        char *data2 = (char *)malloc(sz*3+1);
        char *iter = data2;
        for (int32_t i = 0; i < sz; i++) {
            snprintf(iter,4, "%02X ", (int)data[i]);
            iter = iter+3;
        }
        char *data3 = (char *)malloc(sz*3+10);
        int xs, ys;
        zalamovani(data2, data3,600, &xs, &ys);
        iter = data3;
        while (*iter) {
            wzputs(iter);
            iter = iter + strlen(iter)+1;
        }
        free(data3);
        free(data2);
        free(data);
        return 1;
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
        if (istrcmp(cmd.args, "on") == 0) onoff = 1;
        else if (istrcmp(cmd.args, "off") == 0) onoff = 0;
    } else {
        return process_actions(cmd.command);
    }
    if (onoff != -1) {
        return process_on_off_command(cmd.command, onoff);
    } else
        return process_with_params(cmd.command, cmd.args);
    return 0;
}

static void console_copy_to_clipboard() {
    size_t needsz = 0;
    for (unsigned int i = 0; i < console_max_lines; ++i) {
        if (console_output_lines[i]) needsz = needsz + strlen(console_output_lines[i]) + 2;        
    }
    needsz+=1;
    char *text = (char *)malloc(needsz);
    char *iter = text;
    for (unsigned int i = console_max_lines; i > 0; ) {
        --i;
        if (console_output_lines[i]) {
            size_t len = strlen(console_output_lines[i]);
            memcpy(iter, console_output_lines[i], len);
            iter+=len;
            *iter = '\r';
            ++iter;
            *iter = '\n';
            ++iter;            
        }
    }
    *iter = 0;
    if (copy_text_to_clipboard(text)) {
        copied_text = 20;
    }
    free(text);
} 

static void console_keyboard(EVENT_MSG *msg, void **_) {
    if (msg->msg == E_KEYBOARD) {
        int code = va_arg(msg->data, int);
        int c = code & 0xFF;
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
                console_command = console_input_line;
                PARSED_COMMAND cmd = parse_command(console_input_line);
                char ok = process_command(cmd);
                if (ok) {
                    flush_console_command();
                    console_top_line = 0;
                    console_input_line[0] = 0;
                }
                console_command = NULL;
                free(cmd.cmd_buffer);
                msg->msg = -1;
            } else if (c == 3 && !get_shift_key_state()) {
                console_copy_to_clipboard();                
            } else if (c >=32 && len < console_max_characters) {
                console_input_line[len] = c;
                console_input_line[len+1] = 0;
                msg->msg = -1;
            }
        } else {
            switch (code >> 8) {
                case 'I': console_top_line = MIN(console_max_lines-10, console_top_line+10);break;
                case 'Q': console_top_line = MAX(0, console_top_line-10);break;
                case 'H': console_top_line = MIN(console_max_lines-10, console_top_line+1);break;
                case 'P': console_top_line = MAX(0, console_top_line-1);break;
                case 'G': console_top_line = console_max_characters-10;break;
                case 'O': console_top_line = 0;break;
                default: return;
            }
            msg->msg = -1;
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


void wzprintf(const char *text,...)
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
        wzprint_line[console_max_characters] = 0;
        console_add_line(wzprint_line);
        c = d+1;
        d = strchr(c, '\n');
        wzprint_line[0] = 0;
    }
    strncat(wzprint_line, c, console_max_characters);
    wzprint_line[console_max_characters] = 0;

  }


static void wzputs(const char *text)
  {
    const char *sep = strchr(text, '\n');
    while (sep != NULL) {
        console_add_line_s(text, sep-text);
        text = sep+1;
        sep = strchr(text, '\n');   
    }
    if (text[0] != 0) console_add_line_s(text, strlen(text));
  }





