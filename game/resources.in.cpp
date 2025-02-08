#include <libs/base64.h>


constexpr  binary_data font_data ="@FONT_CONTENT@";
constexpr  binary_data icon_data ="@ICON_CONTENT@";

extern "C" {

const void *LoadDefaultFont(void) {
    return font_data.begin();
}

const void *getWindowIcon(void) {
    return icon_data.begin();
}
size_t getWindowIconSize(void) {
    return std::distance(icon_data.begin(),icon_data.end());
}


}
