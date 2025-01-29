#include <stdarg.h>
#ifndef __EVENT_H
#define __EVENT_H
//#define nodebug  // 0 znamena ze se nealokuje udalost pro chybu
// Tato knihovna definuje zakladni systemove konstanty
// pro system hlaseni a udalosti


// zakladni

#define E_INIT 1      //inicializace udalost (interni)
#define E_ADD 2       //pridani udalosti do stromu
#define E_DONE 3      //odebrani udalosti ze stromu
#define E_IDLE 4      //udalost volana v dobe necinnosti
//#define E_GROUP 5     //vytvareni skupin udalosti
#define E_ADDEND 7    //pridani udalosti do stromu na konec.
#define E_MEMERROR 8  //udalost je vyvolana pri nedostatku pameti
                       // neni prirazena ZADNA standardni akce!!!
#define E_WATCH 6     //udalost majici prednost pred idle
                       //slouzi pro procedury sledujici cinnost hardware
#define E_PRGERROR 9  //udalost vyvolana pri chybe programu
                       //parametrem udalosti je ukazatel na int ktery oznamuje cislo chyb
                      //vysledek zapsat do tohoto intu (0-exit,1-ignorovat)
#define E_ADD_REPEAT 10       //pridani udalosti do stromu


#define ERR_MEMREF -1
#define ERR_ILLEGI -2
// zarizeni

#define E_KEYBOARD 10
#define E_MOUSE 11
#define E_TIMER 12

#define TASK_RUNNING 0
#define TASK_TERMINATING 1




//#define shift_msg(msgg,tg) ((tg.msg=*(int32_t *)msgg->data),(tg.data=(void *)((int32_t *)msgg->data+1)))






#define PROC_GROUP (EV_PROC )1


typedef struct event_msg
  {
    ///message ID
  int32_t msg;
    ///message data
  va_list data;
  }EVENT_MSG;

  typedef void (*EV_PROC)(const EVENT_MSG *,void **) ;


/* event procedura ma dva parametry

 1. je ukazatel na tzv. message, tato struktura je videt dole.
    Predava se cislo udalosti, ktera nastala a ukazatel na dalsi udaje
    tykajici se udalosti
 2. je ukazatel na ukazatel. Tyto 4 bajty jsou volnym mistem pro samotnou
    udalost. Pri instalaci ma obsah NULL. Udalost si muze volne alokovat
    pamet a pouzit ji pro svoji porebu. Pri deinstalaci se nestara o jeji
    dealokaci, o to se postara system
     (ukazatel na user_data)
 */

typedef struct t_event_point
  {
  EV_PROC proc;
  void *user_data;
  char nezavora;
  char nezavirat;
  int calls;
  struct t_event_point *next;
  }T_EVENT_POINT;


typedef struct t_event_root
  {
  int event_msg;
  int used;
  struct t_event_root *next;
  T_EVENT_POINT *list;
  }T_EVENT_ROOT;

extern char exit_wait; // 1 - opousti aktivni cekaci event;
extern char freeze_on_exit; //1 - po opusteni udalosti cela cesta uzamcena
extern char *otevri_zavoru;
//extern int curtask;
//extern char *task_info;

///copies message
static inline EVENT_MSG clone_message(EVENT_MSG *msg) {
    EVENT_MSG out;
    out.msg = msg->msg;
    va_copy(out.data, msg->data);
    return out;
}
///destroys copied message
static inline void destroy_message(EVENT_MSG *msg) {
    va_end(msg->data);
}

void init_events(void);
 // inicalizuje zakladni strom udalosto
void send_message(int message,...);

int send_message_to(int (*cb)(EVENT_MSG *, void *), void *ctx, int message, ...);
 // posila zpravu do stromu
void tree_basics(T_EVENT_ROOT **ev_tree,EVENT_MSG *msg);
 // pripojuje zakladni funkce brany, jako je instalace listu a jejich deinstalace
T_EVENT_ROOT *gate_basics(EVENT_MSG *msg, void **user_data);
 // implementace brany
 /* vstupuji informace, jake dostane brana pri zavolani
    vystupuji informace s jakymi musi vstoupit do stromu.
    Je li MSG = NULL byla zavolana udalost E_DESTROY a brana se musi zlikvidovat
    vysledkem funkce je ukazatel na koren stromu brany.
  */
void enter_event(T_EVENT_ROOT **tree,EVENT_MSG *msg);
 //vstupuje do stromu s udalosti (msg)

static __inline void shift_message(EVENT_MSG *msg) {
    msg->msg = va_arg(msg->data, int);
}

void do_events(void);
void escape(void);

#include <legacy_coroutines.h>
/*
void *task_sleep(void *param);
//#pragma aux task_sleep parm [eax] value [eax]

int add_task(int stack,void *name,...);
 //spusti provadeni funkce v rizenem multitaskingu (switch task)
void term_task(int id_num);
 //informuje task o ukonceni. Uloha by mela zkoncit
void shut_down_task(int id_num);
 //Nasilne ukonci ulohu
void raise_error(int error_number);

EVENT_MSG *task_wait_event(int32_t event_number);
char is_running(int id_num);
*/
void timer(EVENT_MSG *msg);


#define EVENT_HALT -1
#define EVENT_DONE -2
#define EVENT_HALT_DONE -3

#endif



