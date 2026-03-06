#include "game/gamesave.h"
#include "libs/bgraph.h"
#include "libs/memman.h"
#include "libs/event.h"
#include "libs/mouse.h"
#include "globals.h"
#include "libs/vector.h"
#include "platform/ugc.h"


#include <stdio.h>

#include "launcher.h"

typedef TCONTINUE_GAME_INFO TLAUNCHER_ITEM;


static TLAUNCHER_ITEM *create_launcher_item_from_ugc(const UGCItem *ugc) {
    if (ugc->author) {
        size_t label_need = strlen(ugc->name) + strlen(ugc->author) + 4; // 4 for " ()"
        char *buff = (char *)alloca(label_need);
        snprintf(buff, label_need, "%s (%s)", ugc->name, ugc->author);
        return make_load_continue_info(ugc->ddl_path,ugc->lang, NULL, buff,ugc->id);
    }
    return make_load_continue_info(ugc->ddl_path,ugc->lang, NULL, ugc->name,ugc->id);
}

static char check_valid_item(TLAUNCHER_ITEM *itm){
    if (!itm) return 0;
    if (!itm->ddl) return 1;
    FILE *f = fopen(itm->ddl,"r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}



static void destroy_launcher_item(void *item) {
    TLAUNCHER_ITEM *litm = *(TLAUNCHER_ITEM **)item;
    free(litm);
}

typedef struct launcher_state {

    Vector items;
    int selected;
    int top_line;
    float offset;
    float selected_anim_cntr;
    const void *picture;
    void *preview_image;
    int preview_image_index;
    size_t steam_update_counter;
    char selection_done;
    char update_lock;
} TLAUNCHER_STATE;

#define LAUNCHER_STEP 16
#define LAUNCHER_PADDING_X 5
#define LAUNCHER_PADDING_Y 10
#define LAUNCHER_SEL_PADDING 3
#define LAUNCHER_TITLE_SPACE 5
#define LAUNCHER_X 20
#define LAUNCHER_Y 100
#define LAUNCHER_WIDTH 300
#define LAUNCHER_HEIGHT 250
#define LAUNCHER_START LAUNCHER_Y+5
#define SECTION_OFFSET 10

#define PREVIEW_IMAGE_X 480
#define PREVIEW_IMAGE_Y 100

#define SECTION_COLOR RGB555(20,20,30)
#define TEXT_COLOR RGB555(25,25,20)
#define SELECTED_COLOR RGB555(31,31,31)
#define SECTION_RECT_COLOR RGB555(10,10,10)


static const char *sections_names[] = {
    "Br\xA0ny Skeldalu",
    "Adventures"
};

static void on_list_update(TLAUNCHER_STATE *st);
static void navigate_up(TLAUNCHER_STATE *st);
static void navigate_down(TLAUNCHER_STATE *st);

static void launcher_draw(TLAUNCHER_STATE *st) {
    int minx = LAUNCHER_X;
    int maxx = LAUNCHER_X+LAUNCHER_WIDTH;
    int miny = LAUNCHER_Y;
    int maxy = LAUNCHER_Y+LAUNCHER_HEIGHT;
    int width = maxx - minx;
    int sect_idx = 0;


    put_picture(0,0,st->picture);

    trans_bar(LAUNCHER_X-LAUNCHER_PADDING_X, LAUNCHER_Y-LAUNCHER_PADDING_Y, LAUNCHER_WIDTH+2*LAUNCHER_PADDING_X, LAUNCHER_HEIGHT+2*LAUNCHER_PADDING_X, 0);

    int count = (int)vector_size(&st->items);

    if (st->selected_anim_cntr) {
            st->selected_anim_cntr*=2;
            if (st->selected_anim_cntr>500) exit_wait = 1;
    }


    for (int i = 0; i < count; ++i) {
        int y = (int)(LAUNCHER_START + i * LAUNCHER_STEP - st->offset);
        if (st->selected_anim_cntr) {
            if (i < st->selected) {
                y -= st->selected_anim_cntr;
            } else if (i>st->selected) {
                y += st->selected_anim_cntr;
            }
        }
        TLAUNCHER_ITEM *item = *(TLAUNCHER_ITEM **)vector_get(&st->items, i);
        char *txt;
        if (item) txt = (char *)item->label ;
        else txt = (char *)sections_names[sect_idx++];
        int x;

        if (item) {
            x = LAUNCHER_X;
            set_font(H_FONT6, i== st->selected?NOSHADOW(SELECTED_COLOR):NOSHADOW(TEXT_COLOR));
        } else {
            x = LAUNCHER_X+SECTION_OFFSET;
            set_font(H_FLITT5, NOSHADOW(SECTION_COLOR));

        }


        int xs = text_width(txt);
        int ys = text_height(txt);
        if (!item) {
            int textb = LAUNCHER_X+SECTION_OFFSET;
            int texte = LAUNCHER_X+SECTION_OFFSET+xs;
            int right = LAUNCHER_X+LAUNCHER_WIDTH;
            trans_line_x(LAUNCHER_X , y, textb - LAUNCHER_X-LAUNCHER_TITLE_SPACE , SECTION_COLOR);
            trans_line_x(texte+LAUNCHER_TITLE_SPACE , y, right-texte-LAUNCHER_TITLE_SPACE, SECTION_COLOR);
        }
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
            if (st->selected == i) {
                curcolor = 0;
                bar32(x-LAUNCHER_SEL_PADDING, y-ys/2-LAUNCHER_SEL_PADDING,x+LAUNCHER_WIDTH+LAUNCHER_SEL_PADDING,y+ys/2+LAUNCHER_SEL_PADDING);
                rectangle(x-LAUNCHER_SEL_PADDING, y-ys/2-LAUNCHER_SEL_PADDING,x+LAUNCHER_WIDTH+LAUNCHER_SEL_PADDING,y+ys/2+LAUNCHER_SEL_PADDING,SECTION_RECT_COLOR);
            }
            position(x,y-ys/2);
            outtext(txt);
        }
        if (i == st->selected) {
            if (y < miny && st->top_line>0) {
                st->top_line--;
            } else if (y > maxy-ys) {
                st->top_line++;
            }
        }
    }
    if (st->preview_image) {
        put_picture(PREVIEW_IMAGE_X-*(word *)st->preview_image/2, PREVIEW_IMAGE_Y, st->preview_image);
    }
    showview(0,0,0,0);
}

static void *attempt_load_preview(const char *ddl) {
    if (!ddl) return NULL;
    size_t len = strlen(ddl);
    if (len < 11) return NULL;
    char *cpy = malloc(strlen(ddl)+20);
    strcpy(cpy, ddl);
    strcpy(cpy+len-11, "preview.hi");

    FILE *f = fopen_icase(cpy, "rb");
    if (!f) {
        free(cpy);
        return NULL;
    }
    fseek(f,0,SEEK_END);
    long size =ftell(f);
    fseek(f, 0, SEEK_SET);
    void *content = getmem(size);
    fread(content,1,size,f);
    fclose(f);
    free(cpy);
    return content;

}


static void get_list_callback(const UGCItem *items, unsigned int count, void *context) {
    TLAUNCHER_STATE *st = (TLAUNCHER_STATE *)context;
    TLAUNCHER_ITEM *item = NULL;
    vector_push_back(&st->items, &item);
    for (unsigned int i = 0; i < count; ++i) {
        item = create_launcher_item_from_ugc(&items[i]);
        vector_push_back(&st->items, &item);
    }
    if (is_steam_workshop_browser_available()) {
        item = make_load_continue_info(NULL,NULL,NULL,"... workshop ...",1);
        vector_push_back(&st->items, &item);
    }
    st->update_lock = 0;
}

static void adjust_selected_position(TLAUNCHER_STATE *st) {
    if ((size_t)st->selected >= vector_size(&st->items)) {
        st->selected = 0;
    }
    if (*(TLAUNCHER_ITEM **)vector_get(&st->items,st->selected) == NULL) {
        navigate_up(st);
    }

}

static void populate_launcher_static(TLAUNCHER_STATE *st) {
    TLAUNCHER_ITEM *item;
    item = get_load_continue_info();
    if (item && check_valid_item(item)) {
        vector_push_back(&st->items, &item);
    }
    item = NULL;
    vector_push_back(&st->items, &item);
    item = make_load_continue_info(NULL, "CS", NULL, "\x80""esky",0);
    vector_push_back(&st->items, &item);
    item = make_load_continue_info(NULL, "EN", NULL, "English",0);
    vector_push_back(&st->items, &item);

}


static void get_list_callback_update(const UGCItem *items, unsigned int count, void *context) {
    TLAUNCHER_STATE *st = (TLAUNCHER_STATE *)context;
    vector_remove(&st->items,0,vector_size(&st->items));
    populate_launcher_static(st);
    get_list_callback(items, count, context);
    adjust_selected_position(st);
}


static void  request_for_ugc(void (*callback)(const UGCItem *items, unsigned int count, void *context), TLAUNCHER_STATE *st) {
    const char *path =  build_pathname(2, gpathtable[SR_SAVES], "UGC");
    const char *user_ugc = local_strdup(path);
    const char *dlc_path = "./DLC";

    st->update_lock =1;
    UGC_GetList(user_ugc, dlc_path, callback, st);
}


static void redraw_launcher(EVENT_MSG *msg, void **userdata) {
    if (msg->msg == E_INIT) *userdata = va_arg(msg->data, TLAUNCHER_STATE *);
    else if (msg->msg == E_DONE)  *userdata = NULL;
    else {
        TLAUNCHER_STATE *st = (TLAUNCHER_STATE *)*userdata;
        float diff = st->top_line * LAUNCHER_STEP - st->offset;
        st->offset += diff/8.0f;
        launcher_draw(st);
        if (st->preview_image_index != st->selected) {
            free(st->preview_image);
            TLAUNCHER_ITEM *itm = *(TLAUNCHER_ITEM **)vector_get(&st->items, st->selected);
            st->preview_image = attempt_load_preview(itm->ddl);
            st->preview_image_index = st->selected;
            showview(0,0,0,0);
        }
        size_t cnt = get_install_callback_counter();
        if (cnt != st->steam_update_counter && !st->selection_done && !st->update_lock) {
            st->steam_update_counter = cnt;
            request_for_ugc(get_list_callback_update, st);
        }
    }
}


static void navigate_down(TLAUNCHER_STATE *st) {
    const TLAUNCHER_ITEM **items = (const TLAUNCHER_ITEM **)vector_data(&st->items);
    do {
        if (st->selected == (int)vector_size(&st->items)-1) {
            if (items[st->selected] == NULL) navigate_up(st);
            return;
        }
        ++st->selected;
    } while (items[st->selected] == NULL);
}


static void navigate_up(TLAUNCHER_STATE *st) {
    const TLAUNCHER_ITEM **items = (const TLAUNCHER_ITEM **)vector_data(&st->items);
    do {
        if (!st->selected) {
            if (items[st->selected] == NULL) navigate_down(st);
            return;
        }
        --st->selected;
    } while (items[st->selected] == NULL);
}

static void activate_item(TLAUNCHER_STATE *st) {
    TLAUNCHER_ITEM *item = *(TLAUNCHER_ITEM **)vector_get(&st->items, st->selected);
    if (item->ddl == NULL && item->adv_id == 1) {
        open_steam_workshop();
    } else {
        st->selection_done = 1;
        st->selected_anim_cntr = 1;
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
           case 'H': navigate_up(st);break;
           case 31:
           case 'P': navigate_down(st);break;
           case 28:
           case 57:
           case 'M':
           case 18: activate_item(st); break;
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
            if (*(TLAUNCHER_ITEM **)vector_get(&st->items, pos) == NULL) return;
            if (pos >=0 && pos < (int)vector_size(&st->items)) {
                if (pos == st->selected) activate_item(st);
                else st->selected = pos;
            }
        }
    }


}
/*
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
*/

TCONTINUE_GAME_INFO *run_launcher() {


    TLAUNCHER_STATE state = {0};
    vector_init(&state.items, sizeof(TLAUNCHER_ITEM *), destroy_launcher_item);
    state.preview_image_index = -1;
    state.picture = ablock(H_LOADING);
    state.steam_update_counter = get_install_callback_counter();

    populate_launcher_static(&state);
    adjust_selected_position(&state);

/*    for (int i = 0; i < 50; ++i) {
        char buff[50];
        sprintf(buff,"mockup line %d",i);
        item = make_load_continue_info(NULL, NULL, NULL, buff);
        vector_push_back(&state.items, &item);
    }*/
    
    request_for_ugc(get_list_callback, &state);

    send_message(E_ADD, E_TIMER, redraw_launcher, &state);
    send_message(E_ADD, E_KEYBOARD, launcher_keyboard, &state);
    send_message(E_ADD, E_MOUSE, launcher_mouse, &state);
    escape();
    send_message(E_DONE, E_TIMER, redraw_launcher);
    send_message(E_DONE, E_KEYBOARD, launcher_keyboard);
    send_message(E_DONE, E_MOUSE, launcher_mouse);

    TLAUNCHER_ITEM *retval = NULL;

    if (state.selection_done) {
        vector_exchange(&state.items, state.selected, &retval);
    }
    

    ablock_free(state.picture);
    free(state.preview_image);
    vector_destroy(&state.items);

    return retval;
}


