
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
    achievements.cpp
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
    if(STEAM_ENABLED) 
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit
            set(STEAMLIB ${STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api64.lib)
            set(STEAMDLL ${STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api64.dll)
        else()
            # 32-bit
            set(STEAMLIB ${STEAMWORKS_SDK_DIR}/redistributable_bin/win32/steam_api.lib)
            set(STEAMDLL ${STEAMWORKS_SDK_DIR}/redistributable_bin/win64/steam_api.dll)
        endif()
    else()
        set(STEAMLIB "")
        set(STEAMDLL "")
    endif()
    
    target_link_libraries(skeldal ${all_libs} ${STEAMLIB})

    if(STEAMDLL)
        add_custom_command(TARGET skeldal POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${STEAMDLL} $<TARGET_FILE_DIR:skeldal>)
    endif()

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
    target_compile_definitions(skeldal_bin PRIVATE PLATFORM_LINUX)
    if(STEAM_ENABLED) 
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit
            set(STEAMLIB ${STEAMWORKS_SDK_DIR}/redistributable_bin/linux64/libsteam_api.so)
        else()
            # 32-bit
            set(STEAMLIB ${STEAMWORKS_SDK_DIR}/redistributable_bin/linux32/libsteam_api.so)
        endif()
    endif()
    add_custom_command(
        TARGET skeldal_bin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_LIST_DIR}/linux/skeldal.sh
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/skeldal.sh)
    target_link_libraries(skeldal_bin ${all_libs} ${STEAMLIB})    
    message(STATUS "Building for Linux")

elseif(APPLE)   
    target_sources(skeldal_platform PRIVATE
        mac_os/save_folder.cpp
    )
    add_executable(skeldal)
    target_sources(skeldal PRIVATE
        linux/app_start.cpp
    )
    if(STEAM_ENABLED)
        set(STEAMLIB ${STEAMWORKS_SDK_DIR}/redistributable_bin/osx/libsteam_api.dylib)
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")  
    message(STATUS "Building for macOS")
    target_link_libraries(skeldal ${all_libs} ${STEAMLIB})
else() 
    error("Platform not detected, please add new platform here")
endif()
set_property(TARGET skeldal_platform PROPERTY CXX_STANDARD 20)

 add_subdirectory(sdl)

