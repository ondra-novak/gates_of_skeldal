#pragma once

#include <memory>
#include <optional>
#include <SDL2/SDL.h>
#include <mouse.h>

class SDLContext {
public:

    SDLContext();

    enum DisplayMode {
        native_window,
        double_window,
        fullscreen
    };


    void init_screen(DisplayMode mode, const char *title);

    void close_screen();


    uint16_t *get_surface_addr();
    int32_t get_surface_pitch();


    void pool_events();

    void present_rect(uint16_t *pixels, unsigned int pitch, unsigned int x, unsigned int y, unsigned int xs,unsigned ys);

protected:

    struct SDL_Deleter {
        void operator()(SDL_Window *);
        void operator()(SDL_Renderer *);
        void operator()(SDL_Surface *);
        void operator()(SDL_Texture *);
    };

    MS_EVENT ms_event;

    std::unique_ptr<SDL_Window, SDL_Deleter> _window;
    std::unique_ptr<SDL_Renderer, SDL_Deleter> _renderer;
    std::unique_ptr<SDL_Surface, SDL_Deleter> _surface;
    std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;

    std::optional<SDL_TimerID> _active_timer;
    Uint32 _timer_event = 0;

    bool _quit_requested = false;
    bool _fullscreen_mode = false;
    void charge_timer();
};
