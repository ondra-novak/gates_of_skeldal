SET(files error.cpp
)

# Základní knihovna mylib
add_library(skeldal_platform STATIC)

# Přidejte soubory společné pro všechny platformy
target_sources(skeldal_platform PRIVATE
    legacy_coroutines.cpp
    platform.cpp
    int2ascii.c
    istr.c
    file_access.cpp
    config.cpp
    error.cpp
    timer.cpp
    getopt.c
)

# Podmínky pro platformu Windows
if(WIN32)
    target_sources(skeldal_platform PRIVATE
        windows/save_folder.cpp
    )
    target_compile_definitions(skeldal_platform PRIVATE PLATFORM_WINDOWS)
    message(STATUS "Building for Windows")

# Podmínky pro platformu Linux
elseif(UNIX AND NOT APPLE)
    target_sources(skeldal_platform PRIVATE
        linux/save_folder.cpp
        linux/map_file.cpp
    )
    target_compile_definitions(skeldal_platform PRIVATE PLATFORM_LINUX)
    message(STATUS "Building for Linux")

# Podmínky pro platformu macOS
elseif(APPLE)
    target_sources(skeldal_platform PRIVATE
        mac_os/save_folder.cpp
    )
    target_compile_definitions(mylib PRIVATE PLATFORM_MACOS)
    message(STATUS "Building for macOS")
else() 
    error("Platform not detected, please add new platform here")
endif()
set_property(TARGET skeldal_platform PROPERTY CXX_STANDARD 20)

add_subdirectory(sdl)
