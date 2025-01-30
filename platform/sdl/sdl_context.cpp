#include "sdl_context.h"
#include "keyboard_map.h"

#include <atomic>
#include <cassert>
#include "../platform.h"

#include <cmath>
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

void SDLContext::generateCRTTexture(SDL_Renderer* renderer, SDL_Texture** texture, int width, int height) {

    // Vytvoř novou texturu ve správné velikosti
    *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);

    // Zamkni texturu pro přímý přístup k pixelům
    void* pixels;
    int pitch;
    SDL_LockTexture(*texture, nullptr, &pixels, &pitch);

    // Vyplň texturu patternem (liché řádky tmavší)
    Uint32* pixelArray = (Uint32*)pixels;
    Uint32 darkPixel = 0x00000080; // Černá s částečnou průhledností
    Uint32 transparentPixel = 0x00000000;

    for (int y = 0; y < height; y++) {
        Uint32 color = (y % 3 == 0) ? darkPixel : transparentPixel;
        for (int x = 0; x < width; x++) {
            pixelArray[y * (pitch / 4) + x] = color;
        }
    }

    // Odemkni texturu
    SDL_UnlockTexture(*texture);
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
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

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
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            _texture.reset(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 640, 480);
            if (!texture) {
                snprintf(buff, sizeof(buff), "Chyba při vytváření textury: %s\n", SDL_GetError());
                throw std::runtime_error(buff);
            }
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            _texture2.reset(texture);
            _visible_texture = _texture.get();
            _hidden_texture = _texture2.get();
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

         if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    _crt_effect.reset();
                }
         } else if (e.type == SDL_KEYDOWN) {
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
            SDL_Point mspt(e.motion.x, e.motion.y);
            SDL_Rect winrc = get_window_aspect_rect();
            SDL_Point srcpt = to_source_point(winrc, mspt);
            ms_event.event = 1;
            ms_event.event_type = 1;
            ms_event.x = srcpt.x;
            ms_event.y = srcpt.y;
        } else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            int button = e.button.button;
            int up =  e.type == SDL_MOUSEBUTTONUP?1:0;
            ms_event.event = 1;
            int shift = 0;
            switch (button) {
                default: break;
                case 1: ms_event.tl1 = !up; shift = 1; break;
                case 2: ms_event.tl3 = !up; shift = 5; break;
                case 3: ms_event.tl2 = !up; shift = 3; break;
            }
            ms_event.event_type = (1<<(shift+up));
        }

    }
}



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

void SDLContext::show_slide_transition(const SDL_Rect &visible_from,
        const SDL_Rect &visible_where, const SDL_Rect &hidden_from,
        const SDL_Rect &hidden_where) {
    std::lock_guard _(_mx);
    signal_push();
    push_item(DisplayRequest::slide_transition);
    push_item(SlideTransitionReq{visible_from, visible_where,hidden_from, hidden_where});
}

void SDLContext::signal_push() {
    if (_display_update_queue.empty()) {
        SDL_Event event;
        event.type = _update_request_event;
        SDL_PushEvent(&event);
    }

}

void SDLContext::update_screen() {
    std::optional<BlendTransitionReq> blend_transition;
    std::optional<SlideTransitionReq> slide_transition;
    {
        std::lock_guard _(_mx);
        if (_display_update_queue.empty()) return;
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
                    blend_transition.reset();
                    slide_transition.reset();
                }
                break;
                case DisplayRequest::blend_transition:
                    blend_transition.emplace();
                     pop_item(iter, *blend_transition);
                break;
                case DisplayRequest::slide_transition:
                    slide_transition.emplace();
                     pop_item(iter, *slide_transition);
                break;
            }
        }
        _display_update_queue.clear();

    }
    SDL_Rect winrc = get_window_aspect_rect();
    SDL_RenderClear(_renderer.get());
    if (slide_transition) {
        SDL_SetTextureAlphaMod(_hidden_texture, 255);
        SDL_SetTextureAlphaMod(_visible_texture, 255);
        SDL_RenderCopy(_renderer.get(), _hidden_texture, NULL, &winrc);
        SDL_Rect visible_where_win = to_window_rect(winrc, slide_transition->visible_where);
        SDL_Rect hidden_where_win = to_window_rect(winrc, slide_transition->hidden_where);
        SDL_RenderCopy(_renderer.get(), _visible_texture, &slide_transition->visible_from, &visible_where_win);
        SDL_RenderCopy(_renderer.get(), _hidden_texture, &slide_transition->hidden_from, &hidden_where_win);
    }
    else if (blend_transition) {
        SDL_SetTextureAlphaMod(_hidden_texture, 255);
        SDL_RenderCopy(_renderer.get(), _hidden_texture, NULL, &winrc);
        if (blend_transition->phase >= 0) {
            float f = blend_transition->phase;
            SDL_SetTextureAlphaMod(_visible_texture, 255);
            SDL_Rect wrkarea = to_window_rect(winrc, blend_transition->wrkarea);
            SDL_Rect src1 = transition_rect(blend_transition->prev, blend_transition->next, f);
            SDL_RenderCopy(_renderer.get(), _visible_texture, &src1, &wrkarea);
            if (SDL_SetTextureAlphaMod(_hidden_texture, (uint8_t)(255.0f*(f)))!= -1) {
                SDL_Rect trgnxt = to_window_rect(winrc, blend_transition->next);
                SDL_Rect trgwin = transition_rect(trgnxt, wrkarea, f);
                SDL_RenderCopy(_renderer.get(), _hidden_texture, &blend_transition->wrkarea, &trgwin);
            }
        } else {
            float f = blend_transition->phase;
            SDL_Rect wrkarea = to_window_rect(winrc, blend_transition->wrkarea);
            SDL_Rect src1 = transition_rect(blend_transition->prev, blend_transition->next, 1+f);
            SDL_RenderCopy(_renderer.get(), _hidden_texture, &src1, &wrkarea);
            if (SDL_SetTextureAlphaMod(_visible_texture, (uint8_t)(255.0f*(1+f))) != -1) {
                SDL_Rect trgnxt = to_window_rect(winrc, blend_transition->next);
                SDL_Rect trgwin = transition_rect(trgnxt, wrkarea, 1+f);
                SDL_RenderCopy(_renderer.get(), _visible_texture, &blend_transition->wrkarea, &trgwin);
            }
        }
    } else {
        SDL_SetTextureAlphaMod(_visible_texture, 255);
        SDL_RenderCopy(_renderer.get(), _visible_texture, NULL, &winrc);
    }
    if (winrc.w > 1280 && winrc.h > 960) {
        if (!_crt_effect) {
            SDL_Texture *txt;
            generateCRTTexture(_renderer.get(), &txt, 128, std::min<int>(1440, winrc.h));
            _crt_effect.reset(txt);
        }
    }
    SDL_RenderCopy(_renderer.get(), _crt_effect.get(), NULL, &winrc);
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
    push_item(DisplayRequest::swap_visible_buffers);
}

void SDLContext::show_blend_transition(const SDL_Rect &wrkarea, const SDL_Rect &prev,
        const SDL_Rect &next, float phase) {
    std::lock_guard _(_mx);
    signal_push();
    push_item(DisplayRequest::blend_transition);
    push_item(BlendTransitionReq{wrkarea, prev, next, phase});
}

SDL_Rect SDLContext::get_window_aspect_rect() const {
    SDL_Rect w;
    int ww;
    int wh;
    SDL_GetWindowSizeInPixels(_window.get(), &ww, &wh);
    int apw  = wh * 4 / 3;
    int aph =  ww * 3 / 4;
    int fw;
    int fh;
    if (apw > ww) {
        fw = ww;
        fh = aph;
    } else {
        fw = apw;
        fh = wh;
    }
    w.h = fh;
    w.w = fw;
    w.x = (ww - fw)/2;
    w.y = (wh - fh)/2;
    return w;
}

SDL_Point SDLContext::to_window_point(const SDL_Rect &winrc, const SDL_Point &pt) {
    return {
        pt.x * winrc.w / 640 + winrc.x,
        pt.y * winrc.h / 480 + winrc.y,
    };
}

SDL_Point SDLContext::to_source_point(const SDL_Rect &winrc, const SDL_Point &win_pt)  {
    return {
        (win_pt.x - winrc.x) * 640 / winrc.w,
        (win_pt.y - winrc.y) * 480 / winrc.h,
    };
}

int SDLContext::transition_int(int beg, int end, float phase) {
    int w = end - beg;
    return beg + static_cast<int>(std::round(w * phase));
}

SDL_Rect SDLContext::transition_rect(const SDL_Rect &beg, const SDL_Rect &end, float phase) {
    SDL_Rect out;
    out.x = transition_int(beg.x, end.x, phase);
    out.y = transition_int(beg.y, end.y, phase);
    out.w = transition_int(beg.w, end.w, phase);
    out.h = transition_int(beg.h, end.h, phase);
    return out;
}


SDL_Rect SDLContext::to_window_rect(const SDL_Rect &winrc, const SDL_Rect &source_rect) {
    SDL_Point pt1(source_rect.x, source_rect.y);
    SDL_Point pt2(source_rect.x+source_rect.w, source_rect.y+source_rect.h);
    SDL_Point wpt1(to_window_point(winrc, pt1));
    SDL_Point wpt2(to_window_point(winrc, pt2));
    return {wpt1.x, wpt1.y, wpt2.x - wpt1.x, wpt2.y - wpt1.y};



}
