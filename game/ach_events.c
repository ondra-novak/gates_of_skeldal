#include "ach_events.h"
#include <stdint.h>
#include <stdlib.h>
#include <libs/event.h>
#include "globals.h"


void enable_achievements(char enable) {

}

void ach_event_kill_monster(int id) {
    wzprintf("(event) kill monster %d\n", id);
}
void ach_event_pick_item(int id) {
    wzprintf("(event) pick item %d\n", id);
}
void ach_event_end_game() {
    wzprintf("(event) end_game\n");
}
void ach_event_inv_add(int id) {
    wzprintf("(event) inventory add %d\n", id);
}
void ach_event_dialog_paragraph(int pgf) {
    wzprintf("(event) dialog paragraph %d\n", pgf);
}
void ach_event_full_party() {
    wzprintf("(event) full party\n");
}
