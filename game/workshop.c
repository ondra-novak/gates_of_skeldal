
#include "platform/timer.h"

#include <libs/event.h>
#include <libs/memman.h>
#include <libs/bgraph.h>

#include "globals.h"
#include "platform/steam_c_api.h"

const void *pcx_8bit_decomp(const void *p, int32_t *s, int h);

static const char *upload_message = "Connecting...";
static TWORKSHOP_UPLOAD_STATE upload_state = {0};
static  uint32_t stop_anim_time = 0;


static void kbd_escape(EVENT_MSG *msg, void **unused) {
    (void *)unused;
    if (msg->msg == E_KEYBOARD) {
        exit_wait = 1;
    }
}


void finalize_with_message(const char *message) {
    upload_message = message;
    stop_anim_time = get_game_tick_count();
    send_message(E_ADD, E_KEYBOARD, kbd_escape);
}

static void bgr_anim(EVENT_MSG *msg, void **user) {
    switch (msg->msg) {
        case E_TIMER: {
                if (upload_state.done && !stop_anim_time) {
                    finalize_with_message(upload_state.message);                    
                }

                trans_bar25(0, 0, 640, 480);
                uint32_t t = ((stop_anim_time?stop_anim_time:get_game_tick_count())/100) % 8 + 1;
                char name [15];
                snprintf(name, 15,"TELEA0%dA.PCX", t);
                int32_t x;
                const void *data = afile(name,0,&x);
                const void *img = pcx_8bit_decomp(data, &x, 0);
                put_picture(70,80,img);

                const char *title=stop_anim_time?"Press any key":"Upload to Steam Workshop";
                set_font(H_FBIG, RGB555(31,31,31));
                set_aligned_position(320, 100, 1, 1, title);
                outtext(title);
                

                set_font(H_FONT6, RGB555(31,31,31));
                if (upload_message) {
                    char *buff = (char *)malloc(strlen(upload_message)+10);
                    int xs,ys;
                    zalamovani(upload_message, buff, 500, &xs, &ys);
                    int y = 350;
                    char *c = buff;
                    while (*c) {
                        set_aligned_position(320, y, 1, 1, c);
                        outtext(c);
                        y += text_height(c)*3/2;
                        c = strchr(c,0);
                    }
                    free(buff);
                }
                

                showview(0,0,0,0);                
    }
    break;
}
}


void update_upload_state(char running, const char *message, uint64_t upload_bytes, uint64_t total_bytes, void *context) {
 

}

void workshop_publish_ui(const char *path) {
 
    send_message(E_ADD,E_TIMER,bgr_anim);
    steam_upload_to_workshop(path, update_upload_state,NULL);

    escape();

    send_message(E_DONE,E_TIMER,bgr_anim);



}