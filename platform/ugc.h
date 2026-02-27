#include <time.h>

#ifdef __cplusplus 
extern "C" {
#endif

typedef struct  {
    const char *name;
    const char *ddl_path;
    const char *author;
    const char *lang;
    time_t last_played;    
} UGCItem;

typedef struct tag_UGCManager UGCManager;

void UGCSetLocalFoler(const char *path);

void UGC_GetList(void (*callback)(const UGCItem *items, unsigned int count, void *context), void *context);

void UGC_StartPlay(const char *ddl_path);




#ifdef __cplusplus 
}
#endif
