extern char filename[MAX_PATH];

int load_map(char *filename);
int save_map(char *filename);
int save_all_map(void);

long save_section(FILE *f,void *section, int sct_type,long sect_size);
long load_section(FILE *f,void **section, int *sct_type,long *sect_size);
int save_scr_list(FILE *f,TSTR_LIST names,int scr_id);
char check_sound_map();
char *set_password(char *text);
char check_password(char *text);
void encrypt(char *password);

