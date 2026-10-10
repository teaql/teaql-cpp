# Facades publish the canonical global declarations and link existing bodies.
# Module-only targets keep header consumers and layer metadata independent.
function(teaql_add_module package)
    add_library(${package}_module)
    string(REGEX REPLACE "^teaql_" "" component ${package})
    add_library(teaql::${component}_module ALIAS ${package}_module)
    target_compile_features(${package}_module PUBLIC cxx_std_20)
    set_target_properties(${package}_module PROPERTIES CXX_EXTENSIONS OFF
        CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(${package}_module PUBLIC ${package})
    target_sources(${package}_module PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS ${PROJECT_SOURCE_DIR}/${package}/src FILES
        ${PROJECT_SOURCE_DIR}/${package}/src/lib.cppm)
endfunction()

teaql_add_module(teaql_memory)
teaql_add_module(teaql_async)
teaql_add_module(teaql_core)
teaql_add_module(teaql_data_service)
teaql_add_module(teaql_sql)
teaql_add_module(teaql_runtime)
teaql_add_module(teaql_provider_sqlite)
target_link_libraries(teaql_async_module PUBLIC teaql_memory_module)
target_link_libraries(teaql_core_module PUBLIC teaql_memory_module)
target_link_libraries(teaql_data_service_module PUBLIC teaql_core_module teaql_async_module)
target_link_libraries(teaql_sql_module PUBLIC teaql_data_service_module)
target_link_libraries(teaql_runtime_module PUBLIC teaql_sql_module)
target_link_libraries(teaql_provider_sqlite_module PUBLIC teaql_runtime_module)

if(TEAQL_BUILD_EXAMPLES)
    foreach(example IN ITEMS teaql_sqlite_schema_crud teaql_sqlite_relations_graph)
        target_compile_definitions(${example} PRIVATE TEAQL_USE_MODULES=1)
        set_target_properties(${example} PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
        target_link_libraries(${example} PRIVATE teaql_provider_sqlite_module)
    endforeach()
endif()

if(BUILD_TESTING)
    add_executable(teaql_module_contracts tests/contracts.cpp tests/header_bridge.cpp tests/lookup.cpp
        tests/reverse_imports.cpp tests/mixed_imports.cpp tests/teaql_memory_lookup.cpp
        tests/teaql_async_lookup.cpp tests/teaql_core_lookup.cpp tests/teaql_data_service_lookup.cpp
        tests/teaql_sql_lookup.cpp tests/teaql_runtime_lookup.cpp tests/teaql_provider_sqlite_lookup.cpp)
    set_target_properties(teaql_module_contracts PROPERTIES CXX_EXTENSIONS OFF
        CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_contracts PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_import_contracts COMMAND teaql_module_contracts)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set(import_contracts ${CMAKE_CURRENT_BINARY_DIR}/consumers)
    execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/prepare_contracts.py
        --output ${import_contracts} COMMAND_ERROR_IS_FATAL ANY)
    file(READ ${import_contracts}/manifest.json prepared_sources)
    string(JSON prepared_count LENGTH "${prepared_sources}")
    math(EXPR last_prepared "${prepared_count} - 1")
    foreach(prepared_index RANGE ${last_prepared})
        string(JSON prepared_source GET "${prepared_sources}" ${prepared_index} source)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            ${PROJECT_SOURCE_DIR}/${prepared_source})
    endforeach()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS prepare_contracts.py contracts.json)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/tests/support/sqlite_execution_fixture.hpp
        ${PROJECT_SOURCE_DIR}/tests/support/sql_transaction_fixture.hpp)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/contracts.json contract_manifest)
    string(JSON contract_count LENGTH "${contract_manifest}" contracts)
    math(EXPR last_contract "${contract_count} - 1")
    foreach(index RANGE ${last_contract})
        string(JSON package GET "${contract_manifest}" contracts ${index} package)
        string(JSON contract GET "${contract_manifest}" contracts ${index} stem)
        string(JSON source ERROR_VARIABLE source_error GET "${contract_manifest}" contracts ${index} source)
        if(source_error)
            set(source ${package}/tests/${contract}.cpp)
        endif()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/${source})
        add_executable(teaql_module_${contract} ${import_contracts}/${contract}.cpp)
        set_target_properties(teaql_module_${contract} PROPERTIES CXX_EXTENSIONS OFF
            CXX_SCAN_FOR_MODULES ON)
        target_link_libraries(teaql_module_${contract} PRIVATE teaql_provider_sqlite_module)
        if(package STREQUAL "teaql_provider_sqlite")
            target_link_libraries(teaql_module_${contract} PRIVATE
                SQLite3::SQLite3)
        endif()
        string(JSON case_count ERROR_VARIABLE case_error LENGTH "${contract_manifest}" contracts ${index} cases)
        if(case_error)
            add_test(NAME module_${contract} COMMAND teaql_module_${contract})
        else()
            math(EXPR last_case "${case_count} - 1")
            foreach(case_index RANGE ${last_case})
                string(JSON case_name GET "${contract_manifest}" contracts ${index} cases ${case_index} name)
                string(JSON arg_count LENGTH "${contract_manifest}" contracts ${index} cases ${case_index} arguments)
                set(case_args "")
                if(arg_count GREATER 0)
                    math(EXPR last_arg "${arg_count} - 1")
                    foreach(arg_index RANGE ${last_arg})
                        string(JSON arg GET "${contract_manifest}" contracts ${index} cases ${case_index} arguments ${arg_index})
                        # Store canonical paths relative to the source root in the manifest.
                        string(REPLACE "${PROJECT_SOURCE_DIR}/" "" arg "${arg}")
                        if(EXISTS "${PROJECT_SOURCE_DIR}/${arg}")
                            set(arg "${PROJECT_SOURCE_DIR}/${arg}")
                        endif()
                        list(APPEND case_args "${arg}")
                    endforeach()
                endif()
                add_test(NAME module_${case_name} COMMAND teaql_module_${contract} ${case_args})
                string(JSON env_count LENGTH "${contract_manifest}" contracts ${index} cases ${case_index} environment)
                set(case_environment "")
                if(env_count GREATER 0)
                    math(EXPR last_env "${env_count} - 1")
                    foreach(env_index RANGE ${last_env})
                        string(JSON value GET "${contract_manifest}" contracts ${index} cases ${case_index} environment ${env_index})
                        list(APPEND case_environment "${value}")
                    endforeach()
                    set_tests_properties(module_${case_name} PROPERTIES ENVIRONMENT "${case_environment}")
                endif()
            endforeach()
        endif()
    endforeach()
    add_test(NAME module_original_runtime_optimistic_conflict_output_contracts
        COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/teaql_runtime/optimistic_conflict_output.py
        --cpp $<TARGET_FILE:teaql_module_original_runtime_memory_service>)
    foreach(example IN ITEMS sqlite_schema_crud sqlite_relations_graph)
        add_executable(teaql_module_${example} ${import_contracts}/${example}.cpp)
        set_target_properties(teaql_module_${example} PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
        target_link_libraries(teaql_module_${example} PRIVATE teaql_provider_sqlite_module)
        add_test(NAME module_example_${example} COMMAND teaql_module_${example})
        set_tests_properties(module_example_${example} PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endforeach()
    add_test(NAME module_examples_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_runtime_examples.py
        --schema-crud $<TARGET_FILE:teaql_module_sqlite_schema_crud>
        --relations-graph $<TARGET_FILE:teaql_module_sqlite_relations_graph>)
    set_tests_properties(module_examples_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    if(TEAQL_BUILD_EXAMPLES)
        add_test(NAME module_canonical_examples_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_runtime_examples.py
            --schema-crud $<TARGET_FILE:teaql_sqlite_schema_crud>
            --relations-graph $<TARGET_FILE:teaql_sqlite_relations_graph>)
        set_tests_properties(module_canonical_examples_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endif()
