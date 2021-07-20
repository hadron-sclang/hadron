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

### GNU Lightning
ExternalProject_add(lightning
    PREFIX ext
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://git.savannah.gnu.org/git/lightning.git
    GIT_TAG master
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE ON
    CONFIGURE_COMMAND ./bootstrap && ./configure --prefix=${INSTALL_EXT}
    BUILD_COMMAND make -j
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS ${INSTALL_EXT}/lib/liblightning.a
)

### jemalloc
ExternalProject_add(jemalloc
    PREFIX ext
    STEP_TARGETS install
    EXCLUDE_FROM_ALL ON
    GIT_REPOSITORY https://github.com/jemalloc/jemalloc.git
    GIT_TAG 5.2.1
    GIT_PROGRESS ON
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE ON
    CONFIGURE_COMMAND ./autogen.sh --prefix=/Users/luken/src/hadron/build/install-ext --disable-cxx --disable-zone-allocator
    BUILD_COMMAND make -j
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS ${INSTALL_EXT}/lib/libjemalloc.a
)

### lightening
add_library(lightening STATIC
    lightening/lightening/lightening.c
    lightening/lightening/endian.h
    lightening/lightening.h
)
target_include_directories(lightening PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lightening)

#### spdlog
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "require spdlog to use our external fmt")
add_subdirectory(spdlog)

#### xxHash
add_subdirectory(xxHash/cmake_unofficial)
