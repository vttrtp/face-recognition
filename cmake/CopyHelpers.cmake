# Helper functions for copying files to target output directories

# Copy data files next to a target executable/library
# Usage: copy_data_to_target(my_app)
function(copy_data_to_target TARGET_NAME)
    if(EXISTS "${CMAKE_BINARY_DIR}/data")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/data"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_BINARY_DIR}/data"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/data"
            COMMENT "Copying data files to ${TARGET_NAME} output directory"
        )
    endif()
endfunction()

# Copy a shared library next to a target (for runtime loading)
# Creates a stamp file to track changes and re-copy when library is rebuilt
# Usage: copy_library_to_target(my_app my_shared_lib)
function(copy_library_to_target TARGET_NAME LIB_TARGET)
    set(STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${LIB_TARGET}_copied")
    add_custom_command(
        OUTPUT "${STAMP_FILE}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${LIB_TARGET}>"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E touch "${STAMP_FILE}"
        DEPENDS ${LIB_TARGET}
        COMMENT "Copying ${LIB_TARGET} library to ${TARGET_NAME} output directory"
    )
    add_custom_target(copy_${LIB_TARGET}_to_${TARGET_NAME} ALL
        DEPENDS "${STAMP_FILE}"
    )
endfunction()
