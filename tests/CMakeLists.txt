include(CTest)

set(SCTESTS
    expressions/messages
    expressions/primitives

    lexical_analysis/floats
    lexical_analysis/functions
    lexical_analysis/integers
)

foreach(SCTEST ${SCTESTS})
    add_test(NAME ${SCTEST} COMMAND htest "${CMAKE_CURRENT_SOURCE_DIR}/${SCTEST}.sctest")
endforeach()
