#############
# libruntime

# Probably move Heap, Interpreter in here but Schema goes into compiler.

list(APPEND SCLANG_CLASS_FILES
    "${SCLANG_PATH}/Common/Core/Kernel.sc"
    "${SCLANG_PATH}/Common/Core/Object.sc"
)

set(SCHEMA_FILES)

foreach(CLASS_FILE ${SCLANG_CLASS_FILES})
    # Obtain relative path of the output file.
    string(REGEX REPLACE ".sc$" ".cpp" SCHEMA_FILE ${CLASS_FILE})
    set(SCHEMA_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SCHEMA_FILE})
#    message(STATUS "Processing ${CLASS_FILE} to ${SCHEMA_FILE}")
    list(APPEND SCHEMA_FILES ${SCHEMA_FILE})
    # Create custom command which generates {out_file} from the {in_file}
    add_custom_command(OUTPUT ${SCHEMA_FILE}
        COMMAND schema --classFile ${CLASS_FILE} --schemaFile ${SCHEMA_FILE}
        MAIN_DEPENDENCY ${CLASS_FILE}
        DEPENDS schema
        VERBATIM
    )
endforeach()

add_custom_target(ProcessFiles ALL DEPENDS ${SCHEMA_FILES})

#add_library(runtime STATIC
#    ${SCHEMA_FILES}
#)

#target_link_libraries(runtime
#    compiler
#)