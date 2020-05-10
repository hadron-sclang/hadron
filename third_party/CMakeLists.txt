### doctest
add_subdirectory(doctest)

### fmt
add_subdirectory(fmt)

### spdlog
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "require spdlog to use our external fmt")
add_subdirectory(spdlog)

