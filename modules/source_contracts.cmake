# Source checks run independently of an existing header-native build tree.
set(_teaql_source_contract_tool ${PROJECT_SOURCE_DIR}/scripts/module_source_contracts.py)
set(_teaql_source_contract_manifest ${CMAKE_CURRENT_SOURCE_DIR}/source_contracts.json)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${_teaql_source_contract_tool} ${_teaql_source_contract_manifest}
    ${PROJECT_SOURCE_DIR}/tests/parity/frontend_bindings_5_0_6.py)
if(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    set(_teaql_source_contract_profile release)
else()
    set(_teaql_source_contract_profile debug)
endif()
execute_process(COMMAND ${Python3_EXECUTABLE} -B ${_teaql_source_contract_tool} emit
    --build-dir ${PROJECT_BINARY_DIR} --python ${Python3_EXECUTABLE}
    --assert-profile ${_teaql_source_contract_profile} --pointer-bytes ${CMAKE_SIZEOF_VOID_P}
    --output ${CMAKE_CURRENT_BINARY_DIR}/source_contracts.cmake
    COMMAND_ERROR_IS_FATAL ANY)
include(${CMAKE_CURRENT_BINARY_DIR}/source_contracts.cmake)
