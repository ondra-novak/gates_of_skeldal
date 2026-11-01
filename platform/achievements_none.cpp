#include <stddef.h>
#include "achievements.h"

/* No achievement service compiled in (ACHIEVEMENTS_BACKEND=none). */

void achievements_init(void) {}
void achievements_shutdown(void) {}
int8_t set_achievement(const char *) { return -1; }
int8_t clear_achievement(const char *) { return -1; }
char achievements_available(void) { return 0; }
const char *achievements_backend_name(void) { return "none"; }
char *achievements_status(void) { return NULL; }
