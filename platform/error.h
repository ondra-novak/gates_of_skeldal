#pragma once


#ifdef __cplusplus
extern "C" {
#endif


void send_log_impl(const char *format, ...);
void display_error(const char *format, ...);
void throw_exception(const char *text);


#ifdef __cplusplus
}
#endif
