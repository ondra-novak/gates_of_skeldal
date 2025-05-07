#pragma once


#ifdef __cplusplus
extern "C" {
#endif

typedef struct tag_sse_receiver SSE_RECEIVER;
SSE_RECEIVER *sse_receiver_create(const char *host, const char *port);
const char *sse_receiver_receive(SSE_RECEIVER *sse);
void sse_receiver_destroy(SSE_RECEIVER *inst);


#ifdef __cplusplus
}
#endif
