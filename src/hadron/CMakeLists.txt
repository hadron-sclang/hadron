set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED)

include_directories(
    "${INSTALL_EXT}/include"
)

# Hadron build process
# (a) Generate Lexer and Parser using Ragel and Bison
# (b) Using frontend files, build schema binary, process SuperCollider class files to generate schema headers
# (c) Build everything else, primitives, garbage collection, interpreter, etc.

#########################
# frontend source files
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/internal")
configure_file(internal/BuildInfo.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/internal/BuildInfo.hpp")

set(HADRON_FRONTEND_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/internal/BuildInfo.hpp"

    internal/FileSystem.cpp
    internal/FileSystem.hpp

    Hash.cpp
    Hash.hpp
    Slot.hpp
    SourceFile.cpp
    SourceFile.hpp
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
    sprklr
    xxHash::xxhash
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
    "${SCLANG_PATH}/Common/Core/Boolean.sc"
    "${SCLANG_PATH}/Common/Core/Char.sc"
    "${SCLANG_PATH}/Common/Core/Function.sc"
    "${SCLANG_PATH}/Common/Core/Kernel.sc"
    "${SCLANG_PATH}/Common/Core/Nil.sc"
    "${SCLANG_PATH}/Common/Core/Object.sc"
    "${SCLANG_PATH}/Common/Core/Symbol.sc"
    "${SCLANG_PATH}/Common/Core/Thread.sc"
    "${SCLANG_PATH}/Common/Math/Float.sc"
    "${SCLANG_PATH}/Common/Math/Integer.sc"
    "${SCLANG_PATH}/Common/Math/Magnitude.sc"
    "${SCLANG_PATH}/Common/Math/Number.sc"
    "${SCLANG_PATH}/Common/Math/SimpleNumber.sc"
    "${SCLANG_PATH}/Common/Streams/Stream.sc"
)

set(SCHEMA_FILES)

foreach(CLASS_FILE ${SCLANG_CLASS_FILES})
    string(REGEX REPLACE ".sc$" "" SCHEMA_BASE ${CLASS_FILE})
    string(REGEX REPLACE ${SCLANG_PATH} "" SCHEMA_BASE ${SCHEMA_BASE})
    set(SCHEMA_FILE ${CMAKE_CURRENT_BINARY_DIR}/schema/${SCHEMA_BASE}Schema.hpp)
    get_filename_component(SCHEMA_FILE_DIRECTORY ${SCHEMA_FILE} DIRECTORY)
    file(MAKE_DIRECTORY ${SCHEMA_FILE_DIRECTORY})
    list(APPEND SCHEMA_FILES ${SCHEMA_FILE})
endforeach()


list(APPEND HLANG_CLASS_FILES
    "${HLANG_PATH}/HLang/HadronAST.sc"
    "${HLANG_PATH}/HLang/HadronCFG.sc"
    "${HLANG_PATH}/HLang/HadronHIR.sc"
    "${HLANG_PATH}/HLang/HadronParseNode.sc"
)

foreach(CLASS_FILE ${HLANG_CLASS_FILES})
    string(REGEX REPLACE ".sc$" "" SCHEMA_BASE ${CLASS_FILE})
    string(REGEX REPLACE ${HLANG_PATH} "" SCHEMA_BASE ${SCHEMA_BASE})
    set(SCHEMA_FILE ${CMAKE_CURRENT_BINARY_DIR}/schema/${SCHEMA_BASE}Schema.hpp)
    get_filename_component(SCHEMA_FILE_DIRECTORY ${SCHEMA_FILE} DIRECTORY)
    file(MAKE_DIRECTORY ${SCHEMA_FILE_DIRECTORY})
    list(APPEND SCHEMA_FILES ${SCHEMA_FILE})
endforeach()

add_custom_command(OUTPUT ${SCHEMA_FILES} ${CMAKE_CURRENT_BINARY_DIR}/ClassLibraryBootstrap.cpp
    COMMAND schemac --classFiles "${SCLANG_CLASS_FILES}" --libraryPath ${SCLANG_PATH}
            --hlangFiles "${HLANG_CLASS_FILES}" --hlangPath ${HLANG_PATH}
            --schemaPath ${CMAKE_CURRENT_BINARY_DIR}/schema
            --bootstrapPath ${CMAKE_CURRENT_BINARY_DIR}/ClassLibraryBootstrap.cpp
    DEPENDS ${SCLANG_CLASS_FILES} ${HLANG_CLASS_FILES} schemac
    VERBATIM
)

add_custom_target(schemafiles DEPENDS ${SCHEMA_FILES})

find_program(RAGEL_EXECUTABLE ragel REQUIRED)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp"
    COMMAND ${RAGEL_EXECUTABLE} -o "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Lexer.rl"
    MAIN_DEPENDENCY Lexer.rl
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)


if (HADRON_HOST_CPU STREQUAL "x86_64")
    set(HADRON_GENERATOR_FILES
        "GeneratorX64.cpp"
    )
elseif(HADRON_HOST_CPU STREQUAL "aarch64")
    set(HADRON_GENERATOR_FILES
        "GeneratorA64.cpp"
    )
endif()

add_library(hadron STATIC
    ${HADRON_FRONTEND_FILES}
    ${HADRON_GENERATOR_FILES}
    ${SCHEMA_FILES}
    "${CMAKE_CURRENT_BINARY_DIR}/Lexer.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Parser.cpp"

    library/AbstractFunction.hpp
    library/Array.hpp
    library/ArrayedCollection.hpp
    library/Boolean.hpp
    library/Collection.hpp
    library/Dictionary.hpp
    library/Function.hpp
    library/HadronAST.hpp
    library/HadronCFG.cpp
    library/HadronCFG.hpp
    library/HadronHIR.cpp
    library/HadronHIR.hpp
    library/HadronParseNode.hpp
    library/Integer.hpp
    library/Interpreter.cpp
    library/Kernel.hpp
    library/Object.hpp
    library/Schema.hpp
    library/SequenceableCollection.hpp
    library/Set.hpp
    library/Stream.hpp
    library/String.cpp
    library/String.hpp
    library/Symbol.cpp
    library/Symbol.hpp
    library/Thread.hpp

    ASTBuilder.cpp
    ASTBuilder.hpp
    BlockBuilder.cpp
    BlockBuilder.hpp
    ClassLibrary.cpp
    ClassLibrary.hpp
    Generator.cpp
    Generator.hpp
    Heap.cpp
    Heap.hpp
    Lexer.hpp
    Page.cpp
    Page.hpp
    Parser.hpp
    Runtime.cpp
    Runtime.hpp
    SlotDumpJSON.cpp
    SlotDumpJSON.hpp
    SymbolTable.cpp
    SymbolTable.hpp
    ThreadContext.hpp
)

set(HADRON_COMPILER_UNITTESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/Lexer_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Slot_unittests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/library/Array_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/ArrayedCollection_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/Dictionary_unittests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/LibraryTestFixture.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/Set_unittests.cpp

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
    asmjit
    fmt
    spdlog
    xxHash::xxhash
)

target_include_directories(hadron PUBLIC
    include
    ${RAPIDJSON_INCLUDE_DIR}
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
