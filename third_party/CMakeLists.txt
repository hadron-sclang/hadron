include(ExternalProject)

set(INSTALL_EXT "${PROJECT_BINARY_DIR}/install-ext" CACHE PATH "Hadron External Dependencies")
file(MAKE_DIRECTORY "${INSTALL_EXT}")

#### doctest
add_subdirectory(doctest)

#### fmt
add_subdirectory(fmt)

#### gflags
set(GFLAGS_BUILD_STATIC_LIBS ON CACHE BOOL "build gflags as a static library")
set(GFLAGS_BUILD_gflags_LIB ON CACHE BOOL "build multithreaded gflags library")
add_subdirectory(gflags)

#### ghc filesystem (std::fs emulation)
if(APPLE)
    ExternalProject_add(ghc_filesystem
        PREFIX ext
        STEP_TARGETS install
        EXCLUDE_FROM_ALL ON
        GIT_REPOSITORY https://github.com/gulrak/filesystem.git
        GIT_TAG v1.5.8
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_EXT}
                   -DGHC_FILESYSTEM_BUILD_TESTING=OFF
                   -DGHC_FILESYSTEM_BUILD_EXAMPLES=OFF
                   -DGHC_FILESYSTEM_WITH_INSTALL=ON
                   -DCMAKE_BUILD_TYPE=Release
    )
endif()

#### lightening
add_library(lightening STATIC
    lightening/lightening/lightening.c
    lightening/lightening/endian.h
    lightening/lightening.h
)
target_include_directories(lightening PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lightening)

#### rapidjson
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "skip building rapidjson docs")
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "skip building rapidjson examples")
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "skip building rapidjson googletest tests")
set(RAPIDJSON_BUILD_CXX11 OFF CACHE BOOL "prefer cxx17 for rapidjson")
set(RAPIDJSON_BUILD_CXX17 ON CACHE BOOL "prefer cxx17 for rapidjson")
set(RAPIDJSON_ENABLE_INSTRUMENTATION_OPT OFF CACHE BOOL "clang compiler doesn't like -march")
add_subdirectory(rapidjson)
set(RAPIDJSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include CACHE STRING "rapidjson header include path")

#### spdlog
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "require spdlog to use our external fmt")
add_subdirectory(spdlog)

#### sprklr
add_subdirectory(sparkler)

#### xxHash
add_subdirectory(xxHash/cmake_unofficial)
