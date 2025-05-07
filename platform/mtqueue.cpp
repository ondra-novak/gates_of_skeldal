#include "mtqueue.h"

#include <cstring>
#include <malloc.h>
#include <memory>
#include <mutex>
#include <queue>
struct StringDeleter {
    void operator()(char *x) {
        free(x);
    }
};

std::unique_ptr<char, StringDeleter> alloc_string(const char *x) {
    return std::unique_ptr<char, StringDeleter>(strdup(x));
}


typedef struct tag_mtqueue {
    std::queue<std::unique_ptr<char, StringDeleter> > _q;
    std::mutex _mx;


} MTQUEUE;

MTQUEUE *mtqueue_create() {
    return new MTQUEUE();
}
void mtqueue_push(MTQUEUE *q, const char *message) {
    std::lock_guard _(q->_mx);
    q->_q.push(alloc_string(message));
}
char *mtqueue_pop(MTQUEUE *q) {
    std::lock_guard _(q->_mx);
    if (q->_q.empty()) return NULL;
    else {
        char *c = q->_q.front().release();
        q->_q.pop();
        return c;
    }
}
void mtqueue_destroy(MTQUEUE *q) {
    delete q;
}
