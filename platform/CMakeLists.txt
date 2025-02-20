
add_library(skeldal_platform STATIC)

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

set(all_libs 
    skeldal_main 
    skeldal_libs 
    skeldal_platform
    skeldal_sdl
    skeldal_libs
     ${SDL2_LIBRARIES} 
     ${STANDARD_LIBRARIES})
     
if(WIN32)
    target_sources(skeldal_platform PRIVATE
        windows/save_folder.cpp
        windows/map_file.cpp
    )
    add_executable(skeldal WIN32)
    target_sources(skeldal PRIVATE
        windows/app_start.cpp
        windows/skeldal.manifest
        windows/skeldal.rc
    )
    target_compile_definitions(skeldal_platform PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(skeldal ${all_libs})
    message(STATUS "Building for Windows")

elseif(UNIX AND NOT APPLE)
    target_sources(skeldal_platform PRIVATE
        linux/save_folder.cpp
        linux/map_file.cpp
    )
    add_executable(skeldal_bin)
    target_sources(skeldal_bin PRIVATE
        linux/app_start.cpp
    )
    target_compile_definitions(skeldal_platform PRIVATE PLATFORM_LINUX)
    add_custom_command(
        TARGET skeldal_bin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_LIST_DIR}/linux/skeldal.sh
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/skeldal.sh)
    target_link_libraries(skeldal_bin ${all_libs})    
    message(STATUS "Building for Linux")

elseif(APPLE)   
    target_sources(skeldal_platform PRIVATE
        mac_os/save_folder.cpp
    )
    add_executable(skeldal)
    target_sources(skeldal PRIVATE
        linux/app_start.cpp
    )
    target_compile_definitions(mylib PRIVATE PLATFORM_MACOS)
    message(STATUS "Building for macOS")
    target_link_libraries(skeldal ${all_libs})
else() 
    error("Platform not detected, please add new platform here")
endif()
set_property(TARGET skeldal_platform PROPERTY CXX_STANDARD 20)

 add_subdirectory(sdl)

