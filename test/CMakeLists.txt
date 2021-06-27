set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src")

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/GrammarIteratorParser.cpp"
    COMMAND ${INSTALL_EXT}/bin/ragel -o "${CMAKE_CURRENT_BINARY_DIR}/GrammarIteratorParser.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/GrammarIteratorParser.rl"
    MAIN_DEPENDENCY GrammarIteratorParser.rl
    DEPENDS ragel-install
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)


add_executable(parser_exhaustive_test
   "${CMAKE_CURRENT_BINARY_DIR}/GrammarIteratorParser.cpp"
    GrammarIterator.hpp
    GrammarIterator.cpp
    parser_exhaustive_test.cpp
)

target_link_libraries(parser_exhaustive_test
    gflags
    hadron
)

