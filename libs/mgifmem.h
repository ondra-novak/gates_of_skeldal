//!!!! POZOR, NUTNE LINKOVAT SOUBOR LZWA.ASM
#ifndef _MGIFMEM_H



#define _MGIFMEM_H
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

#define SMD_256 1
#define SMD_HICOLOR 2

typedef struct mgif_header
    {
    char sign[4];
    char year[2];
    char eof;
    word ver;
    int32_t frames;
    word snd_chans;
    int32_t snd_freq;
    short ampl_table[256];
    const void *nx_frame;
    int32_t cur_frame;
    short accnums[2];
    int32_t sound_write_pos;
    }MGIF_HEADER_T;

    typedef void (*MGIF_PROC)(MGIF_HEADER_T *hdr, int,const void *,int csize); //prvni cislo akce, druhy data akce


void mgif_install_proc(MGIF_PROC proc);
const void *open_mgif(const void *mgif); //vraci ukazatel na prvni frame
char mgif_play(const void *mgif); //dekoduje a zobrazi frame
void close_mgif(const void *mgif);           //dealokuje buffery pro prehravani
#endif
