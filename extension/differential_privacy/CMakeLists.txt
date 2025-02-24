cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME differential_privacy)



set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})
include_directories(
        include
        OpenDP
)



set(DIFFERENTIAL_PRIVACY_EXTENSION_FILES
        differential_privacy_extension.cpp dp_parser.cpp duckdp_state.cpp)

add_subdirectory(functions)
add_subdirectory(pragma)


link_libraries(${EXTERNAL_LIB})


build_static_extension(differential_privacy ${DIFFERENTIAL_PRIVACY_EXTENSION_FILES})
build_loadable_extension(differential_privacy " " ${DIFFERENTIAL_PRIVACY_EXTENSION_FILES})

add_library(opendp SHARED IMPORTED

)
set_target_properties(opendp PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/extension/differential_privacy/OpenDP/libopendp.dylib"
)

target_link_libraries(${EXTENSION_NAME} opendp)
target_link_libraries(${LOADABLE_EXTENSION_NAME} opendp)

install(
        TARGETS differential_privacy_extension
        EXPORT "${DUCKDB_EXPORT_SET}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
