SET(files error.cpp
    legacy_coroutines.cpp
    platform.cpp
    int2ascii.c
    istr.c
    file_access.cpp
    config.cpp
)

add_library(skeldal_platform_libs  ${files})
set_property(TARGET skeldal_platform_libs PROPERTY CXX_STANDARD 20)

add_subdirectory(sdl)
add_subdirectory(linux)
