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
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/adrian-thurston/colm
    GIT_TAG master
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${INSTALL_EXT} --disable-manual
    BUILD_IN_SOURCE ON
    BUILD_COMMAND make -j
    INSTALL_COMMAND make install
    EXCLUDE_FROM_ALL ON
)

# Ragel state machine generator used to generate the Lexer
ExternalProject_add(ragel
    PREFIX ext
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    DEPENDS colm-install
    GIT_REPOSITORY https://github.com/adrian-thurston/ragel
    GIT_TAG master
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${INSTALL_EXT} --with-colm=${INSTALL_EXT} --disable-manual
    BUILD_IN_SOURCE ON
    BUILD_COMMAND make -j
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

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Assembler.cpp"
    COMMAND ${INSTALL_EXT}/bin/ragel -o "${CMAKE_CURRENT_BINARY_DIR}/Assembler.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Assembler.rl"
    MAIN_DEPENDENCY Assembler.rl
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

# find a version of Bison newer than 3.2
if (APPLE)
    execute_process(
        COMMAND brew --prefix bison
        RESULT_VARIABLE BREW_BISON
        OUTPUT_VARIABLE BREW_BISON_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (BREW_BISON EQUAL 0 AND EXISTS "${BREW_BISON_PREFIX}")
        message(STATUS "Found Bison keg installed by Homebrew at ${BREW_BISON_PREFIX}")
        set(BISON_EXECUTABLE "${BREW_BISON_PREFIX}/bin/bison")
    endif()
endif()
find_package(BISON REQUIRED)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp"
    COMMAND "${BISON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/Parser.yy" -o "${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp" -Wcounterexamples
    MAIN_DEPENDENCY Parser.yy
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

######
# libhadron
add_library(hadron STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/Assembler.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Keywords.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp"

    include/hadron/Assembler.hpp
    include/hadron/CodeGenerator.hpp
    include/hadron/ErrorReporter.hpp
    include/hadron/Function.hpp
    include/hadron/Hash.hpp
    include/hadron/HIR.hpp
    include/hadron/JIT.hpp
    include/hadron/JITMemoryArena.hpp
    include/hadron/Lexer.hpp
    include/hadron/LighteningJIT.hpp
    include/hadron/LSBHashTable.hpp
    include/hadron/MachineCodeRenderer.hpp
    include/hadron/Parser.hpp
    include/hadron/Slot.hpp
    include/hadron/SourceFile.hpp
    include/hadron/SSABuilder.hpp
    include/hadron/SyntaxAnalyzer.hpp
    include/hadron/ThreadContext.hpp
    include/hadron/Type.hpp
    include/hadron/VirtualJIT.hpp

    CodeGenerator.cpp
    ErrorReporter.cpp
    FileSystem.hpp
    Function.cpp
    Hash.cpp
    HIR.cpp
    Interpreter.cpp
    JITMemoryArena.cpp
    LighteningJIT.cpp
    MachineCodeRenderer.cpp
#    Parser.cpp
    Slot.cpp
    SourceFile.cpp
    SSABuilder.cpp
    SyntaxAnalyzer.cpp
    ThreadContext.cpp
    VirtualJIT.cpp
)

file(GLOB HADRON_PUBLIC_HEADERS include/hadron/*.hpp)
set_target_properties(hadron PROPERTIES PUBLIC_HEADER "${HADRON_PUBLIC_HEADERS}")

target_compile_options(hadron PUBLIC
    -g
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

target_compile_definitions(hadron PUBLIC
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
    HADRON_64_BIT
)

target_link_libraries(hadron PUBLIC
    fmt
    spdlog
    xxHash::xxhash
    lightening
    "${INSTALL_EXT}/lib/libjemalloc.a"
)

add_dependencies(hadron
    jemalloc-install
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
    Assembler_unittests.cpp
    ErrorReporter_unittests.cpp
    Lexer_unittests.cpp
    LSBHashTable_unittests.cpp
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

######
# hlang
add_executable(hlang
    hlang.cpp
)

target_link_libraries(hlang
    gflags
    hadron
)
