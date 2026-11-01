#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform achievement service abstraction.
 *
 * Exactly one backend is compiled in, selected by the CMake option
 * ACHIEVEMENTS_BACKEND (steam | galaxy | none):
 *   achievements_steam.cpp  - Steamworks (Steam release)
 *   achievements_galaxy.cpp - GOG Galaxy SDK (GOG release)
 *   achievements_none.cpp   - no-op stubs
 *
 * All functions are safe to call when the service is not running;
 * the game must never depend on their success.
 */

/// Initialize the achievement service (if available). Optional to call.
/// If not called manually, it is auto-initialized on first set_achievement().
void achievements_init(void);
void achievements_shutdown(void);

/// Unlock an achievement by its API name (ignored when service is unavailable)
/**
 * @param id The API name of the achievement.
 * @return 0 on success (or queued), -1 service is not running (diagnostic only)
 */
int8_t set_achievement(const char *id);

/// Clear an achievement by its API name, or "all" (debug/testing only)
/**
 * @param id The API name of the achievement, or "all".
 * @return 0 on success, -1 service is not running (diagnostic only)
 */
int8_t clear_achievement(const char *id);

/// returns non-zero when the achievement service was initialized successfully
char achievements_available(void);

/// Short backend name: "steam", "galaxy" or "none"
const char *achievements_backend_name(void);

/// Human readable diagnostic text for the debug console.
/// Returned buffer is malloc'ed, caller must free() it. NULL when no backend.
char *achievements_status(void);

#ifdef __cplusplus
}
#endif
