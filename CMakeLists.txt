cmake_minimum_required(VERSION 3.16)
project(skeldal)

# Najít SDL2 knihovnu
find_package(SDL2 REQUIRED)
set(STEAMWORKS_SDK_DIR "${CMAKE_SOURCE_DIR}/external/steamworks/")
# Check if Steamworks SDK directories exist
if(NOT EXISTS "${STEAMWORKS_SDK_DIR}/public")
    message(FATAL_ERROR "❌ Could not find Steamworks SDK 'public' headers. 
    Make sure to download the Steamworks SDK and place it in:${STEAMWORKS_SDK_DIR}
    Expected directory: ${STEAMWORKS_SDK_DIR}/public
    ")
endif()
if(NOT EXISTS "${STEAMWORKS_SDK_DIR}/redistributable_bin")
    message(FATAL_ERROR "❌ Could not find Steamworks SDK 'redistributable_bin' libraries.
    Make sure to download the Steamworks SDK and place it in: ${STEAMWORKS_SDK_DIR}
    Expected directory: ${STEAMWORKS_SDK_DIR}/redistributable_bin
    ")
endif()

if (MSVC)
    add_compile_options(/W4 /EHa /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /J)
    set(STANDARD_LIBRARIES "")
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-unused-result -Wno-unused-parameter -Wno-unused-value -Wno-extern-c-compat -funsigned-char)
    set(STANDARD_LIBRARIES "pthread")
endif()

include_directories(.)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)

include_directories( ${SDL2_INCLUDE_DIRS})
include_directories(${STEAMWORKS_SDK_DIR}/public)
add_subdirectory(libs)
add_subdirectory(platform)
add_subdirectory(game)
add_subdirectory(tools)

