set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/)

set(testFiles sdl_minimal_test.cpp sdl_surface_test.cpp
              )

foreach (testFile ${testFiles})
    string(REGEX MATCH "([^\/]+$)" filename ${testFile})
    string(REGEX MATCH "[^.]*" executable_name test_${filename})
    add_executable(${executable_name} ${testFile})
    target_link_libraries(${executable_name} ${SDL2_LIBRARIES} )
    add_test(NAME ${executable_name} COMMAND ${executable_name})
endforeach ()


add_executable(sdl_fullscreen_window sdl_fullscreen_window.cpp)
target_link_libraries(sdl_fullscreen_window ${SDL2_LIBRARIES} )