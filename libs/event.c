#include <platform/platform.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "event.h"
#include "devices.h"
#include <stdlib.h>
#include <time.h>
#include "memman.h"
#include <setjmp.h>
#include <signal.h>
#include <assert.h>
#include <string.h>


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
  r->used=0;
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
           if (cpy.msg==-2)
              {
              p->proc=NULL;
              cpy.msg=ev;
              }
           s=p->next;
           if (!p->calls && p->proc==NULL)
              force_delete_curr(tree,r,p);
           if (cpy.msg==-1)
               break;
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

T_EVENT_POINT *install_event(T_EVENT_ROOT **tree, EVENT_MSG *msg, EV_PROC proc,char end)
//instaluje novou udalost;
  {
  void *user=NULL;
  T_EVENT_POINT *p;
  int ev_num = msg->msg;
  msg->msg = E_INIT;
  proc(msg, &user);
  p=add_event(tree,ev_num,proc,end);
  p->user_data=user;
  return p;
   }

void deinstall_event(T_EVENT_ROOT **tree,int32_t ev_num,EV_PROC proc,void *procdata)
//deinstaluje udalost;
  {
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

typedef void (*initproc)();

void tree_basics(T_EVENT_ROOT **ev_tree,EVENT_MSG *msg)
{
  initproc q;

  if (msg->msg==E_ADD || msg->msg==E_ADDEND)
     {
     T_EVENT_POINT *r;
     shift_message(msg);
     EV_PROC proc = va_arg(msg->data, EV_PROC);
     find_event_msg_proc(*ev_tree,msg->msg,proc,r);
     assert(r==NULL);
     if (r==NULL)
        install_event(ev_tree,msg,proc,msg->msg==E_ADDEND);
     return;
     }
     
  if (msg->msg==E_INIT)
     {      
      q = va_arg(msg->data, initproc);
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
    return r;
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

void tasker(EVENT_MSG *msg,void **_)
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
