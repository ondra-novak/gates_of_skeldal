#include "base64.h"
#include <iostream>
#include <fstream>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Requires name of the input file" << std::endl;
        return 1;
    }

    std::ifstream input(argv[1], std::ios::in|std::ios::binary);
    if (!input) {
        std::cerr << "Failed to open:" << argv[1] << std::endl;
        return 2;
    }
    auto in_iter = std::istreambuf_iterator<char>(input);
    auto in_end = std::istreambuf_iterator<char>();
    auto out_iter = std::ostream_iterator<char>(std::cout);
    base64.encode(in_iter, in_end, out_iter);
    return 0;
}
