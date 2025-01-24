void create_cos_tab();
int transformace(void *screen_data,char *trans_data,int nextline,int box_place0);
void zpetna_transformace(char **data,void *screen,int box_place);

typedef signed char TBOX[4][4];

extern TBOX *rbbox;
extern TBOX *gbbox;
extern TBOX *ibox;

