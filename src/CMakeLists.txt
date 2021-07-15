set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include(ExternalProject)

string(TIMESTAMP TIMEZ UTC HADRON_BUILD_TIMESTAMP)
configure_file(BuildInfo.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp")

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${INSTALL_EXT}/include"
)

# colm dependency to build Ragel
ExternalProject_add(colm
    PREFIX ext
    STEP_TARGETS install
    GIT_REPOSITORY https://github.com/adrian-thurston/colm
    GIT_TAG master
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${INSTALL_EXT} --disable-manual
    BUILD_IN_SOURCE ON
    BUILD_COMMAND make
    INSTALL_COMMAND make install
    EXCLUDE_FROM_ALL ON
)

# Ragel state machine generator used to generate the Lexer
ExternalProject_add(ragel
    PREFIX ext
    STEP_TARGETS install
    DEPENDS colm-install
    GIT_REPOSITORY https://github.com/adrian-thurston/ragel
    GIT_TAG master
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${INSTALL_EXT} --with-colm=${INSTALL_EXT} --disable-manual
    BUILD_IN_SOURCE ON
    BUILD_COMMAND make
    INSTALL_COMMAND make install
    EXCLUDE_FROM_ALL ON
)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp"
    COMMAND ${INSTALL_EXT}/bin/ragel -o "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Lexer.rl"
    MAIN_DEPENDENCY Lexer.rl
    DEPENDS ragel-install
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

# predefined symbol hash table
add_executable(hashkw
    hashkw.cpp
)

target_link_libraries(hashkw
    fmt
    xxHash::xxhash
)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Keywords.hpp"
    COMMAND hashkw
    DEPENDS hashkw
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

######
# libhadron
add_library(hadron STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Keywords.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp"
    ErrorReporter.cpp
    CodeGenerator.cpp
    CompilerContext.cpp
    FileSystem.hpp
    Hash.cpp
    include/hadron/CodeGenerator.hpp
    include/hadron/CompilerContext.hpp
    include/hadron/ErrorReporter.hpp
    include/hadron/Hash.hpp
    include/hadron/JIT.hpp
    include/hadron/Lexer.hpp
    include/hadron/LightningJIT.hpp
    include/hadron/Literal.hpp
    include/hadron/MachineCodeRenderer.hpp
    include/hadron/Parser.hpp
    include/hadron/Slot.hpp
    include/hadron/SyntaxAnalyzer.hpp
    include/hadron/Type.hpp
    include/hadron/VirtualJIT.hpp
    LightningJIT.cpp
    MachineCodeRenderer.cpp
    Parser.cpp
    Slot.cpp
    SyntaxAnalyzer.cpp
    VirtualJIT.cpp
)

file(GLOB HADRON_PUBLIC_HEADERS include/hadron/*.hpp)
set_target_properties(hadron PROPERTIES PUBLIC_HEADER "${HADRON_PUBLIC_HEADERS}")

target_compile_options(hadron PUBLIC -g -Wall -Wextra -Wpedantic -Werror)

target_link_libraries(hadron PUBLIC
    fmt
    spdlog
    xxHash::xxhash
    "${INSTALL_EXT}/lib/liblightning.a"
)

add_dependencies(hadron
    lightning-install
)

if(APPLE)
    add_dependencies(hadron
        ghc_filesystem-install
    )
endif()

install(TARGETS hadron
        ARCHIVE
        PUBLIC_HEADER
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hadron
)

######
# unittests
add_executable(unittests
    ErrorReporter_unittests.cpp
    Lexer_unittests.cpp
    MachineCodeRenderer_unittests.cpp
    Parser_unittests.cpp
    SyntaxAnalyzer_unittests.cpp

    unittests.cpp
)

target_link_libraries(unittests
    doctest
    hadron
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
    hadron
)

######
# hadronc
add_executable(hadronc
    hadronc.cpp
)

target_link_libraries(hadronc
    gflags
    hadron
)
