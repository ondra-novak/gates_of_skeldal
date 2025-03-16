cmake_minimum_required(VERSION 3.16)
project(skeldal)

# Najít SDL2 knihovnu
find_package(SDL2 REQUIRED)

if (MSVC)
    add_compile_options(/W4 /EHa /DNOMINMAX /J)
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
enable_testing()
add_subdirectory(libs)
add_subdirectory(platform)
add_subdirectory(game)

