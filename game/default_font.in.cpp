#include <base64.h>


constexpr  binary_data font_data ="@CONTENT@";

extern "C" {

const void *LoadDefaultFont() {
    return font_data.begin();
}

}


