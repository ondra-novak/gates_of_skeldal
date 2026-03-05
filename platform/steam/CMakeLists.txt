#add_subdirectory(tests)

add_library(skeldal_steam steamservice.cpp)
set_property(TARGET skeldal_steam  PROPERTY CXX_STANDARD 20)
