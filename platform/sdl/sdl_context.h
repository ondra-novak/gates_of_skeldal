#pragma once

#include <memory>
#include <optional>
#include <SDL2/SDL.h>
#include <thread>
#include <vector>
#include <mouse.h>

#include <queue>

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
    void swap_render_buffers();
    void swap_display_buffers();
    void show_blend_transition(const SDL_Rect &wrkarea, const SDL_Rect &prev, const SDL_Rect &next, float phase);
    void show_slide_transition(const SDL_Rect &visible_from, const SDL_Rect &visible_where,
                               const SDL_Rect &hidden_from, const SDL_Rect &hidden_where);

    MS_EVENT getMsEvent()  {
        std::lock_guard _(_mx);
        MS_EVENT out = ms_event;
        ms_event.event = 0;
        return out;
    }

    bool get_shift_key_state() const {
        return _key_shift;
    }
    bool get_control_key_state() const {
        return _key_control;
    }
    bool get_capslock_state() const {
        return _key_capslock;
    }

    bool is_keyboard_ready() const;
    std::uint16_t pop_keyboard_code() ;


protected:

    struct SDL_Deleter {
        void operator()(SDL_Window *);
        void operator()(SDL_Renderer *);
        void operator()(SDL_Surface *);
        void operator()(SDL_Texture *);
    };

    struct BlendTransitionReq {
        SDL_Rect wrkarea;
        SDL_Rect prev;
        SDL_Rect next;
        float phase;
    };

    struct SlideTransitionReq {
        SDL_Rect visible_from;
        SDL_Rect visible_where;
        SDL_Rect hidden_from;
        SDL_Rect hidden_where;

    };

    enum class DisplayRequest {
        update,
        swap_render_buffers,
        swap_visible_buffers,
        blend_transition,
        slide_transition
    };


    MS_EVENT ms_event;
    mutable std::mutex _mx;

    std::unique_ptr<SDL_Window, SDL_Deleter> _window;
    std::unique_ptr<SDL_Renderer, SDL_Deleter> _renderer;
    std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;
    std::unique_ptr<SDL_Texture, SDL_Deleter> _texture2;
    std::unique_ptr<SDL_Texture, SDL_Deleter> _crt_effect;
    SDL_Texture *_visible_texture;
    SDL_Texture *_hidden_texture;

    std::jthread _render_thread;

    bool _quit_requested = false;
    bool _fullscreen_mode = false;
    bool _present = false;
    std::atomic<bool> _key_control = false;
    std::atomic<bool> _key_shift = false;
    std::atomic<bool> _key_capslock = false;


    std::vector<char> _display_update_queue;
    using QueueIter = const char *;
    std::queue<uint16_t> _keyboard_queue;

    Uint32 _update_request_event;


    void event_loop(std::stop_token stp);
    void update_screen();


    template<typename T>
    requires(std::is_trivially_copy_constructible_v<T>)
    void push_item(const T &item);
    void push_item(const std::string_view &item);
    void push_update_msg(const SDL_Rect &rc, const uint16_t *data, int pitch);
    void push_swap_buffers();

    template<typename T>
    requires(std::is_trivially_copy_constructible_v<T>)
    void pop_item(QueueIter &iter, T &item);
    std::string_view pop_data(QueueIter &iter, std::size_t size);

    SDL_Rect get_window_aspect_rect() const;
    static SDL_Rect to_window_rect(const SDL_Rect &winrc, const SDL_Rect &source_rect) ;
    static SDL_Point to_window_point(const SDL_Rect &win_rec, const SDL_Point &pt) ;
    static SDL_Point to_source_point(const SDL_Rect &win_rec, const SDL_Point &win_pt) ;
    static SDL_Rect transition_rect(const SDL_Rect &beg, const SDL_Rect &end, float phase);
    static int transition_int(int beg, int end, float phase);
    void generateCRTTexture(SDL_Renderer* renderer, SDL_Texture** texture, int width, int height);
    void signal_push();



};
