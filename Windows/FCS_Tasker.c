#include <skeldal_win.h>
#include <memman.h>
#include "types.h"
#include "event.h"
#include <debug.h>
#include "FCS_Tasker.h"
#include <assert.h>


#define MAX_FIBERS 32

typedef struct _fcs_taskerstruct
   {
   void *fiberHandle;
   int wait_event;
   char waiting:1;
   char exitFiber:1;
   char usedSlot:1;    
   } FCS_TASKERSTRUCT;

#define THREADDECL __declspec (thread)   

static THREADDECL FCS_TASKERSTRUCT fiberList[MAX_FIBERS];
static THREADDECL int fiberNextSlot=-1;
static THREADDECL void *mainFiber=NULL;
static THREADDECL void *cleaningFiber=NULL;
static THREADDECL int currentFiber=-1;
static THREADDECL int lastTask=-1;
static THREADDECL void *switchData=NULL;
static THREADDECL int task_count=0;

static void CALLBACK CleaningFiber(void *lpParameter)
  {
  do
    {
    SEND_LOG("(TASKER) Cleaning task %d",currentFiber,0);
    assert(currentFiber>=0);
    DeleteFiber(fiberList[currentFiber].fiberHandle);
    fiberList[currentFiber].usedSlot=0;
    currentFiber=-1;
    switchData=NULL;
    task_count--;
    SwitchToFiber(mainFiber);
    }
  while (1);
  }

static void InitFibers()
  {
  mainFiber=ConvertThreadToFiber(NULL);
  cleaningFiber=CreateFiber(0,CleaningFiber,NULL);
  currentFiber=-1;
  fiberNextSlot=0;
  memset(fiberList,0,sizeof(fiberList));  
  SEND_LOG("(TASKER) Fibers inicialized",0,0);
  }
  
static int GetFreeFiberSlot()
  {
  int save=fiberNextSlot;
  do
    {
    if (fiberNextSlot<0) fiberNextSlot+=MAX_FIBERS; 
    if (fiberList[fiberNextSlot].usedSlot==0) return fiberNextSlot--;
    fiberNextSlot--;
    }
  while (fiberNextSlot!=save);
  STOP();
  return 0;
  }

static void CALLBACK RunUserFiber(void *params)
  {
  va_list p=(va_list)switchData;
  TaskerFunctionName fcname=(TaskerFunctionName)params;
  fcname(p);
  SwitchToFiber(cleaningFiber);     
  }  
  
int add_task(int stack,TaskerFunctionName fcname,...)
  {
  if (mainFiber==NULL) InitFibers();
    {
    int fib=GetFreeFiberSlot();
    va_list args;

    fiberList[fib].usedSlot=1;
    fiberList[fib].waiting=0;
    fiberList[fib].exitFiber=0;
    fiberList[fib].fiberHandle=CreateFiber(0,RunUserFiber,fcname);  
    va_start(args,fcname);
    switchData=args;
    currentFiber=fib;
    task_count++;
    SEND_LOG("(TASKER) Adding task %d",fib,0);
    SwitchToFiber(fiberList[fib].fiberHandle);
    return fib;
    }
  }

void term_task(int id_num)
  {
  fiberList[id_num].exitFiber=1;  
  }

char is_running(int id_num)
  {
  return fiberList[id_num].usedSlot;
  }

void suspend_task(int id_num,int msg)
  {
  fiberList[id_num].waiting=1;
  fiberList[id_num].wait_event=msg;
  }

void shut_down_task(int id_num)
  {
  if (fiberList[id_num].usedSlot)
    {
    SEND_LOG("(TASKER) Killing task %d",id_num,0);
    DeleteFiber(fiberList[id_num].fiberHandle);
    task_count--;
    fiberList[id_num].usedSlot=0;
    }
  }

static void *FCSTaskSleep(void *data,int fiber)
  {
  if (mainFiber==NULL) return data;
  if (fiber==-1)
    {
    currentFiber=fiber;
    if (GetCurrentFiber()==mainFiber) return data;
    switchData=data;
    SwitchToFiber(mainFiber);
    return switchData;
    }
  if (fiberList[fiber].usedSlot)
    {
    currentFiber=fiber;
    if (GetCurrentFiber()==fiberList[fiber].fiberHandle) return data;
    switchData=data;
    SwitchToFiber(fiberList[fiber].fiberHandle);
    return switchData;
    }
  return data;
  }


void unsuspend_task(EVENT_MSG *msg)
  {
  int id_num;
  for (id_num=0;id_num<MAX_FIBERS;id_num++) if (fiberList[id_num].usedSlot && fiberList[id_num].waiting && fiberList[id_num].wait_event==msg->msg)
    {    
    FCSTaskSleep(msg,id_num);
    }
  }

void *task_sleep(void *data)
  {
  int i,p=lastTask+1;
  if (currentFiber!=-1)  return FCSTaskSleep(data,-1);
  for (i=0;i<MAX_FIBERS;i++,p++)
    {
    if (p>=MAX_FIBERS) p-=MAX_FIBERS;
    if (fiberList[p].usedSlot && !fiberList[p].waiting) 
      {
      lastTask=p;
      return FCSTaskSleep(data,p);
      }    
    }
  return data;
  }

void *task_wait_event(int32_t event_number)
  {
  void *p;
  suspend_task(currentFiber,event_number);
  p=task_sleep((void *)event_number);
  fiberList[currentFiber].waiting=0;
  if (p==NULL) return NULL;  
  return ((EVENT_MSG *)p)->data;
  }

int q_any_task()
  {
  return task_count;
  }

char task_quitmsg()
  {
  return task_quitmsg_by_id(currentFiber);
  }

char task_quitmsg_by_id(int id)
  {
  return fiberList[id].exitFiber;
  }

char q_is_mastertask()  
  {
  return currentFiber==-1;
  }

int q_current_task()
  {
  return currentFiber;
  }
