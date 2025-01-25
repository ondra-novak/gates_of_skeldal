SET(files automap.c  clk_map.c  dialogy.c  dump.cpp     enemy.c    engine2.c   globmap.c   inv.c    kouzla.c  menu.c skeldal.c    souboje.c   
builder.c  engine1.c  gamesave.c  interfac.c  kniha.c  macros.c  realgame.c  setup.c   sndandmus.c  specproc.c)

add_executable(skeldal  ${files})
target_link_libraries(skeldal skeldal_libs)
