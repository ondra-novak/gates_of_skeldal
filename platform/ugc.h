#include <time.h>

#ifdef __cplusplus 
extern "C" {
#endif

typedef struct  {
    const char *name;
    const char *ddl_path;
    const char *author;
    time_t last_played;    
} UGCItem;

typedef struct tag_UGCManager UGCManager;


void UGCSetLocalFoler(const char *path);
UGCManager *UGC_create();
size_t UGC_Fetch(UGCManager *manager);
UGCItem UGC_GetItem(UGCManager *manager, size_t pos);
void UGC_StartPlay(UGCManager *manager, size_t pos);
void UGC_Destroy(UGCManager *inst);




#ifdef __cplusplus 
}
#endif
