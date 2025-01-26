SET(files automap.c  
clk_map.c 
dialogy.c
dump.cpp
enemy.c
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
temp_storage.cpp)

add_executable(skeldal  ${files})
target_link_libraries(skeldal skeldal_libs skeldal_platform_libs ${SDL2_LIBRARIES} pthread)
