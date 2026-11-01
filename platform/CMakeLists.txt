
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

# exactly one achievements backend (selected in the top-level CMakeLists.txt)
if(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "steam")
    target_sources(skeldal_platform PRIVATE achievements_steam.cpp)
elseif(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "galaxy")
    target_sources(skeldal_platform PRIVATE achievements_galaxy.cpp)
else()
    target_sources(skeldal_platform PRIVATE achievements_none.cpp)
endif()

set(all_libs 
    skeldal_main 
    skeldal_libs 
    skeldal_platform
    skeldal_sdl
    skeldal_libs
     ${SDL2_LIBRARIES}
     ${STANDARD_LIBRARIES})

# import library + runtime shared library of the achievements backend
set(SERVICES_LIB "")
set(SERVICES_DLL "")
     
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
    if(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "steam")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit
            set(SERVICES_LIB ${STEAMWORKS_DIR}/redistributable_bin/win64/steam_api64.lib)
            set(SERVICES_DLL ${STEAMWORKS_DIR}/redistributable_bin/win64/steam_api64.dll)
        else()
            # 32-bit
            set(SERVICES_LIB ${STEAMWORKS_DIR}/redistributable_bin/steam_api.lib)
            set(SERVICES_DLL ${STEAMWORKS_DIR}/redistributable_bin/steam_api.dll)
        endif()
    elseif(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "galaxy")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit package "Windows MSVC 19 (64 bit)"
            set(SERVICES_LIB ${GALAXY_DIR}/Libraries/Galaxy64.lib)
            set(SERVICES_DLL ${GALAXY_DIR}/Libraries/Galaxy64.dll)
        else()
            # 32-bit package "Windows MSVC 19 (32 bit)"
            set(SERVICES_LIB ${GALAXY_DIR}/Libraries/Galaxy.lib)
            set(SERVICES_DLL ${GALAXY_DIR}/Libraries/Galaxy.dll)
        endif()
    endif()
    
    target_link_libraries(skeldal ${all_libs} ${SERVICES_LIB})

    if(SERVICES_DLL)
        add_custom_command(TARGET skeldal POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${SERVICES_DLL} $<TARGET_FILE_DIR:skeldal>)
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
    if(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "steam")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit
            set(SERVICES_LIB ${STEAMWORKS_DIR}/redistributable_bin/linux64/libsteam_api.so)
        else()
            # 32-bit
            set(SERVICES_LIB ${STEAMWORKS_DIR}/redistributable_bin/linux32/libsteam_api.so)
        endif()
    elseif(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "galaxy")
        # GOG Galaxy has no Linux client; the GOG Linux build ships without achievements
        message(FATAL_ERROR "GOG Galaxy backend is supported on Windows only. Use -DACHIEVEMENTS_BACKEND=none for the Linux GOG build")
    endif()
    add_custom_command(
        TARGET skeldal_bin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_LIST_DIR}/linux/skeldal.sh
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/skeldal.sh)
    target_link_libraries(skeldal_bin ${all_libs} ${SERVICES_LIB})    
    message(STATUS "Building for Linux")

elseif(APPLE)   
    target_sources(skeldal_platform PRIVATE
        mac_os/save_folder.cpp
    )
    add_executable(skeldal)
    target_sources(skeldal PRIVATE
        linux/app_start.cpp
    )
    if(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "steam")
        set(SERVICES_LIB ${STEAMWORKS_DIR}/redistributable_bin/osx/libsteam_api.dylib)
    elseif(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "galaxy")
        message(FATAL_ERROR "GOG Galaxy backend is not set up for macOS in this build. Use -DACHIEVEMENTS_BACKEND=none")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")  
    message(STATUS "Building for macOS")
    target_link_libraries(skeldal ${all_libs} ${SERVICES_LIB})
else() 
    error("Platform not detected, please add new platform here")
endif()
set_property(TARGET skeldal_platform PROPERTY CXX_STANDARD 20)

 add_subdirectory(sdl)
