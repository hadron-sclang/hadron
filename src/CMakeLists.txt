set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include(ExternalProject)

include_directories(
    "internal"
    "${INSTALL_EXT}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/internal")
configure_file(internal/BuildInfo.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/internal/BuildInfo.hpp")

# Hadron build process
# (a) Generate Lexer and Parser using Ragel and Bison, compile (maybe into frontend?)
# (b) Schema to process SuperCollider class files to generate Object, Kernel headers
# (c) Everything else, primitives, garbage collection, interpreter (backend)

# Dependency flow:
# server ----------------------\
#   |                          |
#   |                          v
#   \------> runtime -----> compiler
add_subdirectory(compiler)
add_subdirectory(runtime)
add_subdirectory(server)

#############
# unittests
add_executable(unittests
    ${HADRON_COMPILER_UNITTESTS}
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

#############
# hlang
add_executable(hlang
    hlang.cpp
)

target_link_libraries(hlang
    gflags
    hadron
)

#############
# hlangd
add_executable(hlangd
    hlangd.cpp
)

target_link_libraries(hlangd
    gflags
    server
)