set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include(ExternalProject)

string(TIMESTAMP TIMEZ UTC HADRON_BUILD_TIMESTAMP)
configure_file(BuildInfo.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp")

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${INSTALL_EXT}/include"
    "${INSTALL_EXT}/llvm/include"
)

link_directories(
    "${INSTALL_EXT}/llvm/lib"
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

add_library(libhadron STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp"
    ClassLibrary.cpp
    ClassLibrary.hpp
    CodeGenerator.cpp
    CodeGenerator.hpp
    ErrorReporter.cpp
    ErrorReporter.hpp
    FileSystem.hpp
    LLVM.hpp
    Lexer.hpp
    Literal.hpp
    Parser.cpp
    Parser.hpp
    SymbolTable.cpp
    SymbolTable.hpp
    Type.hpp
)

add_dependencies(libhadron
    llvm-install
)

set(LLVM_JIT_LIBS
    "${INSTALL_EXT}/llvm/lib/libLLVMCore.a"
    "${INSTALL_EXT}/llvm/lib/libLLVMSupport.a"
)

target_compile_options(libhadron PUBLIC -g -Wall -Wextra -Wpedantic -Werror)

target_link_libraries(libhadron
    fmt
    spdlog
    xxHash::xxhash
    ${LLVM_JIT_LIBS}
)

if(APPLE)
    add_dependencies(libhadron
        ghc_filesystem-install
    )
endif()

######
# unittests
add_executable(unittests
    ErrorReporter_unittests.cpp
    Lexer_unittests.cpp
    Parser_unittests.cpp
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

