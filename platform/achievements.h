#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// Initialize Steam (if available). Optional to call.
// If not called manually, will auto-init on first achievement set.
void steam_init();
void steam_shutdown();

/// Set an achievement by its API name (if Steam available, otherwise ignored)
/**
 * @param id The API name of the achievement to set.
 * @return 0 on success, -1 steam is not running (for diagnostic only)
 */
int8_t set_achievement(const char* id);

// Clear an achievement by its API name (if Steam available, otherwise ignored)
/**
 * @param id The API name of the achievement to set.
 * @return 0 on success, -1 steam is not running (for diagnostic only)
 */
int8_t clear_achievement(const char* id);

/// returns whether steam is available
/**
 * @return 
 */
char is_steam_available();

char *get_steam_status();

#ifdef __cplusplus
}
#endif