set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

string(TIMESTAMP TIMEZ UTC HADRON_BUILD_TIMESTAMP)
configure_file(BuildInfo.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp")

add_library(libhadron STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp"
    ErrorReporter.cpp
    ErrorReporter.hpp
    Lexer.cpp
    Lexer.hpp
    Parser.cpp
    Parser.hpp
)

target_compile_options(libhadron PUBLIC -Wall -Wextra -pedantic -Werror)
target_compile_definitions(libhadron PUBLIC DEBUG_LEXER)

target_link_libraries(libhadron
    fmt
    spdlog
)

######
# unittests
add_executable(unittests
    ErrorReporter_unittests.cpp
    Lexer_unittests.cpp
    unittests.cpp
)

target_link_libraries(unittests
    doctest
    libhadron
)

add_custom_target(run_unittests ALL
    DEPENDS unittests
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/unittests"
)

######
# vistool
add_executable(vistool
    vistool.cpp
)

target_link_libraries(vistool
    gflags
    libhadron
)

