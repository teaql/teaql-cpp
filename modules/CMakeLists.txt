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
    foreach(example IN ITEMS teaql_sqlite_schema_crud teaql_sqlite_relations_graph teaql_generated_query_expression teaql_custom_checker_behavior)
        target_compile_definitions(${example} PRIVATE TEAQL_USE_MODULES=1)
        set_target_properties(${example} PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
        target_link_libraries(${example} PRIVATE teaql_provider_sqlite_module)
    endforeach()
    if(BUILD_TESTING)
        add_test(NAME module_generated_query_expression COMMAND teaql_generated_query_expression)
        set_tests_properties(module_generated_query_expression PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
        add_test(NAME module_custom_checker_behavior COMMAND teaql_custom_checker_behavior)
        set_tests_properties(module_custom_checker_behavior PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endif()

if(BUILD_TESTING)
    add_executable(teaql_module_runtime_initialization_tests
        ${PROJECT_SOURCE_DIR}/teaql_provider_sqlite/tests/runtime_initialization_tests.cpp)
    target_compile_definitions(teaql_module_runtime_initialization_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_runtime_initialization_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_runtime_initialization_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_runtime_initialization_contracts COMMAND teaql_module_runtime_initialization_tests)
    set_tests_properties(module_runtime_initialization_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_assist_workflow_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/assist_workflow_tests.cpp "${teaql_assist_examples}")
    add_dependencies(teaql_module_assist_workflow_tests teaql_assist_workflow_examples)
    target_include_directories(teaql_module_assist_workflow_tests PRIVATE
        ${PROJECT_BINARY_DIR}/generated_assist ${PROJECT_SOURCE_DIR}/examples/generated/include)
    target_compile_definitions(teaql_module_assist_workflow_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_assist_workflow_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_assist_workflow_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_assist_workflow_contracts COMMAND teaql_module_assist_workflow_tests)
    set_tests_properties(module_assist_workflow_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_business_id_environment_tests
        ${PROJECT_SOURCE_DIR}/teaql_runtime/tests/business_id_environment_tests.cpp)
    target_compile_definitions(teaql_module_business_id_environment_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_business_id_environment_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_business_id_environment_tests PRIVATE teaql_runtime_module)
    add_test(NAME module_business_id_environment_contracts COMMAND teaql_module_business_id_environment_tests)
    set_tests_properties(module_business_id_environment_contracts PROPERTIES ENVIRONMENT
        "TEAQL_LOG_ENDPOINT=off;TEAQL_BUSINESS_ID_KEY_V1_HEX=000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f;TEAQL_BUSINESS_ID_KEY_VERSION=1")
    add_executable(teaql_module_assist_support_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/assist_support_tests.cpp "${teaql_assist_examples}")
    add_dependencies(teaql_module_assist_support_tests teaql_assist_workflow_examples)
    target_include_directories(teaql_module_assist_support_tests PRIVATE
        ${PROJECT_BINARY_DIR}/generated_assist ${PROJECT_SOURCE_DIR}/examples/generated/include)
    target_compile_definitions(teaql_module_assist_support_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_assist_support_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_assist_support_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_assist_support_contracts COMMAND teaql_module_assist_support_tests)
    set_tests_properties(module_assist_support_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_document_round_trip_tests
        ${PROJECT_SOURCE_DIR}/teaql_runtime/tests/document_round_trip_tests.cpp)
    target_compile_definitions(teaql_module_document_round_trip_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_document_round_trip_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_document_round_trip_tests PRIVATE teaql_runtime_module)
    add_test(NAME module_document_round_trip_contracts COMMAND teaql_module_document_round_trip_tests)
    add_test(NAME module_document_service_async_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_document_service_async_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_document_service_async_probe>)
    set_tests_properties(module_document_service_async_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_document_providers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_document_providers_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_document_providers_probe>)
    set_tests_properties(module_document_providers_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_transaction_scope_intent_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_transaction_scope_intent_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_transaction_scope_intent_probe>)
    set_tests_properties(module_transaction_scope_intent_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 180)
    add_test(NAME module_reference_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_reference_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_reference_model_probe>)
    set_tests_properties(module_reference_model_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_document_model_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_document_model_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_document_model_debug_probe>)
    add_test(NAME module_document_round_trip_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_document_round_trip_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_document_round_trip_probe>)
    set_tests_properties(module_document_model_debug_rust_parity
        module_document_round_trip_5_0_6_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_document_boundaries_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_document_boundaries_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_document_round_trip_probe>)
    set_tests_properties(module_document_boundaries_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_context_root_generated tests/context_root_generated.cpp)
    target_compile_definitions(teaql_module_context_root_generated PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_context_root_generated PROPERTIES
        CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_context_root_generated PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_context_root_generated_contracts COMMAND teaql_module_context_root_generated)
    set_tests_properties(module_context_root_generated_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_generated_sample_fields tests/generated_sample_fields.cpp)
    target_compile_definitions(teaql_module_generated_sample_fields PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_sample_fields PROPERTIES
        CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_sample_fields PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_sample_fields_contracts COMMAND teaql_module_generated_sample_fields)
    set_tests_properties(module_generated_sample_fields_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_generated_optional_types_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_optional_types_tests.cpp)
    target_compile_definitions(teaql_module_generated_optional_types_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_optional_types_tests PROPERTIES
        CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_optional_types_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_optional_types_contracts COMMAND teaql_module_generated_optional_types_tests)
    set_tests_properties(module_generated_optional_types_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_generated_inheritance_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_inheritance_tests.cpp)
    target_compile_definitions(teaql_module_generated_inheritance_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_inheritance_tests PROPERTIES
        CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_inheritance_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_inheritance_contracts COMMAND teaql_module_generated_inheritance_tests)
    set_tests_properties(module_generated_inheritance_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_generated_candidates_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_candidates_tests.cpp)
    target_compile_definitions(teaql_module_generated_candidates_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_candidates_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_candidates_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_candidates_contracts COMMAND teaql_module_generated_candidates_tests)
    add_executable(teaql_module_generated_bootstrap_model_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_bootstrap_model_tests.cpp)
    target_compile_definitions(teaql_module_generated_bootstrap_model_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_bootstrap_model_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_bootstrap_model_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_bootstrap_model_contracts COMMAND teaql_module_generated_bootstrap_model_tests)
    add_executable(teaql_module_generated_bootstrap_tests
        ${PROJECT_SOURCE_DIR}/teaql_runtime/tests/generated_bootstrap_tests.cpp)
    target_compile_definitions(teaql_module_generated_bootstrap_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_bootstrap_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_bootstrap_tests PRIVATE teaql_runtime_module)
    add_test(NAME module_generated_bootstrap_contracts COMMAND teaql_module_generated_bootstrap_tests)
    add_executable(teaql_module_generated_inheritance_graph_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_inheritance_graph_tests.cpp)
    target_compile_definitions(teaql_module_generated_inheritance_graph_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_inheritance_graph_tests PROPERTIES
        CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_inheritance_graph_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_inheritance_graph_contracts COMMAND teaql_module_generated_inheritance_graph_tests)
    set_tests_properties(module_generated_inheritance_graph_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_generated_inheritance_variants_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_inheritance_variants_tests.cpp)
    target_compile_definitions(teaql_module_generated_inheritance_variants_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_inheritance_variants_tests PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_inheritance_variants_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_inheritance_variants_contracts COMMAND teaql_module_generated_inheritance_variants_tests)
    set_tests_properties(module_generated_inheritance_variants_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_generated_inheritance_query_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_inheritance_query_tests.cpp)
    target_compile_definitions(teaql_module_generated_inheritance_query_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_inheritance_query_tests PROPERTIES
        CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_inheritance_query_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_inheritance_query_contracts COMMAND teaql_module_generated_inheritance_query_tests)
    add_executable(teaql_module_generated_return_type_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_return_type_tests.cpp)
    target_compile_definitions(teaql_module_generated_return_type_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_return_type_tests PROPERTIES
        CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_return_type_tests PRIVATE teaql_provider_sqlite_module SQLite3::SQLite3)
    add_test(NAME module_generated_return_type_contracts COMMAND teaql_module_generated_return_type_tests)
    set_tests_properties(module_generated_return_type_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_generated_dynamic_field_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_dynamic_field_tests.cpp)
    target_compile_definitions(teaql_module_generated_dynamic_field_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_dynamic_field_tests PROPERTIES
        CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_dynamic_field_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_dynamic_field_contracts COMMAND teaql_module_generated_dynamic_field_tests)
    set_tests_properties(module_generated_dynamic_field_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_module_generated_graph_serialization_tests ${PROJECT_SOURCE_DIR}/tests/generator/generated_graph_serialization_tests.cpp)
    target_compile_definitions(teaql_module_generated_graph_serialization_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_graph_serialization_tests PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_graph_serialization_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_graph_serialization_contracts COMMAND teaql_module_generated_graph_serialization_tests)
    target_link_options(teaql_module_generated_graph_serialization_tests PRIVATE "LINKER:--wrap=_ZN5teaql18EntityRuntimeStateC1ESt15allocator_arg_tNSt3pmr21polymorphic_allocatorISt4byteEERKS0_")
    target_link_options(teaql_module_generated_graph_serialization_tests PRIVATE "LINKER:--wrap=_ZN5teaql18EntityRuntimeStateC2ESt15allocator_arg_tNSt3pmr21polymorphic_allocatorISt4byteEERKS0_")
    add_executable(teaql_module_generated_readonly_property_tests
        ${PROJECT_SOURCE_DIR}/tests/generator/generated_readonly_property_tests.cpp)
    target_compile_definitions(teaql_module_generated_readonly_property_tests PRIVATE TEAQL_USE_MODULES=1)
    set_target_properties(teaql_module_generated_readonly_property_tests PROPERTIES
        CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON)
    target_link_libraries(teaql_module_generated_readonly_property_tests PRIVATE teaql_provider_sqlite_module)
    add_test(NAME module_generated_readonly_property_contracts COMMAND teaql_module_generated_readonly_property_tests)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(teaql_module_generated_readonly_property_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEmm")
    else()
        target_link_options(teaql_module_generated_readonly_property_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEjj")
    endif()
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
        string(JSON contract_import ERROR_VARIABLE import_error GET "${contract_manifest}" contracts ${index} import_module)
        if(import_error)
            set(contract_import teaql_provider_sqlite)
        endif()
        if(NOT TARGET "${contract_import}_module")
            message(FATAL_ERROR "Unknown contract import module: ${contract_import}")
        endif()
        target_link_libraries(teaql_module_${contract} PRIVATE ${contract_import}_module)
        if(package STREQUAL "teaql_provider_sqlite")
            target_link_libraries(teaql_module_${contract} PRIVATE
                SQLite3::SQLite3)
        endif()
        string(JSON case_count ERROR_VARIABLE case_error LENGTH "${contract_manifest}" contracts ${index} cases)
        if(case_error)
            add_test(NAME module_${contract} COMMAND teaql_module_${contract})
        elseif(case_count GREATER 0)
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
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(teaql_module_json_graph_allocation_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEmm")
    else()
        target_link_options(teaql_module_json_graph_allocation_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEjj")
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(teaql_module_json_shape_allocation_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEmm")
    else()
        target_link_options(teaql_module_json_shape_allocation_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEjj")
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(teaql_module_state_cache_cost_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEmm")
    else()
        target_link_options(teaql_module_state_cache_cost_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEjj")
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(teaql_module_allocation_counter_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEmm")
    else()
        target_link_options(teaql_module_allocation_counter_tests PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEjj")
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(teaql_module_hydration_allocation_probe PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEmm")
    else()
        target_link_options(teaql_module_hydration_allocation_probe PRIVATE "LINKER:--wrap=_ZN5teaql5Arena11do_allocateEjj")
    endif()
    set_tests_properties(module_load_state_allocation_contracts PROPERTIES TIMEOUT 180)
    set_tests_properties(module_shared_state_source_contracts PROPERTIES TIMEOUT 180)
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

if(TARGET teaql_module_hydration_sibling_keys_probe)
    add_test(NAME module_hydration_sibling_keys_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_hydration_sibling_keys_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_hydration_sibling_keys_probe>)
endif()

if(TARGET teaql_module_hydration_owner_facets_probe)
    add_test(NAME module_hydration_owner_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_hydration_owner_facets_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_hydration_owner_facets_probe>)
endif()

if(TARGET teaql_module_hydration_owner_facets_async_probe)
    add_test(NAME module_hydration_owner_facets_async_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_hydration_owner_facets_async_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_hydration_owner_facets_async_probe>)
endif()

if(TARGET teaql_module_graph_relation_facets_probe)
    add_test(NAME module_graph_relation_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_graph_relation_facets_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_graph_relation_facets_probe>)
endif()

if(TARGET teaql_module_hydration_flat_facets_probe)
    add_test(NAME module_hydration_flat_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_hydration_flat_facets_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_hydration_flat_facets_probe>)
endif()

if(TARGET teaql_module_loaded_row_graph_probe)
    add_test(NAME module_loaded_row_graph_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_loaded_row_graph_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_loaded_row_graph_probe>)
endif()

if(TARGET teaql_module_relation_hydration_replay_probe)
    add_test(NAME module_relation_hydration_replay_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_hydration_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_relation_hydration_replay_probe>)
    add_test(NAME module_relation_hydration_intent_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_hydration_intent_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_relation_hydration_replay_probe>)
endif()

if(TARGET teaql_module_json_graph_views_probe)
    add_test(NAME module_json_graph_views_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_json_graph_views_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_json_graph_views_probe>)
endif()

if(TARGET teaql_module_json_hydration_probe)
    add_test(NAME module_json_hydration_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_json_hydration_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_json_hydration_probe>)
endif()

if(TARGET teaql_module_generated_json_hydration_probe)
    add_test(NAME module_generated_json_column_alias_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_json_column_alias_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_json_hydration_probe>)
    add_test(NAME module_generated_json_hydration_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_json_hydration_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_json_hydration_probe>)
endif()

if(TARGET teaql_module_generated_inherited_json_probe)
    add_test(NAME module_generated_inherited_json_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_inherited_json_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_inherited_json_probe>)
endif()
if(TARGET teaql_module_generated_json_graph_views_probe)
    add_test(NAME module_generated_json_graph_views_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_json_graph_views_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_json_graph_views_probe>)
endif()

if(TARGET teaql_module_dynamic_audit_projection_probe)
    add_test(NAME module_dynamic_audit_projection_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_audit_projection_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_audit_projection_probe>)
endif()

if(TARGET teaql_module_dynamic_save_audit_probe)
    target_compile_definitions(teaql_module_dynamic_save_audit_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_dynamic_typed_save_audit_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_typed_save_audit_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_save_audit_probe>)
    add_test(NAME module_dynamic_save_audit_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_save_audit_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_save_audit_probe>)
endif()

if(TARGET teaql_module_generated_execution_probe)
    target_compile_definitions(teaql_module_generated_execution_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_pagination_convenience_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_pagination_convenience_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_execution_probe>)
    add_test(NAME module_generated_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_execution.py
        --cpp $<TARGET_FILE:teaql_module_generated_execution_probe>)
endif()

if(TARGET teaql_module_generated_execution_shapes_probe)
    target_compile_definitions(teaql_module_generated_execution_shapes_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_execution_shapes_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_execution_shapes_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_execution_shapes_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_inheritance_probe)
    add_test(NAME module_generated_dynamic_json_inheritance_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_inheritance_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_inheritance_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_scalar_probe)
    target_compile_definitions(teaql_module_generated_dynamic_json_scalar_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_dynamic_json_scalar_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_scalar_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_scalar_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_domains_probe)
    target_compile_definitions(teaql_module_generated_dynamic_json_domains_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_dynamic_json_domains_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_domains_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_domains_probe>)
endif()

if(TARGET teaql_module_generated_enhancement_inputs_probe)
    target_compile_definitions(teaql_module_generated_enhancement_inputs_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_enhancement_inputs_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_enhancement_inputs_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_enhancement_inputs_probe>)
endif()

if(TARGET teaql_module_generated_selection_scope_probe)
    target_compile_definitions(teaql_module_generated_selection_scope_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_selection_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_selection_scope_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_selection_scope_probe>)
endif()

if(TARGET teaql_module_generated_type_match_ranges_probe)
    target_compile_definitions(teaql_module_generated_type_match_ranges_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_type_match_ranges_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_type_match_ranges_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_type_match_ranges_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_privacy_probe)
    target_compile_definitions(teaql_module_generated_dynamic_json_privacy_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_dynamic_json_privacy_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_privacy_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_privacy_probe>)
    add_test(NAME module_generated_dynamic_json_privacy_sink_contracts COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/verify_dynamic_json_privacy_sinks_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_privacy_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_inherited_sql_probe)
    target_compile_definitions(teaql_module_generated_dynamic_json_inherited_sql_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_dynamic_json_inherited_sql_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_inherited_sql_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_inherited_sql_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_composition_probe)
    target_compile_definitions(teaql_module_generated_dynamic_json_composition_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_dynamic_json_composition_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_composition_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_composition_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_facets_probe)
    target_compile_definitions(teaql_module_generated_dynamic_json_facets_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_dynamic_json_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_facets_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_facets_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_persistent_probe)
    target_compile_definitions(teaql_module_generated_dynamic_json_persistent_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_dynamic_json_persistent_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_persistent_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_persistent_probe>)
endif()

if(TARGET teaql_module_expression_diagnostic_text_probe)
    add_test(NAME module_expression_diagnostic_framing_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_expression_diagnostic_framing_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_expression_diagnostic_text_probe>)
    add_test(NAME module_expression_diagnostic_text_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_expression_diagnostic_text_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_expression_diagnostic_text_probe>)
endif()

foreach(topology_extension_case IN ITEMS ordered reversed self_reference_required)
    if(TARGET teaql_module_constant_topology_extensions_${topology_extension_case}_probe)
        add_test(NAME module_constant_topology_extensions_${topology_extension_case}_rust_parity
            COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_constant_topology_extensions_5_0_6.py
            --case ${topology_extension_case}
            --cpp $<TARGET_FILE:teaql_module_constant_topology_extensions_${topology_extension_case}_probe>)
        set_tests_properties(module_constant_topology_extensions_${topology_extension_case}_rust_parity
            PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 180)
    endif()
endforeach()

foreach(inherited_business_case IN ITEMS governed update)
    if(TARGET teaql_module_inherited_business_${inherited_business_case}_probe)
        add_test(NAME module_inherited_business_${inherited_business_case}_rust_parity
            COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_inherited_business_${inherited_business_case}.py
            --cpp $<TARGET_FILE:teaql_module_inherited_business_${inherited_business_case}_probe>)
        set_tests_properties(module_inherited_business_${inherited_business_case}_rust_parity
            PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 30)
    endif()
endforeach()

if(TARGET teaql_module_generated_constant_variants_probe)
    add_test(NAME module_constant_governance_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_constant_governance_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_constant_variants_probe>)
endif()

if(TARGET teaql_module_generated_dynamic_json_probe)
    target_compile_definitions(teaql_module_generated_dynamic_json_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_dynamic_json_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_execution_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_probe>)
    set_tests_properties(module_dynamic_json_execution_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_json_policy_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_json_policy_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_dynamic_json_probe>)
    set_tests_properties(module_dynamic_json_policy_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_derived_query_diagnostics_probe)
    add_test(NAME module_derived_query_diagnostics_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_derived_query_diagnostics_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_derived_query_diagnostics_probe>)
    set_tests_properties(module_derived_query_diagnostics_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_derived_query_composition_probe)
    add_test(NAME module_derived_query_composition_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_derived_query_composition_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_derived_query_composition_probe>)
    set_tests_properties(module_derived_query_composition_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_generated_support_source_probe)
    add_test(NAME module_generated_support_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_support_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_support_source_probe>)
endif()

if(TARGET teaql_module_schema_dynamic_source_probe)
    add_test(NAME module_schema_dynamic_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_schema_dynamic_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_schema_dynamic_source_probe>)
endif()

foreach(registry_probe IN ITEMS entity_decoder json_graph_decoder runtime_graph registry_smart_list_source)
    if(TARGET teaql_module_${registry_probe}_probe)
        if(registry_probe STREQUAL "entity_decoder" OR registry_probe STREQUAL "runtime_graph")
            set(registry_comparator compare_${registry_probe}_5_0_6.py)
        elseif(registry_probe STREQUAL "json_graph_decoder")
            set(registry_comparator compare_json_graph_decoder_5_0_6.py)
        else()
            set(registry_comparator compare_registry_smart_list_source_5_0_6.py)
        endif()
        add_test(NAME module_${registry_probe}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/${registry_comparator}
            --cpp $<TARGET_FILE:teaql_module_${registry_probe}_probe>)
        set_tests_properties(module_${registry_probe}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

foreach(context_probe IN ITEMS context_defaults context_model checker data_store internal_id)
    if(TARGET teaql_module_${context_probe}_probe)
        if(context_probe STREQUAL "data_store")
            set(context_comparator compare_data_store.py)
        else()
            set(context_comparator compare_${context_probe}_5_0_6.py)
        endif()
        add_test(NAME module_${context_probe}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/${context_comparator}
            --cpp $<TARGET_FILE:teaql_module_${context_probe}_probe>)
        set_tests_properties(module_${context_probe}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

if(TARGET teaql_module_check_model_probe)
    add_test(NAME module_check_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_check_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_check_model_probe>)
    set_tests_properties(module_check_model_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_generated_support_probe)
    add_test(NAME module_public_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_support.py
        --cpp $<TARGET_FILE:teaql_module_generated_support_probe>)
    add_test(NAME module_public_facet_logs_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_public_facet_logs_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_support_probe>)
    add_test(NAME module_purposed_query_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_purposed_query_debug.py
        --cpp $<TARGET_FILE:teaql_module_generated_support_probe>)
    set_tests_properties(module_public_facets_rust_parity module_public_facet_logs_rust_parity
        PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_query_keys_probe)
    add_test(NAME module_query_keys_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_keys.py
        --cpp $<TARGET_FILE:teaql_module_query_keys_probe>)
    add_test(NAME module_query_metadata_keys_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_metadata_keys_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_query_keys_probe>)
endif()

if(TARGET teaql_module_deferred_id_set_page_probe)
    target_compile_definitions(teaql_module_deferred_id_set_page_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_deferred_id_set_page_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_deferred_id_set_page_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_deferred_id_set_page_probe>)
    set_tests_properties(module_deferred_id_set_page_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_deferred_plain_page_probe)
    target_compile_definitions(teaql_module_deferred_plain_page_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_deferred_plain_page_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_deferred_plain_page_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_deferred_plain_page_probe>)
    set_tests_properties(module_deferred_plain_page_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_deferred_continuous_page_probe)
    target_compile_definitions(teaql_module_deferred_continuous_page_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_deferred_continuous_page_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_deferred_continuous_page_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_deferred_continuous_page_probe>)
    set_tests_properties(module_deferred_continuous_page_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

foreach(candidate_case IN LISTS TEAQL_SCALAR_CANDIDATE_CASES)
    if(TARGET teaql_module_scalar_candidate_${candidate_case}_probe)
        add_test(NAME module_scalar_candidate_${candidate_case}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_scalar_candidates_5_0_6.py
            --case ${candidate_case} --cpp $<TARGET_FILE:teaql_module_scalar_candidate_${candidate_case}_probe>)
        set_tests_properties(module_scalar_candidate_${candidate_case}_rust_parity PROPERTIES
            ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 30)
    endif()
endforeach()

if(TARGET teaql_module_bootstrap_temporal_extensions_probe AND TARGET teaql_sample_fixed_clock)
    add_test(NAME module_bootstrap_temporal_extensions_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_bootstrap_temporal_extensions_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_bootstrap_temporal_extensions_probe>
        --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
    set_tests_properties(module_bootstrap_temporal_extensions_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_constant_temporal_probe AND TARGET teaql_sample_fixed_clock)
    add_test(NAME module_constant_temporal_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_constant_temporal_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_constant_temporal_probe>
        --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
    set_tests_properties(module_constant_temporal_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
    add_test(NAME module_clock_epoch_boundary_contracts COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_clock_epoch_boundary_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_clock_epoch_boundary_probe>
        --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
endif()

foreach(type_case IN LISTS TEAQL_TYPE_CONFIGURATION_CASES)
    if(TARGET teaql_module_type_configuration_${type_case}_probe)
        add_test(NAME module_type_configuration_${type_case}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_type_configuration_execution_5_0_6.py
            --case ${type_case} --cpp $<TARGET_FILE:teaql_module_type_configuration_${type_case}_probe>)
        set_tests_properties(module_type_configuration_${type_case}_rust_parity PROPERTIES
            ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
    endif()
endforeach()

if(TARGET teaql_module_bootstrap_conflict_probe)
    add_test(NAME module_bootstrap_conflict_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_bootstrap_conflict_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_bootstrap_conflict_probe>)
    set_tests_properties(module_bootstrap_conflict_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 180)
endif()

if(TARGET teaql_module_bootstrap_extensions_probe)
    add_test(NAME module_bootstrap_extensions_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_bootstrap_extensions_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_bootstrap_extensions_probe>)
    set_tests_properties(module_bootstrap_extensions_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 180)
endif()

foreach(metadata_case IN LISTS TEAQL_CONSTANT_METADATA_CASES)
    if(TARGET teaql_module_constant_metadata_${metadata_case}_probe)
        add_test(NAME module_constant_metadata_${metadata_case}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_constant_metadata_execution_5_0_6.py
            --case ${metadata_case} --cpp $<TARGET_FILE:teaql_module_constant_metadata_${metadata_case}_probe>)
        set_tests_properties(module_constant_metadata_${metadata_case}_rust_parity PROPERTIES
            ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
    endif()
endforeach()

foreach(topology_case IN LISTS TEAQL_CONSTANT_TOPOLOGY_CASES)
    if(TARGET teaql_module_constant_topology_${topology_case}_probe)
        add_test(NAME module_constant_topology_${topology_case}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_constant_topology_execution_5_0_6.py
            --case ${topology_case} --cpp $<TARGET_FILE:teaql_module_constant_topology_${topology_case}_probe>)
        set_tests_properties(module_constant_topology_${topology_case}_rust_parity PROPERTIES
            ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
    endif()
endforeach()

if(TARGET teaql_module_data_service_model_probe)
    add_test(NAME module_data_service_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_data_service_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_data_service_model_probe>)
    set_tests_properties(module_data_service_model_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_data_service_request_source_probe)
    add_test(NAME module_data_service_request_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_data_service_request_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_data_service_request_source_probe>)
    set_tests_properties(module_data_service_request_source_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

foreach(state_baseline IN ITEMS i18n i18n_debug_scope entity_runtime data_service_model_scope typed_aggregate_scope)
    if(TARGET teaql_module_${state_baseline}_probe)
        add_test(NAME module_${state_baseline}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${state_baseline}.py
            --cpp $<TARGET_FILE:teaql_module_${state_baseline}_probe>)
        set_tests_properties(module_${state_baseline}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

foreach(logging_baseline IN ITEMS runtime_log audit_event context_logging log_manager context_event)
    if(TARGET teaql_module_${logging_baseline}_probe)
        add_test(NAME module_${logging_baseline}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${logging_baseline}.py
            --cpp $<TARGET_FILE:teaql_module_${logging_baseline}_probe>)
        set_tests_properties(module_${logging_baseline}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

if(TARGET teaql_module_runtime_error_source_probe)
    add_test(NAME module_runtime_error_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_runtime_error_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_error_source_probe>)
    set_tests_properties(module_runtime_error_source_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_sql_log_query_source_probe)
    add_test(NAME module_sql_log_query_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sql_log_query_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sql_log_query_source_probe>)
    set_tests_properties(module_sql_log_query_source_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()
foreach(sql_log_matrix IN ITEMS sql_log sql_model_api)
    if(TARGET teaql_module_${sql_log_matrix}_probe)
        set(sql_log_comparator compare_${sql_log_matrix}_5_0_6.py)
        add_test(NAME module_${sql_log_matrix}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/${sql_log_comparator}
            --cpp $<TARGET_FILE:teaql_module_${sql_log_matrix}_probe>)
        set_tests_properties(module_${sql_log_matrix}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()
if(TARGET teaql_module_log_privacy)
    add_test(NAME module_original_runtime_file_log_privacy_contracts COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/teaql_runtime/file_log_privacy.py
        --cpp $<TARGET_FILE:teaql_module_log_privacy>)
endif()
if(TARGET teaql_module_audit_event_projection_probe)
    add_test(NAME module_audit_event_projection_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_audit_event_projection_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_audit_event_projection_probe>)
endif()

foreach(final_entry IN ITEMS runtime_id_models runtime_error_debug unified_buffer_clone locking inmemory entity_reference round_trip_reference decimal_pointer_scope entity_query_boundaries)
    if(final_entry STREQUAL "runtime_id_models")
        set(final_probe runtime_id_model)
        set(final_test runtime_id_model)
    elseif(final_entry STREQUAL "runtime_error_debug")
        set(final_probe runtime_check)
        set(final_test runtime_error_debug)
    elseif(final_entry STREQUAL "decimal_pointer_scope")
        set(final_probe decimal)
        set(final_test decimal_pointer_scope)
    elseif(final_entry STREQUAL "entity_query_boundaries")
        set(final_probe entity_query)
        set(final_test entity_query_boundary)
    elseif(final_entry STREQUAL "locking")
        set(final_probe locking)
        set(final_test context_locking)
    else()
        set(final_probe ${final_entry})
        set(final_test ${final_entry})
    endif()
    if(TARGET teaql_module_${final_probe}_probe)
        add_test(NAME module_${final_test}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${final_entry}.py
            --cpp $<TARGET_FILE:teaql_module_${final_probe}_probe>)
        set_tests_properties(module_${final_test}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()
if(TARGET teaql_module_log_model_probe)
    add_test(NAME module_log_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_log_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_log_model_probe>)
    set_tests_properties(module_log_model_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_log_formatter_probe)
    add_test(NAME module_log_formatter_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_log_formatter.py
        --cpp ${CMAKE_CURRENT_BINARY_DIR})
    set_tests_properties(module_log_formatter_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off;TEAQL_PARITY_CPP_PREFIX=teaql_module_")
endif()

foreach(runtime_baseline IN ITEMS runtime_registry runtime_context runtime_check telemetry telemetry_model_scope registry_debug_scope schema_runtime business_service runtime_business_id)
    if(TARGET teaql_module_${runtime_baseline}_probe)
        add_test(NAME module_${runtime_baseline}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${runtime_baseline}.py
            --cpp $<TARGET_FILE:teaql_module_${runtime_baseline}_probe>)
        set_tests_properties(module_${runtime_baseline}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

# Replay the complete existing kernel corpora through their canonical imported
# observers. Original-snapshot cases share the existing multi-mode observer.
if(TARGET teaql_module_generated_load_state_bulk_probe)
    set_tests_properties(module_generated_load_state_bulk_contracts PROPERTIES TIMEOUT 180)
    add_test(NAME module_generated_load_state_bulk_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_load_state_bulk_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_load_state_bulk_probe>)
    set_tests_properties(module_generated_load_state_bulk_rust_parity PROPERTIES TIMEOUT 180)
endif()

if(TARGET teaql_module_generated_member_alias_probe)
    add_test(NAME module_generated_member_alias_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_member_aliases_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_member_alias_probe>)
    set_tests_properties(module_generated_member_alias_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(Python3_Interpreter_FOUND)
    add_test(NAME module_dynamic_fields_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_fields.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_fields_probe>)
    set_tests_properties(module_dynamic_fields_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_storage_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_storage.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_storage_probe>)
    set_tests_properties(module_dynamic_storage_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_ledger_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_ledger.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_ledger_probe>)
    set_tests_properties(module_dynamic_ledger_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_governance_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_governance.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_governance_probe>)
    set_tests_properties(module_dynamic_governance_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_graph_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_graph_debug.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_graph_debug_probe>)
    set_tests_properties(module_dynamic_graph_debug_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_query.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_query_probe>)
    set_tests_properties(module_dynamic_query_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_relation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_relation.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_query_probe>)
    set_tests_properties(module_dynamic_relation_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_provider_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_provider.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_provider_probe>)
    set_tests_properties(module_dynamic_provider_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_sqlite_dynamic_storage_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sqlite_dynamic_storage.py
        --cpp $<TARGET_FILE:teaql_module_sqlite_dynamic_storage_probe>)
    add_test(NAME module_sqlite_dynamic_storage_boundaries_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sqlite_dynamic_storage_boundaries_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sqlite_dynamic_storage_probe>)
    set_tests_properties(module_sqlite_dynamic_storage_rust_parity
        module_sqlite_dynamic_storage_boundaries_rust_parity
        PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_field_layout_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_field_layout.py
        --cpp $<TARGET_FILE:teaql_module_field_layout_probe>)
    add_test(NAME module_loaded_snapshot_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_loaded_snapshot.py
        --cpp $<TARGET_FILE:teaql_module_loaded_snapshot_probe>)
    add_test(NAME module_intent_trace_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_intent_trace.py
        --cpp $<TARGET_FILE:teaql_module_loaded_snapshot_probe>)
    set_tests_properties(module_intent_trace_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_dynamic_carrier_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_carrier.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_carrier_probe>)
    add_test(NAME module_original_snapshot_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_original_snapshot.py
        --cpp $<TARGET_FILE:teaql_module_loaded_snapshot_probe>)
    add_test(NAME module_readonly_schema_hash_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_readonly_schema_hash_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_readonly_schema_hash_probe>)
    add_test(NAME module_compact_row_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_compact_row_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_compact_row_api_probe>)
    set_tests_properties(module_field_layout_rust_parity module_loaded_snapshot_rust_parity
        module_dynamic_carrier_rust_parity module_original_snapshot_rust_parity
        module_readonly_schema_hash_5_0_6_rust_parity module_compact_row_api_rust_parity
        PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

foreach(core_baseline IN ITEMS chrono_value timestamp_api system_time system_time_arithmetic_scope presentation presentation_api core_graph_api search)
    if(TARGET teaql_module_${core_baseline}_probe)
        add_test(NAME module_${core_baseline}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${core_baseline}.py
            --cpp $<TARGET_FILE:teaql_module_${core_baseline}_probe>)
        set_tests_properties(module_${core_baseline}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

foreach(protocol_entry IN ITEMS query stream executor_defaults_scope execution transaction executor_kind cached_partition)
    if(protocol_entry STREQUAL "executor_defaults_scope")
        set(protocol_probe executor_defaults_scope_probe)
        set(protocol_comparator compare_executor_defaults_scope.py)
        set(protocol_test module_executor_defaults_scope_rust_parity)
    elseif(protocol_entry STREQUAL "execution" OR protocol_entry STREQUAL "transaction" OR protocol_entry STREQUAL "executor_kind")
        set(protocol_probe sql_execution_protocol_probe)
        set(protocol_comparator compare_sql_${protocol_entry}.py)
        set(protocol_test module_sql_${protocol_entry}_rust_parity)
    elseif(protocol_entry STREQUAL "cached_partition")
        set(protocol_probe sql_cached_partition_probe)
        set(protocol_comparator compare_sql_cached_partition_5_0_6.py)
        set(protocol_test module_sql_cached_partition_rust_parity)
    else()
        set(protocol_probe sql_${protocol_entry}_probe)
        set(protocol_comparator compare_sql_${protocol_entry}.py)
        set(protocol_test module_sql_${protocol_entry}_rust_parity)
    endif()
    if(TARGET teaql_module_${protocol_probe})
        add_test(NAME ${protocol_test} COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/${protocol_comparator}
            --cpp $<TARGET_FILE:teaql_module_${protocol_probe}>)
        set_tests_properties(${protocol_test} PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

foreach(sql_entry IN ITEMS dialect_helpers dialect_in projection_scope transport schema mutation dialect_hooks dialect grouped_partition)
    if(sql_entry STREQUAL "dialect_helpers" OR sql_entry STREQUAL "dialect_in")
        set(sql_probe sql_dialect_helper_probe)
    elseif(sql_entry STREQUAL "dialect_hooks")
        set(sql_probe sql_mutation_probe)
    else()
        set(sql_probe sql_${sql_entry}_probe)
    endif()
    if(TARGET teaql_module_${sql_probe})
        if(sql_entry STREQUAL "grouped_partition")
            set(sql_comparator compare_sql_grouped_partition_5_0_6.py)
        else()
            set(sql_comparator compare_sql_${sql_entry}.py)
        endif()
        add_test(NAME module_sql_${sql_entry}_rust_parity
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/parity/${sql_comparator}
                --cpp $<TARGET_FILE:teaql_module_${sql_probe}>)
        set_tests_properties(module_sql_${sql_entry}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

foreach(sqlite_stem IN ITEMS schema decoding bindings error_model native_diagnostics simple_error from_sql_error from_sql_raw_debug)
    if(sqlite_stem STREQUAL "decoding")
        set(sqlite_probe sqlite_decoding_checked_probe)
    elseif(sqlite_stem STREQUAL "from_sql_raw_debug")
        set(sqlite_probe sqlite_from_sql_error_probe)
    else()
        set(sqlite_probe sqlite_${sqlite_stem}_probe)
    endif()
    if(TARGET teaql_module_${sqlite_probe})
        add_test(NAME module_sqlite_${sqlite_stem}_rust_parity
            COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/parity/compare_sqlite_${sqlite_stem}.py
                --cpp $<TARGET_FILE:teaql_module_${sqlite_probe}>)
        set_tests_properties(module_sqlite_${sqlite_stem}_rust_parity PROPERTIES
            ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

if(TARGET teaql_module_resource_bindings_probe)
    add_test(NAME module_resource_bindings_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resource_bindings_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_resource_bindings_probe>)
    set_tests_properties(module_resource_bindings_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    foreach(inherited_checker_case document intermediate_record invoice receipt special_invoice)
        if(TARGET teaql_module_inherited_checker_helpers_${inherited_checker_case}_probe)
            add_test(NAME module_inherited_checker_helpers_${inherited_checker_case}_rust_parity
                COMMAND ${Python3_EXECUTABLE}
                    ${PROJECT_SOURCE_DIR}/tests/parity/compare_inherited_checker_helpers_5_0_6.py
                    --cpp-dir ${CMAKE_CURRENT_BINARY_DIR}
                    --cpp-prefix teaql_module_inherited_checker_helpers_
                    --model ${inherited_checker_case}
                    --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
            set_tests_properties(module_inherited_checker_helpers_${inherited_checker_case}_rust_parity
                PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 180)
        endif()
    endforeach()
endif()

if(TARGET teaql_module_generated_reverse_many_helpers_probe)
    add_test(NAME module_generated_reverse_many_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_reverse_many_helpers_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_reverse_many_helpers_probe>)
    set_tests_properties(module_generated_reverse_many_helpers_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_entity_helper_entries_probe)
    add_test(NAME module_entity_helper_entries_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_helper_entries_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_helper_entries_probe>)
    set_tests_properties(module_entity_helper_entries_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_entity_constructor_helpers_probe AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME module_entity_constructor_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_constructor_helpers_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_constructor_helpers_probe>
        --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
    set_tests_properties(module_entity_constructor_helpers_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_recursive_attachment_helpers_probe)
    add_test(NAME module_recursive_attachment_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_recursive_attachment_helpers_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_recursive_attachment_helpers_probe>)
    set_tests_properties(module_recursive_attachment_helpers_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_initialized_snapshot_cache_probe)
    add_test(NAME module_initialized_snapshot_cache_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_initialized_snapshot_cache_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_initialized_snapshot_cache_probe>)
    set_tests_properties(module_initialized_snapshot_cache_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_generated_wire_mapping_lifetime_tests)
    add_test(NAME module_generated_wire_mapping_context_contracts
        COMMAND teaql_module_generated_wire_mapping_lifetime_tests)
endif()

if(TARGET teaql_module_generated_behavior_hooks_probe)
    add_test(NAME module_generated_behavior_hooks_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_behavior_hooks_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_behavior_hooks_probe>)
    set_tests_properties(module_generated_behavior_hooks_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_checker_clock_composition_probe)
    add_test(NAME module_checker_clock_composition_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_checker_clock_composition_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_checker_clock_composition_probe>
        --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
    set_tests_properties(module_checker_clock_composition_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_checker_clock_scalars_probe)
    add_test(NAME module_checker_clock_scalars_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_checker_clock_scalars_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_checker_clock_scalars_probe>
        --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
    set_tests_properties(module_checker_clock_scalars_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_checker_clock_text_probe)
    add_test(NAME module_checker_clock_text_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_checker_clock_text_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_checker_clock_text_probe>
        --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
    set_tests_properties(module_checker_clock_text_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_protected_behavior_probe)
    add_test(NAME module_protected_behavior_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_protected_behavior_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_protected_behavior_probe>)
    set_tests_properties(module_protected_behavior_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_inherited_business_extensions_probe)
    add_test(NAME module_inherited_business_extensions_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_inherited_business_extensions_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_inherited_business_extensions_probe>)
    set_tests_properties(module_inherited_business_extensions_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_expression_model_types_probe)
    add_test(NAME module_expression_model_types_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_expression_model_types_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_expression_model_types_probe>)
endif()

if(TARGET teaql_module_expression_business_ids_probe)
    add_test(NAME module_expression_business_ids_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_expression_business_ids_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_expression_business_ids_probe>)
endif()

if(TARGET teaql_module_inherited_sample_execution_probe)
    add_test(NAME module_inherited_sample_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_inherited_sample_execution_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_inherited_sample_execution_probe>)
endif()

if(TARGET teaql_module_sample_progress_probe)
    add_test(NAME module_sample_progress_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sample_progress_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sample_progress_probe>)
endif()
if(TARGET teaql_module_sample_data_logging_tests)
    add_test(NAME module_sample_data_logging_callback_close COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/check_sample_log_callback_close.py
        --cpp $<TARGET_FILE:teaql_module_sample_data_logging_tests>)
endif()

if(TARGET teaql_module_generated_request_state_probe)
    add_test(NAME module_generated_request_state_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_request_state_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_request_state_probe>)
endif()

if(TARGET teaql_module_generated_core_query_view_probe)
    add_test(NAME module_generated_core_query_view_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_core_query_view_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_core_query_view_probe>)
endif()

if(TARGET teaql_module_generated_scalar_membership_probe)
    target_compile_definitions(teaql_module_generated_scalar_membership_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_scalar_membership_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_scalar_membership_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_scalar_membership_probe>)
endif()

if(TARGET teaql_module_generated_scalar_comparison_probe)
    target_compile_definitions(teaql_module_generated_scalar_comparison_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_scalar_comparison_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_scalar_comparison_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_scalar_comparison_probe>)
endif()

if(TARGET teaql_module_generated_projection_text_probe)
    target_compile_definitions(teaql_module_generated_projection_text_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_projection_text_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_projection_text_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_projection_text_probe>)
endif()

if(TARGET teaql_module_generated_query_field_branches_probe)
    add_test(NAME module_generated_query_field_branches_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_query_field_branches_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_query_field_branches_probe>)
endif()

if(TARGET teaql_module_generated_query_field_extensions_probe)
    add_test(NAME module_generated_query_field_extensions_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_query_field_extensions_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_query_field_extensions_probe>)
endif()

if(TARGET teaql_module_generated_query_field_sql_probe)
    target_compile_definitions(teaql_module_generated_query_field_sql_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_query_field_sql_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_query_field_sql_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_query_field_sql_probe>)
endif()

if(TARGET teaql_module_generated_relation_selection_inputs_probe)
    target_compile_definitions(teaql_module_generated_relation_selection_inputs_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_relation_selection_inputs_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_selection_inputs_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_relation_selection_inputs_probe>)
endif()

if(TARGET teaql_module_generated_relation_analysis_inputs_probe)
    target_compile_definitions(teaql_module_generated_relation_analysis_inputs_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_relation_analysis_inputs_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_analysis_inputs_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_relation_analysis_inputs_probe>)
endif()

if(TARGET teaql_module_embedded_graph_boundary_tests)
    target_compile_definitions(teaql_module_embedded_graph_boundary_tests PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
endif()

if(TARGET teaql_module_generated_analysis_adapter_inputs_probe)
    target_compile_definitions(teaql_module_generated_analysis_adapter_inputs_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_analysis_adapter_inputs_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_analysis_inputs_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_analysis_adapter_inputs_probe>)
endif()

if(TARGET teaql_module_generated_matching_statistics_inputs_probe)
    target_compile_definitions(teaql_module_generated_matching_statistics_inputs_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_matching_statistics_inputs_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_matching_statistics_inputs_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_matching_statistics_inputs_probe>)
endif()

if(TARGET teaql_module_generated_matching_statistics_adapters_probe)
    target_compile_definitions(teaql_module_generated_matching_statistics_adapters_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_matching_statistics_adapters_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_matching_statistics_inputs_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_matching_statistics_adapters_probe>)
endif()

if(TARGET teaql_module_runtime_connection_tests)
    target_link_libraries(teaql_module_runtime_connection_tests PRIVATE SQLite3::SQLite3)
endif()

if(TARGET teaql_module_generated_i18n_tests)
    target_compile_definitions(teaql_module_generated_i18n_tests PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
endif()

if(TARGET teaql_module_i18n_probe)
    add_test(NAME module_generated_i18n_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_i18n_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_i18n_probe>)
    set_tests_properties(module_generated_i18n_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_generated_business_environment_probe)
    add_test(NAME module_generated_business_environment_startup_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_business_environment_startup_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_business_environment_probe>)
    set_tests_properties(module_generated_business_environment_startup_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 180)
endif()

if(TARGET teaql_module_runtime_configuration_kernel_probe)
    add_test(NAME module_runtime_configuration_kernel_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_runtime_configuration_kernel_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_configuration_kernel_probe>)
    set_tests_properties(module_runtime_configuration_kernel_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_runtime_connection_probe)
    add_test(NAME module_runtime_connection_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_runtime_connection_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_connection_probe>)
    set_tests_properties(module_runtime_connection_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_runtime_assembly_config_probe)
    add_test(NAME module_runtime_assembly_configuration_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_runtime_connection_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_assembly_config_probe> --assembly)
    set_tests_properties(module_runtime_assembly_configuration_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_generated_runtime_configuration_probe)
    add_test(NAME module_generated_runtime_configuration_kernel_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_runtime_configuration_kernel_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_runtime_configuration_probe>)
    add_test(NAME module_generated_runtime_assembly_configuration_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_runtime_connection_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_runtime_configuration_probe> --assembly)
    set_tests_properties(module_generated_runtime_configuration_kernel_rust_parity
        module_generated_runtime_assembly_configuration_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_service_runtime_error_tests)
    add_test(NAME module_runtime_service_errors_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_runtime_service_errors_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_service_runtime_error_tests>)
endif()

if(TARGET teaql_module_generated_business_runtime_configuration_probe)
    add_test(NAME module_generated_business_runtime_configuration_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_business_runtime_configuration_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_business_runtime_configuration_probe>)
    set_tests_properties(module_generated_business_runtime_configuration_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 180)
endif()

if(TARGET teaql_module_generated_filename_configuration_tests)
    add_test(NAME module_generated_filename_configuration_contracts COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_filename_configuration.py
        --cpp $<TARGET_FILE:teaql_module_generated_filename_configuration_tests>)
endif()

if(TARGET teaql_module_generated_registry_schema_probe)
    add_test(NAME module_generated_registry_schema_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_generated_registry_schema_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_registry_schema_probe>)
endif()

if(TARGET teaql_module_sample_type_probe)
    add_test(NAME module_sample_types_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_sample_types_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sample_type_probe>)
    set_tests_properties(module_sample_types_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 600)
endif()

if(TARGET teaql_module_custom_sample_probe)
    add_test(NAME module_custom_sample_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_custom_sample_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_custom_sample_probe>)
    set_tests_properties(module_custom_sample_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 600)
endif()

if(TARGET teaql_module_catalog_sample_probe)
    add_test(NAME module_catalog_sample_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_catalog_sample_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_catalog_sample_probe>)
    set_tests_properties(module_catalog_sample_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 600)
endif()

if(TARGET teaql_module_business_sample_probe)
    add_test(NAME module_business_sample_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/scripts/verify_business_sample_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_business_sample_probe>)
    set_tests_properties(module_business_sample_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 240)
endif()

if(TARGET teaql_module_query_predicate_execution_probe)
    target_compile_definitions(teaql_module_query_predicate_execution_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_query_predicate_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_predicate_execution_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_query_predicate_execution_probe>)
    set_tests_properties(module_query_predicate_execution_rust_parity PROPERTIES TIMEOUT 600 ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_query_alias_probe)
    target_compile_definitions(teaql_module_query_alias_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_query_alias_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_alias_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_query_alias_probe>)
    set_tests_properties(module_query_alias_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()
if(TARGET teaql_module_query_projection_identity_tests)
    add_test(NAME module_query_projection_identity_contracts COMMAND teaql_module_query_projection_identity_tests)
endif()

if(TARGET teaql_module_relation_identity_probe)
    target_compile_definitions(teaql_module_relation_identity_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_relation_identity_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_identity_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_relation_identity_probe>)
    set_tests_properties(module_relation_identity_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 600)
endif()
if(TARGET teaql_module_relation_identity_tests)
    target_compile_definitions(teaql_module_relation_identity_tests PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_relation_identity_generated_contracts COMMAND teaql_module_relation_identity_tests)
    set_tests_properties(module_relation_identity_generated_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()
if(TARGET teaql_module_relation_identity_kernel_tests)
    add_test(NAME module_relation_identity_kernel_contracts COMMAND teaql_module_relation_identity_kernel_tests)
endif()

if(TARGET teaql_module_relation_graph_save_probe)
    target_compile_definitions(teaql_module_relation_graph_save_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_relation_graph_save_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_graph_save_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_relation_graph_save_probe>)
    set_tests_properties(module_relation_graph_save_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 600)
endif()

if(TARGET teaql_module_ledger_composition_tests)
    target_compile_definitions(teaql_module_ledger_composition_tests PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_generated_ledger_composition_contracts COMMAND teaql_module_ledger_composition_tests)
    set_tests_properties(module_generated_ledger_composition_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_scalar_diagnostic_repair_probe)
    target_compile_definitions(teaql_module_scalar_diagnostic_repair_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_scalar_diagnostic_repair_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_scalar_diagnostic_repair_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_scalar_diagnostic_repair_probe>)
    set_tests_properties(module_scalar_diagnostic_repair_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_scalar_diagnostic_tests)
    add_test(NAME module_generated_scalar_diagnostic_contracts COMMAND teaql_module_scalar_diagnostic_tests)
    set_tests_properties(module_generated_scalar_diagnostic_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_expression_entry_tests)
    add_test(NAME module_expression_entry_contracts COMMAND teaql_module_expression_entry_tests)
endif()
if(TARGET teaql_module_expression_root_probe)
    target_compile_definitions(teaql_module_expression_root_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_expression_root_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_expression_roots_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_expression_root_probe>)
    set_tests_properties(module_expression_root_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_execution_options_probe)
    add_test(NAME module_execution_options_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_execution_options_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_execution_options_probe>)
    set_tests_properties(module_execution_options_rust_parity PROPERTIES TIMEOUT 120)
endif()

if(TARGET teaql_module_execution_result_models_probe)
    target_compile_definitions(teaql_module_execution_result_models_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_execution_result_models_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_execution_result_models_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_execution_result_models_probe>)
    add_test(NAME module_execution_result_model_states_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_execution_result_model_states_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_execution_result_models_probe>)
    set_tests_properties(module_execution_result_model_states_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
    set_tests_properties(module_execution_result_models_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 300)
endif()

if(TARGET teaql_module_entry_lookup_probe)
    target_compile_definitions(teaql_module_entry_lookup_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_entry_lookup_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entry_lookup_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entry_lookup_probe>)
    add_test(NAME module_entry_lookup_context_contracts COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/verify_entry_lookup_context_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entry_lookup_probe>)
    set_tests_properties(module_entry_lookup_rust_parity module_entry_lookup_context_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_hot_execution_probe)
    target_compile_definitions(teaql_module_hot_execution_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_hot_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_hot_execution_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_hot_execution_probe>)
    set_tests_properties(module_hot_execution_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 300)
endif()

if(TARGET teaql_module_stream_shapes_probe)
    target_compile_definitions(teaql_module_stream_shapes_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_stream_shapes_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_stream_shapes_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_stream_shapes_probe>)
    set_tests_properties(module_stream_shapes_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 300)
endif()

if(TARGET teaql_module_composed_general_probe)
    target_compile_definitions(teaql_module_composed_general_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_composed_general_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_composed_general_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_composed_general_probe>)
    set_tests_properties(module_composed_general_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 300)
endif()

if(TARGET teaql_module_query_result_marker_probe)
    add_test(NAME module_query_result_marker_runtime_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_result_marker_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_query_result_marker_probe>)
endif()

if(TARGET teaql_module_general_query_guards_probe)
    target_compile_definitions(teaql_module_general_query_guards_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_general_query_guards_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_general_query_guards_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_general_query_guards_probe>)
    set_tests_properties(module_general_query_guards_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_business_id_lookups_probe)
    target_compile_definitions(teaql_module_business_id_lookups_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_business_id_lookups_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_business_id_lookups_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_business_id_lookups_probe>)
    set_tests_properties(module_business_id_lookups_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_deferred_facet_page_probe)
    target_compile_definitions(teaql_module_deferred_facet_page_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_deferred_facet_page_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_deferred_facet_page_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_deferred_facet_page_probe>)
    set_tests_properties(module_deferred_facet_page_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_deferred_resource_replacement_probe)
    target_compile_definitions(teaql_module_deferred_resource_replacement_probe PRIVATE TEAQL_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME module_deferred_resource_replacement_contracts COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/verify_deferred_resource_replacement_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_deferred_resource_replacement_probe>)
    set_tests_properties(module_deferred_resource_replacement_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

# Publish the core protocol comparisons for their imported consumers.
foreach(core_protocol IN ITEMS business_id core_api core_aux_debug core_primitives meta_api)
    if(TARGET teaql_module_${core_protocol}_probe)
        set(core_comparator compare_${core_protocol}_5_0_6.py)
        if(core_protocol STREQUAL "core_aux_debug")
            set(core_comparator compare_core_aux_debug.py)
        endif()
        add_test(NAME module_${core_protocol}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/${core_comparator}
            --cpp $<TARGET_FILE:teaql_module_${core_protocol}_probe>)
        set_tests_properties(module_${core_protocol}_rust_parity PROPERTIES
            ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endforeach()

if(TARGET teaql_module_meta_api_probe AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_test(NAME module_core_metadata_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_core_metadata_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_meta_api_probe>)
    set_tests_properties(module_core_metadata_source_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_eval_indexed_probe)
    add_test(NAME module_eval_indexed_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_eval_indexed_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_eval_indexed_probe>)
    add_test(NAME module_eval_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_eval_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_eval_probe>)
endif()

if(TARGET teaql_module_entity_source_probe)
    add_test(NAME module_entity_registration_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_registration_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_registration_api_probe>)
    add_test(NAME module_base_entity_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_base_entity_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_base_entity_api_probe>)
    add_test(NAME module_entity_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_debug_probe>)
    add_test(NAME module_entity_defaults_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_defaults_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_defaults_api_probe>)
    add_test(NAME module_core_shape_presentation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_core_shape_presentation.py
        --cpp $<TARGET_FILE:teaql_module_core_shape_presentation_probe>)
    add_test(NAME module_entity_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_source_probe>)
endif()

if(TARGET teaql_module_runtime_state_bridge_probe)
    add_test(NAME module_audited_auto_save_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_audited_auto_save_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_audited_auto_save_probe>)
    add_test(NAME module_runtime_state_bridge_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_runtime_state_bridge_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_state_bridge_probe>)
endif()

if(TARGET teaql_module_generated_runtime_state_bridge_probe)
    add_test(NAME module_generated_runtime_state_bridge_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_runtime_state_bridge_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_generated_runtime_state_bridge_probe>)
endif()

foreach(g12_matrix IN ITEMS generated_runtime_assembly generated_sample_state generated_sample_execution)
    if(TARGET teaql_module_${g12_matrix}_probe)
        add_test(NAME module_${g12_matrix}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${g12_matrix}.py
            --cpp $<TARGET_FILE:teaql_module_${g12_matrix}_probe>)
        set_tests_properties(module_${g12_matrix}_rust_parity PROPERTIES
            ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
    endif()
endforeach()

if(TARGET teaql_module_generated_sample_execution_probe)
    set_tests_properties(module_generated_sample_execution_rust_parity PROPERTIES TIMEOUT 1800)
    add_test(NAME module_generated_sample_failures_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_sample_failures.py
        --cpp $<TARGET_FILE:teaql_module_generated_sample_execution_probe>)
    set_tests_properties(module_generated_sample_failures_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_generated_sample_fields_probe AND TARGET teaql_sample_fixed_clock)
    add_test(NAME module_generated_sample_fields_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_generated_sample_fields.py
        --cpp $<TARGET_FILE:teaql_module_generated_sample_fields_probe>
        --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
    set_tests_properties(module_generated_sample_fields_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_core_like_expression_source_probe)
    foreach(expression_matrix IN ITEMS query_api query_debug core_like_expression_source)
        add_test(NAME module_${expression_matrix}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${expression_matrix}_5_0_6.py
            --cpp $<TARGET_FILE:teaql_module_${expression_matrix}_probe>)
    endforeach()
    add_test(NAME module_core_expression_boundaries_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_core_expression_boundaries_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_query_api_probe>)
endif()

if(TARGET teaql_module_safe_expression_empty_source_probe)
    foreach(safe_expression_matrix IN ITEMS safe_expression_api safe_expression_empty_source)
        add_test(NAME module_${safe_expression_matrix}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${safe_expression_matrix}_5_0_6.py
            --cpp $<TARGET_FILE:teaql_module_${safe_expression_matrix}_probe>)
    endforeach()
endif()

if(TARGET teaql_module_mutation_model_api_probe)
    foreach(mutation_matrix IN ITEMS mutation_model_api mutation_debug)
        add_test(NAME module_${mutation_matrix}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${mutation_matrix}_5_0_6.py
            --cpp $<TARGET_FILE:teaql_module_${mutation_matrix}_probe>)
    endforeach()
    add_test(NAME module_core_mutation_boundaries_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_core_mutation_boundaries_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_mutation_model_api_probe>)
endif()

if(TARGET teaql_module_sql_bindings_source_probe)
    add_test(NAME module_sql_bindings_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sql_bindings_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sql_bindings_source_probe>)
endif()

if(TARGET teaql_module_sql_types_probe)
    add_test(NAME module_sql_types_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sql_types_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sql_types_probe>)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_test(NAME module_sql_types_boundaries_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_sql_types_boundaries_5_0_6.py
            --cpp $<TARGET_FILE:teaql_module_sql_types_probe>)
        add_test(NAME module_sql_type_models_boundaries_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_sql_type_models_boundaries_5_0_6.py
            --cpp $<TARGET_FILE:teaql_module_sql_model_api_probe>)
    endif()
endif()

if(TARGET teaql_module_model_debug_probe)
    add_test(NAME module_model_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_model_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_model_debug_probe>)
    add_test(NAME module_core_query_boundaries_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_core_query_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_query_api_probe>)
endif()

if(TARGET teaql_module_core_query_intent_source_probe)
    add_test(NAME module_core_query_intent_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_core_query_intent_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_core_query_intent_source_probe>)
endif()

if(TARGET teaql_module_query_schema_builder_probe)
    add_test(NAME module_query_schema_builder_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_schema_builder_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_query_schema_builder_probe>)
endif()

if(TARGET teaql_module_query_full_state_probe)
    add_test(NAME module_query_full_state_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_full_state_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_query_full_state_probe>)
endif()

if(TARGET teaql_module_row_extend_error_probe)
    add_test(NAME module_row_extend_error_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_row_extend_error_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_row_extend_error_probe>)
    add_test(NAME module_dynamic_field_error_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_field_error_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_row_extend_error_probe>)
endif()

if(TARGET teaql_module_row_read_sidecar_probe)
    add_test(NAME module_row_read_sidecar_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_row_read_sidecar_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_row_read_sidecar_probe>)
endif()

if(TARGET teaql_module_row_enhancement_probe)
    add_test(NAME module_row_enhancement_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_row_enhancement_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_row_enhancement_probe>)
endif()

if(TARGET teaql_module_row_enhancement_probe)
    add_test(NAME module_row_constructor_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_row_constructor_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_row_enhancement_probe>)
endif()

if(TARGET teaql_module_inmemory_source_probe AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_test(NAME module_inmemory_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_inmemory_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_inmemory_source_probe>)
endif()

if(TARGET teaql_module_loaded_view_move_probe)
    add_test(NAME module_loaded_view_move_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_loaded_view_move_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_loaded_view_move_probe>)
endif()

if(TARGET teaql_module_entity_runtime_model_probe)
    add_test(NAME module_entity_runtime_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_runtime_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_runtime_model_probe>)
endif()

if(TARGET teaql_module_entity_runtime_equality_probe)
    add_test(NAME module_entity_runtime_equality_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_runtime_equality_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_runtime_equality_probe>)
endif()

if(TARGET teaql_module_entity_runtime_dynamic_debug_probe)
    add_test(NAME module_entity_runtime_dynamic_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_runtime_dynamic_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_runtime_dynamic_debug_probe>)
endif()

if(TARGET teaql_module_mutation_descriptor_cache_probe)
    add_test(NAME module_mutation_descriptor_cache_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_mutation_descriptor_cache_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_mutation_descriptor_cache_probe>)
endif()
if(TARGET teaql_module_sqlite_chunk_capacity_probe)
    add_test(NAME module_sqlite_chunk_capacity_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sqlite_chunk_capacity_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sqlite_chunk_capacity_probe>)
    set_tests_properties(module_sqlite_chunk_capacity_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_sqlite_async_lease_probe)
    add_test(NAME module_sqlite_async_lease_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sqlite_async_lease_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sqlite_async_lease_probe>)
    set_tests_properties(module_sqlite_async_lease_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off" TIMEOUT 120)
endif()

if(TARGET teaql_module_sqlite_lease_binding_probe)
    add_test(NAME module_sqlite_lease_binding_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sqlite_lease_binding_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sqlite_lease_binding_probe>)
    set_tests_properties(module_sqlite_lease_binding_rust_parity PROPERTIES
        ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_test(NAME module_sqlite_lease_binding_guard_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_sqlite_lease_binding_guard_5_0_6.py
            --cpp $<TARGET_FILE:teaql_module_sqlite_lease_binding_probe>)
        set_tests_properties(module_sqlite_lease_binding_guard_rust_parity PROPERTIES
            ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endif()
endif()

if(TARGET teaql_module_sql_dynamic_stream_source_probe)
    add_test(NAME module_sql_dynamic_stream_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sql_dynamic_stream_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sql_dynamic_stream_source_probe>)
endif()
if(TARGET teaql_module_sql_executor_error_probe)
    add_test(NAME module_sql_executor_error_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sql_executor_error_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sql_executor_error_probe>)
endif()
if(TARGET teaql_module_sql_execution_protocol_probe)
    add_test(NAME module_mutation_execution_source_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_mutation_execution_source_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sql_execution_protocol_probe>)
endif()
if(TARGET teaql_module_sql_diagnostics_guard_probe)
    add_test(NAME module_sql_diagnostics_guard_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_sql_diagnostics_guard_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_sql_diagnostics_guard_probe>)
endif()

if(TARGET teaql_module_value_api_probe)
    add_test(NAME module_value_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_value_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_value_api_probe>)
endif()

if(TARGET teaql_module_value_owned_api_probe)
    add_test(NAME module_value_owned_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_value_owned_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_value_owned_api_probe>)
endif()

if(TARGET teaql_module_value_probe)
    add_test(NAME module_value_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_values.py
        --cpp $<TARGET_FILE:teaql_module_value_probe>)
endif()

if(TARGET teaql_module_datatype_debug_probe)
    add_test(NAME module_datatype_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_datatype_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_datatype_debug_probe>)
endif()

if(TARGET teaql_module_smart_list_api_probe)
    add_test(NAME module_smart_list_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_smart_list_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_smart_list_api_probe>)
endif()

if(TARGET teaql_module_request_api_probe)
    add_test(NAME module_request_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_request_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_request_api_probe>)
endif()

if(TARGET teaql_module_request_helpers_probe)
    add_test(NAME module_request_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_request_helpers_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_request_helpers_probe>)
endif()

if(TARGET teaql_module_value_probe)
    add_test(NAME module_format_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_formats.py
        --cpp $<TARGET_FILE:teaql_module_value_probe>)
endif()

if(TARGET teaql_module_dynamic_search_clone_probe)
    add_test(NAME module_dynamic_search_clone_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_dynamic_search_clone_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_dynamic_search_clone_probe>)
endif()

if(TARGET teaql_module_original_core_search_tests)
    add_test(NAME module_original_core_dynamic_search_warning_privacy COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/check_search_warning_canary.py
        --cpp $<TARGET_FILE:teaql_module_original_core_search_tests>)
endif()

# Preserve the complete event-model observer through the runtime-only import.
if(TARGET teaql_module_event_model_probe)
    add_test(NAME module_event_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_event_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_event_model_probe>)
endif()

if(TARGET teaql_module_policy_model_probe)
    add_test(NAME module_policy_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_policy_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_policy_model_probe>)
endif()

if(TARGET teaql_module_mutation_policy_probe)
    add_test(NAME module_mutation_policy_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_mutation_policy_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_mutation_policy_probe>)
endif()

if(TARGET teaql_module_policy_plan_probe)
    add_test(NAME module_policy_plan_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_policy_plan_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_policy_plan_probe>)
endif()

if(TARGET teaql_module_relation_key_snapshot_probe)
    add_test(NAME module_relation_key_snapshot_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_key_snapshot_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_relation_key_snapshot_probe>)
    set_tests_properties(module_relation_key_snapshot_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_runtime_service_types_probe)
    add_test(NAME module_runtime_service_types_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_runtime_service_types_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_service_types_probe>)
    set_tests_properties(module_runtime_service_types_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()
if(TARGET teaql_module_relation_model_probe)
    add_test(NAME module_relation_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_relation_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_relation_model_probe>)
    set_tests_properties(module_relation_model_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_runtime_statement_trace_probe)
    add_test(NAME module_runtime_statement_trace_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_runtime_statement_trace_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_statement_trace_probe>)
    set_tests_properties(module_runtime_statement_trace_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_runtime_base_probe)
    foreach(probe_comparator IN ITEMS runtime_base runtime_base_intent runtime_query_protocol)
        add_test(NAME module_${probe_comparator}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${probe_comparator}_5_0_6.py
            --cpp $<TARGET_FILE:teaql_module_runtime_base_probe>)
        set_tests_properties(module_${probe_comparator}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endforeach()
endif()

if(TARGET teaql_module_runtime_mutation_protocol_probe)
    add_test(NAME module_runtime_mutation_protocol_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_runtime_mutation_protocol_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_mutation_protocol_probe>)
    set_tests_properties(module_runtime_mutation_protocol_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_context_query_probe)
    foreach(probe_comparator IN ITEMS context_query context_query_intent)
        add_test(NAME module_${probe_comparator}_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/parity/compare_${probe_comparator}_5_0_6.py
            --cpp $<TARGET_FILE:teaql_module_context_query_probe>)
        set_tests_properties(module_${probe_comparator}_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    endforeach()
endif()

if(TARGET teaql_module_mutation_intent_privacy_probe)
    add_test(NAME module_mutation_intent_privacy_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_mutation_intent_privacy_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_mutation_intent_privacy_probe>)
    set_tests_properties(module_mutation_intent_privacy_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_context_mutation_cache_probe)
    add_test(NAME module_context_mutation_cache_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_context_mutation_cache_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_context_mutation_cache_probe>)
    set_tests_properties(module_context_mutation_cache_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_context_factory_probe)
    add_test(NAME module_context_factory_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_context_factory_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_context_factory_probe>)
    set_tests_properties(module_context_factory_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_context_privacy_preparation_probe)
    add_test(NAME module_context_privacy_preparation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_context_privacy_preparation_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_context_privacy_preparation_probe>)
    set_tests_properties(module_context_privacy_preparation_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_context_helpers_probe)
    add_test(NAME module_context_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_context_helpers_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_context_helpers_probe>)
    set_tests_properties(module_context_helpers_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_loaded_row_graph_probe)
    add_test(NAME module_loaded_graph_protocol_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_loaded_graph_protocol_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_loaded_row_graph_probe>)
    set_tests_properties(module_loaded_graph_protocol_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_enhanced_aggregate_probe)
    add_test(NAME module_enhanced_aggregate_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_enhanced_aggregate_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_enhanced_aggregate_probe>)
    set_tests_properties(module_enhanced_aggregate_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_enhanced_aggregate_probe)
    add_test(NAME module_query_entry_protocol_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_entry_protocol_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_enhanced_aggregate_probe>)
    set_tests_properties(module_query_entry_protocol_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_enhanced_aggregate_probe)
    add_test(NAME module_query_entry_cursor_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_query_entry_cursor_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_enhanced_aggregate_probe>)
    set_tests_properties(module_query_entry_cursor_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_enhanced_aggregate_probe)
    add_test(NAME module_aggregate_membership_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_aggregate_membership_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_enhanced_aggregate_probe>)
    set_tests_properties(module_aggregate_membership_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_resolved_select_preparation_probe)
    add_test(NAME module_resolved_select_preparation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_select_preparation_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_resolved_select_preparation_probe>)
    set_tests_properties(module_resolved_select_preparation_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_resolved_mutation_preparation_probe)
    add_test(NAME module_resolved_mutation_preparation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_mutation_preparation_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_resolved_mutation_preparation_probe>)
    set_tests_properties(module_resolved_mutation_preparation_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_runtime_mutation_probe)
    add_test(NAME module_resolved_mutation_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_mutation_execution_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_mutation_probe>)
    set_tests_properties(module_resolved_mutation_execution_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_runtime_mutation_probe)
    add_test(NAME module_resolved_mutation_privacy_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_mutation_privacy_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_runtime_mutation_probe>)
    set_tests_properties(module_resolved_mutation_privacy_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_resolved_dynamic_rows_probe)
    add_test(NAME module_resolved_dynamic_rows_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_dynamic_rows_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_resolved_dynamic_rows_probe>)
    set_tests_properties(module_resolved_dynamic_rows_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_entity_query_probe)
    add_test(NAME module_resolved_query_schema_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_query_schema_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_query_probe>)
    set_tests_properties(module_resolved_query_schema_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_entity_query_probe)
    add_test(NAME module_resolved_query_typed_schema_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_query_typed_schema_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_query_probe>)
    set_tests_properties(module_resolved_query_typed_schema_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_resolved_query_schema_context_contracts COMMAND teaql_module_entity_query_probe --schema-self-test)
endif()

if(TARGET teaql_module_entity_query_probe)
    add_test(NAME module_entity_query_capture_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_query_capture_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_query_probe>)
    set_tests_properties(module_entity_query_capture_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
endif()

if(TARGET teaql_module_entity_stream_probe)
    add_test(NAME module_entity_stream_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_entity_stream_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_stream_probe>)
    add_test(NAME module_resolved_stream_schema_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_stream_schema_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_stream_probe>)
    set_tests_properties(module_entity_stream_rust_parity module_resolved_stream_schema_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_resolved_stream_schema_context_contracts COMMAND teaql_module_entity_stream_probe --schema-self-test)
endif()

if(TARGET teaql_module_entity_query_probe)
    add_test(NAME module_resolved_enhanced_typed_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_enhanced_typed_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_query_probe>)
    set_tests_properties(module_resolved_enhanced_typed_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_resolved_dynamic_loaded_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_dynamic_loaded_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_query_probe>)
    set_tests_properties(module_resolved_dynamic_loaded_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_resolved_dynamic_guard_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_dynamic_guard_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_query_probe>)
    set_tests_properties(module_resolved_dynamic_guard_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_resolved_dynamic_typed_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tests/parity/compare_resolved_dynamic_typed_5_0_6.py
        --cpp $<TARGET_FILE:teaql_module_entity_query_probe>)
    set_tests_properties(module_resolved_dynamic_typed_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME module_resolved_dynamic_typed_context_contracts COMMAND teaql_module_entity_query_probe --dynamic-self-test)
endif()
