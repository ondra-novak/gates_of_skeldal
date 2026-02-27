#include "libs/strlite.h"
#include "libs/bgraph.h"
#include "libs/memman.h"
#include "libs/event.h"
#include "libs/mouse.h"
#include "globals.h"

#include "launcher.h"


typedef struct launcher_state {
    TSTR_LIST item_list;
    TSTR_LIST ddl_list;
    TSTR_LIST lang_list;
    int count;
    int selected;
    float offset;
    float selected_anim_cntr;
} TLAUNCHER_STATE;

#define LAUNCHER_STEP 32
#define LAUNCHER_START 200
#define LAUNCHER_PADDING_X 25
#define LAUNCHER_PADDING_Y 25
#define LAUNCHER_SEL_PADDING 3

static void launcher_draw(TLAUNCHER_STATE *st) {
    int minx = LAUNCHER_PADDING_X;
    int maxx = 640-LAUNCHER_PADDING_X;
    int miny = LAUNCHER_PADDING_Y;
    int maxy = 480-LAUNCHER_PADDING_Y;
    int width = maxx - minx;
    put_picture(0,0,ablock(H_LOADING));

    for (int i = 0; i < st->count; ++i) {
        set_font(H_FBIG, (RGB555(31,31,(i == st->selected?0:31))|FONT_TSHADOW));
        int y = (int)(LAUNCHER_START + i * LAUNCHER_STEP - st->offset);
        if (st->selected_anim_cntr) {
            if (i < st->selected) {
                y -= st->selected_anim_cntr;
            } else if (i>st->selected) {
                y += st->selected_anim_cntr;
            }
            st->selected_anim_cntr*=1.02;
            if (st->selected_anim_cntr>500) exit_wait = 1;
        }
        char *txt = st->item_list[i];
        int xs = text_width(txt);
        int ys = text_height(txt);
        if (i == 0) trans_bar(minx, LAUNCHER_START - ys/2-LAUNCHER_SEL_PADDING, width, ys+LAUNCHER_SEL_PADDING*2,0);
        while (xs > width) {
            char *trg = malloc(strlen(txt)+10);
            int d1, d2;
            zalamovani(txt, trg,xs, &d1, &d2);
            strcat(trg,"...");
            str_replace(&st->item_list, i, trg);
            txt = st->item_list[i];
            free(trg);
            xs = text_width(txt);
        }
        if (y >= miny && y < maxy-ys) {
            int x = (640-xs)/2;
            position(x,y-ys/2);
            outtext(txt);
        }
    }
    showview(0,0,0,0);
}

static void redraw_launcher(EVENT_MSG *msg, void **userdata) {
    if (msg->msg == E_INIT) *userdata = va_arg(msg->data, TLAUNCHER_STATE *);
    else if (msg->msg == E_DONE)  *userdata = NULL;
    else {
        TLAUNCHER_STATE *st = (TLAUNCHER_STATE *)*userdata;
        float diff = st->selected * LAUNCHER_STEP - st->offset;
        st->offset += diff/8.0f;
        launcher_draw(st);
    }
}


static void launcher_keyboard(EVENT_MSG *msg, void **userdata) {
    if (msg->msg == E_INIT) *userdata = va_arg(msg->data, TLAUNCHER_STATE *);
    else if (msg->msg == E_DONE)  *userdata = NULL;
    else {
        TLAUNCHER_STATE *st = (TLAUNCHER_STATE *)*userdata;
        if (st->selected_anim_cntr) return;
        int c = quit_request_as_escape(va_arg(msg->data,int));
       switch(c>>8) {
           case 1: exit_wait = 1;break;
           case 17:
           case 'H': if (st->selected>0) st->selected--;break;
           case 31:
           case 'P': if (st->selected<st->count-1) st->selected++;break;
           case 28:
           case 57:
           case 'M':
           case 18: st->selected_anim_cntr = 1; break;
           default:break;
       }
    }
}

static void launcher_mouse(EVENT_MSG *msg, void **userdata) {
    if (msg->msg == E_INIT) *userdata = va_arg(msg->data, TLAUNCHER_STATE *);
    else if (msg->msg == E_DONE)  *userdata = NULL;
    else {
        TLAUNCHER_STATE *st = (TLAUNCHER_STATE *)*userdata;
        if (st->selected_anim_cntr) return;
        const MS_EVENT *ev = va_arg(msg->data, const MS_EVENT *);
        if (ev->tl1) {
            int y = ev->y;
            y -= LAUNCHER_START - st->offset;
            int pos = (y+LAUNCHER_STEP/2) / LAUNCHER_STEP;
            if (pos >=0 && pos < st->count) {
                if (pos == st->selected) st->selected_anim_cntr = 1.0;
                else st->selected = pos;
            }
        }
    }


}


TLAUNCHER_SELECTION *run_launcher() {

    TLAUNCHER_STATE state;
    state.ddl_list = create_list(10);
    state.item_list = create_list(10);
    state.lang_list = create_list(10);
    state.offset = 0;
    state.selected = 0;
    state.count = 2;
    state.selected_anim_cntr = 0;

    str_replace(&state.item_list, 0, "\x80""esky");
    str_replace(&state.item_list, 1, "English");
    str_replace(&state.lang_list, 0,"CZ");
    str_replace(&state.lang_list, 1,"EN");

    for (int i = 0; i < 15; ++i) {
        char text[122];
        snprintf(text, sizeof(text),"Example adventure %d - Ondrej Novak",i);
        str_replace(&state.item_list,i+2, text);
        str_replace(&state.ddl_list,i+2, "EXAMPLE.DDL");
        str_replace(&state.lang_list,i+2, "CZ");
        state.count++;
    }

    send_message(E_ADD, E_TIMER, redraw_launcher, &state);
    send_message(E_ADD, E_KEYBOARD, launcher_keyboard, &state);
    send_message(E_ADD, E_MOUSE, launcher_mouse, &state);
    escape();
    send_message(E_DONE, E_TIMER, redraw_launcher);
    send_message(E_DONE, E_KEYBOARD, launcher_keyboard);
    send_message(E_DONE, E_MOUSE, launcher_mouse);

    TLAUNCHER_SELECTION *retval = NULL;

    if (state.selected_anim_cntr) {

        const char *ddl = state.ddl_list[state.selected];
        const char *lang = state.lang_list[state.selected];
        size_t ddlsz = ddl?strlen(ddl)+1:0;
        size_t langsz = lang?strlen(lang)+1:0;
        size_t need_sz = sizeof(TLAUNCHER_SELECTION) + ddlsz+langsz;
        retval = (TLAUNCHER_SELECTION *)malloc(need_sz);
        char *ddlstor = (char *)retval + sizeof(TLAUNCHER_SELECTION);
        char *langstor = ddlstor + ddlsz;
        if (ddl) {
            memcpy(ddlstor, ddl, ddlsz);
            retval->ddl_file = ddlstor;
        } else {
            retval->ddl_file = NULL;
        }
        if (langsz) {
            retval->lang = langstor;
            memcpy(langstor, lang, langsz);
        } else {
            retval->lang = NULL;
        }





    }

    release_list(state.ddl_list);
    release_list(state.item_list);
    release_list(state.lang_list);

    return retval;


    /*const char *str_label = "START";
      TSTR_LIST lst = create_list(100);
      TSTR_LIST ddl_lst = create_list(100);
      CTL3D ctl = {0,0,4,0};
      int selected = 0;
      size_t item_count = 1;



      UGCManager *ugc = UGC_create();
      size_t ugccount = UGC_Fetch(ugc);;
      for (size_t i = 0; i < ugccount; ++i) {
        UGCItem item = UGC_GetItem(ugc, i);
        char buff[60];
        const char *title = item.name;
        size_t tlen = strlen(title);
        const char *author = item.author;
        size_t alen = strlen(author);
        size_t reserve = sizeof(buff)-4;
        char d1 = 0;
        char d2 = 0;
        if (tlen + alen > reserve) {
          if (alen < reserve/2) {tlen = reserve - alen - 3;d1 = 1;}
          else if (tlen < reserve/2) {alen = reserve - tlen - 3;d2=1;}
          else {
            tlen = reserve/2-3; d1 = 1;
            alen = reserve/2-3; d2 = 1;
          }
        }
        char *iter = buff;
        for (size_t i = 0; i < tlen; ++i) *iter++ = title[i];
        if (d1) for (size_t i = 0; i < 3; ++i) *iter++='.';
        memcpy(iter, " - ",3); iter+=3;
        for (size_t i = 0; i < alen; ++i) *iter++ = author[i];
        if (d2) for (size_t i = 0; i < 3; ++i) *iter++='.';
        *iter = 0;
        str_add(&lst, buff);
        str_add(&ddl_lst, item.ddl_path);
        ++item_count;
      }

      str_add(&lst, "Czech");
      str_add(&lst, "English");
      str_add(&ddl_lst, "CZ.DDL");
      str_add(&ddl_lst, "EN.DDL");
      item_count+=2;



      curcolor = RGB555(0,0,0);
      set_font(H_FBIG,RGB555_ALPHA(31,31,31));
      add_window(120,60,400,300,H_WINTXTR,3,20,20);
      define(-1,20,10,1,1,0,label,str_label);
      set_font(H_FKNIHA,RGB555_ALPHA(31,31,31));
      define(9,15,38,335,212,0,&listbox,lst,RGB555(16,16,16),0);
      property(&ctl,NULL,NULL,RGB555(0,0,0));c_default(0);
      if (item_count>19) {
        define(10,355,38,20,212,0,scroll_bar_v,0,item_count-19,19,RGB555(8,8,8));
        property(&ctl,NULL,NULL,RGB555(10,10,10));
      }
      define(20,20,20,60,20,2,button,"Cancel");property(def_border(5,BAR_COLOR),NULL,NULL,BAR_COLOR);on_control_change(terminate_gui);
      define(30,90,20,60,20,2,button,"Ok");property(def_border(5,BAR_COLOR),NULL,NULL,BAR_COLOR);on_control_change(terminate_gui);
      redraw_window();
      send_message(E_ADD,E_KEYBOARD,save_dialog_keyboards);
      send_message(E_ADD,E_MOUSE,save_dialog_keyboards);
      escape();
      send_message(E_DONE,E_KEYBOARD,save_dialog_keyboards);
      send_message(E_DONE,E_MOUSE,save_dialog_keyboards);
      int butt = o_aktual->id;
      get_value(0,9,&selected);
      char *selddl = strdup(ddl_lst[selected]);
      release_list(lst);
      release_list(ddl_lst);
      close_current();
      if (butt != 30 || selddl == NULL || selddl[0] == 0) {
        free(selddl);
        UGC_Destroy(ugc);
        return butt != 30?NULL:"";
      }
      launcher_ddl_file = selddl;
      UGC_StartPlay(ugc, selected);
      UGC_Destroy(ugc);
      atexit(&free_ddl_file_name);
      return launcher_ddl_file;
      */
    return NULL;
}
