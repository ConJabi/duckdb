set(DIFFERENTIAL_PRIVACY_EXTENSION_FILES
        ${DIFFERENTIAL_PRIVACY_EXTENSION_FILES}
        ${CMAKE_CURRENT_SOURCE_DIR}/add_bounds_to_column.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/add_null_replacement.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/make_table_private.cpp
        PARENT_SCOPE
)