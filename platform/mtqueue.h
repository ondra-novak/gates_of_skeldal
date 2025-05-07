#pragma once

#ifdef __cplusplus
extern "C" {
#endif


typedef struct tag_mtqueue MTQUEUE;
///Create multithread queue
MTQUEUE *mtqueue_create();
///push to queue (string is copied)
/**
 * @param q queue
 * @param message message (string is copied)
 */
void mtqueue_push(MTQUEUE *q, const char *message);
///pop from the queue
/**
 *
 * @param q queue
 * @return NULL, if queue is empty, or string. You have to release
 * string by calling free() when you finish.
 */
char *mtqueue_pop(MTQUEUE *q);

///destroy the queue
void mtqueue_destroy(MTQUEUE *q);


#ifdef __cplusplus
}
#endif
