cmake_minimum_required(VERSION 3.28)
project(skeldal)

# Najít SDL2 knihovnu
find_package(SDL2 REQUIRED)

if (MSVC)
    add_compile_options(/W4 /EHsc /DNOMINMAX)
    set(STANDARD_LIBRARIES "")
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-value)
    set(STANDARD_LIBRARIES "pthread")
endif()

include_directories(platform libs)


include_directories( ${SDL2_INCLUDE_DIRS})
add_compile_options(-funsigned-char)
enable_testing()
add_subdirectory(libs)
add_subdirectory(platform)
add_subdirectory(game)
