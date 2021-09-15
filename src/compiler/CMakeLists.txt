
include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${INSTALL_EXT}/include"
)

#############
# libhadron

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

if (APPLE)
    # Default installed Bison is 2.x, we need a newer version from homebrew.
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
    COMMAND "${BISON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/Parser.yy" -o "${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp" -Wall -Werror
    MAIN_DEPENDENCY Parser.yy
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

add_library(hadron STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/Assembler.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Keywords.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp"

    include/hadron/Assembler.hpp
    include/hadron/BlockBuilder.hpp
    include/hadron/BlockSerializer.hpp
    include/hadron/Emitter.hpp
    include/hadron/ErrorReporter.hpp
    include/hadron/Function.hpp
    include/hadron/Hash.hpp
    include/hadron/Heap.hpp
    include/hadron/HIR.hpp
    include/hadron/JIT.hpp
    include/hadron/JITMemoryArena.hpp
    include/hadron/Lexer.hpp
    include/hadron/LifetimeAnalyzer.hpp
    include/hadron/LifetimeInterval.hpp
    include/hadron/LighteningJIT.hpp
    include/hadron/LinearBlock.hpp
    include/hadron/LSBHashTable.hpp
    include/hadron/MoveScheduler.hpp
    include/hadron/Parser.hpp
    include/hadron/RegisterAllocator.hpp
    include/hadron/Resolver.hpp
    include/hadron/Slot.hpp
    include/hadron/SourceFile.hpp
    include/hadron/ThreadContext.hpp
    include/hadron/Type.hpp
    include/hadron/VirtualJIT.hpp

    BlockBuilder.cpp
    BlockSerializer.cpp
    Emitter.cpp
    ErrorReporter.cpp
    Hash.cpp
    Heap.cpp
    HIR.cpp
    Interpreter.cpp
    JITMemoryArena.cpp
    LifetimeAnalyzer.cpp
    LifetimeInterval.cpp
    LighteningJIT.cpp
    MoveScheduler.cpp
    RegisterAllocator.cpp
    Resolver.cpp
    Slot.cpp
    SourceFile.cpp
    ThreadContext.cpp
    VirtualJIT.cpp
)

set(HADRON_COMPILER_UNITTESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/Assembler_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ErrorReporter_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lexer_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LifetimeInterval_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LSBHashTable_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MoveScheduler_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Parser_unittests.cpp
    PARENT_SCOPE
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
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
    HADRON_64_BIT
)

target_link_libraries(hadron PUBLIC
    fmt
    spdlog
    xxHash::xxhash
    lightening
    "${INSTALL_EXT}/lib/libjemalloc.a"
)

target_include_directories(hadron PUBLIC
    include
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
