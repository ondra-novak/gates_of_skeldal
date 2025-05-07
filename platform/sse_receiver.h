#pragma once

#include "mtqueue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tag_sse_receiver SSE_RECEIVER;

///Install sse receiver
/**
 * @param q mtqueue, which receives messages received by the receiver
 * @param host host
 * @param port port
 * @return pointer to instance of receiver
 */
SSE_RECEIVER *sse_receiver_install(MTQUEUE *q, const char *host, const char *port);

///Stops the receiver
/**
 * @param inst instance of receiver
 * @note the associated queue is not destroyed
 */
void sse_receiver_stop(SSE_RECEIVER *inst);


#ifdef __cplusplus
}
#endif
