set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

include(ExternalProject)

include_directories(
    "internal"
    "${INSTALL_EXT}/include"
)

string(TIMESTAMP TIMEZ UTC HADRON_BUILD_TIMESTAMP)
configure_file(internal/BuildInfo.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.hpp")

add_subdirectory(compiler)

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

###########
# vistool
add_executable(vistool
    vistool.cpp
)

target_link_libraries(vistool
    gflags
    hadron
)
