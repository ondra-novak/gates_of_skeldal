cmake_minimum_required(VERSION 3.16)
project(skeldal)

# Najít SDL2 knihovnu
find_package(SDL2 REQUIRED)

set(STEAMWORKS_SDK_DIR "" CACHE PATH "Path to Steamworks SDK directory")

if(NOT STEAMWORKS_SDK_DIR)
    set(POSSIBLE_STEAMWORKS_DIR "${CMAKE_SOURCE_DIR}/external/steamworks")
    if(EXISTS "${POSSIBLE_STEAMWORKS_DIR}/public")
        if(NOT EXISTS "${STEAMWORKS_SDK_DIR}/redistributable_bin") 
            set(STEAMWORKS_SDK_DIR "${POSSIBLE_STEAMWORKS_DIR}" CACHE PATH "Path to Steamworks SDK directory" FORCE)
        endif()
    endif()    
    if (NOT STEAMWORKS_SDK_DIR)
         message("!!! Steam is not installed")
         message("!!! compiling without steam and without achievements support")
         message("!!! To enable steam, set STEAMWORKS_SDK_DIR to correct value")
         set(STEAMWORKS_SDK_DIR "none" CACHE PATH "Path to Steamworks SDK directory" FORCE) 
    endif()
endif()

if (MSVC)
    add_compile_options(/W4 /EHa /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /J)
    set(STANDARD_LIBRARIES "")
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-unused-result -Wno-unused-parameter -Wno-unused-value -Wno-extern-c-compat -funsigned-char)
    set(STANDARD_LIBRARIES "pthread")
endif()



if(${STEAMWORKS_SDK_DIR} STREQUAL "none")
    set(STEAM_ENABLED 0)
else()
    add_compile_definitions(STEAM_ENABLED)
    include_directories(${STEAMWORKS_SDK_DIR}/public)
    set(STEAM_ENABLED 1)

endif()

set(VERSION_IN "${CMAKE_SOURCE_DIR}/version.in.h")
set(VERSION_OUT "${CMAKE_BINARY_DIR}/version.h")

find_package(Git QUIET)

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        set(GIT_VERSION "unknown")
    endif()
else()
    set(GIT_VERSION "unknown")
endif()

set(APP_VERSION "${GIT_VERSION}")
configure_file(${VERSION_IN} ${VERSION_OUT} @ONLY)

include_directories(${CMAKE_BINARY_DIR})

include_directories(.)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)

include_directories( ${SDL2_INCLUDE_DIRS})
add_subdirectory(libs)
add_subdirectory(platform)
add_subdirectory(game)
add_subdirectory(tools)

