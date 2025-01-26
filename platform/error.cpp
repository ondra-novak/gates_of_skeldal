#include <iostream>
extern "C" {
#include "error.h"
}


void display_error(const char *text) {
    std::cerr << "ERROR:" << text << std::endl;
}
