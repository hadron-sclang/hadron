set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include(ExternalProject)

include_directories(
    "${INSTALL_EXT}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

add_subdirectory(hadron)
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
#add_executable(hlang
#    hlang.cpp
#)

#target_link_libraries(hlang
#    gflags
#    hadron
#)

#############
# hlangd
add_executable(hlangd
    hlangd.cpp
)

target_link_libraries(hlangd
    gflags
    server
)