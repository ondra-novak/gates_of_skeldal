#ifndef _FCS_TASKER_H_
#define _FCS_TASKER_H_

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libs/event.h>


typedef void (*TaskerFunctionName)(va_list);

//void tasker(EVENT_MSG *msg,void **data);
//int create_task(void);
///start new task
/**
 * @param stack stack size - ignored by some platforms
 * @param fcname function to start
 * @param ... arguments
 * @return id of task
 *
 * @note task is started immediatelly.
 * @note arguments are valid only until task is suspended
 */
int add_task(int stack,TaskerFunctionName fcname,...);
///terminate task
/**
 * request to terminate given task. The task can check this flag by
 * task_quitmsg(void)

 */
void term_task(int id_num);
void term_task_wait(int id_num);
///returns true, if task is running
char is_running(int id_num);
//void suspend_task(int id_num,int msg);
//void shut_down_task(int id_num);
///resumes task waiting on given message event

void unsuspend_task(EVENT_MSG *msg);

///sleep current task and switch to another
void task_sleep(void);

void task_sleep_for(unsigned int time_ms);
///sleep current task and wake up when given event is triggered
/**
 * @param event_number waiting number
 * @return received message, NULL if task_term called
 */
EVENT_MSG *task_wait_event(int32_t event_number);
///returns count of tasks
int q_any_task(void);
///returns 1 if terminate is requested
char task_quitmsg(void);
//char task_quitmsg_by_id(int id);
char q_is_mastertask(void);
///returns id of current task
int q_current_task(void);


#ifdef __cplusplus
}
#endif

#endif
