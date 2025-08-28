#include "BGraph2.h"
#include "../platform.h"

#include "sdl_context.h"
#include "global_context.h"


static std::unique_ptr<uint16_t[]> screen_buffer;
static std::unique_ptr<uint16_t[]> buffer2nd;
static uint16_t *render_target;
static uint16_t screen_pitch = 640;

int game_display_init(const INI_CONFIG_SECTION *display_section,
            const char *title,
            int (*game_thread)(va_list), ...) {

    va_list args;
    va_start(args,game_thread);

    SDLContext::VideoConfig cfg  = {};
    const char *aspect_str;

    aspect_str = ini_get_string(display_section, "aspect_ratio", "4:3");
    if (sscanf(aspect_str, "%d:%d",&cfg.aspect_x, &cfg.aspect_y) != 2) {
        cfg.aspect_x = cfg.aspect_y = 0;
    }
    cfg.fullscreen = ini_get_boolean(display_section, "fullscreen", 1) == 1;
    const char *comp = ini_get_string(display_section, "composer", "auto");
    if (istrcmp(comp, "hardware") == 0 || istrcmp(comp, "hw") == 0) {
        cfg.composer = SDL_RENDERER_ACCELERATED;
    } else if (istrcmp(comp, "software") == 0 || istrcmp(comp, "sw") == 0) {
        cfg.composer = SDL_RENDERER_SOFTWARE;
    } else {
        cfg.composer = 0;
    }
    cfg.scale_quality = ini_get_string(display_section, "scale_quality", "auto");
    cfg.window_height = ini_get_int(display_section, "window_height", 480);
    cfg.window_width = ini_get_int(display_section, "window_width", 640);

    const char *filter = ini_get_string(display_section, "crt_filter", "none");
    if (istrcmp(filter,"none") == 0) cfg.crt_filter = SDLContext::CrtFilterType::none;
    else if (istrcmp(filter,"scanlines") == 0) cfg.crt_filter = SDLContext::CrtFilterType::scanlines;
    else if (istrcmp(filter,"scanlines_2") == 0) cfg.crt_filter = SDLContext::CrtFilterType::scanlines_2;
    else if (istrcmp(filter,"rgbmatrix_2") == 0) cfg.crt_filter = SDLContext::CrtFilterType::rgb_matrix_2;
    else if (istrcmp(filter,"rgbmatrix_3") == 0) cfg.crt_filter = SDLContext::CrtFilterType::rgb_matrix_3;
    else cfg.crt_filter = SDLContext::CrtFilterType::autoselect;

    cfg.cursor_size = ini_get_int(display_section, "cursor_size", 100)*0.01f;

    screen_pitch = 640;



    return get_sdl_global_context().init_window(cfg, title, [&]{
        screen_buffer = std::make_unique<uint16_t[]>(screen_pitch*480);
        buffer2nd = std::make_unique<uint16_t[]>(screen_pitch*480);
        std::fill(screen_buffer.get(), screen_buffer.get()+screen_pitch*480,0);
        render_target = screen_buffer.get();
        return game_thread(args);
    });

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
    auto &sdl = get_sdl_global_context();
    SDL_Rect hidden_from = {};
    SDL_Rect hidden_where = {};
    SDL_Rect visible_from = {0,0,640,480};
    SDL_Rect visible_where = {x,y,640,480};
    sdl.show_slide_transition(visible_from, visible_where, hidden_from, hidden_where);

}

void game_display_show_mouse(const unsigned short *mouse_image, int finger_x, int finger_y) {
    get_sdl_global_context().show_mouse_cursor(mouse_image,{finger_x, finger_y});
}
void game_display_hide_mouse() {
    get_sdl_global_context().hide_mouse_cursor();

}

void StripBlt(const void *data, unsigned int startline, uint32_t width) {

    unsigned short *start=startline*GetScreenPitch()+GetScreenAdr();
    while (width--)
    {
      memcpy(start,data,640*2);
      data=(void *)(reinterpret_cast<const short *>(data)+GetScreenPitch());
      start=start+GetScreenPitch();
    }

}

static void DXCopyRects64(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys) {
    get_sdl_global_context().present_rect(screen_buffer.get(), screen_pitch, x,y,xs,ys);

}


void game_display_update_rect(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys)
  {

  if (x>DxGetResX() || y>DxGetResY()) return;
  if (xs==0) xs=DxGetResX();
  if (ys==0) ys=DxGetResY();
  if (x+xs>DxGetResX()) xs=DxGetResX()-x;
  if (y+ys>DxGetResY()) ys=DxGetResY()-y;
  DXCopyRects64(x,y,xs,ys);
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


char  game_display_is_quit_requested() {
    return get_sdl_global_context().is_quit_requested()?1:0;
}

void game_display_cancel_quit_request() {
    return get_sdl_global_context().cancel_quit_request();
}

void game_display_set_icon(const void *icon_data, size_t icon_size) {
    auto &sdl = get_sdl_global_context();
    sdl.set_window_icon(icon_data, icon_size);
}

///Load sprite HI format, returns ID of sprite
void game_display_load_sprite(int sprite_id, const unsigned short *hi_image) {
    auto &sdl = get_sdl_global_context();
    return sdl.load_sprite(sprite_id, hi_image);
}
///show and place sprite at given coordinates
void game_display_place_sprite(int sprite_id, int x, int y) {
    auto &sdl = get_sdl_global_context();
    sdl.place_sprite(sprite_id,x, y);
}
///show and place (and scale) sprite at given coordinates
void game_display_scale_sprite(int sprite_id, int x, int y, int w, int h) {
    auto &sdl = get_sdl_global_context();
    sdl.scale_sprite(sprite_id, x, y, w,h);
}
///hide sprite
void game_display_hide_sprite(int sprite_id) {
    auto &sdl = get_sdl_global_context();
    sdl.hide_sprite(sprite_id);
}
///unload sprite and free index
void game_display_unload_sprite(int sprite_id) {
    auto &sdl = get_sdl_global_context();
    sdl.unload_sprite(sprite_id);
}
void game_display_sprite_set_zindex(int sprite_id, int zindex) {
    auto &sdl = get_sdl_global_context();
    sdl.sprite_set_zindex(sprite_id, zindex);
}

void game_display_disable_crt_effect_temporary(char disable) {
    auto &sdl = get_sdl_global_context();
    sdl.disable_crt_effect_temprary(disable?true:false);
}
