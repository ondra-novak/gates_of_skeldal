#ifndef _SKELDAL_LOGFILE_HEADER
#define _SKELDAL_LOGFILE_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOGFILE
void send_log_impl(const char *format, ...) __attribute__((format(printf, 1, 2)));
#define SEND_LOG(...) send_log_impl( __VA_ARGS__)
#else
#define SEND_LOG(...)
#endif


#ifdef __cplusplus
}
#endif

#endif
