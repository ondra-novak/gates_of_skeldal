char all_finder();
char select_disk();
char read_script(char *filename,char *source_path,char *target_path);
long check_size(int gr);
long get_disk_free(char disk);
void view_line(OBJREC *o);
char *get_cdrom();
char get_max_res();
char do_script(int);
void done_bar(); //define(...done_bar,max);
char cascade_mkdir(char *path);
char validate_path(char *path);
char cascade_delete(char ignore_sav); //maze soubory  a podadresare. Pripadne ignoruje soubory SAV

void clean_up(); //maze zkopirovane soubory a adresare
void purge_file_list();
void add_to_list(char *name);


#define INST_MIN 0
#define INST_MED 1
#define INST_MAX 2
