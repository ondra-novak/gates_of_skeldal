add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/default_font.base64
                   COMMAND file_to_base64 ${CMAKE_CURRENT_LIST_DIR}/boldcz.fon  > ${CMAKE_BINARY_DIR}/default_font.base64
                   DEPENDS ${CMAKE_CURRENT_LIST_DIR}/boldcz.fon)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/icon.base64
                   COMMAND file_to_base64 ${CMAKE_CURRENT_LIST_DIR}/icon.bmp  > ${CMAKE_BINARY_DIR}/icon.base64
                   DEPENDS ${CMAKE_CURRENT_LIST_DIR}/icon.bmp)


add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/resources.cpp
    COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_BINARY_DIR}/resources.cpp -DIN_FONT=${CMAKE_BINARY_DIR}/default_font.base64 -DIN_ICON=${CMAKE_BINARY_DIR}/icon.base64 -DTEMPLATE=${CMAKE_CURRENT_LIST_DIR}/resources.in.cpp -P ${CMAKE_CURRENT_LIST_DIR}/resources_to_cpp.cmake
    DEPENDS ${CMAKE_BINARY_DIR}/default_font.base64 ${CMAKE_BINARY_DIR}/icon.base64 ${CMAKE_CURRENT_LIST_DIR}/resources_to_cpp.cmake ${CMAKE_CURRENT_LIST_DIR}/resources.in.cpp
    )
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
console.c
gen_stringtable.c
advconfig.c
temp_storage.cpp
lang.c
ach_events.c
${CMAKE_BINARY_DIR}/resources.cpp
)

add_library(skeldal_main  ${files})
set_property(TARGET skeldal_main PROPERTY CXX_STANDARD 20)
