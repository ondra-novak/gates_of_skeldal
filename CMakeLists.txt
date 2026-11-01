cmake_minimum_required(VERSION 3.16)
project(skeldal)

# Najít SDL2 knihovnu
find_package(SDL2 REQUIRED)

# ---------------------------------------------------------------------------
# Achievements backend
#
#   -DACHIEVEMENTS_BACKEND=auto|steam|galaxy|none   (default: auto)
#       auto   : steam if the Steamworks SDK is found, else galaxy if the
#                GOG Galaxy SDK is found, else none
#       steam  : Steamworks  (SDK in external/steamworks or -DSTEAMWORKS_SDK_DIR=...)
#       galaxy : GOG Galaxy  (SDK in external/galaxy     or -DGALAXY_SDK_DIR=...)
#                needs GOG client credentials, see GOG_CREDENTIALS_FILE below
#       none   : no achievements
#
#   -DGOG_CREDENTIALS_FILE=<path>  CMake file with
#          set(GOG_CLIENT_ID "...")
#          set(GOG_CLIENT_SECRET "...")
#       default: <source>/gog_credentials.cmake, then <source>/../gog_credentials.cmake
# ---------------------------------------------------------------------------
set(ACHIEVEMENTS_BACKEND "auto" CACHE STRING "Achievements backend: auto|steam|galaxy|none")
set_property(CACHE ACHIEVEMENTS_BACKEND PROPERTY STRINGS auto steam galaxy none)
set(STEAMWORKS_SDK_DIR "" CACHE PATH "Path to Steamworks SDK directory")
set(GALAXY_SDK_DIR "" CACHE PATH "Path to GOG Galaxy SDK directory")
set(GOG_CREDENTIALS_FILE "" CACHE FILEPATH "CMake file defining GOG_CLIENT_ID and GOG_CLIENT_SECRET")

# Older versions of this file FORCE-cached the literal "none" into
# STEAMWORKS_SDK_DIR when no SDK was found. CMake treats that string as true,
# so an existing build directory would resolve to the steam backend and then
# fail on the SDK check below - ignore it.
set(STEAMWORKS_DIR "")
if(STEAMWORKS_SDK_DIR AND NOT STEAMWORKS_SDK_DIR STREQUAL "none")
    set(STEAMWORKS_DIR "${STEAMWORKS_SDK_DIR}")
endif()
if(NOT STEAMWORKS_DIR AND EXISTS "${CMAKE_SOURCE_DIR}/external/steamworks/public/steam/steam_api.h")
    set(STEAMWORKS_DIR "${CMAKE_SOURCE_DIR}/external/steamworks")
endif()
set(GALAXY_DIR "${GALAXY_SDK_DIR}")
if(NOT GALAXY_DIR AND EXISTS "${CMAKE_SOURCE_DIR}/external/galaxy/Include/galaxy/GalaxyApi.h")
    set(GALAXY_DIR "${CMAKE_SOURCE_DIR}/external/galaxy")
endif()

if(ACHIEVEMENTS_BACKEND STREQUAL "auto")
    if(STEAMWORKS_DIR)
        set(ACHIEVEMENTS_BACKEND_RESOLVED "steam")
    elseif(GALAXY_DIR AND WIN32)
        # not picked automatically elsewhere: there is no Galaxy client for
        # Linux/macOS, so the platform CMakeLists would abort the build
        set(ACHIEVEMENTS_BACKEND_RESOLVED "galaxy")
    else()
        set(ACHIEVEMENTS_BACKEND_RESOLVED "none")
    endif()
else()
    set(ACHIEVEMENTS_BACKEND_RESOLVED "${ACHIEVEMENTS_BACKEND}")
endif()

if(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "steam")
    if(NOT EXISTS "${STEAMWORKS_DIR}/public/steam/steam_api.h")
        message(FATAL_ERROR "Steamworks SDK not found. Extract it to external/steamworks or set STEAMWORKS_SDK_DIR")
    endif()
    add_compile_definitions(STEAM_ENABLED)
    include_directories(${STEAMWORKS_DIR}/public)
elseif(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "galaxy")
    if(NOT EXISTS "${GALAXY_DIR}/Include/galaxy/GalaxyApi.h")
        message(FATAL_ERROR "GOG Galaxy SDK not found. Extract it to external/galaxy or set GALAXY_SDK_DIR")
    endif()
    if(NOT GOG_CREDENTIALS_FILE)
        if(EXISTS "${CMAKE_SOURCE_DIR}/gog_credentials.cmake")
            set(GOG_CREDENTIALS_FILE "${CMAKE_SOURCE_DIR}/gog_credentials.cmake")
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/../gog_credentials.cmake")
            set(GOG_CREDENTIALS_FILE "${CMAKE_SOURCE_DIR}/../gog_credentials.cmake")
        endif()
    endif()
    if(NOT GOG_CREDENTIALS_FILE OR NOT EXISTS "${GOG_CREDENTIALS_FILE}")
        message(FATAL_ERROR "GOG credentials file not found. Set GOG_CREDENTIALS_FILE to a CMake file defining GOG_CLIENT_ID and GOG_CLIENT_SECRET")
    endif()
    include("${GOG_CREDENTIALS_FILE}")
    if(NOT GOG_CLIENT_ID OR NOT GOG_CLIENT_SECRET OR GOG_CLIENT_ID MATCHES "^PUT_" OR GOG_CLIENT_SECRET MATCHES "^PUT_")
        message(FATAL_ERROR "GOG_CLIENT_ID / GOG_CLIENT_SECRET are not set in ${GOG_CREDENTIALS_FILE}")
    endif()
    configure_file("${CMAKE_SOURCE_DIR}/platform/gog_credentials.in.h" "${CMAKE_BINARY_DIR}/gog_credentials.h" @ONLY)
    add_compile_definitions(GALAXY_ENABLED)
    include_directories(${GALAXY_DIR}/Include)
    message(STATUS "GOG credentials: ${GOG_CREDENTIALS_FILE}")
elseif(ACHIEVEMENTS_BACKEND_RESOLVED STREQUAL "none")
    message("!!! compiling without achievements support")
    message("!!! set ACHIEVEMENTS_BACKEND to steam or galaxy and provide the SDK to enable them")
else()
    message(FATAL_ERROR "Unknown ACHIEVEMENTS_BACKEND '${ACHIEVEMENTS_BACKEND}' (use auto|steam|galaxy|none)")
endif()
message(STATUS "Achievements backend: ${ACHIEVEMENTS_BACKEND_RESOLVED}")

if (MSVC)
    add_compile_options(/W4 /EHa /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /J)
    set(STANDARD_LIBRARIES "")
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-unused-result -Wno-unused-parameter -Wno-unused-value -Wno-extern-c-compat -funsigned-char)
    set(STANDARD_LIBRARIES "pthread")
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

# -DAPP_VERSION_OVERRIDE=GOG-1.1.0 replaces the git-derived version shown in the main menu
set(APP_VERSION_OVERRIDE "" CACHE STRING "Override the version string (default: git describe)")
if(APP_VERSION_OVERRIDE)
    set(APP_VERSION "${APP_VERSION_OVERRIDE}")
else()
    set(APP_VERSION "${GIT_VERSION}")
endif()
message(STATUS "Version string: ${APP_VERSION}")
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
