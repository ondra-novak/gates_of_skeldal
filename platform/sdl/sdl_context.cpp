#include "sdl_context.h"
#include "keyboard_map.h"

#include <atomic>
#include <cassert>
#include "../platform.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
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

void SDLContext::SDL_Audio_Deleter::operator()(SDL_AudioDeviceID x) {
    SDL_CloseAudioDevice(x);
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


void SDLContext::generateCRTTexture(SDL_Renderer* renderer, SDL_Texture** texture, int width, int height, CrtFilterType type) {

    if (type == CrtFilterType::autoselect) {
        if (height > 1680) type = CrtFilterType::rgb_matrix_3;
        else if (height >= 1200) type = CrtFilterType::scanlines_2;
        else type = CrtFilterType::scanlines;
    }

    if (type == CrtFilterType::scanlines || type == CrtFilterType::scanlines_2) {
        width = 32;
    }
    // Vytvoř novou texturu ve správné velikosti
    *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_MUL);


    // Zamkni texturu pro přímý přístup k pixelům
    void* pixels;
    int pitch;
    SDL_LockTexture(*texture, nullptr, &pixels, &pitch);

    Uint32* pixelArray = (Uint32*)pixels;

    if (type == CrtFilterType::scanlines) {



        Uint32 darkPixel = 0xA0A0A0FF;
        Uint32 transparentPixel = 0xFFFFFFC0;

        for (int y = 0; y < height; y++) {
            Uint32 color = ((y & 1)== 0) ? darkPixel : transparentPixel;
            for (int x = 0; x < width; x++) {
                pixelArray[y * (pitch / 4) + x] = color;
            }
        }
    }
    else if (type == CrtFilterType::scanlines_2) {


        Uint32 darkPixel = 0x808080FF;
        Uint32 transparentPixel = 0xFFFFFFE0;

        for (int y = 0; y < height; y++) {
            Uint32 color = (y % 3== 2) ? darkPixel : transparentPixel;
            for (int x = 0; x < width; x++) {
                pixelArray[y * (pitch / 4) + x] = color;
            }
        }
    } else {

        static Uint32 red_pixel = 0xFF8080A0;
        static Uint32 green_pixel = 0x80FF80A0;
        static Uint32 blue_pixel = 0x8080FFA0;
        static Uint32 dark_pixel = 0x000000C0;
        for (int y = 2; y < height; y++) {
            if (type == CrtFilterType::rgb_matrix_2) {
                for (int x = 2; x < width; x+=3) {
                    Uint32 *pos = pixelArray+y*(pitch/4)+x-2;
                    if ((y % 3) == 2) {
                        pos[0] = pos[1] = pos[2] = dark_pixel;
                    } else {
                        pos[0] = red_pixel;
                        pos[1] = green_pixel;
                        pos[2] = blue_pixel;
                    }
                }
            } else {
                for (int x = 2; x < width; x+=3) {
                    Uint32 *pos = pixelArray+y*(pitch/4)+x-2;
                    if ((y & 3) == 3) {
                        pos[0] = pos[1] = pos[2] = dark_pixel;
                    } else {
                        pos[0] = red_pixel;
                        pos[1] = green_pixel;
                        pos[2] = blue_pixel;
                    }
                }
            }
        }

    }

    // Odemkni texturu
    SDL_UnlockTexture(*texture);
}



void SDLContext::init_video(const VideoConfig &config, const char *title) {
    char buff[256];
    static Uint32 update_request_event = SDL_RegisterEvents(1);
    _update_request_event = update_request_event;

    assert(!_render_thread.joinable());


    int width = config.window_width;
    int height = config.window_height;
    aspect_x = config.aspect_x;
    aspect_y = config.aspect_y;
    _crt_filter = config.crt_filter ;


    _fullscreen_mode = config.fullscreen;

    std::atomic<bool> done = false;
    std::exception_ptr e;
    _render_thread = std::jthread([&](std::stop_token stp){
        bool err = false;
        try {
            SDL_Window *window = SDL_CreateWindow(title,
                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                        width, height, SDL_WINDOW_RESIZABLE|(_fullscreen_mode?SDL_WINDOW_FULLSCREEN_DESKTOP:0));

            if (!window) {
                snprintf(buff, sizeof(buff), "SDL Error create window: %s\n", SDL_GetError());
                throw std::runtime_error(buff);
            }

            _window.reset(window);
            SDL_Renderer *renderer = SDL_CreateRenderer(_window.get(), -1, config.composer);
            if (!renderer) {
                snprintf(buff,sizeof(buff), "Failed to create composer: %s\n", SDL_GetError());
                throw std::runtime_error(buff);
            }

            SDL_RendererInfo rinfo;
            SDL_GetRendererInfo(renderer, &rinfo);

            if (istrcmp(config.scale_quality, "auto") == 0) {
                if (rinfo.flags & SDL_RENDERER_ACCELERATED) {
                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
                }
            } else {
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, config.scale_quality);
            }

            _renderer.reset(renderer);
            SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB1555, SDL_TEXTUREACCESS_STREAMING, 640, 480);
            if (!texture) {
                snprintf(buff, sizeof(buff), "Chyba při vytváření textury: %s\n", SDL_GetError());
                throw std::runtime_error(buff);
            }
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            _texture.reset(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB1555, SDL_TEXTUREACCESS_STREAMING, 640, 480);
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

void SDLContext::close_video() {
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
            _quit_requested = true;
            if (_quit_callback) _quit_callback();
        } else if (e.type == exit_loop_event) {
            break;
        } else if (e.type == _update_request_event) {
            update_screen();
        } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    _crt_effect.reset();
                }
         } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            _key_capslock = e.key.keysym.mod & KMOD_CAPS;
            _key_shift =e.key.keysym.mod & KMOD_SHIFT;
            _key_control  =e.key.keysym.mod & KMOD_CTRL;
            if (e.type == SDL_KEYDOWN) {
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
            }
        } else if (e.type == SDL_MOUSEMOTION) {
            SDL_Point mspt(e.motion.x, e.motion.y);
            SDL_Rect winrc = get_window_aspect_rect();
            SDL_Point srcpt = to_source_point(winrc, mspt);
            ms_event.event = 1;
            ms_event.event_type |= 1;
            ms_event.x = srcpt.x;
            ms_event.y = srcpt.y;
            if (_mouse) {
                _mouse_rect.x = e.motion.x;
                _mouse_rect.y = e.motion.y;
                refresh_screen();
            }
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
            ms_event.event_type |= (1<<(shift+up));
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

void SDLContext::refresh_screen() {
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
    if (winrc.h >= 720 && _crt_filter != CrtFilterType::none) {
        if (!_crt_effect) {
            SDL_Texture *txt;
              generateCRTTexture(_renderer.get(), &txt, winrc.w, winrc.h, _crt_filter);
            _crt_effect.reset(txt);
        }
    }
    if (_mouse) {
        SDL_Rect recalc_rect = to_window_rect(winrc, _mouse_rect);
        SDL_Point f= to_window_point({0,0,winrc.w, winrc.h}, _mouse_finger);
        recalc_rect.x = _mouse_rect.x - f.x;
        recalc_rect.y = _mouse_rect.y - f.y;
        SDL_RenderCopy(_renderer.get(), _mouse.get(), NULL, &recalc_rect);
    }
    SDL_RenderCopy(_renderer.get(), _crt_effect.get(), NULL, &winrc);
    SDL_RenderPresent(_renderer.get());

}

void SDLContext::update_screen() {
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
                case DisplayRequest::show_mouse_cursor: {
                    SDL_Rect r;
                    pop_item(iter, r);
                    std::string_view data = pop_data(iter, r.w*r.h*2);
                    _mouse.reset(SDL_CreateTexture(_renderer.get(), SDL_PIXELFORMAT_ARGB1555, SDL_TEXTUREACCESS_STREAMING, r.w, r.h));
                    SDL_SetTextureBlendMode(_mouse.get(), SDL_BLENDMODE_BLEND);
                    _mouse_rect.w = r.w;
                    _mouse_rect.h = r.h;
                    SDL_UpdateTexture(_mouse.get(), NULL, data.data(), r.w*2);
                }
                break;
                case DisplayRequest::hide_mouse_cursor: {
                    _mouse.reset();
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
    refresh_screen();
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
    auto sz = _display_update_queue.size();
    _display_update_queue.resize(sz+rc.w*rc.h*2);
    short *trg = reinterpret_cast<short *>(_display_update_queue.data()+sz);
    for (int yp = 0; yp < rc.h; ++yp) {
        for (int xp = 0; xp < rc.w; ++xp) {
            *trg = data[xp] ^ 0x8000;
            ++trg;
        }
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
    SDL_GetWindowSize(_window.get(), &ww, &wh);
    if (aspect_x && aspect_y) {
        int apw  = wh * aspect_x / aspect_y;
        int aph =  ww * aspect_y / aspect_x;
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
    } else {
        w.x = 0;
        w.y = 0;
        w.w = ww;
        w.h = wh;
    }
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
void SDLContext::set_quit_callback(std::function<void()> fn) {
    _quit_callback = std::move(fn);
}

SDLContext::AudioInfo SDLContext::init_audio(const AudioConfig &config, SDL_AudioCallback cb, void *cb_ctx) {
    _audio.reset();
    SDL_AudioSpec aspec = {};
    aspec.callback = cb;
    aspec.userdata = cb_ctx;
    aspec.channels = 2;
    aspec.format = AUDIO_F32;
    aspec.freq = 44100;
    aspec.samples = 1024;
    SDL_AudioSpec obtaied = {};
    auto id = SDL_OpenAudioDevice(config.audioDevice, 0, &aspec, &obtaied, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE|SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (id < 1) {
        std::ostringstream err;
        err << "SDL Error create audio:" << SDL_GetError() << "\n";
        int n = SDL_GetNumAudioDevices(0);
        if (n == 0) err << "No audio devices are available" << "\n";
        else {
            err << "Available audio devices:";
            char sep = ' ';
            for (int i = 0; i < n; ++i) {
                err << sep << SDL_GetAudioDeviceName(i, 0);
                sep = ',';
            }
        }
        throw std::runtime_error(err.str());
    }
    _audio.reset(id);

    return {obtaied.freq};
}
void SDLContext::pause_audio(bool pause) {
    SDL_AudioDeviceID id = _audio.get();
    SDL_PauseAudioDevice(id, pause?1:0);
}

void SDLContext::close_audio() {
    _audio.reset();
}

void SDLContext::set_window_icon(const void *icon_data, size_t icon_size) {
    SDL_Surface *surface = SDL_LoadBMP_RW(SDL_RWFromConstMem(icon_data, icon_size), 1);
    if (surface == 0) {
        char buff[256];
        snprintf(buff,sizeof(buff),"Can't load icon: %s", SDL_GetError());
        display_error(buff);
        std::ofstream x("test.dat", std::ios::out|std::ios::binary|std::ios::trunc);
        x.write(reinterpret_cast<const char *>(icon_data), icon_size);
    } else {
        SDL_SetWindowIcon(_window.get(), surface);
    }
}

extern "C" {
void put_picture_ex(unsigned short x,unsigned short y,const void *p, unsigned short *target_addr, size_t pitch);
}
void SDLContext::show_mouse_cursor(const unsigned short *ms_hi_format, SDL_Point finger) {
    std::lock_guard _(_mx);
    signal_push();
    push_item(DisplayRequest::show_mouse_cursor);
    SDL_Rect rc;
    rc.w= ms_hi_format[0];
    rc.h =ms_hi_format[1];
    _mouse_finger = finger;
    push_item(rc);
    auto sz = _display_update_queue.size();
    auto imgsz = rc.w*rc.h;
    _display_update_queue.resize(sz+imgsz*2);
    unsigned short *trg = reinterpret_cast<unsigned short *>(_display_update_queue.data()+sz);
    std::fill(trg, trg+imgsz, 0x8000);
    put_picture_ex(0, 0, ms_hi_format, trg, rc.w);
    std::transform(trg, trg+imgsz, trg, [](unsigned short &x)->unsigned short {return x ^ 0x8000;});
}

void SDLContext::hide_mouse_cursor() {
    std::lock_guard _(_mx);
    signal_push();
    push_item(DisplayRequest::hide_mouse_cursor);

}
