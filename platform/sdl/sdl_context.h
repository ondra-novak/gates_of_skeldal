#pragma once

#include <memory>
#include <optional>
#include <SDL.h>
#include <thread>
#include <mutex>
#include <vector>
#include <libs/mouse.h>
#include <functional>
#include "unique_value.h"

#include <queue>

class SDLContext {
public:

    SDLContext();

    enum class CrtFilterType {
        none,
        autoselect,
        scanlines,
        scanlines_2,
        rgb_matrix_2,
        rgb_matrix_3

    };

    struct VideoConfig {
        int window_width;
        int window_height;
        CrtFilterType  crt_filter;
        int composer;
        const char *scale_quality;
        bool fullscreen;
        int aspect_x;
        int aspect_y;
    };

    struct AudioConfig {
        const char *audioDevice;
    };

    struct AudioInfo {
        int freq;
    };

    void init_video(const VideoConfig &config, const char *title);

    void set_window_icon(const void *icon_data, size_t icon_size);

    void close_video();

    AudioInfo init_audio(const AudioConfig &config, SDL_AudioCallback cb, void *cb_ctx);
    void pause_audio(bool pause);
    void close_audio();


    void present_rect(uint16_t *pixels, unsigned int pitch, unsigned int x, unsigned int y, unsigned int xs,unsigned ys);
    void swap_render_buffers();
    void swap_display_buffers();
    void show_blend_transition(const SDL_Rect &wrkarea, const SDL_Rect &prev, const SDL_Rect &next, float phase);
    void show_slide_transition(const SDL_Rect &visible_from, const SDL_Rect &visible_where,
                               const SDL_Rect &hidden_from, const SDL_Rect &hidden_where);

    void set_quit_callback(std::function<void()> fn);
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

    bool is_quit_requested() const {
        return _quit_requested;
    }
    void cancel_quit_request() {
        _quit_requested = false;
    }

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

    struct SDL_Audio_Deleter {
        void operator()(SDL_AudioDeviceID x);
    };




    MS_EVENT ms_event;
    mutable std::mutex _mx;
    int aspect_x = 4;
    int aspect_y = 3;
    CrtFilterType _crt_filter= CrtFilterType::autoselect;
    std::function<void()> _quit_callback;

    std::unique_ptr<SDL_Window, SDL_Deleter> _window;
    std::unique_ptr<SDL_Renderer, SDL_Deleter> _renderer;
    std::unique_ptr<SDL_Texture, SDL_Deleter> _texture;
    std::unique_ptr<SDL_Texture, SDL_Deleter> _texture2;
    std::unique_ptr<SDL_Texture, SDL_Deleter> _crt_effect;
    unique_value<SDL_AudioDeviceID, SDL_Audio_Deleter> _audio;
    SDL_Texture *_visible_texture;
    SDL_Texture *_hidden_texture;

    std::jthread _render_thread;

    bool _fullscreen_mode = false;
    bool _present = false;
    std::atomic<bool> _key_control = false;
    std::atomic<bool> _key_shift = false;
    std::atomic<bool> _key_capslock = false;
    std::atomic<bool> _quit_requested = false;



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
    void generateCRTTexture(SDL_Renderer* renderer, SDL_Texture** texture, int width, int height, CrtFilterType type);
    void signal_push();



};
