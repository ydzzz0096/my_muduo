add_library(net_lib STATIC
    EventLoop.cpp
    Channel.cpp
    Poller.cpp
    EpollPoller.cpp
    InetAddress.cpp 
    Socket.cpp        
)

target_include_directories(net_lib INTERFACE ${CMAKE_SOURCE_DIR})
target_link_libraries(net_lib PRIVATE base_lib)