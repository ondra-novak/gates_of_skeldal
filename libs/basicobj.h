
#define E_STATUS_LINE 60

extern word *msg_box_font;
extern word *msg_icn_font;

int msg_box(char *title, char icone, char *text, ... );


void highlight(CTL3D *c,word color);
CTL3D *def_border(int btype,int color);
void xor_rectangle(int x,int y,int xs,int ys);

// status lines
void status_line(EVENT_MSG *msg,T_EVENT_ROOT **user_data);
void *status_mem_info(EVENT_MSG *msg);
void *mouse_xy(EVENT_MSG *msg);
void *show_time(EVENT_MSG *msg);

// objects
//void sample(OBJREC *o);
void button(OBJREC *o);
void win_label(OBJREC *o);
void check_box(OBJREC *o);
void radio_butts(OBJREC *o);
void toggle_button(OBJREC *o);
void input_line(OBJREC *o);
void label(OBJREC *o);
void mid_label(OBJREC *o);
void scroll_bar_v(OBJREC *o);
void scroll_button(OBJREC *o);
void scroll_support();
void scroll_bar_h(OBJREC *o);
void button2(OBJREC *o);
void resizer(OBJREC *o);


