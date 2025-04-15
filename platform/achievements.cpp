#include "achievements.h"
#include <steam/steam_api.h>
#include "error.h"
#include <string.h>
#include <sstream>

extern "C" {
    #include <libs/event.h>
}

static char steam_available = 0;
static char steam_initialized = 0;


void run_steam_callbacks(const EVENT_MSG *msg,void **)  {
    if (msg->msg == E_IDLE) {
        SteamAPI_RunCallbacks();
    }
}

void achievements_init()
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

void achievements_shutdown()
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

    if (SteamUserStats() && SteamUserStats()->ClearAchievement(id)) {
        SteamUserStats()->StoreStats();
        return 0;
    } else {
        return -1;
    }    
}

char is_steam_available()
{
    return steam_available;
}

char *get_steam_status()
{
    std::ostringstream oss;
    int num_achievements = SteamUserStats()->GetNumAchievements();
    oss << "SteamAPI_Init: " << ( SteamAPI_Init() ? "success" : "failure") << "\n";
    oss << "UserStats pointer: " <<  SteamUserStats() << "\n";
    oss << "Is Steam overlay enabled:" << (SteamUtils()->IsOverlayEnabled() ? "yes" : "no") <<  "\n";
    oss << "AppID: "<< SteamUtils()->GetAppID() << "\n";
    oss << "Num Achievements: " << num_achievements << "\n";
    for (int i = 0; i < num_achievements; ++i) {
        const char* name = SteamUserStats()->GetAchievementName(i);
        bool achieved = false;
        SteamUserStats()->GetAchievement(name, &achieved);

        oss << "[" << i << "] " << name << " - " << (achieved ? "Yes" : "No") << "\n";
    }

    std::string str = oss.str();
    char *out  = strdup(str.c_str());
    return out;
}
