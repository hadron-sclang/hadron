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

#### spdlog
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "require spdlog to use our external fmt")
add_subdirectory(spdlog)

#### xxHash
add_subdirectory(xxHash/cmake_unofficial)
