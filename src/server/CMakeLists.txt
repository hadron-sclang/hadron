#############
# libserver

add_library(server STATIC
    JSONTransport.hpp
    JSONTransport.cpp
)

target_link_libraries(server
    hadron
)