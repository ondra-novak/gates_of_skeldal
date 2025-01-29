#include "BGraph2.h"

#include "sdl_context.h"
#include "global_context.h"


static std::unique_ptr<uint16_t[]> screen_buffer;
static std::unique_ptr<uint16_t[]> buffer2nd;
static uint16_t *render_target;
static uint16_t screen_pitch = 640;

char DXInit64(char inwindow,int zoom,int monitor, int refresh) {

    SDLContext::DisplayMode mode;
    if (inwindow) {
        if (zoom) {
            mode = SDLContext::double_window;
        } else {
            mode = SDLContext::native_window;
        }
    } else {
        mode = SDLContext::fullscreen;
    }

    screen_pitch = 640;
    get_sdl_global_context().init_screen(mode, "Skeldal"); //todo allow change
    screen_buffer = std::make_unique<uint16_t[]>(screen_pitch*480);
    buffer2nd = std::make_unique<uint16_t[]>(screen_pitch*480);
    std::fill(screen_buffer.get(), screen_buffer.get()+screen_pitch*480,0);
    render_target = screen_buffer.get();

    return 1;
}

void DXCloseMode() {
    get_sdl_global_context().close_screen();
}


uint16_t *GetScreenAdr() {
    return render_target;
}
uint16_t *GetBuffer2nd() {
    return buffer2nd.get();

}
int32_t GetScreenPitch() {
    return screen_pitch;
}
int32_t GetBuffer2ndPitch() {
    return screen_pitch;
}
int32_t GetScreenSizeBytes() {
    return screen_pitch * 480 * 2;
}

void RedirectScreen(uint16_t *newaddr) {
    render_target = newaddr;
}
void RestoreScreen() {
    render_target = screen_buffer.get();
}
void RedirectScreenBufferSecond() {
    render_target = buffer2nd.get();
}
int DxGetResX() {
    return 640;
}
int DxGetResY() {
    return 480;
}
void setvesa_displaystart(int x,int y){

}
void StripBlt(void *data, unsigned int startline, uint32_t width) {

    unsigned short *start=startline*GetScreenPitch()+GetScreenAdr();
    while (width--)
    {
      memcpy(start,data,640*2);
      data=(void *)(reinterpret_cast<short *>(data)+GetScreenPitch());
      start=start+GetScreenPitch();
    }

}

void DXCopyRects64(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys) {
    get_sdl_global_context().present_rect(screen_buffer.get(), screen_pitch, x,y,xs,ys);

}

void *DxPrepareWalk(int ypos) {
    auto &sdl = get_sdl_global_context();
    sdl.swap_render_buffers();
    sdl.present_rect(screen_buffer.get(), screen_pitch, 0,0,640,480);//new is in hidden buffer
    return NULL;
}
void DxZoomWalk(void *handle, int ypos, int *points,float phase, void *lodka) {
    auto &sdl = get_sdl_global_context();
    if (phase>1.0) phase=1.0f;
    if (phase<-1.0) phase=-1.0f;
    SDL_Rect rc1;
    rc1.x=0;
    rc1.y=0+ypos;
    rc1.w=640;
    rc1.h=360;
    SDL_Rect rc2;
    rc2.x=points[0];
    rc2.y=points[1]+ypos;
    rc2.w=points[2]-rc2.x;
    rc2.h=points[3]-rc2.y;
    sdl.show_blend_transition({0,ypos,640,360},rc1, rc2, phase);
}
void DxDoneWalk(void *handle) {
    auto &sdl = get_sdl_global_context();
    sdl.swap_display_buffers(); //present hidden buffer
}

void *DxPrepareTurn(int ypos) {
    auto &sdl = get_sdl_global_context();
    sdl.swap_render_buffers();
    sdl.present_rect(screen_buffer.get(), screen_pitch, 0,0,640,480);//new is in hidden buffer
    return NULL;
}
void DxTurn(void *handle, int ypos,int border, float phase, void *lodka) {
    auto &sdl = get_sdl_global_context();
    if (phase>1.0) phase=1.0f;
    if (phase<-1.0) phase=-1.0f;
    int width = 640-2*border;
    SDL_Rect visible_from{0,ypos,640,360};
    SDL_Rect visible_where;
    SDL_Rect hidden_from{0,ypos,640,360};
    SDL_Rect hidden_where;

    if (phase > 0) {
        hidden_from.x = border;
    } else if (phase < 0) {
        visible_from.x = border;
    } else {
        return ;
    }
    visible_from.w-= border;
    hidden_from.w -= border;
    visible_where = visible_from;
    hidden_where = hidden_from;

    int xsep = border + static_cast<int>(std::round(width * std::abs(phase)));
    if (phase > 0) {
        xsep = 640-xsep;
        hidden_where.x = xsep;
        visible_where.x = xsep - visible_where.w;
    } else {
        hidden_where.x = xsep - visible_where.w;
        visible_where.x = xsep;
    }

    auto crop_rects = [](SDL_Rect &primary, SDL_Rect &secondary) {
        if (primary.x < 0) {
            secondary.x -= primary.x*2/3;
            secondary.w += primary.x*2/3;
            primary.w += primary.x;
            primary.x = 0;
        }
        int extra = 640 - (primary.x + primary.w);;
        if (extra < 0) {
            primary.w += extra;
            secondary.w += extra*2/3;
        }

    };
    crop_rects(visible_where, visible_from);
    crop_rects(hidden_where, hidden_from);
    int adjw = static_cast<int>(hidden_from.w / (1.0+2.0*std::abs(phase)));
    if (phase < 0) {
        hidden_from.x += adjw;
    }
    hidden_from.w -= adjw;
    sdl.show_slide_transition(visible_from, visible_where, hidden_from, hidden_where);
}
void DxDoneTurn(void *handle) {
    auto &sdl = get_sdl_global_context();
    sdl.swap_display_buffers(); //present hidden buffer

}
void DxTurnLeftRight(char right, float phase, int border, int ypos, int *last) {

}
