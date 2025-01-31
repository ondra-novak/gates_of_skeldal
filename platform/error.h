#pragma once


#ifdef __cplusplus
extern "C" {
#endif


void display_error(const char *text);
void send_log_impl(const char *format, ...);

#ifdef __cplusplus
}
#endif
