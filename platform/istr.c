#include "platform.h"

#include <ctype.h>
int stricmp(const char *a, const char *b) {
    while (*a && *b) {
        char ca = toupper(*a);
        char cb = toupper(*b);
        if (ca<cb) return -1;
        if (ca>cb) return 1;
        ++a;
        ++b;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

void strupr(char *c) {
    while (*c) {
        *c = toupper(*c);
        ++c;
    }
}
