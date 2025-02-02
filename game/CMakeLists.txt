add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/default_font.base64
                   COMMAND base64 ${CMAKE_CURRENT_LIST_DIR}/boldcz.fon -w 0  > ${CMAKE_BINARY_DIR}/default_font.base64
                   DEPENDS ${CMAKE_CURRENT_LIST_DIR}/boldcz.fon)


add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/default_font.cpp
    COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_BINARY_DIR}/default_font.cpp -DIN=${CMAKE_BINARY_DIR}/default_font.base64 -DTEMPLATE=${CMAKE_CURRENT_LIST_DIR}/default_font.in.cpp -P ${CMAKE_CURRENT_LIST_DIR}/default_font_to_cpp.cmake
    DEPENDS ${CMAKE_BINARY_DIR}/default_font.base64 ${CMAKE_CURRENT_LIST_DIR}/default_font_to_cpp.cmake ${CMAKE_CURRENT_LIST_DIR}/default_font.in.cpp 
    )
SET(files automap.c  
clk_map.c 
dialogy.c
dump.cpp
enemy.c
engine1.c
engine2.c
globmap.c
inv.c
kouzla.c
menu.c
skeldal.c
souboje.c   
builder.c  
engine1.c  
gamesave.c  
interfac.c  
kniha.c  
macros.c  
realgame.c  
setup.c
chargen.c   
sndandmus.c  
specproc.c
advconfig.c
temp_storage.cpp
${CMAKE_BINARY_DIR}/default_font.cpp
)

add_executable(skeldal  ${files})
target_link_libraries(skeldal 
    skeldal_libs 
    skeldal_platform
    skeldal_sdl
    skeldal_libs
     ${SDL2_LIBRARIES} pthread)
