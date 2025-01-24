//!!!! POZOR, NUTNE LINKOVAT SOUBOR LZWA.ASM

typedef void (*MGIF_PROC)(int,void *); //prvni cislo akce, druhy data akce

#define MGIF "MGIF"
#define MGIF_Y "97"
#define VER 0x100
#define MGIF_EMPTY  0
#define MGIF_LZW    1
#define MGIF_DELTA  2
#define MGIF_PAL    3
#define MGIF_SOUND  4
#define MGIF_TEXT   5
#define MGIF_COPY   6
#define MGIF_SINIT  7


void mgif_install_proc(MGIF_PROC proc);
void *open_mgif(void *mgif); //vraci ukazatel na prvni frame
void *mgif_play(void *mgif); //dekoduje a zobrazi frame
void close_mgif();           //dealokuje buffery pro prehravani
