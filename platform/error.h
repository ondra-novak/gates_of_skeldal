#pragma once


#ifdef __cplusplus
extern "C" {
#endif


void send_log_impl(const char *format, ...);
void display_error(const char *format, ...);


#ifdef __cplusplus
}
#endif
