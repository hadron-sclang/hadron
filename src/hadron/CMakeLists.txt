include_directories(
    "${INSTALL_EXT}/include"
)

# Hadron build process
# (a) Generate Lexer and Parser using Ragel and Bison
# (b) Using frontend files, build schema binary, process SuperCollider class files to generate schema headers
# (c) Build everything else, primitives, garbage collection, interpreter, etc.

#########################
# frontend source files

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

# predefined symbol hash table
add_executable(hashkw
    Hash.cpp
    Hash.hpp
    Slot.hpp
    Type.hpp

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
        message(STATUS "Found Bison binary installed by Homebrew at ${BREW_BISON_PREFIX}")
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

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/internal")
configure_file(internal/BuildInfo.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/internal/BuildInfo.hpp")

set(HADRON_FRONTEND_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Keywords.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/internal/BuildInfo.hpp"

    internal/FileSystem.cpp
    internal/FileSystem.hpp

    ErrorReporter.cpp
    ErrorReporter.hpp
    Hash.cpp
    Hash.hpp
    Lexer.hpp
    Parser.hpp
    Slot.hpp
    SourceFile.cpp
    SourceFile.hpp
    Type.hpp
)

#############
# schemac
add_executable(schemac
    ${HADRON_FRONTEND_FILES}
    schemac.cpp
)

target_link_libraries(schemac
    gflags
    fmt
    spdlog
    xxHash::xxhash
)

#############
# libhadron
list(APPEND SCLANG_CLASS_FILES
    "${SCLANG_PATH}/Common/Collections/Array.sc"
    "${SCLANG_PATH}/Common/Collections/ArrayedCollection.sc"
    "${SCLANG_PATH}/Common/Collections/Collection.sc"
    "${SCLANG_PATH}/Common/Collections/Dictionary.sc"
    "${SCLANG_PATH}/Common/Collections/SequenceableCollection.sc"
    "${SCLANG_PATH}/Common/Collections/Set.sc"
    "${SCLANG_PATH}/Common/Collections/String.sc"
    "${SCLANG_PATH}/Common/Core/AbstractFunction.sc"
#   "${SCLANG_PATH}/Common/Core/Boolean.sc"
    "${SCLANG_PATH}/Common/Core/Char.sc"
    "${SCLANG_PATH}/Common/Core/Function.sc"
    "${SCLANG_PATH}/Common/Core/Kernel.sc"
    "${SCLANG_PATH}/Common/Core/Nil.sc"
    "${SCLANG_PATH}/Common/Core/Object.sc"
    "${SCLANG_PATH}/Common/Core/Symbol.sc"
#    "${SCLANG_PATH}/Common/Math/Float.sc"
#    "${SCLANG_PATH}/Common/Math/Integer.sc"
    "${SCLANG_PATH}/Common/Math/Magnitude.sc"
#    "${SCLANG_PATH}/Common/Math/Number.sc"
#    "${SCLANG_PATH}/Common/Math/SimpleNumber.sc"
)

set(SCHEMA_FILES)

foreach(CLASS_FILE ${SCLANG_CLASS_FILES})
    string(REGEX REPLACE ".sc$" "" SCHEMA_BASE ${CLASS_FILE})
    string(REGEX REPLACE ${SCLANG_PATH} "" SCHEMA_BASE ${SCHEMA_BASE})
    set(SCHEMA_FILE ${CMAKE_CURRENT_BINARY_DIR}/schema/${SCHEMA_BASE}Schema.hpp)
    set(CASE_FILE ${CMAKE_CURRENT_BINARY_DIR}/case/${SCHEMA_BASE}PrimitiveCases.inl)
    get_filename_component(SCHEMA_FILE_DIRECTORY ${SCHEMA_FILE} DIRECTORY)
    file(MAKE_DIRECTORY ${SCHEMA_FILE_DIRECTORY})
    get_filename_component(CASE_FILE_DIRECTORY ${CASE_FILE} DIRECTORY)
    file(MAKE_DIRECTORY ${CASE_FILE_DIRECTORY})
    list(APPEND SCHEMA_FILES ${SCHEMA_FILE})
    add_custom_command(OUTPUT ${SCHEMA_FILE} ${CASE_FILE}
        COMMAND schemac --classFile ${CLASS_FILE} --schemaFile ${SCHEMA_FILE} --caseFile ${CASE_FILE}
        MAIN_DEPENDENCY ${CLASS_FILE}
        DEPENDS schemac
        VERBATIM
    )
endforeach()

add_custom_target(schemafiles ALL DEPENDS ${SCHEMA_FILES})

add_library(hadron STATIC
    ${HADRON_FRONTEND_FILES}
    ${SCHEMA_FILES}

    library/Array.hpp
    library/ArrayedCollection.hpp
    library/Collection.hpp
    library/Kernel.hpp
    library/Object.hpp
    library/Schema.hpp
    library/SequenceableCollection.hpp
    library/String.hpp
    library/Symbol.cpp
    library/Symbol.hpp

    Arch.hpp
    AST.hpp
    ASTBuilder.cpp
    ASTBuilder.hpp
    BlockBuilder.cpp
    BlockBuilder.hpp
    BlockSerializer.cpp
    BlockSerializer.hpp
    ClassLibrary.cpp
    ClassLibrary.hpp
    Emitter.cpp
    Emitter.hpp
    Heap.cpp
    Heap.hpp
    HIR.cpp
    HIR.hpp
    JIT.hpp
    LifetimeAnalyzer.cpp
    LifetimeAnalyzer.hpp
    LifetimeInterval.cpp
    LifetimeInterval.hpp
    LighteningJIT.cpp
    LighteningJIT.hpp
    LinearBlock.hpp
    MoveScheduler.cpp
    MoveScheduler.hpp
    OpcodeIterator.cpp
    OpcodeIterator.hpp
    Page.cpp
    Page.hpp
    Pipeline.cpp
    Pipeline.hpp
    PrimitiveDispatcher.cpp
    PrimitiveDispatcher.hpp
    RegisterAllocator.cpp
    RegisterAllocator.hpp
    Resolver.cpp
    Resolver.hpp
    Runtime.cpp
    Runtime.hpp
    Scope.hpp
    SymbolTable.cpp
    SymbolTable.hpp
    ThreadContext.hpp
    VirtualJIT.cpp
    VirtualJIT.hpp
)

set(HADRON_COMPILER_UNITTESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/ErrorReporter_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Lexer_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LifetimeInterval_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MoveScheduler_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OpcodeIterator_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Parser_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Slot_unittests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/library/ArrayedCollection_unittests.cpp

    PARENT_SCOPE
)

file(GLOB HADRON_PUBLIC_HEADERS *.hpp)
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
)

target_include_directories(hadron PUBLIC
    include
)

add_dependencies(hadron
    schemafiles
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
