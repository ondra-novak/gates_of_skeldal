add_executable(ddl_ar ddl_ar.cpp ddl_ar_class.cpp)
add_executable(pcx_diff_tool pcx_diff_tool.c)
add_executable(deenc deenc.c)

set_property(TARGET ddl_ar PROPERTY CXX_STANDARD 20)
