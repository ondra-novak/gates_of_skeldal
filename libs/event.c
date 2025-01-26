#include <skeldal_win.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "event.h"
#include "devices.h"
#include <mem.h>
//#include <dpmi.h>
#include <malloc.h>
#include <bios.h>
//#include <i86.h>
#include <time.h>
#include "memman.h"
#include <setjmp.h>
#include <signal.h>
#include <assert.h>

static jmp_buf jmpenv;

#define find_event_msg(where,what,res) \
     {\
     res=(where);\
     while (res!=NULL && res->event_msg!=(what)) res=res->next;\
     }
#define find_event_proc(where,what,res) \
     {\
     res=(where);\
     while (res!=NULL && res->proc!=(what)) res=res->next;\
     }

#define find_event_msg_proc(where,xmsg,xproc,res) \
     {\
     T_EVENT_ROOT *pt;     \
     find_event_msg(where,xmsg,pt);\
     if (pt) find_event_proc(pt->list,xproc,res) else res=NULL;\
     }



char exit_wait=0;
T_EVENT_ROOT *ev_tree=NULL;
char freeze_on_exit=0;
int32_t ev_buff_msg[EVENT_BUFF_SIZE]={0};
void *ev_buff_dta[EVENT_BUFF_SIZE]={NULL};
int ev_poz=0;
char *otevri_zavoru;

void **tasklist_sp;
void **tasklist_low;
void **tasklist_top;
int *tasklist_events;
char *task_info;
int taskcount=0;
int foretask=0;
int nexttask=0;
int32_t taskparam;

int32_t err_last_stack;
void *err_to_go;


T_EVENT_ROOT *add_event_message(T_EVENT_ROOT **tree,int msg)
  {
  T_EVENT_ROOT *r,*r1;

  if (*tree==NULL)
     {
     *tree=getmem(sizeof(T_EVENT_ROOT));
     r=*tree;
     r->next=NULL;
     }
  else
     {
     r=getmem(sizeof(T_EVENT_ROOT));
     r1=*tree;
     while (r1->next!=NULL) r1=r1->next;
     r->next=NULL;
     r1->next=r;
     }
  r->event_msg=msg;
  //r->used=0;
  r->list=NULL;
  return r;
  }

T_EVENT_POINT *add_event(T_EVENT_ROOT **tree,int msg,EV_PROC proc,char end)
  {
  T_EVENT_ROOT *r;
  T_EVENT_POINT *p;

  find_event_msg(*tree,msg,r);
  if (r==NULL)
        {
        r=add_event_message(tree,msg);
        p=r->list=New(T_EVENT_POINT);
        p->next=NULL;
        }
  else if (end && r->list!=NULL)
     {
     T_EVENT_POINT *q=r->list;
     p=getmem(sizeof(T_EVENT_POINT));
     while (q->next!=NULL) q=q->next;
     p->next=NULL;
     q->next=p;
     }
  else
     {
     p=getmem(sizeof(T_EVENT_POINT));
     p->next=r->list;
     r->list=p;
     }
  p->proc=proc;
  p->nezavora=1;
  p->nezavirat=0;
  p->user_data=NULL;
  p->calls=0;
  return p;
  }

void delete_event_msg(T_EVENT_ROOT **tree,int msg)
  {
  T_EVENT_ROOT *r;

  r=*tree;
  if (r==NULL) return;
  if (r->event_msg==msg)
     {
     if (r->used) return;
     *tree=r->next;
     free(r);
     }
  else
     {
     T_EVENT_ROOT *p;
     while ((p=r->next)!=NULL && p->event_msg!=msg) r=p;
     if (p!=NULL)
        {
        if (p->used) return;
        r->next=p->next;
        free(p);
        }
     }
  }

void delete_event(T_EVENT_ROOT **tree,int msg,EV_PROC proc)
  {
  T_EVENT_ROOT *r;
  T_EVENT_POINT *p;

  find_event_msg(*tree,msg,r);
  if (r==NULL) return;
  p=r->list;
  if (p->proc==proc)
     {
     r->list=p->next;
     free(p);
     }
  else
     {
     T_EVENT_POINT *q;
     while ((q=p->next)!=NULL && q->proc!=proc) p=q;
     if (q!=NULL)
        {
        p->next=q->next;
        free(q);
        }
     }
  if (r->list==NULL) delete_event_msg(tree,msg);
  }

void force_delete_curr (T_EVENT_ROOT **tree,T_EVENT_ROOT *r, T_EVENT_POINT *p)
  {
  T_EVENT_POINT *q;


  q=r->list;
  if (q==p)
     {
     r->list=p->next;
     free(p);
     tree;
     if (r->list==NULL) delete_event_msg(tree,r->event_msg);
     }
  else
     {
     while (q->next!=p) q=q->next;
     q->next=p->next;
     free(p);
     }
  }

/*
static void unsuspend_task(EVENT_MSG *msg)
  {
  int i;
  int nt;

  nt=nexttask;
  for(i=1;i<taskcount;i++) if (tasklist_sp[i]!=NULL && task_info[i] & TASK_EVENT_WAITING && tasklist_events[i]==msg->msg)
     {
     nexttask=i;
     task_info[nexttask]&=~TASK_EVENT_WAITING;
     task_sleep(msg->data);
     }
  nexttask=nt;
  }
*/
void enter_event(T_EVENT_ROOT **tree,EVENT_MSG *msg)
  {
  T_EVENT_ROOT *r;
  T_EVENT_POINT *p,*s;
  int ev=msg->msg;

  find_event_msg(*tree,msg->msg,r);
  if (r!=NULL)
     {
     r->used++;
     s=r->list;
     for(p=r->list;p!=NULL;)
        {
        s=p->next;
        if (p->proc!=NULL && p->nezavora)
           {
           T_EVENT_POINT *z=p;
           if (p->proc==PROC_GROUP) z=(T_EVENT_POINT *)p->user_data;
           p->nezavora=p->nezavirat;
           otevri_zavoru=&p->nezavora;
           p->calls++;
           EVENT_MSG cpy;
           cpy.msg = msg->msg;
           va_copy(cpy.data, msg->data);
           z->proc(&cpy,&(z->user_data));
           va_end(cpy.data);
           p->calls--;
           p->nezavora=1;
           if (msg->msg==-2)
              {
              p->proc=NULL;
              msg->msg=ev;
              }
           s=p->next;
           if (!p->calls && p->proc==NULL)
              force_delete_curr(tree,r,p);
           if (msg->msg==-1) break;
           }
/*        if (p->next!=s)
           if (r->list!=p)
              {
              for(q=r->list;q!=NULL;q=q->next)
                 if (q->next==p)
                    {
                    s=p->next;
                    break;
                    }
                 else if (q->next==s) break;
              break;
              }
           else
              s=p->next;*/
        p=s;
        }
     r->used--;
  /*   for(p=r->list;p!=NULL;)
        {
        s=p->next;
        if (p->proc==NULL)
              force_delete_curr(tree,r,p);
        p=s;
        }*/
     }
  unsuspend_task(msg);
  }

typedef struct call_proc_context_t {
    EV_PROC proc;
    void **user;
}call_proc_context;

static int call_proc(EVENT_MSG *msg, void *ctx) {
    call_proc_context *c = ctx;
    c->proc(msg, c->user);
    return 0;
}

T_EVENT_POINT *install_event(T_EVENT_ROOT **tree,int32_t ev_num,EV_PROC proc,void *procdata,char end)
//instaluje novou udalost;
  {
  EVENT_MSG x;
  void *user=NULL;
  T_EVENT_POINT *p;
  call_proc_context ctx = {proc, &user};

  send_message_to(call_proc,&ctx, E_INIT, procdata);
  p=add_event(tree,ev_num,proc,end);
  p->user_data=user;
  return p;
   }

void deinstall_event(T_EVENT_ROOT **tree,int32_t ev_num,EV_PROC proc,void *procdata)
//deinstaluje udalost;
  {
  EVENT_MSG x;
  T_EVENT_ROOT *r;
  T_EVENT_POINT *p;

  find_event_msg(*tree,ev_num,r);
  if (r==NULL) return;
  find_event_proc(r->list,proc,p);
  if (p==NULL) return;
  call_proc_context ctx = {proc, &p->user_data};
  send_message_to(call_proc, &ctx, E_DONE, procdata);
  if (p->user_data!=NULL) free(p->user_data);
  p->proc=NULL;
  p->user_data=NULL;
  if (!p->calls) force_delete_curr(tree,r,p);
  }

void tree_basics(T_EVENT_ROOT **ev_tree,EVENT_MSG *msg)
{
  char *p;
  void (*q)();
  EVENT_MSG tg;

  if (msg->msg==E_ADD || msg->msg==E_ADDEND)
     {
     T_EVENT_POINT *r;
     shift_message(msg);
     EV_PROC proc = va_arg(msg->data, EV_PROC);
     void *procdata = va_arg(msg->data, void *);
     find_event_msg_proc(*ev_tree,msg->msg,proc,r);
     assert(r==NULL);
     if (r==NULL)
        install_event(ev_tree,msg->msg,proc,procdata,msg->msg==E_ADDEND);
     return;
     }
  if (msg->msg==E_INIT)
     {
      q = va_arg(msg->data, void (*)());
      q();
     return;
     }
  if (msg->msg==E_DONE)
     {
     shift_message(msg);
     EV_PROC proc = va_arg(msg->data, EV_PROC);
     void *procdata = va_arg(msg->data, void *);
     deinstall_event(ev_tree,msg->msg,proc,procdata);
     return;
     }
/*  if (msg->msg==E_GROUP)
     {
     int pocet,i,addev;
     T_EVENT_POINT *pp;
     EVENT_MSG *tgm=&tg;

     shift_message(msg);
     addev = msg->msg;
     if (addev!=E_ADD || addev!=E_ADDEND) return;
     shift_message(msg);
     int pocet = msg->msg;
     va_list va_procdata;
     va_copy(tmp, msg->data);
     for (int i = 0; i < pocet; ++i) va_arg(va_procdata, EV_PROC);
     for (int i = 0; i < pocet; ++i) {
         EV_PROC proc = va_arg(msg->data, EV_PROC);
         void *procdata = va_arg(va_procdata, void *)
         if (i == 0) {
             pp=install_event(ev_tree,proc,procdata,((int32_t *)proc+1),addev==E_ADDEND);
         }
     }

     int32_t *p;void *proc;
     shift_msg(msg,tg);addev=tg.msg;
     shift_msg(tgm,tg);
     pocet=tg.msg;
     p=tg.data;proc=p+pocet*sizeof(int32_t);
     for(i=0;i<pocet;i++,p++) if (i!=0)
                                {
                                T_EVENT_POINT *q;
                                q=add_event(ev_tree,*p,proc,addev==E_ADDEND);
                                q->user_data=(void *)pp;
                                q->proc=PROC_GROUP;
                                }
        else
           pp=install_event(ev_tree,*p,proc,((int32_t *)proc+1),addev==E_ADDEND);
     }
*/

}

int send_message_to(int (*cb)(EVENT_MSG *, void *), void *ctx, int message, ...) {
    EVENT_MSG x;
    x.msg = message;
    va_start(x.data, message);
    int r = cb(&x, ctx);
    va_end(x.data);
}

static void send_message_to_tree(EVENT_MSG *x) {
    if (x->msg==E_ADD || x->msg==E_INIT || x->msg==E_DONE) {
        tree_basics(&ev_tree,x);
    }
    else {
        enter_event(&ev_tree,x);
    }
}

void send_message(int message,...)
  {
    EVENT_MSG x;
    x.msg = message;
    va_start(x.data, message);
    send_message_to_tree(&x);
    va_end(x.data);
  }




void timer(EVENT_MSG *msg)
  {
  static uint32_t lasttime=0;
  if (msg->msg==E_WATCH)
     {
     uint32_t tm=get_game_tick_count()/TIMERSPEED;
     if (tm==lasttime) return;
     lasttime=tm;
     send_message(E_TIMER,tm);
     return;
     }
  }

void tasker(EVENT_MSG *msg,void **)
  {


  switch (msg->msg)
     {
     case E_INIT:
/*           tasklist_sp=New(void *);
           tasklist_low=New(void *);
           tasklist_top=New(void *);
           task_info=New(char);
           taskcount=1;
           memset(task_info,0,taskcount);*/
           break;
     case E_WATCH:
     case E_IDLE:
     default:
           if (q_any_task()>=1)
              task_sleep();
           break;
     case E_DONE:
           {
/*           int i;
           memset(task_info,1,taskcount);
           do
              {
              for (i=1;i<taskcount;i++)
                 if (tasklist_sp[i]!=NULL) break;
              if (i!=taskcount) task_sleep();
              }
           while (i<taskcount);
           free(tasklist_sp);
           free(tasklist_low);
           free(task_info);*/
           }
           break;
     }
  }


/*void except_free_stack(void *ptr);
#pragma aux except_free_stack parm [eax]=\
     "mov  esp,eax"\
     "sub  esp,10h"\
     "popf"\
     "pop  ax"\
     "mov  ebp,esp"\
     "lss  esp,[ebp]"\
     "mov  ebp,esp"


void except_GPF()
#pragma aux except_GPF parm[]
  {
  raise_error(ERR_MEMREF);
  }

*/
void init_events()
  {
  send_message(E_ADD,E_WATCH,keyboard);
  send_message(E_ADD,E_WATCH,timer);
  send_message(E_ADD,E_WATCH,tasker);
#ifdef nodebug
  alloc_exception(0xD,except_GPF);
  alloc_exception(0xE,except_GPF);
#endif
  }

static char do_events_called=0;

void do_events()
  {
  do_events_called=1;
  if (!q_is_mastertask()) task_sleep();
  else
     {
     send_message(E_WATCH);
     send_message(E_IDLE);
     }
  }


void error_show(int error_number)
  {
  send_message(E_PRGERROR,&error_number);
  if (!error_number) abort();
  }


void escape()
  {

  exit_wait=0;
  do
     {
     send_message(E_WATCH);
     send_message(E_IDLE);
     if (do_events_called==0)  ShareCPU();
     else do_events_called=0;
     }
  while (!exit_wait);
  exit_wait=0;
  }

T_EVENT_ROOT *gate_basics(EVENT_MSG *msg, void **user_data)
  {
  T_EVENT_ROOT *p;

  memcpy(&p,user_data,4);
  shift_message(msg);
  if (msg->msg==E_ADD || msg->msg==E_INIT || msg->msg==E_DONE)
      tree_basics((T_EVENT_ROOT **)user_data,msg);
  return p;
  }
/*
int create_task()
  {
  int i;

  for(i=1;i<taskcount;i++)
     if (tasklist_sp[i]==NULL) break;
  if (i>=taskcount)
     {
     taskcount++;
     tasklist_sp=grealloc(tasklist_sp,taskcount*4);
     tasklist_low=grealloc(tasklist_low,taskcount*4);
     tasklist_top=grealloc(tasklist_top,taskcount*4);
     task_info=grealloc(task_info,taskcount);
     tasklist_events=grealloc(tasklist_events,taskcount*sizeof(int));
     }
  return i;
  }
*/
void task_terminating();
/*
int32_t getflags();
#pragma aux getflags = \
  "pushfd"\
  "pop  eax"\
  ;*/
/*
int add_task(int stack,void *name,...)
  {
  int task,i;
  int32_t *sp,*spp;

  task=create_task();
  if (task==-1)
     {
     send_message(E_MEMERROR);
     return -1;
     }
  sp=malloc(stack);
  if (sp==NULL)
     {
     send_message(E_MEMERROR);
     return -1;
     }
  spp=(int32_t *)((char *)sp+stack-17*4);
  memset(sp,0,stack);
  memcpy(spp,&name,17*4);
  *spp=(int32_t )task_terminating;
  spp--;*spp=(int32_t)name;
  for(i=0;i<9;i++)
     {
     spp--;
     *spp=0;
     if (i==5) *spp=(int32_t)((char *)sp+stack);
     }
  tasklist_low[task]=(void *)sp;
  tasklist_top[task]=(void *)((char *)sp+stack);
  tasklist_sp[task]=(void *)spp;
  task_info[task]=TASK_RUNNING;
  return task;
  }

void term_task(int id_num)
  {
  task_info[id_num]=TASK_TERMINATING;
  return;
  }

char is_running(int id_num)
  {
  return tasklist_sp[id_num]!=NULL;
  }

static void suspend_task(int id_num,int msg)
  {
  task_info[id_num]|=TASK_EVENT_WAITING;
  tasklist_events[id_num]=msg;
  }

void shut_down_task(int id_num)
  {
  free(tasklist_low[id_num]);
  tasklist_sp[id_num]=0;
  if (nexttask==id_num) nexttask=0;
  }
*/
/*void raise_error(int error_number)
  {
  longjmp(jmpenv,error_number);
  }
*/
/*static void unsuspend_task_by_event(EVENT_MSG *msg,int **idnum)
  {
  if (msg->msg==E_INIT)
     {
     *idnum=New(int);
     **idnum=*(int *)msg->data;
     }
  else
     {
     int nt=nexttask;

     nexttask=**idnum;
     free(*idnum);
     *idnum=NULL;
     task_info[nexttask]&=~TASK_EVENT_WAITING;
     task_sleep(msg->data);
     msg->msg=-2;
     nexttask=nt;
     }
  }
*/
/*void *task_wait_event(int32_t event_number)
  {
  if (!curtask) return NULL;
  suspend_task(curtask,event_number);
  return task_sleep();
  }
*/
