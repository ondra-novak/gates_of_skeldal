
#include "platform/timer.h"

#include <libs/event.h>
#include <libs/memman.h>
#include <libs/bgraph.h>
#include <sys/types.h>
#include <threads.h>

#include "globals.h"
#include "platform/steam_c_api.h"

const void *pcx_8bit_decomp(const void *p, int32_t *s, int h);

#define MAX_PROGRESSBAR_WITH 400
#define TOP_TEXT_Y 50
#define BOTTOM_TEXT_Y 400
#define PROGRESS_BAR_Y 350
#define PROGRESS_BAR_HEIGH 20

typedef struct {
    char *progress_message;
    int result;
    uint64_t progress_bar;
    mtx_t mx;
} WORKSHOP_UPLOAD_STATE;




static void kbd_escape(EVENT_MSG *msg, void **unused) {
    (void *)unused;
    if (msg->msg == E_KEYBOARD) {
        exit_wait = 1;
    }
}


void finalize_with_message(int result, WORKSHOP_UPLOAD_STATE *st, char *message) {
    if (!st->result && result) {
        st->result = result;
        send_message(E_ADD, E_KEYBOARD, kbd_escape);
    }
    if (st->progress_message) free(st->progress_message);
    
    st->progress_message = message;
    st->progress_bar = MAX_PROGRESSBAR_WITH;
}

static void bgr_anim(EVENT_MSG *msg, void **user) {
    WORKSHOP_UPLOAD_STATE *st = (WORKSHOP_UPLOAD_STATE *)*user;
    switch (msg->msg) {
        case E_INIT: *user = va_arg(msg->data, void *);break;
        case E_DONE: *user = NULL; break;
        case E_TIMER: {
                mtx_lock(&st->mx);
                trans_bar25(0, 0, 640, 480);
                if (!st->result) {
                    uint32_t t = (get_game_tick_count()/100) % 8 + 1;
                    char name [15];
                    snprintf(name, 15,"TELEA0%dA.PCX", t);
                    int32_t x;
                    const void *data = afile(name,0,&x);
                    const void *img = pcx_8bit_decomp(data, &x, 0);
                    put_picture(70,80,img);
                    ablock_free(img);
                    ablock_free(data);
                } else if (st->result>0) {
                    int32_t sz;
                    word *data = (word *)afile(".PRVIMG",0, &sz);                    
                    put_picture((640-data[0])/2,80,data);
                    ablock_free(data);
                }

                const char *title=st->result?"Press any key":"Upload to Steam Workshop";
                set_font(H_FBIG, RGB555(31,31,31));
                set_aligned_position(320, TOP_TEXT_Y, 1, 1, title);
                outtext(title);

                int pl = (640-MAX_PROGRESSBAR_WITH)/2;
                
                if (st->progress_bar) {
                    rectangle(pl-1, PROGRESS_BAR_Y-1, pl+MAX_PROGRESSBAR_WITH+1, PROGRESS_BAR_Y+PROGRESS_BAR_HEIGH    , RGB555(20,20,20));
                    trans_bar(pl,PROGRESS_BAR_Y, st->progress_bar,PROGRESS_BAR_HEIGH,st->result<0?RGB555(31,0,0):RGB555(0,31,0));
                }


                set_font(H_FONT6, RGB555(31,31,31));
                if (st->progress_message) {
                    char *buff = (char *)malloc(strlen(st->progress_message)+10);
                    int xs,ys;
                    zalamovani(st->progress_message, buff, 500, &xs, &ys);
                    int y = BOTTOM_TEXT_Y;
                    char *c = buff;
                    while (*c) {
                        set_aligned_position(320, y, 1, 1, c);
                        outtext(c);
                        y += text_height(c)*3/2;
                        c = strchr(c,0)+1;
                    }
                    free(buff);
                }
                mtx_unlock(&st->mx);
                

                showview(0,0,0,0);                                
    }
    break;
}
}



static void update_upload_state(int running, const char *message, uint64_t upload_bytes, uint64_t total_bytes, void *context) {
    WORKSHOP_UPLOAD_STATE *st = (WORKSHOP_UPLOAD_STATE *)context;
    mtx_lock(&st->mx);
    if (running) {
        finalize_with_message(running, st, strdup(message));
    } else if (message) {
        free(st->progress_message);
        st->progress_message = strdup(message);
    } 
    if (total_bytes) {    
        st->progress_bar = (int)(MAX_PROGRESSBAR_WITH * (float)upload_bytes / (float)total_bytes);            
    }

    mtx_unlock(&st->mx);
}

void workshop_publish_ui(const char *path) {
 
    WORKSHOP_UPLOAD_STATE st;
    memset(&st, 0, sizeof(st));;
    st.progress_message = strdup("Connecting Steam...");
    mtx_init(&st.mx, mtx_plain);

    send_message(E_ADD,E_TIMER,bgr_anim, &st);
    steam_upload_to_workshop(path, update_upload_state,&st);

    escape();

    send_message(E_DONE,E_TIMER,bgr_anim);
    free(st.progress_message);
    mtx_destroy(&st.mx);



}