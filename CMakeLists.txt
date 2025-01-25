cmake_minimum_required(VERSION 3.28)
project(skeldal)

include_directories(platform libs)
add_compile_options(-funsigned-char)
add_subdirectory(libs)
add_subdirectory(game)
