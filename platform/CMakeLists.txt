SET(files error.cpp legacy_coroutines.cpp)

add_library(skeldal_platform_libs  ${files})
set_property(TARGET skeldal_platform_libs PROPERTY CXX_STANDARD 20)

add_subdirectory(sdl)