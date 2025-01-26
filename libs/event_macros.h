/*
 * event_macros.h
 *
 *  Created on: 26. 1. 2025
 *      Author: ondra
 */

#ifndef LIBS_EVENT_MACROS_H_
#define LIBS_EVENT_MACROS_H_

#include "event.h"

#define EVENT_PROC(name) void name(EVENT_MSG *msg,void **user_ptr)
#define WHEN_MSG(msg_num) if (msg->msg==msg_num)
#define UNTIL_MSG(msg_num) if (msg->msg!=msg_num)
#define GET_DATA(data_type) (va_arg(msg->data, data_type))
#define GET_DATA_PTR(data_type) (va_arg(msg->data, data_type *))
#define GET_USER(data_type) (*(data_type *)user_ptr)
#define SAVE_USER_PTR(p) (*user_ptr=p)
#define GET_USER_PTR() user_ptr
#define EVENT_RETURN(value) msg->msg=value
#define GET_MSG_VAR() msg
#define GET_MSG() msg->msg
#define TASK_GET_TERMINATE() ((task_info[cur_task] & TASK_TERMINATING)!=0)





#endif /* LIBS_EVENT_MACROS_H_ */
