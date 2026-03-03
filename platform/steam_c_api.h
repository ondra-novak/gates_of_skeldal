#ifndef _STEAM_C_API_SKELDAL
#define _STEAM_C_API_SKELDAL

#include "SDL_stdinc.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


void initialize_steam_client();
void shutdown_steam_client();

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

typedef struct tag_workshop_upload_state {
    char done;
    char need_legal;
    const char *message;
    Uint32 upload_bytes;
    Uint32 total_bytes;
    int stage;
    char *buffer;
} TWORKSHOP_UPLOAD_STATE;

typedef void (*workshop_update_cb)(char running, const char *message, uint64_t upload_bytes, uint64_t total_bytes, void *context);

void steam_upload_to_workshop(const char *file, workshop_update_cb callback, void *context);


#ifdef __cplusplus
}
#endif
#endif
