#include "platform.h"



static char *render_int(char *where, int i, int radix) {
    if (i == 0) return where;
    char *r = render_int(where, i/radix, radix);
    int p = i % radix;
    if (p<=0) {
        *r = p + '0';
    } else {
        *r = p + 'A' - 10;
    }
    return r+1;
}

const char * int2ascii(int i, char *c, int radix) {
    if (i == 0) {
        c[0] = '0';
        c[1] = 0;
        return c;
    }
    if (i<0) {
        c[0] = '-';
        *render_int(c+1,-i,radix) = 0;
    } else {
        *render_int(c,i,radix) = 0;
    }
    return c;
}
