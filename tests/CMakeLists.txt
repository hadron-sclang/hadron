include(CTest)

add_test(NAME lexical_analysis/floats COMMAND htest ${CMAKE_CURRENT_SOURCE_DIR}/lexical_analysis/floats.sctest)
add_test(NAME lexical_analysis/integers COMMAND htest ${CMAKE_CURRENT_SOURCE_DIR}/lexical_analysis/integers.sctest)
