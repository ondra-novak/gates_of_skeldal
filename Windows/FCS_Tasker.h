#ifndef _FCS_TASKER_H_
#define _FCS_TASKER_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*TaskerFunctionName)(va_list);

void tasker(EVENT_MSG *msg,void **data);
int create_task();
int add_task(int stack,TaskerFunctionName fcname,...);
void term_task(int id_num);
char is_running(int id_num);
void suspend_task(int id_num,int msg);
void shut_down_task(int id_num);
void unsuspend_task(EVENT_MSG *msg);
void *task_sleep(void *data);
void *task_wait_event(long event_number);
int q_any_task();
char task_quitmsg();
char task_quitmsg_by_id(int id);
char q_is_mastertask();
int q_current_task();


#ifdef __cplusplus
}
#endif

#endif
