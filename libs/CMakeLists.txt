SET(files basicobj.c
          bgraph2.c
          bgraph2a.c
          bmouse.c
          devices.c
          event.c
          gui.c
          inicfg.c
          memman.c
          mgifmapmem.c
          mgifmem.c
          mgifplaya.c
          pcx.c
          wav_mem.c
          strlists.c
          cztable.c
          music.cpp
          string_table.cpp )

add_library(skeldal_libs  ${files})

add_executable(file_to_base64 file_to_base64.cpp)

set_property(TARGET skeldal_libs PROPERTY CXX_STANDARD 20)
set_property(TARGET file_to_base64 PROPERTY CXX_STANDARD 20)
