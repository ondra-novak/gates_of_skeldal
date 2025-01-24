void edit_sector(int source);
void open_sector_win(void);
void info_sector(int sector);
void open_draw_win(void);
long get_draw_flags(void);
void edit_side(int source,int smer);
void sector_details_call(int sect_num);
void chozeni2(EVENT_MSG *msg,OBJREC *o);
void jdi_na_sektor(int s);


void string_list_sup();
void str_line(OBJREC *o);
char *strs(int num);
long vals(int num);
void test_int(void);
void fill_bit_fields(int win,int id_start,int source,int count);
long get_bit_fields(int win,int id_start,int count);
void value_store(OBJREC *o);


