#ifndef _SKELDAL_GAMESAVE_H_
#define _SKELDAL_GAMESAVE_H_

#include <stdint.h>
typedef struct load_continue_tag {
    ///active adventure ddl, can be NULL (original)
    const char *ddl;        
    ///lang selected ui lang (must not be NULL)
    const char *lang;
    ///saved game name (ignored for initialize)
    const char *save;
    ///text shown in menu (localized)
    const char *label;
    ///adventure id (when ddl is not NULL)
    uint64_t adv_id;
} TCONTINUE_GAME_INFO;

///Initialize load continue info
void initialize_load_continue(const TCONTINUE_GAME_INFO *info);
///Retrieves load continue info
/**
@return pointer to structure or NULL. If NULL returned, no continue informations are stored
If pointer returned, you must free it to release memory
 */
TCONTINUE_GAME_INFO *get_load_continue_info();
TCONTINUE_GAME_INFO *make_load_continue_info(const char *ddl, const char *lang, const char *save_name, const char *label, uint64_t id);

void save_last_contine_info(const char *save_name);
void load_map_is_dlc();

const char *get_adventure_save_subfolder();

#define SAVE_NAME_SIZE 32

typedef struct GameSlot {
    char fname[SAVE_NAME_SIZE+1];
    char label[SAVE_NAME_SIZE+1];
    char is_autosave;
    char origin_game;        
    char name_is_valid;
    char is_new_slot;
} TGAME_SAVE_SLOT;

 const char *get_slot_full_path(const TGAME_SAVE_SLOT *slot);

#endif