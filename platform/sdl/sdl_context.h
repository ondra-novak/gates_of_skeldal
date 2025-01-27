#pragma once

#include <memory>
#include <optional>
#include <SDL2/SDL.h>
#include <thread>
#include <vector>
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


    void present_rect(uint16_t *pixels, unsigned int pitch, unsigned int x, unsigned int y, unsigned int xs,unsigned ys);

    MS_EVENT getMsEvent()  {
        std::lock_guard _(_mx);
        MS_EVENT out = ms_event;
        ms_event.event = 0;
        return out;
    }

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
    std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;

    std::jthread _render_thread;

    bool _quit_requested = false;
    bool _fullscreen_mode = false;
    bool _present = false;
    void charge_timer();

    void event_loop(std::stop_token stp);
    std::mutex _mx;

    struct UpdateMsg {
        SDL_Rect rc;
        std::vector<short> data;
    };

    std::vector<UpdateMsg> _display_update_queue;
    Uint32 _update_request_event;

    void update_screen();

};
