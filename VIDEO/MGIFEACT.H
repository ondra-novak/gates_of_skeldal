#ifndef _MGIFEACT_H
#include <libs/mgifmem.h>

#define _MGIFEACT_H

#define EX_NO_ERROR 0
#define EX_NOT_FOUND -1
#define EX_NO_SOUND -2
#define EX_READ_ERROR -3
#define EX_SOUND_ERROR -4

typedef struct frame_defs
  {
  long frame_start;
  long display_start;
  long pal_start;
  long sound_start;
  word displ_type;
  long display_size;
  long track_size;
  char changed;
  char last_vol[2];
  long vol_save;
  }FRAME_DEFS_T;

extern FRAME_DEFS_T *mgf_frames;
extern MGIF_HEADER_T mgf_header;
extern char *mgif_filename;
extern long total_frames;
extern int samples_total;
extern word frame_shift;

int examine_mgif_file(char (*progress)(int perc));
void set_mgif(char *filename);

typedef struct track_data_t
  {
  char vpoint1;
  char vpoint2;
  char changed;
  int time;  //0 odpovida restartu. Pak vpointy neplati
  struct track_data_t *next;
  }TRACK_DATA_T;

typedef struct track_info_t
  {
  char *sample_name;
  char *user_name;
  int loop_start;
  int loop_end;
  TRACK_DATA_T *levy,*pravy;
  int *starts;
  int starts_count;
  char muted;
  }TRACK_INFO_T;

extern TRACK_INFO_T *smp_prg;

int add_sample(char *sample_name);
void remove_sample(int id);
void build_sample_list(void *list);
char get_vpoint(TRACK_DATA_T *track,int frame,TRACK_DATA_T *vp); //neexistuje-li spocita se (return 1)
void set_vpoint(TRACK_DATA_T **track,int frame,int vp1,int vp2); //pokud neexistuje tak se vytvori
void add_vpoint(TRACK_DATA_T **track,int frame);
void add_restart(TRACK_INFO_T *ti,int frame);
void delete_restart(TRACK_INFO_T *ti,int frame);
void delete_vpoint(TRACK_DATA_T **track,int frame);
void change_vpoint_spacing(TRACK_DATA_T *track,int frame,int value);

void lzw_decode(void *source,char *target);
void init_lzw_compressor(int dic_size);
void reinit_lzw();
void done_lzw_compressor();

void preview_block(int start_frame,int x, int y);
void stop_preview();
void build_frame(int frame_num,int *exit);//TASK!
#pragma aux build_frame parm[]

void save_project(char *project_name);
int load_project(char *prj_name);
void *get_project_name(char *name);

void read_vol_table(TRACK_INFO_T *tr);
void compare_vol_table(TRACK_INFO_T *tr);
void set_vol_table_nul(void);

void set_changed_full_track(TRACK_INFO_T *tr);
void warn_size_mistmach(void);

int get_inserted_frames(int frame); //tracuje film a odhaduje kolik framu bylo pridano
int get_deleted_frames(void);
void insert_global(int frame,int count); //prida count framu od framu frame.
void delete_global(int frame,int count); //prida count framu od framu frame.



#endif
