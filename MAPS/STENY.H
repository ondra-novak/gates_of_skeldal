#define T_LIST_SIZE 20
typedef char t_side_names[256][T_LIST_SIZE];


extern TSTR_LIST side_flgs;
extern TSTR_LIST side_names;
extern TSTR_LIST floors;
extern TSTR_LIST ceils;
extern TSTR_LIST oblouky;
extern TSTR_LIST vzhled_veci;
extern TSTR_LIST pohledy_veci;
extern TSTR_LIST dlg_names;
extern TSTR_LIST weapons;
extern TSTR_LIST weapons_pos;
extern TSTR_LIST mob_procs;
extern TSTR_LIST wall_procs;
extern int *dlg_pgfs;

#define NSID "SID"
#define NFLR "FLR"
#define NCEI "CEI"
#define NOBL "OBL"
#define SCR ".SCR"

int string_list(char *c,int akt);
void read_side_list(char *filename,TSTR_LIST *outpt,int relative,int structlen);
void read_dlg_list(char *filename,TSTR_LIST *outpt,int **nums);
TSTR_LIST build_static_list(char *c);
int pgf2name(int num);
void read_side_script_one(char *filename,char *nick,TSTR_LIST *outpt,int relative,int structlen);
void read_full_side_script(char *filename);
void read_spec_procs();

