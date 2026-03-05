
#include <stdint.h>
#ifdef __cplusplus 
extern "C" {
#endif

typedef struct  {
    const char *name;
    const char *ddl_path;
    const char *author;
    const char *lang;
    uint64_t id;
} UGCItem;

typedef struct tag_UGCManager UGCManager;


void UGC_GetList(const char *ugc_user_path, 
                 const char *ugc_dlc_path,
                 void (*callback)(const UGCItem *items, unsigned int count, void *context), void *context);






#ifdef __cplusplus 
}
#endif
