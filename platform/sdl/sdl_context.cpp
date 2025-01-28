#include "sdl_context.h"
#include "keyboard_map.h"

#include <atomic>
#include <cassert>
#include "../platform.h"

#include <iostream>
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
    static Uint32 update_request_event = SDL_RegisterEvents(1);
    _update_request_event = update_request_event;

    assert(!_render_thread.joinable());


    int width = 640;
    int height = 480;
    if (mode == double_window) {
        width*=2;
        height*=2;
    }

    _fullscreen_mode = mode == fullscreen;

    std::atomic<bool> done = false;
    std::exception_ptr e;
    _render_thread = std::jthread([&](std::stop_token stp){
        bool err = false;
        try {
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
            SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 640, 480);
            if (!texture) {
                snprintf(buff, sizeof(buff), "Chyba při vytváření textury: %s\n", SDL_GetError());
                throw std::runtime_error(buff);
            }
            _texture.reset(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 640, 480);
            if (!texture) {
                snprintf(buff, sizeof(buff), "Chyba při vytváření textury: %s\n", SDL_GetError());
                throw std::runtime_error(buff);
            }
            _texture2.reset(texture);
            _visible_texture = _texture.get();
            _hidden_texture = _texture.get();
        } catch (...) {
            e = std::current_exception();
            err = true;
        }
        done = true;
        done.notify_all();
        SDL_ShowCursor(SDL_DISABLE);
        if (!err) event_loop(stp);
        _texture.reset();
        _texture2.reset();
        _renderer.reset();
        _window.reset();
    });

    done.wait(false);
    if (e) {
        _render_thread.join();
        std::rethrow_exception(e);
    }


}

void SDLContext::close_screen() {
    _render_thread.request_stop();
    _render_thread.join();
}

void SDLContext::event_loop(std::stop_token stp) {

    static Uint32 exit_loop_event = SDL_RegisterEvents(1);
    std::stop_callback stopcb(stp,[&]{
        SDL_Event event;
        event.type = exit_loop_event;
        SDL_PushEvent(&event);
    });

    SDL_Event e;
    while (SDL_WaitEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return;
        }
        if (e.type == exit_loop_event) {
            return;
        }
        if (e.type == _update_request_event) {
            update_screen();
        }

        if (e.type == SDL_KEYDOWN) {
            _key_capslock = e.key.keysym.mod & KMOD_CAPS;
            _key_shift =e.key.keysym.mod & KMOD_SHIFT;
            _key_control  =e.key.keysym.mod & KMOD_CTRL;
            if (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)) {
                _fullscreen_mode = !_fullscreen_mode;
                SDL_SetWindowFullscreen(_window.get(), _fullscreen_mode ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
            } else {
                auto code =sdl_keycode_map.get_bios_code(e.key.keysym.scancode,
                        e.key.keysym.mod & KMOD_SHIFT, e.key.keysym.mod & KMOD_CTRL);
                if (code) {
                    std::lock_guard _(_mx);
                    _keyboard_queue.push(code);
                }
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

    }
}


/*
void SDLContext::pool_events() {
    SDL_RenderClear(_renderer.get());
    SDL_RenderCopy(_renderer.get(), _texture.get(), NULL, NULL);
    SDL_RenderPresent(_renderer.get());
    ms_event.event = 0;
    SDL_Event e;
    while (true) {
       if (SDL_WaitEvent(&e)) {
           if (e.type == SDL_QUIT) {
               _quit_requested = true;
               return;
           if (e.type == SDL_KEYDOWN) {
               if (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)) {
                   _fullscreen_mode = !_fullscreen_mode;
                   SDL_SetWindowFullscreen(_window.get(), _fullscreen_mode ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
               }
           } else if (e.type == SDL_MOUSEMOTION) {
               std::lock_guard _(_mx);
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
               std::lock_guard _(_mx);
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
}*/


void SDLContext::present_rect(uint16_t *pixels, unsigned int pitch,
        unsigned int x, unsigned int y, unsigned int xs, unsigned ys) {


    auto beg = pixels + y * pitch + x;
    SDL_Rect r = {static_cast<int>(x),
                  static_cast<int>(y),
                  static_cast<int>(xs),
                  static_cast<int>(ys)};

    std::lock_guard _(_mx);
    signal_push();
    push_update_msg(r, beg, pitch);
}

std::uint16_t SDLContext::pop_keyboard_code()  {
    std::lock_guard _(_mx);
    std::uint16_t out = _keyboard_queue.front();
    _keyboard_queue.pop();
    return out;
}

bool SDLContext::is_keyboard_ready() const {
    std::lock_guard _(_mx);
    return !_keyboard_queue.empty();
}

void SDLContext::signal_push() {
    if (_display_update_queue.empty()) {
        SDL_Event event;
        event.type = _update_request_event;
        SDL_PushEvent(&event);
    }

}

void SDLContext::update_screen() {
    {
        std::lock_guard _(_mx);
        QueueIter iter = _display_update_queue.data();
        QueueIter end = iter + _display_update_queue.size();
        while (iter != end) {
            DisplayRequest req;
            pop_item(iter, req);
            switch (req) {
                case DisplayRequest::update: {
                    SDL_Rect r;
                    pop_item(iter, r);
                    std::string_view data = pop_data(iter, r.w*r.h*2);
                    SDL_UpdateTexture(_texture.get(), &r, data.data(), r.w*2);
                }
                break;
                case DisplayRequest::swap_render_buffers: {
                    std::swap(_texture,_texture2);
                }
                break;
                case DisplayRequest::swap_visible_buffers: {
                    std::swap(_visible_texture,_hidden_texture);
                }
                break;
            }
        }
        _display_update_queue.clear();
    }
    SDL_RenderClear(_renderer.get());
    SDL_RenderCopy(_renderer.get(), _visible_texture, NULL, NULL);
    SDL_RenderPresent(_renderer.get());
}

template<typename T>
requires(std::is_trivially_copy_constructible_v<T>)
void  SDLContext::push_item(const T &item) {
    auto b = reinterpret_cast<const char *>(&item);
    auto e = b + sizeof(T);
    auto sz = _display_update_queue.size();
    _display_update_queue.resize(sz + sizeof(T));
    std::copy(b, e, _display_update_queue.begin()+sz);
}

void SDLContext::push_item(const std::string_view &item) {
    auto sz = _display_update_queue.size();
    _display_update_queue.resize(sz + item.size());
    std::copy(item.begin(), item.end(),  _display_update_queue.begin()+sz);
}

void SDLContext::push_update_msg(const SDL_Rect &rc, const uint16_t *data, int pitch) {
    push_item(DisplayRequest::update);
    push_item(rc);
    for (int yp = 0; yp < rc.h; ++yp) {
        push_item(std::string_view(reinterpret_cast<const char *>(data), rc.w*2));
        data += pitch;
    }
}

template<typename T>
requires(std::is_trivially_copy_constructible_v<T>)
void SDLContext::pop_item(QueueIter &iter, T &item) {
    std::copy(iter, iter+sizeof(T), reinterpret_cast<char *>(&item));
    iter += sizeof(T);
}
std::string_view SDLContext::pop_data(QueueIter &iter, std::size_t size) {
    const char *c = iter;
    iter += size;
    return std::string_view(c, size);
}

void SDLContext::swap_render_buffers() {
    std::lock_guard _(_mx);
    signal_push();
    push_item(DisplayRequest::swap_render_buffers);
}

void SDLContext::swap_display_buffers() {
    std::lock_guard _(_mx);
    signal_push();
    push_item(DisplayRequest::swap_render_buffers);
}

