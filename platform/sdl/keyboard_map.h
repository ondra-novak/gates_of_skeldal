#include <cstdint>

struct SDLKey2Bios {
    SDL_Scancode scan_code;
    uint16_t normal;
    uint16_t shift;
    uint16_t ctrl;

};

class KeyCodeMap {
public:

    template<int n>
    constexpr KeyCodeMap(const SDLKey2Bios (&codes)[n]) {
        for (const SDLKey2Bios &x: codes) {
            int idx = static_cast<int>(x.scan_code);
            normal[idx] = x.normal;
            w_ctrl[idx] = x.ctrl;
            w_shift[idx] = x.shift;
        }
    }


    constexpr uint16_t get_bios_code(SDL_Scancode scan, bool shift, bool control) const {
        if (control) return w_ctrl[static_cast<int>(scan)];
        if (shift) return w_shift[static_cast<int>(scan)];
        return normal[static_cast<int>(scan)];
    }


protected:
    uint16_t normal[SDL_NUM_SCANCODES] = {};
    uint16_t w_shift[SDL_NUM_SCANCODES] = {};
    uint16_t w_ctrl[SDL_NUM_SCANCODES] = {};
};

constexpr auto  sdl_keycode_map =  KeyCodeMap({
    {SDL_SCANCODE_A, 0x1E61, 0x1E41, 0x1E01},
    {SDL_SCANCODE_B,0x3062,0x3042,0x3002},
    {SDL_SCANCODE_C,0x2E63,0x2E43,0x2E03},
    {SDL_SCANCODE_D,0x2064,0x2044,0x2004},
    {SDL_SCANCODE_E,0x1265,0x1245,0x1205},
    {SDL_SCANCODE_F,0x2166,0x2146,0x2106},
    {SDL_SCANCODE_G,0x2267,0x2247,0x2207},
    {SDL_SCANCODE_H,0x2368,0x2348,0x2308},
    {SDL_SCANCODE_I,0x1769,0x1749,0x1709},
    {SDL_SCANCODE_J,0x246A,0x244A,0x240A},
    {SDL_SCANCODE_K,0x256B,0x254B,0x250B},
    {SDL_SCANCODE_L,0x266C,0x264C,0x260C},
    {SDL_SCANCODE_M,0x326D,0x324D,0x320D},
    {SDL_SCANCODE_N,0x316E,0x314E,0x310E},
    {SDL_SCANCODE_O,0x186F,0x184F,0x180F},
    {SDL_SCANCODE_P,0x1970,0x1950,0x1910},
    {SDL_SCANCODE_Q,0x1071,0x1051,0x1011},
    {SDL_SCANCODE_R,0x1372,0x1352,0x1312},
    {SDL_SCANCODE_S,0x1F73,0x1F53,0x1F13},
    {SDL_SCANCODE_T,0x1474,0x1454,0x1414},
    {SDL_SCANCODE_U,0x1675,0x1655,0x1615},
    {SDL_SCANCODE_V,0x2F76,0x2F56,0x2F16},
    {SDL_SCANCODE_W,0x1177,0x1157,0x1117},
    {SDL_SCANCODE_X,0x2D78,0x2D58,0x2D18},
    {SDL_SCANCODE_Y,0x1579,0x1559,0x1519},
    {SDL_SCANCODE_Z,0x2C7A,0x2C5A,0x2C1A},
    {SDL_SCANCODE_1,0x0231,0x0221,0x7800},
    {SDL_SCANCODE_2,0x0332,0x0340,0x0300},
    {SDL_SCANCODE_3,0x0433,0x0423,0x7A00},
    {SDL_SCANCODE_4,0x0534,0x0524,0x7B00},
    {SDL_SCANCODE_5,0x0635,0x0625,0x7C00},
    {SDL_SCANCODE_6,0x0736,0x075E,0x071E},
    {SDL_SCANCODE_7,0x0837,0x0826,0x7E00},
    {SDL_SCANCODE_8,0x0938,0x092A,0x7F00},
    {SDL_SCANCODE_9,0x0A39,0x0A28,0x8000},
    {SDL_SCANCODE_0,0x0B30,0x0B29,0x8100},
    {SDL_SCANCODE_MINUS,0x0C2D,0x0C5F,0x0C1F},
    {SDL_SCANCODE_EQUALS,0x0D3D,0x0D2B,0x8300},
    {SDL_SCANCODE_LEFTBRACKET,0x1A5B,0x1A7B,0x1A1B},
    {SDL_SCANCODE_RIGHTBRACKET,0x1B5D,0x1B7D,0x1B1D},
    {SDL_SCANCODE_SEMICOLON,0x273B,0x273A,0x2700},
    {SDL_SCANCODE_APOSTROPHE,0x2827,0x2822,0},
    {SDL_SCANCODE_GRAVE,0x2960,0x297E,0},
    {SDL_SCANCODE_BACKSLASH,0x2B5C,0x2B7C,0x2B1C},
    {SDL_SCANCODE_COMMA,0x332C,0x333C,0},
    {SDL_SCANCODE_PERIOD,0x342E,0x343E,0},
    {SDL_SCANCODE_SLASH,0x352F,0x353F,0},
    {SDL_SCANCODE_F1 ,0x3B00,0x5400,0x5E00},
    {SDL_SCANCODE_F2 ,0x3C00,0x5500,0x5F00},
    {SDL_SCANCODE_F3 ,0x3D00,0x5600,0x6000},
    {SDL_SCANCODE_F4 ,0x3E00,0x5700,0x6100},
    {SDL_SCANCODE_F5 ,0x3F00,0x5800,0x6200},
    {SDL_SCANCODE_F6 ,0x4000,0x5900,0x6300},
    {SDL_SCANCODE_F7 ,0x4100,0x5A00,0x6400},
    {SDL_SCANCODE_F8 ,0x4200,0x5B00,0x6500},
    {SDL_SCANCODE_F9 ,0x4300,0x5C00,0x6600},
    {SDL_SCANCODE_F10,0x4400,0x5D00,0x6700},
    {SDL_SCANCODE_F11,0x8500,0x8700,0x8900},
    {SDL_SCANCODE_F12,0x8600,0x8800,0x8A00},
    {SDL_SCANCODE_BACKSPACE,0x0E08,0x0E08,0x0E7F},
    {SDL_SCANCODE_DELETE,0x5300,0x5300,0x9300},
    {SDL_SCANCODE_DOWN,0x5000,0x5000,0x9100},
    {SDL_SCANCODE_END,0x4F00,0x4F00,0x7500},
    {SDL_SCANCODE_RETURN,0x1C0D,0x1C0D,0x1C0A},
    {SDL_SCANCODE_ESCAPE,0x011B,0x011B,0x011B},
    {SDL_SCANCODE_HOME,0x4700,0x4700,0x7700},
    {SDL_SCANCODE_INSERT,0x5200,0x5200,0x9200},
    {SDL_SCANCODE_KP_MULTIPLY,0x372A,0x9600,0x3700},
    {SDL_SCANCODE_KP_MINUS,0x4A2D,0x4A2D,0x8E00},
    {SDL_SCANCODE_KP_PLUS,0x4E2B,0x4E2B,0x4E00},
    {SDL_SCANCODE_KP_DIVIDE,0x352F,0x352F,0x9500},
    {SDL_SCANCODE_LEFT,0x4B00,0x4B00,0x7300},
    {SDL_SCANCODE_PAGEDOWN,0x5100,0x5100,0x7600},
    {SDL_SCANCODE_PAGEUP,0x4900,0x4900,0x8400},
    {SDL_SCANCODE_PRINTSCREEN,0x7200,0,0},
    {SDL_SCANCODE_RIGHT,0x4D00,0x4D00,0x7400},
    {SDL_SCANCODE_SPACE,0x3920,0x3920,0x3920},
    {SDL_SCANCODE_TAB,0x0F09,0x0F09,0x9400},
    {SDL_SCANCODE_UP,0x4800,0x4800,0x8D00},
    {SDL_SCANCODE_KP_1,0x4F31,0x4F00,0x7500},
    {SDL_SCANCODE_KP_2,0x5032,0x5000,0x9100},
    {SDL_SCANCODE_KP_3,0x5133,0x5100,0x7600},
    {SDL_SCANCODE_KP_4,0x4B34,0x4B00,0x7300},
    {SDL_SCANCODE_KP_5,0x4C35,0x4C00,0x8F00},
    {SDL_SCANCODE_KP_6,0x4D36,0x4D00,0x7400},
    {SDL_SCANCODE_KP_7,0x4737,0x4700,0x7700},
    {SDL_SCANCODE_KP_8,0x4838,0x4800,0x8D00},
    {SDL_SCANCODE_KP_9,0x4939,0x4900,0x8400},
    {SDL_SCANCODE_KP_0,0x5230,0x5200,0x9200},
    {SDL_SCANCODE_KP_PERIOD,0x532E,0x5300,0x9300},
    {SDL_SCANCODE_KP_ENTER,0x1C0D,0x1C0D,0x1C0D}
});


