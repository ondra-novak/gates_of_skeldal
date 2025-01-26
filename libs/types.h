#define byte char
#define integer signed short
#define word unsigned short
#define longint int32_t


#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#define PATH_SEPARATOR_CHR '\\'
#else
#define PATH_SEPARATOR "/"
#define PATH_SEPARATOR_CHR '/'
#endif
