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
          strlite.c
          wav_mem.c
          strlists.c
          swaper.c )

add_library(skeldal_libs  ${files})

