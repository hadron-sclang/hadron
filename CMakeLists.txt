cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.9)

project(hadron VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED)
set(CMAKE_C_STANDARD 99) # Lightening uses C99 extensions

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(default_build_type "Debug")
else()
    set(GIT_BRANCH "not_a_git_checkout")
    set(GIT_COMMIT_HASH "na")
    set(default_build_type "RelWithDebInfo")
endif()

message(STATUS "hadron v${PROJECT_VERSION}, ${GIT_BRANCH}@${GIT_COMMIT_HASH}")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Determine host operating system and processor type. Host builds only supported for now, cross compiling is TBD.
if(APPLE)
    if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(HADRON_HOST_CPU "x86_64")
    else()
        # CMake documentation suggests that uname -m is returning inconsistent values on Apple silicon, so we assume
        # anying that isn't x86_64 must be 64-bit ARM.
        set(HADRON_HOST_CPU "aarch64")
    endif()
else()
    message(ERROR "TODO: add CPU detection to host OS")
endif()

message(STATUS "hadron detected host CPU ${HADRON_HOST_CPU}")

if(NOT DEFINED SCLANG_PATH)
     set(SCLANG_PATH "${CMAKE_SOURCE_DIR}/third_party/bootstrap/SCClassLibrary" CACHE STRING "Path to SCClassLibrary")
     # TODO: set back to supercollider once Hadron can compile the production class library.
     #    set(SCLANG_PATH "${CMAKE_SOURCE_DIR}/third_party/supercollider/SCClassLibrary" CACHE
     #        STRING "Path to SCClassLibrary")
endif()

set(HLANG_PATH "${CMAKE_SOURCE_DIR}/classes" CACHE STRING "Path to the HLang Class Library")

add_subdirectory(third_party)
add_subdirectory(src)
