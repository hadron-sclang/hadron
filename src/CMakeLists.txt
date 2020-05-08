set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

string(TIMESTAMP TIMEZ UTC HADRON_BUILD_TIMESTAMP)
configure_file(BuildInfo.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp")

add_library(libhadron STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp"
    Lexer.cpp
    Lexer.hpp
)

add_executable(run_unittests
    Lexer_unittests.cpp
    run_unittests.cpp
)

target_link_libraries(run_unittests
    doctest
    libhadron
)
