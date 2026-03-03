#include "libs/bgraph.h"
#include "libs/memman.h"
#include "libs/event.h"
#include "libs/mouse.h"
#include "globals.h"
#include "libs/vector.h"
#include "platform/ugc.h"

#include "launcher.h"


typedef struct {
    char *label;
    char *ddl_path;
    char *lang;    
} TLAUNCHER_ITEM;

static void create_launcher_item(const UGCItem *ugc, TLAUNCHER_ITEM *item) {
    size_t label_need = strlen(ugc->name) + (ugc->author?strlen(ugc->author):0) + 4; // 4 for " ()"
    size_t needsz = label_need+ strlen(ugc->lang)+(ugc->ddl_path?strlen(ugc->ddl_path):0)+2;
    char *buff = malloc(needsz);
    buff[needsz] = 0;
    if (ugc->author) snprintf(buff, needsz, "%s (%s)", ugc->name, ugc->author);
    else strcpy(buff, ugc->name);
    item->label = buff;

    char *end = strchr(buff,0)+1;
    item->lang = end;
    strcpy(end, ugc->lang);

    if (ugc->ddl_path) {
        end = strchr(end, 0)+1;
        item->ddl_path = end;
        strcpy(end, ugc->ddl_path);
    } else {
        item->ddl_path = NULL;
    }
}

static UGCItem initial_items[] = {
    (UGCItem){"\x80""esky", NULL, NULL, "CS"},
    (UGCItem){"English", NULL,NULL, "EN"}
};

static void destroy_launcher_item(void *item) {
    free(((TLAUNCHER_ITEM*)item)->label);
}


typedef struct launcher_state {

    Vector items;
    int selected;
    float offset;
    float selected_anim_cntr;
    void *picture;
} TLAUNCHER_STATE;

#define LAUNCHER_STEP 16
#define LAUNCHER_START 240
#define LAUNCHER_PADDING_X 25
#define LAUNCHER_PADDING_Y 25
#define LAUNCHER_SEL_PADDING 3


static void launcher_draw(TLAUNCHER_STATE *st) {
    int minx = LAUNCHER_PADDING_X;
    int maxx = 640-LAUNCHER_PADDING_X;
    int miny = LAUNCHER_PADDING_Y;
    int maxy = 480-LAUNCHER_PADDING_Y;
    int width = maxx - minx;

    put_picture(0,0,st->picture);
    

    int count = (int)vector_size(&st->items);

    for (int i = 0; i < count; ++i) {
        set_font(H_FONT6, (RGB555(31,31,(i == st->selected?0:31))|FONT_TSHADOW));
        int y = (int)(LAUNCHER_START + i * LAUNCHER_STEP - st->offset);
        if (st->selected_anim_cntr) {
            if (i < st->selected) {
                y -= st->selected_anim_cntr;
            } else if (i>st->selected) {
                y += st->selected_anim_cntr;
            }
            st->selected_anim_cntr*=1.15;
            if (st->selected_anim_cntr>500) exit_wait = 1;
        }
        TLAUNCHER_ITEM *item = vector_get(&st->items, i);
        char *txt = item->label ;
        int xs = text_width(txt);
        int ys = text_height(txt);
//        if (i == 0) trans_bar(minx, LAUNCHER_START - ys/2-LAUNCHER_SEL_PADDING, width, ys+LAUNCHER_SEL_PADDING*2,0);
        while (xs > width) {
            char *trg = malloc(strlen(txt)+10);
            int d1, d2;
            zalamovani(txt, trg,xs, &d1, &d2); //create break in text, so it fits into width
            size_t orig_len = strlen(txt);
            size_t new_len = strlen(trg);
            if (new_len +3 < orig_len) { //result must fit into original buffer
                txt[new_len] = 0;   //shorten original string
                strcat(txt,"..."); //add ellipsis
            }
            xs = text_width(txt);
        }
        if (y >= miny && y < maxy-ys) {
            int x = LAUNCHER_PADDING_X;
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
           case 'P': if (st->selected<(int)vector_size(&st->items)-1) st->selected++;break;
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
            if (pos >=0 && pos < (int)vector_size(&st->items)) {
                if (pos == st->selected) st->selected_anim_cntr = 1.0;
                else st->selected = pos;
            }
        }
    }


}

static void get_list_callback(const UGCItem *items, unsigned int count, void *context) {
    TLAUNCHER_STATE *st = (TLAUNCHER_STATE *)context;
    for (unsigned int i = 0; i < count; ++i) {
        TLAUNCHER_ITEM item;
        create_launcher_item(&items[i], &item);
        vector_push_back(&st->items, &item);
    }
}

static void *create_background() {
    word *w = (word *)ablock(H_LOADING);
    size_t pixels = 640*480;
    word *buffer = (word *)malloc(sizeof(word)*(3+pixels));
    buffer[0] = 640;
    buffer[1] = 480;
    buffer[2] = 15;
    put_picture_ex(0, 0, w, buffer+3, 640, 480);
    for (size_t i = 0; i < pixels; ++i) {
        word px = buffer[i+3];
        int r = (px >> 10) & 0x1F;
        int g = (px >> 5) & 0x1F;
        int b = px & 0x1F;
        int gray = MIN((299*r+587*g+114*b)/1500,31);
        buffer[i+3] = RGB555(gray,gray,gray>>1);

    }
    return buffer;
    
}

TLAUNCHER_SELECTION *run_launcher() {

    const char *path =  build_pathname(2, gpathtable[SR_SAVES], "UGC");
    const char *user_ugc = local_strdup(path);
    const char *dlc_path = "./DLC";

    TLAUNCHER_STATE state = {0};
    vector_init(&state.items, sizeof(TLAUNCHER_ITEM), destroy_launcher_item);
    state.offset = 0;
    state.selected = 0;
    state.selected_anim_cntr = 0;
    state.picture = create_background();

    TLAUNCHER_ITEM item;
    create_launcher_item(&initial_items[0], &item);
    vector_push_back(&state.items, &item);
    create_launcher_item(&initial_items[1], &item);
    vector_push_back(&state.items, &item);

    UGC_GetList(user_ugc, dlc_path, get_list_callback, &state);

    send_message(E_ADD, E_TIMER, redraw_launcher, &state);
    send_message(E_ADD, E_KEYBOARD, launcher_keyboard, &state);
    send_message(E_ADD, E_MOUSE, launcher_mouse, &state);
    escape();
    send_message(E_DONE, E_TIMER, redraw_launcher);
    send_message(E_DONE, E_KEYBOARD, launcher_keyboard);
    send_message(E_DONE, E_MOUSE, launcher_mouse);

    TLAUNCHER_SELECTION *retval = NULL;

    if (state.selected_anim_cntr) {

        TLAUNCHER_ITEM *selected_item = vector_get(&state.items, state.selected);
        const char *ddl = selected_item->ddl_path;
        const char *lang = selected_item->lang;
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

    ablock_free(state.picture);
    vector_destroy(&state.items);

    return retval;
}
