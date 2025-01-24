#define CPERR_OPEN 1
#define CPERR_WRITE 2
#define CPERR_READ 3

extern void (*cpy_error)(int,char *);
extern void (*cpy_progress)(long);
void cpy_file(char *source,char *target);
void cpy_flush();
void install_cpy();
