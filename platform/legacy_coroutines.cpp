#include "legacy_coroutines.h"


#include <thread>
#include <atomic>
#include <memory>
#include <queue>
#include <unordered_map>

struct TaskInfo {
    int id;
    std::thread thr;
    std::atomic<bool> resume_flag = {false};
    std::chrono::system_clock::time_point _wake_up_after = {};
    int wake_up_msg = -1;
    bool request_exit = false;
    bool exited = false;

    TaskInfo(int id):id(id) {}
};

using TaskList = std::unordered_map<int, std::unique_ptr<TaskInfo> >;
static int next_task_id = 1;
static TaskList task_list;

static int get_new_task_id()  {
    int id = next_task_id;
    next_task_id =  (next_task_id + 1) & 0xFFFF;
    if (task_list.find(id) != task_list.end()) id = get_new_task_id();
    return id;
}

static std::atomic<bool> resume_master_flag = {false};
static TaskInfo *current_task_inst = NULL;
static EVENT_MSG *cur_message = NULL;

struct MsgQueue {
    EVENT_MSG *msg;
    TaskInfo *sender;
};

static int recursion_count = 0;


static void task_after_wakeup(TaskInfo *info) {
    info->wake_up_msg = -1;
    info->_wake_up_after = {};

}


static void switch_to_task(TaskInfo *task) {
    if (task == current_task_inst) return;
    if (task == NULL) {
        TaskInfo *me = current_task_inst;
        current_task_inst = NULL;
        resume_master_flag = true;
        resume_master_flag.notify_all();
        me->resume_flag.wait(false);
        me->resume_flag = false;
        task_after_wakeup(me);
    } else if (current_task_inst == NULL) {
        if (task->exited) return ;
        current_task_inst = task;
        task->resume_flag = true;
        task->resume_flag.notify_all();
        resume_master_flag.wait(false);
        resume_master_flag = false;
    } else {
        if (task->exited) return ;
        TaskInfo *me = current_task_inst;
        current_task_inst = task;
        task->resume_flag = true;
        task->resume_flag.notify_all();
        me->resume_flag.wait(false);
        me->resume_flag = false;
        task_after_wakeup(me);
    }
}

static void clean_task_table() {
    if (recursion_count) return;
    for (auto iter = task_list.begin(); iter != task_list.end();) {
        if (iter->second->exited) {
            iter = task_list.erase(iter);
        } else {
            ++iter;
        }
    }
}

static void clean_up_current_task() {
    TaskInfo *me = current_task_inst;
    if (!me) return;
    me->thr.detach();
    me->exited = true;
    current_task_inst = NULL;
    resume_master_flag = true;
    resume_master_flag.notify_all();
}

static void broadcast_message(EVENT_MSG *msg) {
    ++recursion_count;
    for (auto &[id, task]: task_list) {
        if (task->wake_up_msg == msg->msg && task.get() != current_task_inst) {
            EVENT_MSG cpy;
            cpy.msg = msg->msg;
            va_copy(cpy.data, msg->data);
            cur_message = &cpy;
            switch_to_task(task.get());
            va_end(cpy.data);
            cur_message = NULL;
        }
    }
    --recursion_count;
    clean_task_table();
}


int add_task(int stack,TaskerFunctionName fcname,...) {
    int id = get_new_task_id();
    auto st = task_list.emplace(id, std::make_unique<TaskInfo>(id));
    TaskInfo *new_task = st.first->second.get();
    va_list args;
    va_start(args, fcname);
    new_task->thr = std::thread([&]{
       new_task->resume_flag.wait(false);
       new_task->resume_flag = false;
       fcname(args);
       clean_up_current_task();
    });
    switch_to_task(new_task);
    return id;
}

void term_task(int id_num) {
    auto iter = task_list.find(id_num);
    if (iter != task_list.end()) {
        iter->second->request_exit = true;
        switch_to_task(iter->second.get());
    }
}
char is_running(int id_num) {
    if (id_num < 0) return false;
    auto iter = task_list.find(id_num);
    if (iter == task_list.end()) return false;
    return !iter->second->exited;
}
void unsuspend_task(EVENT_MSG *msg) {
    broadcast_message(msg);
}
void task_sleep(void) {
    if (current_task_inst) {
        switch_to_task(NULL);
    } else {
        auto now = std::chrono::system_clock::now();
        recursion_count++;
        for (auto &[id, task]: task_list) {
            if (task->_wake_up_after < now && task->wake_up_msg == -1) {
                switch_to_task(task.get());
            }
        }
        recursion_count--;
        clean_task_table();
    }
}
EVENT_MSG *task_wait_event(int32_t event_number) {
    if (current_task_inst == NULL || current_task_inst->request_exit || current_task_inst->exited) return NULL;
    current_task_inst->wake_up_msg = event_number;
    switch_to_task(NULL);
    return cur_message;
}
int q_any_task() {
    return (int)task_list.size();
}
char task_quitmsg() {
    if (current_task_inst == NULL) return 0;
    return current_task_inst->request_exit?1:0;
}

void term_task_wait(int id_num) {
    term_task(id_num);
    while (is_running(id_num)) {
        task_sleep();
    }
}

char q_is_mastertask() {
    return current_task_inst == NULL;
}
int q_current_task() {
    return current_task_inst?current_task_inst->id:-1;
}
void task_sleep_for(unsigned int time_ms) {
    if (current_task_inst) {
        current_task_inst->_wake_up_after = std::chrono::system_clock::now() + std::chrono::milliseconds(time_ms);
    }
}

