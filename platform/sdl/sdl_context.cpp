#include "sdl_context.h"
#include "../platform.h"

#include <stdexcept>
void SDLContext::SDL_Deleter::operator ()(SDL_Window* window) {
    SDL_DestroyWindow(window);
}

void SDLContext::SDL_Deleter::operator ()(SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
}

void SDLContext::SDL_Deleter::operator ()(SDL_Surface* surface) {
    SDL_FreeSurface(surface);
}

void SDLContext::SDL_Deleter::operator ()(SDL_Texture* texture) {
    SDL_DestroyTexture(texture);
}

struct SDL_INIT_Context {

    SDL_INIT_Context() {
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO) < 0) {
            SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
            exit(1);
        }
        inited = true;
    }
    ~SDL_INIT_Context() {
        SDL_Quit();
    }


    bool inited = false;
};

static SDL_INIT_Context init_context = {};

SDLContext::SDLContext() {
    if (!init_context.inited) throw std::runtime_error("SDL not inited");

}

void SDLContext::init_screen(DisplayMode mode, const char *title) {
    char buff[256];

    int width = 640;
    int height = 480;
    if (mode == double_window) {
        width*=2;
        height*=2;
    }
    SDL_Window *window = SDL_CreateWindow(title,
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                width, height, SDL_WINDOW_RESIZABLE|(mode==fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0));

    if (!window) {
        snprintf(buff, sizeof(buff), "SDL Error create window: %s\n", SDL_GetError());
        throw std::runtime_error(buff);
    }

    _window.reset(window);
    SDL_Renderer *renderer = SDL_CreateRenderer(_window.get(), -1, 0);
    if (!renderer) {
        snprintf(buff,sizeof(buff), "Chyba při vytváření rendereru: %s\n", SDL_GetError());
        throw std::runtime_error(buff);
    }
    _renderer.reset(renderer);
    // Vytvoření softwarového backbufferu (surface)
    SDL_Surface *backbuffer = SDL_CreateRGBSurfaceWithFormat(0, 640, 480, 16, SDL_PIXELFORMAT_RGB565);
    if (!backbuffer) {
        snprintf(buff,sizeof(buff), "Chyba při vytváření surface: %s\n", SDL_GetError());
        throw std::runtime_error(buff);
    }
    _surface.reset(backbuffer);
    // Vytvoření textury pro zobrazení backbufferu
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 640, 480);
    if (!texture) {
        snprintf(buff, sizeof(buff), "Chyba při vytváření textury: %s\n", SDL_GetError());
        throw std::runtime_error(buff);
    }
    _texture.reset(texture);

    SDL_LockSurface(_surface.get());

    if (!_timer_event) _timer_event = SDL_RegisterEvents(1);
    _fullscreen_mode = mode == fullscreen;
}

void SDLContext::close_screen() {
    SDL_UnlockSurface(_surface.get());
    _texture.reset();
    _surface.reset();
    _renderer.reset();
    _window.reset();

}

uint16_t* SDLContext::get_surface_addr() {
    return reinterpret_cast<uint16_t *>(_surface->pixels);
}


int32_t SDLContext::get_surface_pitch() {
    return _surface->pitch;
}


void  SDLContext::charge_timer() {
    _active_timer = SDL_AddTimer(1000/TIMERSPEED, [](Uint32 interval, void *param) -> Uint32 {
        SDLContext *me = reinterpret_cast<SDLContext *>(param);
        SDL_Event* event = (SDL_Event*)param;
          event->type = me->_timer_event;
          SDL_PushEvent(event);
          return 0;
    }, this);

}
void SDLContext::pool_events() {
    if (!_active_timer.has_value()) charge_timer();
    SDL_Event e;
    while (true) {
       if (SDL_WaitEvent(&e)) {
           if (e.type == SDL_QUIT) {
               _quit_requested = true;
               return;
           }
           if (e.type == _timer_event) {
               break;
           }
           if (e.type == SDL_KEYDOWN) {
               if (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)) {
                   _fullscreen_mode = !_fullscreen_mode;
                   SDL_SetWindowFullscreen(_window.get(), _fullscreen_mode ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
               }
           } else if (e.type == SDL_MOUSEMOTION) {
               int mouseX = e.motion.x;
               int mouseY = e.motion.y;
               int windowWidth;
               int windowHeight;
               SDL_GetWindowSize(_window.get(), &windowWidth, &windowHeight);
               float normalizedX = (float)mouseX / windowWidth;
               float normalizedY = (float)mouseY / windowHeight;
               ms_event.event = 1;
               ms_event.event_type = 1;
               ms_event.x = (int16_t)(640*normalizedX);
               ms_event.y = (int16_t)(480*normalizedY);
           } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
               int mouseX = e.button.x;
               int mouseY = e.button.y;
               int button = e.button.button;
               int up =  e.type == SDL_MOUSEBUTTONUP?1:0;
               ms_event.event = 1;
               ms_event.event_type = (1<<(2*button-1+up));
               switch (button) {
                   default: break;
                   case 1: ms_event.tl1 = !up; break;
                   case 2: ms_event.tl2 = !up; break;
                   case 3: ms_event.tl3 = !up; break;
               }
           }

       } else {
           throw std::runtime_error("SDL_WaitEvent error");
       }
    }
    charge_timer();

}

void SDLContext::present_rect(uint16_t *pixels, unsigned int pitch,
        unsigned int x, unsigned int y, unsigned int xs, unsigned ys) {

    auto beg = pixels + y * pitch + y;
    SDL_Rect r = {static_cast<int>(x),
                  static_cast<int>(y),
                  static_cast<int>(xs),
                  static_cast<int>(ys)};
    SDL_UpdateTexture(_texture.get(), &r, beg, pitch*2);
    SDL_RenderClear(_renderer.get());
    SDL_RenderCopy(_renderer.get(), _texture.get(), NULL, NULL);
    SDL_RenderPresent(_renderer.get());


}
