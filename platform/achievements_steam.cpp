#include <string.h>
#include <stdlib.h>
#include <sstream>
#include "achievements.h"
#include "error.h"
#include "platform.h"
#include <steam/steam_api.h>

extern "C" {
    #include <libs/event.h>
}

/* Steamworks backend (ACHIEVEMENTS_BACKEND=steam). Achievements only; no DRM,
 * no ownership check - a failed SteamAPI_Init() just disables achievements. */

static char steam_available = 0;
static char steam_initialized = 0;


static void run_steam_callbacks(const EVENT_MSG *msg,void **)  {
    if (msg->msg == E_IDLE) {
        SteamAPI_RunCallbacks();
    }
}

void achievements_init(void)
{
    if (!steam_initialized) {
        steam_initialized = 1;
        steam_available = SteamAPI_Init();
        if (steam_available) {
            send_message(E_ADD, E_IDLE, &run_steam_callbacks);
            SteamUserStats()->RequestUserStats(SteamUser()->GetSteamID());
        } else {
            steam_available = 0;
        }
    }
}

void achievements_shutdown(void)
{
    if (steam_available) {
        send_message(E_DONE, E_IDLE, &run_steam_callbacks);
        SteamAPI_Shutdown();
        steam_available = 0;
        steam_initialized = 0;
    }
}

int8_t set_achievement(const char *id)
{
    achievements_init();

    if (!steam_available) {
        return -1;
    }

    if (SteamUserStats() && SteamUserStats()->SetAchievement(id)) {
        SteamUserStats()->StoreStats();
        return 0;
    } else {
        return -1;
    }

}

int8_t clear_achievement(const char *id)
{
    achievements_init();

    if (!steam_available) {
        return -1;
    }

    if (istrcmp(id, "all") == 0) {
        unsigned int cnt = SteamUserStats()->GetNumAchievements();
        for (unsigned int i = 0; i < cnt; ++i) {
            SteamUserStats()->ClearAchievement(SteamUserStats()->GetAchievementName(i));
        }
        SteamUserStats()->StoreStats();
        return 0;
    } else {
        if (SteamUserStats() && SteamUserStats()->ClearAchievement(id)) {
            SteamUserStats()->StoreStats();
            return 0;
        } else {
            return -1;
        }
    }
}

char achievements_available(void)
{
    return steam_available;
}

const char *achievements_backend_name(void)
{
    return "steam";
}

char *achievements_status(void)
{
    std::ostringstream oss;
    oss << "Backend: steam\n";
    oss << "SteamAPI_Init: " << (steam_available ? "success" : "failure") << "\n";
    if (steam_available) {
        oss << "UserStats pointer: " <<  SteamUserStats() << "\n";
        oss << "Is Steam overlay enabled:" << (SteamUtils()->IsOverlayEnabled() ? "yes" : "no") <<  "\n";
        oss << "AppID: "<< SteamUtils()->GetAppID() << "\n";
        oss << "Num Achievements: " << SteamUserStats()->GetNumAchievements() << "\n";
    }

    std::string str = oss.str();
    char *out  = strdup(str.c_str());
    return out;
}
