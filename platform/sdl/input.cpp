#include "global_context.h"
#include "input.h"
#include "../platform.h"

char get_control_key_state() {
    return get_sdl_global_context().get_control_key_state()?1:0;
}
char get_shift_key_state() {
    return get_sdl_global_context().get_shift_key_state()?1:0;
}
char get_capslock_state() {
    return get_sdl_global_context().get_capslock_state()?1:0;
}
uint32_t _bios_keybrd(int mode) {
    if (mode == _KEYBRD_READY) {
        return get_sdl_global_context().is_keyboard_ready()?1:0;
    } else if (mode == _KEYBRD_READ) {
        return get_sdl_global_context().pop_keyboard_code();
    } else {
        return 0;
    }

}

void SetWheelMapping(char up, char down) { //todo

}



void get_ms_event(MS_EVENT *event) {
    *event = get_sdl_global_context().getMsEvent();

}

void ShareCPU() {
    if (q_is_mastertask()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}


void init_joystick(const INI_CONFIG_SECTION *section) {
    SDLContext::JoystickConfig cfg = {};
    cfg.buttons[0] = SDLContext::JoystickButton::lclick;
    cfg.buttons[1] = SDLContext::JoystickButton::rclick;
    cfg.buttons[2] = SDLContext::JoystickButton::enter;
    cfg.buttons[3] = SDLContext::JoystickButton::space;
    cfg.buttons[5] = SDLContext::JoystickButton::F1;
    cfg.buttons[4] = SDLContext::JoystickButton::F2;
    cfg.buttons_mod[4] = SDLContext::JoystickButton::F5;
    cfg.buttons[6] = SDLContext::JoystickButton::F3;
    cfg.buttons[8] = SDLContext::JoystickButton::ctrl_lclick;
    cfg.buttons[10] = SDLContext::JoystickButton::mod_key;
    cfg.buttons_mod[10] = SDLContext::JoystickButton::mod_key;
    cfg.buttons[11] = SDLContext::JoystickButton::map;
    cfg.buttons[12] = SDLContext::JoystickButton::merge;
    cfg.buttons[14] = SDLContext::JoystickButton::left;
    cfg.buttons[13] = SDLContext::JoystickButton::right;
    cfg.buttons[9] = SDLContext::JoystickButton::backspace;
    cfg.buttons_mod[5] = SDLContext::JoystickButton::escape;
    cfg.buttons_mod[9] = SDLContext::JoystickButton::F4;
    cfg.buttons_mod[14] = SDLContext::JoystickButton::F6;
    cfg.buttons_mod[2] = SDLContext::JoystickButton::F7;
    cfg.buttons_mod[3] = SDLContext::JoystickButton::F8;
    cfg.buttons_mod[1] = SDLContext::JoystickButton::F9;
    cfg.buttons_mod[0] = SDLContext::JoystickButton::F10;
    cfg.buttons_mod[11] = SDLContext::JoystickButton::F11;
    cfg.buttons_mod[14] = SDLContext::JoystickButton::F12;
    cfg.buttons_mod[13] = SDLContext::JoystickButton::backspace;
    cfg.enabled = true;
    cfg.walk_deadzone = 0x4000;
    cfg.cursor_deadzone = 1;

    cfg.enabled = ini_get_boolean(section, "enabled", 1) != 0;
    cfg.swap_axis = ini_get_boolean(section, "swap_sticks", 0) != 0;
    cfg.walk_deadzone = ini_get_int(section,"walk_deadzone", 0x4000);
    cfg.cursor_deadzone = ini_get_int(section,"cursor_deadzone", 0x800);
    auto bcount = std::distance(std::begin(cfg.buttons),std::end(cfg.buttons));
    for (std::ptrdiff_t i = 0; i < bcount; ++i) {
        char buff[100];
        {
            snprintf(buff,sizeof(buff),"button%d", static_cast<int>(i));
            const char *v = ini_get_string(section, buff, NULL);
            if (v) {
                auto n = cfg.buttons[i] = SDLContext::button_from_string(v);
                if (n == SDLContext::JoystickButton::mod_key) {
                    cfg.buttons_mod[i] = n;
                }
            }
        }
        {
            snprintf(buff,sizeof(buff),"mod+button%d", static_cast<int>(i));
            const char *v = ini_get_string(section, buff, NULL);
            if (v) {
                cfg.buttons_mod[i] = SDLContext::button_from_string(v);
            }
        }
    }
    get_sdl_global_context().configure_controller(cfg);
}

char is_joystick_used() {
    return get_sdl_global_context().is_joystick_used()?1:0;
}
char is_joystick_enabled() {
    return get_sdl_global_context().is_joystick_enabled()?1:0;
}
char copy_text_to_clipboard(const char *text) {
    return !SDL_SetClipboardText(text);
}
