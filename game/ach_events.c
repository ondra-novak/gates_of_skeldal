#include "ach_events.h"
#include <platform/achievements.h>
#include <stdint.h>
#include <stdlib.h>
#include <libs/event.h>
#include "globals.h"

#define DEFACH(x) static const char * x = #x; static char reported_##x = 0
#define IS_ACH_REPORTED(x) (reported_##x)

DEFACH(ACH_ALL_PARTY);
DEFACH(ACH_END_GAME);
DEFACH(ACH_FIRE);
DEFACH(ACH_FIRST_KILL);
DEFACH(ACH_FOUNTAIN);
DEFACH(ACH_HAMMER);
DEFACH(ACH_HEART);
DEFACH(ACH_ORC_TREASURE);
DEFACH(ACH_SCROLL);

#define item_orc_treasure 72
#define item_scroll_victory 152
#define item_hearth_earth 150
#define item_wind_hammer 149
#define item_mitchel_water 151
#define item_eternal_fire 148



static char achievements_enabled = 0;

#define UNLOCK_ACH(x) do {if (achievements_enabled && !reported_##x) set_achievement(x); reported_##x = 1;} while(0)

void enable_achievements(char enable) {
    achievements_enabled = enable;
}

void ach_event_kill_monster(int id) {
    (void)id;
    UNLOCK_ACH(ACH_FIRST_KILL);
}
void ach_event_pick_item(int id) {
    switch (id) {
        case item_orc_treasure: UNLOCK_ACH(ACH_ORC_TREASURE); break;
        case item_eternal_fire: UNLOCK_ACH(ACH_FIRE); break;
        case item_hearth_earth: UNLOCK_ACH(ACH_HEART);break;
        case item_mitchel_water: UNLOCK_ACH(ACH_FOUNTAIN);break;
        case item_scroll_victory: UNLOCK_ACH(ACH_SCROLL);break;
        case item_wind_hammer: UNLOCK_ACH(ACH_HAMMER);break;
        default:break;        
    }
}
void ach_event_end_game() {
    UNLOCK_ACH(ACH_END_GAME);
}
void ach_event_inv_add(int id) {
    (void)id;
    //wzprintf("(event) inventory add %d\n", id);
}
void ach_event_dialog_paragraph(int pgf) {
    (void)pgf;
    //wzprintf("(event) dialog paragraph %d\n", pgf);
}
void ach_event_full_party() {
    UNLOCK_ACH(ACH_ALL_PARTY);
}
