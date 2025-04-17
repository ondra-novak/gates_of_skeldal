#ifndef SKELDAL_ACH_EVENTS_
#define SKELDAL_ACH_EVENTS_


typedef struct thuman THUMAN;


void ach_event_kill_monster(int id);
void ach_event_pick_item(int id);
void ach_event_end_game();
void ach_event_inv_add(int id);
void ach_event_dialog_paragraph(int pgf);
void ach_event_full_party();

void enable_achievements(char enable);

#endif
