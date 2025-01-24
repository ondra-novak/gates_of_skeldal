#define SMD_256 1
#define SMD_HICOLOR 2
#define SMD_256_FULL (1+128)
#define SMD_HICOLOR_FULL (2+128)
void play_animation(char *filename,char mode,int posy,char sound);
void set_title_list(char **titles);
void set_play_attribs(void *screen,char rdraw,char bm,char colr64);
