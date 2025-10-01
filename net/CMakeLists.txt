# net/CMakeLists.txt
add_library(net_lib STATIC
    EventLoop.cpp  
)


target_include_directories(net_lib INTERFACE ${CMAKE_SOURCE_DIR})
# net_lib 依赖于 base_lib
target_link_libraries(net_lib PRIVATE base_lib)