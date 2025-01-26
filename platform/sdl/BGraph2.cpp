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
    render_target = screen_buffer.get();

    return 0;
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
      data=(void *)(reinterpret_cast<char *>(data)+get_sdl_global_context().get_surface_pitch());
      start=start+GetScreenPitch();
    }

}

void DXCopyRects64(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys) {
    get_sdl_global_context().present_rect(screen_buffer.get(), screen_pitch, x,y,xs,ys);

}

void *DxPrepareWalk(int ypos) {
    return 0;
}
void DxZoomWalk(void *handle, int ypos, int *points,float phase, void *lodka) {

}
void DxDoneWalk(void *handle) {

}

void *DxPrepareTurn(int ypos) {
    return 0;
}
void DxTurn(void *handle, char right, int ypos,int border, float phase, void *lodka) {

}
void DxDoneTurn(void *handle) {

}
void DxTurnLeftRight(char right, float phase, int border, int ypos, int *last) {

}
