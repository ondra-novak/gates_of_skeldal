#include "steam_c_api.h"
#include "platform/sdl/BGraph2.h"

#ifdef STEAM_ENABLED
#include "platform/steam/steamservice.hpp"

static std::unique_ptr<SteamService> steam_service;

void initialize_steam_client() {

    steam_service = std::make_unique<SteamService>();
    if (steam_service->is_available()) {
        steam_service.reset();
        set_steam_callback(nullptr);
    } else {
        set_steam_callback([]{
            steam_service->run_callbacks();
        });
    }
}


int8_t set_achievement(const char *id)
{
    if (steam_service) {
        return steam_service->set_achievement(id)?0:-1;
    }
    return -1;
}

int8_t clear_achievement(const char *id)
{
    if (steam_service) {
        return steam_service->clear_achievement(id)?0:-1;
    }
    return -1;
}

char is_steam_available()
{
    return steam_service?1:0;
}

#else
void initialize_steam_client() {}
int8_t set_achievement(const char *id) {return -1;}
int8_t clear_achievement(const char *id) {return -1;}
char is_steam_available() {return 0;}
#endif
