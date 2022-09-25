#############
# libserver
include_directories(
    ../
    "${CMAKE_CURRENT_BINARY_DIR}/../"
)

find_program(gperf_program gperf)
if (NOT gperf_program)
    message(FATAL_ERROR "Could not find gperf")
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/LSPMethods.cpp"
    COMMAND ${gperf_program} --output-file=${CMAKE_CURRENT_BINARY_DIR}/LSPMethods.cpp ${CMAKE_CURRENT_SOURCE_DIR}/LSPMethods.cpp.in
    MAIN_DEPENDENCY LSPMethods.cpp.in
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

add_library(server STATIC
    "${CMAKE_CURRENT_BINARY_DIR}/LSPMethods.cpp"
    HadronServer.hpp
    HadronServer.cpp
    JSONTransport.hpp
    JSONTransport.cpp
    LSPTypes.hpp
)

target_link_libraries(server
    hadron
)

target_compile_options(server PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
     -Wall -Wextra -Wpedantic -Werror>
     $<$<CXX_COMPILER_ID:MSVC>:
     /W4 /WX /DNOMINMAX>
)
target_compile_definitions(server PUBLIC
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)