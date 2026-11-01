#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "achievements.h"
#include "platform.h"
#include <galaxy/GalaxyApi.h>
#include "gog_credentials.h"

extern "C" {
    #include <libs/event.h>
}

/* GOG Galaxy backend (ACHIEVEMENTS_BACKEND=galaxy).
 *
 * Flow (all asynchronous, driven by galaxy::api::ProcessData() on E_IDLE):
 *   Init()  ->  SignInGalaxy()  ->  OnAuthSuccess
 *           ->  RequestUserStatsAndAchievements()  ->  OnUserStatsAndAchievementsRetrieveSuccess
 *           ->  stats_ready = true, queued unlocks are flushed
 *
 * Unlocks requested before stats are ready are queued. Nothing here is fatal:
 * without the Galaxy client the game simply runs without achievements
 * (GOG DRM-free requirement). The SDK reports errors through GetError(),
 * it does not throw.
 */

namespace {

using namespace galaxy::api;

char galaxy_initialized = 0;     // achievements_init() has run
char galaxy_available = 0;       // Init() succeeded
bool signed_in = false;
bool stats_ready = false;
bool auth_failed = false;   // sign-in failed; stop queueing unlocks nobody will flush
std::vector<std::string> pending_unlocks;
std::string last_error;

bool check_error(const char *where) {
    const IError *err = GetError();
    if (!err) return false;
    last_error = std::string(where) + ": " + err->GetName() + " - " + err->GetMsg();
    return true;
}

const char *auth_failure_name(IAuthListener::FailureReason r) {
    switch (r) {
        case IAuthListener::FAILURE_REASON_GALAXY_SERVICE_NOT_AVAILABLE: return "Galaxy service not available (client not installed or not running)";
        case IAuthListener::FAILURE_REASON_GALAXY_SERVICE_NOT_SIGNED_IN: return "Galaxy client not signed in";
        case IAuthListener::FAILURE_REASON_CONNECTION_FAILURE: return "connection failure";
        case IAuthListener::FAILURE_REASON_NO_LICENSE: return "no license";
        case IAuthListener::FAILURE_REASON_INVALID_CREDENTIALS: return "invalid client credentials";
        case IAuthListener::FAILURE_REASON_GALAXY_NOT_INITIALIZED: return "Galaxy not initialized";
        case IAuthListener::FAILURE_REASON_EXTERNAL_SERVICE_FAILURE: return "external service failure";
        default: return "undefined";
    }
}

bool store_now() {
    Stats()->StoreStatsAndAchievements();
    return !check_error("StoreStatsAndAchievements");
}

void flush_pending() {
    if (pending_unlocks.empty()) return;
    for (const std::string &id : pending_unlocks) {
        Stats()->SetAchievement(id.c_str());
        check_error("SetAchievement (queued)");
    }
    pending_unlocks.clear();
    store_now();
}

class AuthListener : public SelfRegisteringListener<IAuthListener> {
    void OnAuthSuccess() override {
        signed_in = true;
        auth_failed = false;
        Stats()->RequestUserStatsAndAchievements();
        check_error("RequestUserStatsAndAchievements");
    }
    void OnAuthFailure(FailureReason reason) override {
        signed_in = false;
        auth_failed = true;
        pending_unlocks.clear();
        last_error = std::string("SignInGalaxy failed: ") + auth_failure_name(reason);
    }
    void OnAuthLost() override {
        signed_in = false;
        stats_ready = false;
        auth_failed = true;
        pending_unlocks.clear();
        last_error = "Galaxy authentication lost";
    }
};

class StatsRetrieveListener : public SelfRegisteringListener<IUserStatsAndAchievementsRetrieveListener> {
    void OnUserStatsAndAchievementsRetrieveSuccess(GalaxyID) override {
        stats_ready = true;
        flush_pending();
    }
    void OnUserStatsAndAchievementsRetrieveFailure(GalaxyID, FailureReason reason) override {
        stats_ready = false;
        std::ostringstream oss;
        oss << "RequestUserStatsAndAchievements failed, reason " << static_cast<int>(reason);
        last_error = oss.str();
    }
};

class StoreListener : public SelfRegisteringListener<IStatsAndAchievementsStoreListener> {
    void OnUserStatsAndAchievementsStoreSuccess() override {}
    void OnUserStatsAndAchievementsStoreFailure(FailureReason reason) override {
        std::ostringstream oss;
        oss << "StoreStatsAndAchievements failed, reason " << static_cast<int>(reason);
        last_error = oss.str();
    }
};

AuthListener *auth_listener = nullptr;
StatsRetrieveListener *stats_listener = nullptr;
StoreListener *store_listener = nullptr;

void run_galaxy_callbacks(const EVENT_MSG *msg, void **) {
    if (msg->msg == E_IDLE) {
        ProcessData();
    }
}

} // namespace


void achievements_init(void)
{
    if (galaxy_initialized) return;
    galaxy_initialized = 1;

    Init(InitOptions(GOG_CLIENT_ID, GOG_CLIENT_SECRET));
    if (check_error("Init")) {
        galaxy_available = 0;
        return;
    }
    galaxy_available = 1;

    // Listeners register themselves with the ListenerRegistrar (requires Init() done)
    auth_listener = new AuthListener;
    stats_listener = new StatsRetrieveListener;
    store_listener = new StoreListener;

    send_message(E_ADD, E_IDLE, &run_galaxy_callbacks);

    // requireOnline=false: offline sign-in through the local client is enough,
    // achievements are cached by the client and synced later.
    User()->SignInGalaxy(false, 15);
    check_error("SignInGalaxy");
}

void achievements_shutdown(void)
{
    if (galaxy_available) {
        send_message(E_DONE, E_IDLE, &run_galaxy_callbacks);
        // listeners must be unregistered before Shutdown()
        delete auth_listener;  auth_listener = nullptr;
        delete stats_listener; stats_listener = nullptr;
        delete store_listener; store_listener = nullptr;
        Shutdown();
    }
    galaxy_available = 0;
    galaxy_initialized = 0;
    signed_in = false;
    stats_ready = false;
    auth_failed = false;
    pending_unlocks.clear();
}

int8_t set_achievement(const char *id)
{
    achievements_init();
    if (!galaxy_available) return -1;

    if (!stats_ready) {
        // nothing will ever flush the queue once sign-in has failed
        if (auth_failed) return -1;
        // sign-in / stats retrieval still in progress - unlock once ready
        if (std::find(pending_unlocks.begin(), pending_unlocks.end(), id) == pending_unlocks.end()) {
            pending_unlocks.push_back(id);
        }
        return 0;
    }

    Stats()->SetAchievement(id);
    if (check_error("SetAchievement")) return -1;
    return store_now() ? 0 : -1;
}

int8_t clear_achievement(const char *id)
{
    achievements_init();
    if (!galaxy_available || !stats_ready) return -1;

    if (istrcmp(id, "all") == 0) {
        uint32_t cnt = Stats()->GetAchievementsNumber();
        if (check_error("GetAchievementsNumber")) return -1;
        for (uint32_t i = 0; i < cnt; ++i) {
            char name[256];
            Stats()->GetAchievementNameCopy(i, name, sizeof(name));
            if (check_error("GetAchievementNameCopy")) return -1;
            Stats()->ClearAchievement(name);
            check_error("ClearAchievement");
        }
    } else {
        Stats()->ClearAchievement(id);
        if (check_error("ClearAchievement")) return -1;
    }
    return store_now() ? 0 : -1;
}

char achievements_available(void)
{
    return galaxy_available;
}

const char *achievements_backend_name(void)
{
    return "galaxy";
}

char *achievements_status(void)
{
    std::ostringstream oss;
    oss << "Backend: galaxy\n";
    oss << "Init: " << (galaxy_available ? "success" : "failure") << "\n";
    if (galaxy_available) {
        oss << "Signed in: " << (User()->SignedIn() ? "yes" : "no") << "\n";
        oss << "Logged on (online): " << (User()->IsLoggedOn() ? "yes" : "no") << "\n";
        oss << "Stats ready: " << (stats_ready ? "yes" : "no") << "\n";
        oss << "Pending unlocks: " << pending_unlocks.size() << "\n";
        if (stats_ready) {
            uint32_t cnt = Stats()->GetAchievementsNumber();
            oss << "Num Achievements: " << cnt << "\n";
            for (uint32_t i = 0; i < cnt; ++i) {
                char name[256];
                Stats()->GetAchievementNameCopy(i, name, sizeof(name));
                bool unlocked = false;
                uint32_t unlock_time = 0;
                Stats()->GetAchievement(name, unlocked, unlock_time);
                oss << "  " << name << ": " << (unlocked ? "unlocked" : "locked") << "\n";
            }
        }
    }
    if (!last_error.empty()) {
        oss << "Last error: " << last_error << "\n";
    }

    std::string str = oss.str();
    char *out = strdup(str.c_str());
    return out;
}
