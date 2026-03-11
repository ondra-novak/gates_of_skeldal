
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus 
extern "C" {
#endif

typedef struct  {
    const char *name;
    const char *ddl_path;
    const char *author;
    const char *lang;
    uint64_t id;
    char downloading;
} UGCItem;

typedef struct tag_UGCManager UGCManager;


typedef struct{
    const char *ddl;
    uint64_t id;
} UGCItemLastSave;

void UGC_GetList(const char *ugc_user_path, 
                 const char *ugc_dlc_path,
                 const UGCItemLastSave *last_save,
                 void (*callback)(const UGCItem *items, unsigned int count, void *context), void *context);


///For every relevant install event, this is increaded
size_t get_install_callback_counter();

void open_steam_workshop();
char is_steam_workshop_browser_available();
void ugc_start_play(const char *ddl_path, uint64_t id);
void start_editor();
char did_editor_exit();

#ifdef __cplusplus 
}
#endif
