# Shared contract and Rust parity registration. Fixtures explicitly link the
# Context/provider packages they use; production lower layers remain independent. Included from the workspace root
# so existing executable locations and CTest names remain stable.
if(BUILD_TESTING)
    if(Python3_Interpreter_FOUND)
        add_test(NAME workspace_paths_5_0_6_contracts COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_module_paths.py)
        add_test(NAME workspace_path_negative_controls COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_module_paths.py)
    endif()
    add_executable(teaql_runtime_namespace_tests teaql_runtime/tests/runtime_namespace_tests.cpp)
    target_link_libraries(teaql_runtime_namespace_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_public_namespace_contracts COMMAND teaql_runtime_namespace_tests)
    if(Python3_Interpreter_FOUND AND CMAKE_NM)
        add_test(NAME runtime_layer_boundaries COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_layer_boundaries.py
            --runtime $<TARGET_FILE:teaql_runtime> --nm ${CMAKE_NM}
            --metadata ${CMAKE_CURRENT_BINARY_DIR}/teaql_layer_contracts.txt)
    endif()
    add_executable(teaql_entity_runtime_model_tests teaql_runtime/tests/entity_runtime_model_tests.cpp)
    target_link_libraries(teaql_entity_runtime_model_tests PRIVATE teaql::runtime)
    add_test(NAME entity_runtime_model_contracts COMMAND teaql_entity_runtime_model_tests)
    add_executable(teaql_entity_runtime_model_probe tests/parity/entity_runtime_model_probe.cpp)
    target_link_libraries(teaql_entity_runtime_model_probe PRIVATE teaql::runtime)
    add_test(NAME entity_runtime_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_runtime_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_entity_runtime_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_business_clock_scope_tests teaql_runtime/tests/business_clock_scope_tests.cpp)
    target_link_libraries(teaql_business_clock_scope_tests PRIVATE teaql::runtime)
    add_test(NAME business_clock_scope_contracts COMMAND teaql_business_clock_scope_tests)
    add_test(NAME business_clock_reentrant_time COMMAND teaql_business_clock_scope_tests reentrant-time)
    add_test(NAME business_clock_reentrant_date COMMAND teaql_business_clock_scope_tests reentrant-date)
    add_executable(teaql_runtime_callback_scope_tests teaql_runtime/tests/runtime_callback_scope_tests.cpp)
    target_link_libraries(teaql_runtime_callback_scope_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_callback_scope_contracts COMMAND teaql_runtime_callback_scope_tests)
    add_test(NAME runtime_callback_registry_close COMMAND teaql_runtime_callback_scope_tests registry)
    add_test(NAME runtime_callback_relation_loads_close COMMAND teaql_runtime_callback_scope_tests loads)
    add_test(NAME runtime_callback_select_behavior_close COMMAND teaql_runtime_callback_scope_tests select-behavior)
    add_test(NAME runtime_callback_select_policy_close COMMAND teaql_runtime_callback_scope_tests select-policy)
    add_test(NAME runtime_callback_business_slot_close COMMAND teaql_runtime_callback_scope_tests business-slot)
    add_test(NAME runtime_callback_business_key_close COMMAND teaql_runtime_callback_scope_tests business-key)
    add_test(NAME runtime_callback_business_allocator_close COMMAND teaql_runtime_callback_scope_tests business-allocator)
    add_executable(teaql_context_provider_borrow_tests teaql_runtime/tests/context_provider_borrow_tests.cpp)
    target_link_libraries(teaql_context_provider_borrow_tests PRIVATE teaql::runtime)
    add_test(NAME context_provider_borrow_contracts COMMAND teaql_context_provider_borrow_tests)
    add_test(NAME context_provider_reentrant_registry COMMAND teaql_context_provider_borrow_tests reentrant-registry)
    add_test(NAME context_provider_reentrant_schema COMMAND teaql_context_provider_borrow_tests reentrant-schema)
    add_executable(teaql_context_provider_dispatch_tests teaql_runtime/tests/context_provider_dispatch_tests.cpp)
    target_link_libraries(teaql_context_provider_dispatch_tests PRIVATE teaql::runtime)
    add_test(NAME context_dispatch_lifecycle_contracts COMMAND teaql_context_provider_dispatch_tests lifecycle)
    add_test(NAME context_dispatch_exception_contracts COMMAND teaql_context_provider_dispatch_tests exceptions)
    add_test(NAME context_dispatch_shutdown_contracts COMMAND teaql_context_provider_dispatch_tests shutdown)
    add_test(NAME context_dispatch_entity_close COMMAND teaql_context_provider_dispatch_tests entity)
    add_test(NAME context_dispatch_all_entities_close COMMAND teaql_context_provider_dispatch_tests all-entities)
    add_test(NAME context_dispatch_checker_close COMMAND teaql_context_provider_dispatch_tests checker)
    add_test(NAME context_dispatch_telemetry_noop_close COMMAND teaql_context_provider_dispatch_tests telemetry-noop)
    add_test(NAME context_dispatch_telemetry_start_close COMMAND teaql_context_provider_dispatch_tests telemetry-start)
    add_executable(teaql_runtime_id_model_tests teaql_runtime/tests/runtime_id_model_tests.cpp)
    target_link_libraries(teaql_runtime_id_model_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_id_model_contracts COMMAND teaql_runtime_id_model_tests)
    add_executable(teaql_runtime_id_model_probe tests/parity/runtime_id_model_probe.cpp)
    target_link_libraries(teaql_runtime_id_model_probe PRIVATE teaql::runtime)
    add_test(NAME runtime_id_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_id_models.py
        --cpp $<TARGET_FILE:teaql_runtime_id_model_probe>
        "--profile=$<IF:$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>,release,debug>"
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_context_defaults_tests teaql_runtime/tests/context_defaults_tests.cpp)
    target_link_libraries(teaql_context_defaults_tests PRIVATE teaql::runtime)
    add_test(NAME context_defaults_contracts COMMAND teaql_context_defaults_tests)
    add_executable(teaql_context_defaults_probe tests/parity/context_defaults_probe.cpp)
    target_link_libraries(teaql_context_defaults_probe PRIVATE teaql::runtime)
    add_test(NAME context_defaults_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_defaults_5_0_6.py
        --cpp $<TARGET_FILE:teaql_context_defaults_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_reference_model_tests teaql_runtime/tests/reference_model_tests.cpp)
    target_link_libraries(teaql_reference_model_tests PRIVATE teaql::runtime)
    add_test(NAME reference_model_contracts COMMAND teaql_reference_model_tests)
    add_executable(teaql_reference_model_probe tests/parity/reference_model_probe.cpp)
    target_link_libraries(teaql_reference_model_probe PRIVATE teaql::runtime)
    add_test(NAME reference_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_reference_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_reference_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_round_trip_reference_tests teaql_runtime/tests/round_trip_reference_tests.cpp)
    add_executable(teaql_document_round_trip_probe tests/parity/document_round_trip_probe.cpp)
    add_executable(teaql_generated_dynamic_json_probe tests/parity/generated_dynamic_json_probe.cpp)
    add_executable(teaql_generated_execution_probe tests/parity/generated_execution_probe.cpp)
    target_link_libraries(teaql_generated_execution_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    target_compile_definitions(teaql_generated_execution_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_execution.py
        --cpp $<TARGET_FILE:teaql_generated_execution_probe>)
    set_tests_properties(generated_execution_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_execution_tests teaql_runtime/tests/generated_execution_tests.cpp)
    add_executable(teaql_generated_aggregation_cache_tests teaql_runtime/tests/generated_aggregation_cache_tests.cpp)
    target_link_libraries(teaql_generated_aggregation_cache_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_aggregation_cache_tests PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_aggregation_cache_context_contracts COMMAND teaql_generated_aggregation_cache_tests)
    set_tests_properties(generated_aggregation_cache_context_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_aggregation_cache_probe tests/parity/generated_aggregation_cache_probe.cpp)
    target_link_libraries(teaql_generated_aggregation_cache_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_aggregation_cache_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_aggregation_cache_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_aggregation_cache.py
        --cpp $<TARGET_FILE:teaql_generated_aggregation_cache_probe>)
    set_tests_properties(generated_aggregation_cache_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_facets_async_tests teaql_runtime/tests/generated_facets_async_tests.cpp)
    target_link_libraries(teaql_generated_facets_async_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_facets_async_tests PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_facets_async_contracts COMMAND teaql_generated_facets_async_tests)
    set_tests_properties(generated_facets_async_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_facets_tests teaql_runtime/tests/generated_facets_tests.cpp)
    target_link_libraries(teaql_generated_facets_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_facets_tests PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_facets_context_contracts COMMAND teaql_generated_facets_tests)
    set_tests_properties(generated_facets_context_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_facets_probe tests/parity/generated_facets_probe.cpp)
    target_link_libraries(teaql_generated_facets_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_facets_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_facets.py
        --cpp $<TARGET_FILE:teaql_generated_facets_probe>)
    set_tests_properties(generated_facets_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_relation_analysis_tests teaql_runtime/tests/generated_relation_analysis_tests.cpp)
    target_link_libraries(teaql_generated_relation_analysis_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME generated_relation_analysis_context_contracts COMMAND teaql_generated_relation_analysis_tests)
    add_executable(teaql_generated_relation_analysis_probe tests/parity/generated_relation_analysis_probe.cpp)
    target_link_libraries(teaql_generated_relation_analysis_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_relation_analysis_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_relation_analysis_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_relation_analysis.py
        --cpp $<TARGET_FILE:teaql_generated_relation_analysis_probe>)
    set_tests_properties(generated_relation_analysis_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_aggregation_probe tests/parity/generated_aggregation_probe.cpp)
    target_link_libraries(teaql_generated_aggregation_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_aggregation_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_aggregation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_aggregation.py
        --cpp $<TARGET_FILE:teaql_generated_aggregation_probe>)
    set_tests_properties(generated_aggregation_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_general_query_tests teaql_runtime/tests/general_query_tests.cpp)
    add_executable(teaql_generated_field_criteria_probe tests/parity/generated_field_criteria_probe.cpp)
    target_link_libraries(teaql_generated_field_criteria_probe PRIVATE teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_field_criteria_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_field_criteria_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_field_criteria.py
        --cpp $<TARGET_FILE:teaql_generated_field_criteria_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    set_tests_properties(generated_field_criteria_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_general_query_probe tests/parity/general_query_probe.cpp)
    target_link_libraries(teaql_general_query_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    target_compile_definitions(teaql_general_query_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME general_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_general_query.py
        --cpp $<TARGET_FILE:teaql_general_query_probe>)
    target_link_libraries(teaql_general_query_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME general_query_context_contracts COMMAND teaql_general_query_tests)
    target_link_libraries(teaql_generated_execution_tests PRIVATE teaql::runtime)
    add_test(NAME generated_execution_context_contracts COMMAND teaql_generated_execution_tests)
    target_link_libraries(teaql_generated_dynamic_json_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    target_compile_definitions(teaql_generated_dynamic_json_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_dynamic_json_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_dynamic_json.py
        --cpp $<TARGET_FILE:teaql_generated_dynamic_json_probe>)
    set_tests_properties(generated_dynamic_json_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_dynamic_json_tests teaql_runtime/tests/generated_dynamic_json_tests.cpp)
    target_link_libraries(teaql_generated_dynamic_json_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME generated_dynamic_json_context_contracts COMMAND teaql_generated_dynamic_json_tests)
    add_executable(teaql_document_round_trip_tests teaql_runtime/tests/document_round_trip_tests.cpp)
    target_link_libraries(teaql_document_round_trip_tests PRIVATE teaql::runtime)
    add_test(NAME document_round_trip_context_contracts COMMAND teaql_document_round_trip_tests)
    target_link_libraries(teaql_document_round_trip_probe PRIVATE teaql::runtime)
    add_test(NAME document_round_trip_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_document_round_trip_5_0_6.py
        --cpp $<TARGET_FILE:teaql_document_round_trip_probe>)
    target_link_libraries(teaql_round_trip_reference_tests PRIVATE teaql::runtime)
    add_test(NAME round_trip_reference_context_contracts COMMAND teaql_round_trip_reference_tests)
    add_test(NAME round_trip_reference_reentrant_owner COMMAND teaql_round_trip_reference_tests reentrant-owner)
    add_test(NAME round_trip_reference_reentrant_output COMMAND teaql_round_trip_reference_tests reentrant-output)
    add_executable(teaql_round_trip_reference_probe tests/parity/round_trip_reference_probe.cpp)
    target_link_libraries(teaql_round_trip_reference_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME round_trip_reference_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_round_trip_reference.py --cpp $<TARGET_FILE:teaql_round_trip_reference_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_entity_reference_tests teaql_runtime/tests/entity_reference_tests.cpp)
    target_link_libraries(teaql_entity_reference_tests PRIVATE teaql::runtime)
    add_test(NAME entity_reference_context_contracts COMMAND teaql_entity_reference_tests)
    add_test(NAME entity_reference_reentrant_owner COMMAND teaql_entity_reference_tests reentrant-owner)
    add_test(NAME entity_reference_reentrant_output COMMAND teaql_entity_reference_tests reentrant-output)
    add_executable(teaql_entity_reference_probe tests/parity/entity_reference_probe.cpp)
    target_link_libraries(teaql_entity_reference_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME entity_reference_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_reference.py --cpp $<TARGET_FILE:teaql_entity_reference_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_reference_crypto_tests teaql_runtime/tests/reference_crypto_tests.cpp)
    target_link_libraries(teaql_reference_crypto_tests PRIVATE teaql::runtime)
    add_test(NAME reference_crypto_nist_contracts COMMAND teaql_reference_crypto_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/nist_gcm_cases.json)
    add_executable(teaql_inmemory_tests teaql_runtime/tests/inmemory_tests.cpp)
    target_link_libraries(teaql_inmemory_tests PRIVATE teaql::runtime)
    add_test(NAME inmemory_context_contracts COMMAND teaql_inmemory_tests)
    add_executable(teaql_inmemory_probe tests/parity/inmemory_probe.cpp)
    target_link_libraries(teaql_inmemory_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME inmemory_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_inmemory.py --cpp $<TARGET_FILE:teaql_inmemory_probe>
            "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_locking_tests teaql_runtime/tests/locking_tests.cpp)
    target_link_libraries(teaql_locking_tests PRIVATE teaql::runtime)
    add_test(NAME context_locking_contracts COMMAND teaql_locking_tests)
    add_executable(teaql_locking_probe tests/parity/locking_probe.cpp)
    target_link_libraries(teaql_locking_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME context_locking_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_locking.py --cpp $<TARGET_FILE:teaql_locking_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_runtime_examples_tests teaql_provider_sqlite/tests/runtime_examples_tests.cpp)
    target_link_libraries(teaql_runtime_examples_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME runtime_examples_context_contracts COMMAND teaql_runtime_examples_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/runtime_examples_provenance_5_0_6.json)
    set_tests_properties(runtime_examples_context_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_transaction_move_only_tests teaql_runtime/tests/transaction_move_only_tests.cpp)
    target_link_libraries(teaql_transaction_move_only_tests PRIVATE teaql::runtime)
    add_test(NAME transaction_move_only_contracts COMMAND teaql_transaction_move_only_tests)
    add_executable(teaql_transaction_provider_error_tests teaql_runtime/tests/transaction_provider_error_tests.cpp)
    target_link_libraries(teaql_transaction_provider_error_tests PRIVATE teaql::runtime)
    add_test(NAME transaction_provider_error_contracts COMMAND teaql_transaction_provider_error_tests)
    add_executable(teaql_transaction_move_only_probe tests/parity/transaction_move_only_probe.cpp)
    target_link_libraries(teaql_transaction_move_only_probe PRIVATE teaql::runtime)
    add_test(NAME transaction_move_only_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_transaction_move_only_5_0_6.py
        --cpp $<TARGET_FILE:teaql_transaction_move_only_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_ledger_move_only_tests teaql_provider_sqlite/tests/ledger_move_only_tests.cpp)
    target_link_libraries(teaql_ledger_move_only_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME ledger_move_only_contracts COMMAND teaql_ledger_move_only_tests)
    set_tests_properties(ledger_move_only_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_ledger_move_only_probe tests/parity/ledger_move_only_probe.cpp)
    target_link_libraries(teaql_ledger_move_only_probe PRIVATE teaql::runtime)
    add_test(NAME ledger_move_only_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_ledger_move_only_5_0_6.py
        --cpp $<TARGET_FILE:teaql_ledger_move_only_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_policy_model_tests teaql_runtime/tests/policy_model_tests.cpp)
    target_link_libraries(teaql_policy_model_tests PRIVATE teaql::runtime)
    add_test(NAME policy_model_contracts COMMAND teaql_policy_model_tests)
    add_executable(teaql_policy_model_probe tests/parity/policy_model_probe.cpp)
    target_link_libraries(teaql_policy_model_probe PRIVATE teaql::runtime)
    add_test(NAME policy_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_policy_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_policy_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_event_model_tests teaql_runtime/tests/event_model_tests.cpp)
    target_link_libraries(teaql_event_model_tests PRIVATE teaql::runtime)
    add_test(NAME event_model_contracts COMMAND teaql_event_model_tests)
    add_executable(teaql_event_model_probe tests/parity/event_model_probe.cpp)
    add_executable(teaql_audit_event_projection_probe tests/parity/audit_event_projection_probe.cpp)
    target_link_libraries(teaql_audit_event_projection_probe PRIVATE teaql::runtime)
    add_test(NAME audit_event_projection_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_audit_event_projection_5_0_6.py
        --cpp $<TARGET_FILE:teaql_audit_event_projection_probe>)
    target_link_libraries(teaql_event_model_probe PRIVATE teaql::runtime)
    add_test(NAME event_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_event_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_event_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_check_model_tests teaql_runtime/tests/check_model_tests.cpp)
    target_link_libraries(teaql_check_model_tests PRIVATE teaql::runtime)
    add_test(NAME check_model_contracts COMMAND teaql_check_model_tests)
    add_executable(teaql_check_model_probe tests/parity/check_model_probe.cpp)
    target_link_libraries(teaql_check_model_probe PRIVATE teaql::runtime)
    add_test(NAME check_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_check_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_check_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_relation_model_tests teaql_runtime/tests/relation_model_tests.cpp)
    target_link_libraries(teaql_relation_model_tests PRIVATE teaql::runtime)
    add_test(NAME relation_model_contracts COMMAND teaql_relation_model_tests)
    add_executable(teaql_relation_model_probe tests/parity/relation_model_probe.cpp)
    target_link_libraries(teaql_relation_model_probe PRIVATE teaql::runtime)
    add_test(NAME relation_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_relation_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_foreign_work_tests teaql_runtime/tests/runtime_foreign_work_tests.cpp)
    target_link_libraries(teaql_runtime_foreign_work_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_foreign_query_cleanup COMMAND teaql_runtime_foreign_work_tests query)
    add_test(NAME runtime_foreign_mutation_cleanup COMMAND teaql_runtime_foreign_work_tests mutation)
    add_executable(teaql_unified_buffer_clone_tests teaql_runtime/tests/unified_buffer_clone_tests.cpp)
    target_link_libraries(teaql_unified_buffer_clone_tests PRIVATE teaql::runtime)
    add_test(NAME unified_buffer_clone_contracts COMMAND teaql_unified_buffer_clone_tests)
    add_executable(teaql_unified_buffer_clone_probe tests/parity/unified_buffer_clone_probe.cpp)
    target_link_libraries(teaql_unified_buffer_clone_probe PRIVATE teaql::runtime)
    add_test(NAME unified_buffer_clone_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_unified_buffer_clone.py
        --cpp $<TARGET_FILE:teaql_unified_buffer_clone_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_log_model_tests teaql_runtime/tests/log_model_tests.cpp)
    target_link_libraries(teaql_log_model_tests PRIVATE teaql::runtime)
    add_test(NAME log_model_contracts COMMAND teaql_log_model_tests)
    add_executable(teaql_log_model_probe tests/parity/log_model_probe.cpp)
    target_link_libraries(teaql_log_model_probe PRIVATE teaql::runtime)
    add_test(NAME log_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_log_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_log_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_transaction_scope_tests teaql_provider_sqlite/tests/transaction_scope_tests.cpp)
    target_link_libraries(teaql_transaction_scope_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME transaction_scope_context_contracts COMMAND teaql_transaction_scope_tests)
    set_tests_properties(transaction_scope_context_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_transaction_scope_probe tests/parity/transaction_scope_probe.cpp)
    target_link_libraries(teaql_transaction_scope_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME transaction_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_transaction_scope_5_0_6.py --cpp $<TARGET_FILE:teaql_transaction_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_audited_auto_save_tests teaql_provider_sqlite/tests/audited_auto_save_tests.cpp)
    target_link_libraries(teaql_audited_auto_save_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME audited_auto_save_context_contracts COMMAND teaql_audited_auto_save_tests)
    add_executable(teaql_audited_auto_save_probe tests/parity/audited_auto_save_probe.cpp)
    target_link_libraries(teaql_audited_auto_save_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME audited_auto_save_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_audited_auto_save_5_0_6.py --cpp $<TARGET_FILE:teaql_audited_auto_save_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_audited_save_probe tests/parity/audited_save_probe.cpp)
    target_link_libraries(teaql_audited_save_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME audited_save_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_audited_save_5_0_6.py --cpp $<TARGET_FILE:teaql_audited_save_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_audited_save_tests teaql_provider_sqlite/tests/audited_save_tests.cpp)
    target_link_libraries(teaql_audited_save_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME audited_save_context_contracts COMMAND teaql_audited_save_tests)
    add_executable(teaql_graph_saver_tests teaql_provider_sqlite/tests/graph_saver_tests.cpp)
    target_link_libraries(teaql_graph_saver_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME graph_saver_context_contracts COMMAND teaql_graph_saver_tests)
    add_executable(teaql_graph_saver_probe tests/parity/graph_saver_probe.cpp)
    target_link_libraries(teaql_graph_saver_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME graph_saver_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_saver_5_0_6.py --cpp $<TARGET_FILE:teaql_graph_saver_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_ledger_execution_tests teaql_provider_sqlite/tests/ledger_execution_tests.cpp)
    target_link_libraries(teaql_ledger_execution_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME ledger_execution_context_contracts COMMAND teaql_ledger_execution_tests)
    set_tests_properties(ledger_execution_context_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_ledger_execution_probe tests/parity/ledger_execution_probe.cpp)
    target_link_libraries(teaql_ledger_execution_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME ledger_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_ledger_execution_5_0_6.py --cpp $<TARGET_FILE:teaql_ledger_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_entity_graph_execution_tests teaql_provider_sqlite/tests/entity_graph_execution_tests.cpp)
    target_link_libraries(teaql_entity_graph_execution_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_graph_execution_context_contracts COMMAND teaql_entity_graph_execution_tests)
    add_executable(teaql_graph_execution_probe tests/parity/graph_execution_probe.cpp)
    add_executable(teaql_recursive_graph_write_tests teaql_provider_sqlite/tests/recursive_graph_write_tests.cpp)
    target_link_libraries(teaql_recursive_graph_write_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME recursive_graph_write_context_contracts COMMAND teaql_recursive_graph_write_tests)
    add_test(NAME recursive_graph_write_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_recursive_graph_write_5_0_6.py
        --cpp $<TARGET_FILE:teaql_graph_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_graph_node_command_tests teaql_runtime/tests/graph_node_command_tests.cpp)
    target_link_libraries(teaql_graph_node_command_tests PRIVATE teaql::runtime)
    add_test(NAME graph_node_command_context_contracts COMMAND teaql_graph_node_command_tests)
    add_test(NAME graph_node_command_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_node_command_5_0_6.py
        --cpp $<TARGET_FILE:teaql_graph_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_graph_save_adapter_tests teaql_runtime/tests/graph_save_adapter_tests.cpp)
    target_link_libraries(teaql_graph_save_adapter_tests PRIVATE teaql::runtime)
    add_test(NAME graph_save_adapter_context_contracts COMMAND teaql_graph_save_adapter_tests)
    add_test(NAME graph_save_adapter_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_save_adapter_5_0_6.py
        --cpp $<TARGET_FILE:teaql_graph_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_graph_execution_probe PRIVATE teaql::runtime)
    add_executable(teaql_ledger_plan_tests teaql_runtime/tests/ledger_plan_tests.cpp)
    target_link_libraries(teaql_ledger_plan_tests PRIVATE teaql::runtime)
    add_test(NAME ledger_plan_context_contracts COMMAND teaql_ledger_plan_tests)
    add_executable(teaql_ledger_plan_probe tests/parity/ledger_plan_probe.cpp)
    target_link_libraries(teaql_ledger_plan_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME ledger_plan_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_ledger_plan_5_0_6.py --cpp $<TARGET_FILE:teaql_ledger_plan_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    if(Python3_Interpreter_FOUND)
        add_test(NAME graph_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_execution_5_0_6.py --cpp $<TARGET_FILE:teaql_graph_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_runtime_mutation_tests teaql_provider_sqlite/tests/runtime_mutation_tests.cpp)
    target_link_libraries(teaql_runtime_mutation_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME runtime_mutation_context_contracts COMMAND teaql_runtime_mutation_tests)
    add_test(NAME runtime_mutation_reentrant_prepare COMMAND teaql_runtime_mutation_tests reentrant-prepare)
    add_test(NAME runtime_mutation_reentrant_event COMMAND teaql_runtime_mutation_tests reentrant-event)
    add_executable(teaql_runtime_mutation_probe tests/parity/runtime_mutation_probe.cpp)
    target_link_libraries(teaql_runtime_mutation_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME runtime_mutation_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_mutation_5_0_6.py --cpp $<TARGET_FILE:teaql_runtime_mutation_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_entity_graph_plan_tests teaql_provider_sqlite/tests/entity_graph_plan_tests.cpp)
    target_link_libraries(teaql_entity_graph_plan_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_graph_plan_context_contracts COMMAND teaql_entity_graph_plan_tests)
    add_test(NAME internal_id_reentrant_close COMMAND teaql_internal_id_tests reentrant-close)
    add_test(NAME internal_id_reentrant_floor COMMAND teaql_internal_id_tests reentrant-floor)
    add_executable(teaql_entity_graph_plan_probe tests/parity/entity_graph_plan_probe.cpp)
    add_test(NAME graph_plan_scopes_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_plan_scopes_5_0_6.py
        --cpp $<TARGET_FILE:teaql_entity_graph_plan_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_entity_graph_plan_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME entity_graph_plan_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_graph_plan_5_0_6.py --cpp $<TARGET_FILE:teaql_entity_graph_plan_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_policy_plan_probe tests/parity/policy_plan_probe.cpp)
    target_link_libraries(teaql_policy_plan_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME policy_plan_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_policy_plan_5_0_6.py --cpp $<TARGET_FILE:teaql_policy_plan_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_mutation_policy_tests teaql_runtime/tests/mutation_policy_tests.cpp)
    target_link_libraries(teaql_mutation_policy_tests PRIVATE teaql::runtime)
    add_test(NAME mutation_policy_context_contracts COMMAND teaql_mutation_policy_tests)
    add_test(NAME mutation_policy_reentrant_close COMMAND teaql_mutation_policy_tests reentrant-close)
    add_executable(teaql_mutation_policy_probe tests/parity/mutation_policy_probe.cpp)
    target_link_libraries(teaql_mutation_policy_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME mutation_policy_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_mutation_policy_5_0_6.py --cpp $<TARGET_FILE:teaql_mutation_policy_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_save_preparation_tests teaql_runtime/tests/save_preparation_tests.cpp)
    target_link_libraries(teaql_save_preparation_tests PRIVATE teaql::runtime)
    add_test(NAME save_preparation_context_contracts COMMAND teaql_save_preparation_tests)
    add_executable(teaql_save_preparation_probe tests/parity/save_preparation_probe.cpp)
    target_link_libraries(teaql_save_preparation_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME save_preparation_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_save_preparation_5_0_6.py --cpp $<TARGET_FILE:teaql_save_preparation_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_graph_fix_tests teaql_runtime/tests/graph_fix_tests.cpp)
    target_link_libraries(teaql_graph_fix_tests PRIVATE teaql::runtime)
    add_test(NAME graph_fix_context_contracts COMMAND teaql_graph_fix_tests)
    add_executable(teaql_graph_fix_probe tests/parity/graph_fix_probe.cpp)
    target_link_libraries(teaql_graph_fix_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME graph_fix_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_fix_5_0_6.py --cpp $<TARGET_FILE:teaql_graph_fix_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_checker_tests teaql_runtime/tests/checker_tests.cpp)
    target_link_libraries(teaql_checker_tests PRIVATE teaql::runtime)
    add_test(NAME checker_context_contracts COMMAND teaql_checker_tests)
    add_test(NAME checker_reentrant_close COMMAND teaql_checker_tests reentrant-close)
    add_test(NAME checker_registration_reentrant_close COMMAND teaql_checker_tests registration-close)
    add_test(NAME checker_typed_direct_reentrant_close COMMAND teaql_checker_tests typed-direct-close)
    add_executable(teaql_checker_probe tests/parity/checker_probe.cpp)
    target_link_libraries(teaql_checker_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME checker_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_checker_5_0_6.py --cpp $<TARGET_FILE:teaql_checker_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_i18n_tests teaql_runtime/tests/i18n_tests.cpp)
    target_link_libraries(teaql_i18n_tests PRIVATE teaql::runtime)
    add_test(NAME i18n_context_contracts COMMAND teaql_i18n_tests)
    add_executable(teaql_i18n_probe tests/parity/i18n_probe.cpp)
    target_link_libraries(teaql_i18n_probe PRIVATE teaql::runtime)
    add_executable(teaql_i18n_debug_scope_probe tests/parity/i18n_debug_scope_probe.cpp)
    target_link_libraries(teaql_i18n_debug_scope_probe PRIVATE teaql::runtime)
    add_test(NAME i18n_debug_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_i18n_debug_scope.py
        --cpp $<TARGET_FILE:teaql_i18n_debug_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    if(Python3_Interpreter_FOUND)
        add_test(NAME i18n_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_i18n.py --cpp $<TARGET_FILE:teaql_i18n_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_sqlite_schema_probe tests/parity/sqlite_schema_probe.cpp)
    target_link_libraries(teaql_sqlite_schema_probe PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_executable(teaql_provider_sqlite_schema_tests teaql_provider_sqlite/tests/provider_sqlite_schema_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_schema_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME provider_sqlite_schema_contracts COMMAND teaql_provider_sqlite_schema_tests)
    add_executable(teaql_original_sql_binding_tests tests/parity/teaql_sql/binding_tests.cpp)
    add_executable(teaql_original_query_execution_tests tests/parity/teaql_sql/execution_tests.cpp)
    add_executable(teaql_original_mutation_execution_tests tests/parity/teaql_sql/mutation_execution_tests.cpp)
    target_link_libraries(teaql_original_mutation_execution_tests PRIVATE teaql::runtime teaql::sql)
    add_test(NAME original_sql_mutation_execution_contracts COMMAND teaql_original_mutation_execution_tests)
    target_link_libraries(teaql_original_query_execution_tests PRIVATE teaql::runtime teaql::sql)
    add_test(NAME original_sql_query_execution_contracts COMMAND teaql_original_query_execution_tests)
    target_link_libraries(teaql_original_sql_binding_tests PRIVATE teaql::sql)
    add_test(NAME original_sql_binding_policy_contracts COMMAND teaql_original_sql_binding_tests)
    add_executable(teaql_original_sql_rendering_tests tests/parity/teaql_sql/log_rendering_tests.cpp)
    add_executable(teaql_original_sql_default_tests tests/parity/teaql_sql/default_contracts.cpp)
    add_executable(teaql_original_sql_executor_tests tests/parity/teaql_sql/executor_contracts.cpp)
    target_link_libraries(teaql_original_sql_executor_tests PRIVATE teaql::runtime)
    add_test(NAME original_sql_executor_contracts COMMAND teaql_original_sql_executor_tests)
    add_executable(teaql_original_sql_compiler_tests tests/parity/teaql_sql/compiler.cpp)
    target_link_libraries(teaql_original_sql_compiler_tests PRIVATE teaql::sql teaql::runtime)
    add_test(NAME original_sql_compiler_contracts COMMAND teaql_original_sql_compiler_tests)
    target_link_libraries(teaql_original_sql_default_tests PRIVATE teaql::sql)
    add_test(NAME original_sql_default_contracts COMMAND teaql_original_sql_default_tests)
    add_executable(teaql_original_diagnostic_stream_tests tests/parity/teaql_sql/diagnostic_stream.cpp)
    target_link_libraries(teaql_original_diagnostic_stream_tests PRIVATE teaql::runtime)
    add_test(NAME original_sql_diagnostic_stream_contracts COMMAND teaql_original_diagnostic_stream_tests)
    target_link_libraries(teaql_original_sql_rendering_tests PRIVATE teaql::sql)
    add_test(NAME original_sql_log_rendering_contracts COMMAND teaql_original_sql_rendering_tests)
    add_executable(teaql_original_business_clock_tests tests/parity/teaql_runtime/business_clock.cpp)
    add_executable(teaql_original_entity_status_tests tests/parity/teaql_runtime/entity_status.cpp)
    target_link_libraries(teaql_original_entity_status_tests PRIVATE teaql::memory teaql::runtime)
    add_test(NAME original_runtime_entity_status_contracts COMMAND teaql_original_entity_status_tests)
    add_executable(teaql_original_cache_relation_tests tests/parity/teaql_runtime/cache_relation.cpp)
    target_link_libraries(teaql_original_cache_relation_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_cache_relation_contracts COMMAND teaql_original_cache_relation_tests)
    add_executable(teaql_original_inmemory_tests tests/parity/teaql_runtime/inmemory_engine.cpp)
    target_link_libraries(teaql_original_inmemory_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_inmemory_contracts COMMAND teaql_original_inmemory_tests)
    add_executable(teaql_original_checker_locale_tests tests/parity/teaql_runtime/checker_locale.cpp)
    target_link_libraries(teaql_original_checker_locale_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_checker_locale_contracts COMMAND teaql_original_checker_locale_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql_runtime/locale_cases_v1.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql_runtime/builtin_i18n_v1.json)
    add_executable(teaql_original_log_telemetry_tests tests/parity/teaql_runtime/log_telemetry.cpp)
    target_link_libraries(teaql_original_log_telemetry_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_log_telemetry_contracts COMMAND teaql_original_log_telemetry_tests)
    add_executable(teaql_original_policy_graph_tests tests/parity/teaql_runtime/policy_graph.cpp)
    target_link_libraries(teaql_original_policy_graph_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_policy_graph_contracts COMMAND teaql_original_policy_graph_tests)
    add_executable(teaql_original_log_privacy_tests tests/parity/teaql_runtime/log_privacy.cpp)
    target_link_libraries(teaql_original_log_privacy_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_log_privacy_contracts COMMAND teaql_original_log_privacy_tests)
    add_executable(teaql_original_context_tests tests/parity/teaql_runtime/context.cpp)
    target_link_libraries(teaql_original_context_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_context_contracts COMMAND teaql_original_context_tests)
    if(Python3_Interpreter_FOUND)
        add_test(NAME original_runtime_file_log_privacy_contracts COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql_runtime/file_log_privacy.py
            --cpp $<TARGET_FILE:teaql_original_log_privacy_tests>
            "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_original_masking_tests tests/parity/teaql_runtime/masking_contract_tests.cpp)
    add_executable(teaql_original_flat_graph_tests tests/parity/teaql_runtime/flat_identity_graph.cpp)
    add_executable(teaql_original_ledger_tests tests/parity/teaql_runtime/test_incremental_ledger.cpp)
    target_link_libraries(teaql_original_ledger_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_incremental_ledger_contracts COMMAND teaql_original_ledger_tests)
    target_link_libraries(teaql_original_flat_graph_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_flat_identity_graph_contracts COMMAND teaql_original_flat_graph_tests)
    target_link_libraries(teaql_original_masking_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_masking_contracts COMMAND teaql_original_masking_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql_runtime/data/masking_v1.tsv)
    target_link_libraries(teaql_original_business_clock_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_business_clock_contracts COMMAND teaql_original_business_clock_tests)
    add_executable(teaql_original_business_permutation_tests tests/parity/teaql_runtime/business_id_permutation.cpp)
    target_link_libraries(teaql_original_business_permutation_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_business_permutation_contracts COMMAND teaql_original_business_permutation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql_runtime/data/business_id_permutation_v1.csv)
    add_executable(teaql_original_provider_search_tests tests/parity/teaql_provider_sqlite/dynamic_search.cpp)
    add_executable(teaql_original_provider_storage_tests tests/parity/teaql_provider_sqlite/storage.cpp)
    target_link_libraries(teaql_original_provider_storage_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME original_provider_storage_contracts COMMAND teaql_original_provider_storage_tests)
    add_executable(teaql_original_provider_schema_tests tests/parity/teaql_provider_sqlite/schema.cpp)
    target_link_libraries(teaql_original_provider_schema_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME original_provider_schema_contracts COMMAND teaql_original_provider_schema_tests)
    add_executable(teaql_original_provider_query_tests tests/parity/teaql_provider_sqlite/query_rows.cpp)
    target_link_libraries(teaql_original_provider_query_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME original_provider_query_contracts COMMAND teaql_original_provider_query_tests)
    add_executable(teaql_original_provider_transaction_tests tests/parity/teaql_provider_sqlite/transactions.cpp)
    target_link_libraries(teaql_original_provider_transaction_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME original_provider_transaction_contracts COMMAND teaql_original_provider_transaction_tests)
    add_executable(teaql_original_entity_save_tests tests/parity/teaql_runtime/entity_save.cpp)
    target_link_libraries(teaql_original_entity_save_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_entity_save_contracts COMMAND teaql_original_entity_save_tests)
    add_executable(teaql_original_round_trip_reference_tests tests/parity/teaql_runtime/round_trip_reference.cpp)
    target_link_libraries(teaql_original_round_trip_reference_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_round_trip_reference_contracts COMMAND teaql_original_round_trip_reference_tests)
    add_executable(teaql_original_runtime_registration_tests tests/parity/teaql_runtime/registration.cpp)
    target_link_libraries(teaql_original_runtime_registration_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_registration_contracts COMMAND teaql_original_runtime_registration_tests)
    add_executable(teaql_original_runtime_service_tests tests/parity/teaql_runtime/service_boundaries.cpp)
    target_link_libraries(teaql_original_runtime_service_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_service_boundary_contracts COMMAND teaql_original_runtime_service_tests)
    add_executable(teaql_original_runtime_validation_tests tests/parity/teaql_runtime/validation.cpp)
    target_link_libraries(teaql_original_runtime_validation_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_validation_contracts COMMAND teaql_original_runtime_validation_tests)
    add_executable(teaql_original_runtime_events_relations_tests tests/parity/teaql_runtime/events_relations.cpp)
    target_link_libraries(teaql_original_runtime_events_relations_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_events_relations_contracts COMMAND teaql_original_runtime_events_relations_tests)
    add_executable(teaql_original_runtime_typed_query_tests tests/parity/teaql_runtime/typed_queries.cpp)
    target_link_libraries(teaql_original_runtime_typed_query_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_typed_query_contracts COMMAND teaql_original_runtime_typed_query_tests)
    add_executable(teaql_original_runtime_aggregate_tests tests/parity/teaql_runtime/aggregates_cache.cpp)
    target_link_libraries(teaql_original_runtime_aggregate_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_aggregate_cache_contracts COMMAND teaql_original_runtime_aggregate_tests)
    add_executable(teaql_original_runtime_pagination_tests tests/parity/teaql_runtime/pagination.cpp)
    target_link_libraries(teaql_original_runtime_pagination_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_pagination_contracts COMMAND teaql_original_runtime_pagination_tests)
    add_executable(teaql_original_runtime_memory_service_tests tests/parity/teaql_runtime/memory_service.cpp)
    target_link_libraries(teaql_original_runtime_memory_service_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME original_runtime_memory_service_contracts COMMAND teaql_original_runtime_memory_service_tests)
    add_executable(teaql_original_runtime_context_tail_tests tests/parity/teaql_runtime/context_tail.cpp)
    target_link_libraries(teaql_original_runtime_context_tail_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_context_tail_contracts COMMAND teaql_original_runtime_context_tail_tests)
    add_executable(teaql_original_runtime_graph_reference_tests tests/parity/teaql_runtime/graph_reference_lifetime.cpp)
    target_link_libraries(teaql_original_runtime_graph_reference_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_graph_reference_contracts COMMAND teaql_original_runtime_graph_reference_tests)
    if(Python3_Interpreter_FOUND)
        add_test(NAME original_runtime_optimistic_conflict_output_contracts
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql_runtime/optimistic_conflict_output.py
            --cpp $<TARGET_FILE:teaql_original_runtime_memory_service_tests>
            "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    set_tests_properties(original_provider_query_contracts PROPERTIES TIMEOUT 60)
    set_tests_properties(original_provider_schema_contracts PROPERTIES TIMEOUT 60)
    target_link_libraries(teaql_original_provider_search_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME original_provider_dynamic_search_contracts COMMAND teaql_original_provider_search_tests)
    add_executable(teaql_original_provider_guard_tests tests/parity/teaql_provider_sqlite/guarded_mutation.cpp)
    target_link_libraries(teaql_original_provider_guard_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME original_provider_guarded_mutation_contracts COMMAND teaql_original_provider_guard_tests)
    set_tests_properties(original_provider_dynamic_search_contracts original_provider_guarded_mutation_contracts
        PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_provider_sqlite_unicode_tests teaql_provider_sqlite/tests/provider_sqlite_unicode_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_unicode_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_unicode_contracts COMMAND teaql_provider_sqlite_unicode_tests)
    set_tests_properties(provider_sqlite_unicode_contracts PROPERTIES TIMEOUT 180)
    add_executable(teaql_runtime_base_probe tests/parity/runtime_base_probe.cpp)
    target_link_libraries(teaql_runtime_base_probe PRIVATE teaql::runtime)
    add_executable(teaql_context_query_probe tests/parity/context_query_probe.cpp)
    target_link_libraries(teaql_context_query_probe PRIVATE teaql::runtime)
    add_executable(teaql_relation_execution_probe tests/parity/relation_execution_probe.cpp)
    target_link_libraries(teaql_relation_execution_probe PRIVATE teaql::runtime)
    add_executable(teaql_query_keys_probe tests/parity/query_keys_probe.cpp)
    target_link_libraries(teaql_query_keys_probe PRIVATE teaql::runtime)
    add_executable(teaql_query_keys_tests teaql_runtime/tests/query_keys_tests.cpp)
    target_link_libraries(teaql_query_keys_tests PRIVATE teaql::runtime)
    add_test(NAME query_keys_contracts COMMAND teaql_query_keys_tests)
    add_executable(teaql_id_set_build_probe tests/parity/id_set_build_probe.cpp)
    target_link_libraries(teaql_id_set_build_probe PRIVATE teaql::runtime)
    add_executable(teaql_id_set_build_tests teaql_runtime/tests/id_set_build_tests.cpp)
    target_link_libraries(teaql_id_set_build_tests PRIVATE teaql::runtime)
    add_test(NAME id_set_build_contracts COMMAND teaql_id_set_build_tests)
    set_tests_properties(id_set_build_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_query_store_model_tests teaql_runtime/tests/query_store_model_tests.cpp)
    target_link_libraries(teaql_query_store_model_tests PRIVATE teaql::runtime)
    add_test(NAME query_store_model_contracts COMMAND teaql_query_store_model_tests)
    add_executable(teaql_query_store_model_probe tests/parity/query_store_model_probe.cpp)
    target_link_libraries(teaql_query_store_model_probe PRIVATE teaql::runtime)
    add_test(NAME query_store_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_query_store_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_query_store_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_query_store_probe tests/parity/query_store_probe.cpp)
    target_link_libraries(teaql_query_store_probe PRIVATE teaql::runtime)
    add_executable(teaql_query_store_tests teaql_runtime/tests/query_store_tests.cpp)
    target_link_libraries(teaql_query_store_tests PRIVATE teaql::runtime)
    add_test(NAME query_store_contracts COMMAND teaql_query_store_tests)
    set_tests_properties(query_store_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_relation_enhancement_probe tests/parity/relation_enhancement_probe.cpp)
    target_link_libraries(teaql_relation_enhancement_probe PRIVATE teaql::runtime)
    add_executable(teaql_relation_hydration_probe tests/parity/relation_hydration_probe.cpp)
    target_link_libraries(teaql_relation_hydration_probe PRIVATE teaql::runtime)
    add_executable(teaql_relation_execution_tests teaql_provider_sqlite/tests/relation_execution_tests.cpp)
    target_link_libraries(teaql_relation_execution_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME relation_execution_contracts COMMAND teaql_relation_execution_tests)
    set_tests_properties(relation_execution_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_relation_hydration_tests teaql_provider_sqlite/tests/relation_hydration_tests.cpp)
    target_link_libraries(teaql_relation_hydration_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME relation_hydration_contracts COMMAND teaql_relation_hydration_tests)
    set_tests_properties(relation_hydration_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_relation_enhancement_tests teaql_provider_sqlite/tests/relation_enhancement_tests.cpp)
    target_link_libraries(teaql_relation_enhancement_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME relation_enhancement_contracts COMMAND teaql_relation_enhancement_tests)
    set_tests_properties(relation_enhancement_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_context_query_tests teaql_runtime/tests/context_query_tests.cpp)
    target_link_libraries(teaql_context_query_tests PRIVATE teaql::runtime)
    add_test(NAME context_query_contracts COMMAND teaql_context_query_tests)
    set_tests_properties(context_query_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_entity_service_tests teaql_provider_sqlite/tests/entity_service_tests.cpp)
    target_link_libraries(teaql_entity_service_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_service_contracts COMMAND teaql_entity_service_tests)
    add_executable(teaql_enhanced_entities_tests teaql_provider_sqlite/tests/enhanced_entities_tests.cpp)
    target_link_libraries(teaql_enhanced_entities_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME enhanced_entities_contracts COMMAND teaql_enhanced_entities_tests)
    add_executable(teaql_runtime_move_only_tests teaql_runtime/tests/runtime_move_only_tests.cpp)
    target_link_libraries(teaql_runtime_move_only_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_move_only_query_contracts COMMAND teaql_runtime_move_only_tests)
    add_executable(teaql_runtime_completion_tests teaql_runtime/tests/runtime_completion_tests.cpp)
    target_link_libraries(teaql_runtime_completion_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_generic_completion_contracts COMMAND teaql_runtime_completion_tests)
    add_executable(teaql_enhanced_entities_probe tests/parity/enhanced_entities_probe.cpp)
    target_link_libraries(teaql_enhanced_entities_probe PRIVATE teaql::provider_sqlite)
    set_tests_properties(enhanced_entities_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_entity_stream_tests teaql_provider_sqlite/tests/entity_stream_tests.cpp)
    target_link_libraries(teaql_entity_stream_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_stream_contracts COMMAND teaql_entity_stream_tests)
    add_executable(teaql_id_set_page_tests teaql_provider_sqlite/tests/id_set_page_tests.cpp)
    target_link_libraries(teaql_id_set_page_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME id_set_page_contracts COMMAND teaql_id_set_page_tests)
    add_executable(teaql_id_set_page_probe tests/parity/id_set_page_probe.cpp)
    target_link_libraries(teaql_id_set_page_probe PRIVATE teaql::runtime)
    set_tests_properties(id_set_page_contracts PROPERTIES TIMEOUT 30)
    set_tests_properties(entity_stream_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_json_graph_decoder_probe tests/parity/json_graph_decoder_probe.cpp)
    target_link_libraries(teaql_json_graph_decoder_probe PRIVATE teaql::runtime)
    add_test(NAME json_graph_decoder_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_json_graph_decoder_5_0_6.py
        --cpp $<TARGET_FILE:teaql_json_graph_decoder_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_json_graph_views_probe tests/parity/json_graph_views_probe.cpp)
    target_link_libraries(teaql_json_graph_views_probe PRIVATE teaql::runtime)
    add_test(NAME json_graph_views_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_json_graph_views_5_0_6.py
        --cpp $<TARGET_FILE:teaql_json_graph_views_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME json_graph_views_context_contracts COMMAND teaql_json_graph_views_probe --self-test)
    add_executable(teaql_relation_hydration_replay_probe tests/parity/relation_hydration_replay_probe.cpp)
    target_link_libraries(teaql_relation_hydration_replay_probe PRIVATE teaql::runtime)
    add_test(NAME relation_hydration_intent_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_hydration_intent_5_0_6.py
        --cpp $<TARGET_FILE:teaql_relation_hydration_replay_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_loaded_row_graph_probe tests/parity/loaded_row_graph_probe.cpp)
    target_link_libraries(teaql_loaded_row_graph_probe PRIVATE teaql::runtime)
    add_test(NAME loaded_row_graph_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_loaded_row_graph_5_0_6.py
        --cpp $<TARGET_FILE:teaql_loaded_row_graph_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME loaded_row_graph_context_contracts COMMAND teaql_loaded_row_graph_probe --self-test)
    add_executable(teaql_hydration_flat_facets_probe tests/parity/hydration_flat_facets_probe.cpp)
    target_link_libraries(teaql_hydration_flat_facets_probe PRIVATE teaql::runtime)
    add_test(NAME hydration_flat_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_hydration_flat_facets_5_0_6.py
        --cpp $<TARGET_FILE:teaql_hydration_flat_facets_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME hydration_flat_facets_context_close COMMAND teaql_hydration_flat_facets_probe --self-test)
    add_executable(teaql_graph_relation_facets_probe tests/parity/graph_relation_facets_probe.cpp)
    target_link_libraries(teaql_graph_relation_facets_probe PRIVATE teaql::runtime)
    add_test(NAME graph_relation_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_relation_facets_5_0_6.py
        --cpp $<TARGET_FILE:teaql_graph_relation_facets_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME graph_relation_facets_context_contracts COMMAND teaql_graph_relation_facets_probe --self-test)
    add_executable(teaql_hydration_owner_facets_async_probe tests/parity/hydration_owner_facets_async_probe.cpp)
    target_link_libraries(teaql_hydration_owner_facets_async_probe PRIVATE teaql::runtime)
    add_test(NAME hydration_owner_facets_async_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_hydration_owner_facets_async_5_0_6.py
        --cpp $<TARGET_FILE:teaql_hydration_owner_facets_async_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME hydration_owner_facets_async_context_close COMMAND teaql_hydration_owner_facets_async_probe --self-test)
    add_executable(teaql_hydration_owner_facets_probe tests/parity/hydration_owner_facets_probe.cpp)
    target_link_libraries(teaql_hydration_owner_facets_probe PRIVATE teaql::runtime)
    add_test(NAME hydration_owner_facets_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_hydration_owner_facets_5_0_6.py
        --cpp $<TARGET_FILE:teaql_hydration_owner_facets_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_hydration_sibling_keys_probe tests/parity/hydration_sibling_keys_probe.cpp)
    target_link_libraries(teaql_hydration_sibling_keys_probe PRIVATE teaql::runtime)
    add_test(NAME hydration_sibling_keys_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_hydration_sibling_keys_5_0_6.py
        --cpp $<TARGET_FILE:teaql_hydration_sibling_keys_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_relation_planning_extensions_probe tests/parity/relation_planning_extensions_probe.cpp)
    target_link_libraries(teaql_relation_planning_extensions_probe PRIVATE teaql::runtime)
    add_test(NAME relation_planning_extensions_contracts COMMAND teaql_relation_planning_extensions_probe --self-test)
    add_executable(teaql_entity_service_probe tests/parity/entity_service_probe.cpp)
    target_link_libraries(teaql_entity_service_probe PRIVATE teaql::runtime)
    add_executable(teaql_typed_aggregate_scope_probe tests/parity/typed_aggregate_scope_probe.cpp)
    target_link_libraries(teaql_typed_aggregate_scope_probe PRIVATE teaql::runtime)
    add_executable(teaql_typed_aggregate_tests teaql_runtime/tests/typed_aggregate_tests.cpp)
    target_link_libraries(teaql_typed_aggregate_tests PRIVATE teaql::runtime)
    add_test(NAME typed_aggregate_context_contracts COMMAND teaql_typed_aggregate_tests)
    add_test(NAME typed_aggregate_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_typed_aggregate_scope.py
        --cpp $<TARGET_FILE:teaql_typed_aggregate_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_entity_query_probe tests/parity/entity_query_probe.cpp)
    target_link_libraries(teaql_entity_query_probe PRIVATE teaql::runtime)
    add_executable(teaql_entity_stream_probe tests/parity/entity_stream_probe.cpp)
    target_link_libraries(teaql_entity_stream_probe PRIVATE teaql::runtime)
    add_executable(teaql_entity_decoder_tests teaql_provider_sqlite/tests/entity_decoder_tests.cpp)
    target_link_libraries(teaql_entity_decoder_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_decoder_contracts COMMAND teaql_entity_decoder_tests)
    add_executable(teaql_entity_decoder_probe tests/parity/entity_decoder_probe.cpp)
    target_link_libraries(teaql_entity_decoder_probe PRIVATE teaql::runtime)
    add_executable(teaql_entity_runtime_probe tests/parity/entity_runtime_probe.cpp)
    target_link_libraries(teaql_entity_runtime_probe PRIVATE teaql::runtime)
    add_executable(teaql_entity_runtime_tests teaql_runtime/tests/entity_runtime_tests.cpp)
    target_link_libraries(teaql_entity_runtime_tests PRIVATE teaql::runtime)
    add_test(NAME entity_runtime_contracts COMMAND teaql_entity_runtime_tests)
    add_executable(teaql_request_trace_scope_tests ../teaql_runtime/tests/request_trace_scope_tests.cpp)
    target_link_libraries(teaql_request_trace_scope_tests PRIVATE teaql::runtime)
    add_test(NAME request_trace_scope_contracts COMMAND teaql_request_trace_scope_tests)
    add_executable(teaql_runtime_graph_model_probe tests/parity/runtime_graph_model_probe.cpp)
    add_executable(teaql_graph_repository_probe tests/parity/graph_repository_probe.cpp)
    target_link_libraries(teaql_graph_repository_probe PRIVATE teaql::runtime)
    add_test(NAME graph_repository_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_repository_5_0_6.py
        --cpp $<TARGET_FILE:teaql_graph_repository_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_runtime_graph_model_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_graph_model_tests teaql_runtime/tests/runtime_graph_model_tests.cpp)
    target_link_libraries(teaql_runtime_graph_model_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_graph_model_contracts COMMAND teaql_runtime_graph_model_tests)
    add_test(NAME runtime_graph_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_graph_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_runtime_graph_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_graph_probe tests/parity/runtime_graph_probe.cpp)
    target_link_libraries(teaql_runtime_graph_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_graph_tests teaql_runtime/tests/runtime_graph_tests.cpp)
    target_link_libraries(teaql_runtime_graph_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_graph_contracts COMMAND teaql_runtime_graph_tests)
    add_executable(teaql_scoped_comments_probe tests/parity/scoped_comments_probe.cpp)
    target_link_libraries(teaql_scoped_comments_probe PRIVATE teaql::runtime)
    add_test(NAME scoped_comments_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_scoped_comments_5_0_6.py
        --cpp $<TARGET_FILE:teaql_scoped_comments_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_context_event_probe tests/parity/context_event_probe.cpp)
    target_link_libraries(teaql_context_event_probe PRIVATE teaql::runtime)
    add_executable(teaql_context_event_tests teaql_runtime/tests/context_event_tests.cpp)
    target_link_libraries(teaql_context_event_tests PRIVATE teaql::runtime)
    add_test(NAME context_event_contracts COMMAND teaql_context_event_tests)
    add_executable(teaql_telemetry_tests teaql_runtime/tests/telemetry_tests.cpp)
    target_link_libraries(teaql_telemetry_tests PRIVATE teaql::runtime)
    add_test(NAME telemetry_contracts COMMAND teaql_telemetry_tests)
    set_tests_properties(telemetry_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_telemetry_probe tests/parity/telemetry_probe.cpp)
    target_link_libraries(teaql_telemetry_probe PRIVATE teaql::runtime)
    add_executable(teaql_telemetry_model_scope_probe tests/parity/telemetry_model_scope_probe.cpp)
    target_link_libraries(teaql_telemetry_model_scope_probe PRIVATE teaql::runtime)
    add_test(NAME telemetry_model_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_telemetry_model_scope.py
        --cpp $<TARGET_FILE:teaql_telemetry_model_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_context_logging_tests teaql_runtime/tests/context_logging_tests.cpp)
    target_link_libraries(teaql_context_logging_tests PRIVATE teaql::runtime)
    add_test(NAME context_logging_contracts COMMAND teaql_context_logging_tests)
    add_executable(teaql_context_logging_probe tests/parity/context_logging_probe.cpp)
    target_link_libraries(teaql_context_logging_probe PRIVATE teaql::runtime)
    add_executable(teaql_log_manager_tests teaql_runtime/tests/log_manager_tests.cpp)
    target_link_libraries(teaql_log_manager_tests PRIVATE teaql::runtime)
    add_test(NAME log_manager_contracts COMMAND teaql_log_manager_tests)
    add_executable(teaql_log_manager_probe tests/parity/log_manager_probe.cpp)
    target_link_libraries(teaql_log_manager_probe PRIVATE teaql::runtime)
    add_executable(teaql_log_formatter_tests teaql_runtime/tests/log_formatter_tests.cpp)
    target_link_libraries(teaql_log_formatter_tests PRIVATE teaql::runtime)
    add_test(NAME log_formatter_contracts COMMAND teaql_log_formatter_tests)
    add_executable(teaql_log_formatter_probe tests/parity/log_formatter_probe.cpp)
    target_link_libraries(teaql_log_formatter_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_log_tests teaql_runtime/tests/runtime_log_tests.cpp)
    target_link_libraries(teaql_runtime_log_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_log_contracts COMMAND teaql_runtime_log_tests)
    add_executable(teaql_runtime_log_probe tests/parity/runtime_log_probe.cpp)
    target_link_libraries(teaql_runtime_log_probe PRIVATE teaql::runtime)
    add_executable(teaql_audit_event_tests teaql_runtime/tests/audit_event_tests.cpp)
    target_link_libraries(teaql_audit_event_tests PRIVATE teaql::runtime)
    add_test(NAME audit_event_contracts COMMAND teaql_audit_event_tests)
    add_test(NAME audit_event_context_reentrant_close COMMAND teaql_audit_event_tests context-close)
    add_test(NAME audit_event_owner_reentrant_close COMMAND teaql_audit_event_tests owner-close)
    add_test(NAME audit_event_payload_reentrant_close COMMAND teaql_audit_event_tests event-close)
    add_test(NAME audit_event_result_reentrant_close COMMAND teaql_audit_event_tests result-close)
    add_executable(teaql_audit_event_probe tests/parity/audit_event_probe.cpp)
    target_link_libraries(teaql_audit_event_probe PRIVATE teaql::runtime)
    add_executable(teaql_schema_runtime_probe tests/parity/schema_runtime_probe.cpp)
    target_link_libraries(teaql_schema_runtime_probe PRIVATE teaql::runtime)
    add_executable(teaql_schema_runtime_tests teaql_runtime/tests/schema_runtime_tests.cpp)
    target_link_libraries(teaql_schema_runtime_tests PRIVATE teaql::runtime)
    add_test(NAME schema_runtime_contracts COMMAND teaql_schema_runtime_tests)
    add_executable(teaql_generated_bootstrap_tests teaql_runtime/tests/generated_bootstrap_tests.cpp)
    target_link_libraries(teaql_generated_bootstrap_tests PRIVATE teaql::runtime)
    add_test(NAME generated_bootstrap_contracts COMMAND teaql_generated_bootstrap_tests)
    set_tests_properties(generated_bootstrap_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_bootstrap_classifier_probe tests/parity/bootstrap_classifier_probe.cpp)
    target_link_libraries(teaql_bootstrap_classifier_probe PRIVATE teaql::runtime)
    add_test(NAME bootstrap_classifier_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_bootstrap_classifier.py
        --cpp $<TARGET_FILE:teaql_bootstrap_classifier_probe>)
    add_executable(teaql_internal_id_tests teaql_provider_sqlite/tests/internal_id_tests.cpp)
    target_link_libraries(teaql_internal_id_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME internal_id_contracts COMMAND teaql_internal_id_tests)
    add_executable(teaql_internal_id_probe tests/parity/internal_id_probe.cpp)
    target_link_libraries(teaql_internal_id_probe PRIVATE teaql::runtime)
    add_executable(teaql_business_service_tests teaql_provider_sqlite/tests/business_service_tests.cpp)
    target_link_libraries(teaql_business_service_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME business_service_contracts COMMAND teaql_business_service_tests)
    add_executable(teaql_business_service_probe tests/parity/business_service_probe.cpp)
    target_link_libraries(teaql_business_service_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_business_id_tests teaql_runtime/tests/runtime_business_id_tests.cpp)
    target_link_libraries(teaql_runtime_business_id_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_business_id_contracts COMMAND teaql_runtime_business_id_tests)
    add_executable(teaql_runtime_business_id_probe tests/parity/runtime_business_id_probe.cpp)
    target_link_libraries(teaql_runtime_business_id_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_registry_tests teaql_runtime/tests/runtime_registry_tests.cpp)
    target_link_libraries(teaql_runtime_registry_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_registry_contracts COMMAND teaql_runtime_registry_tests)
    add_executable(teaql_runtime_registry_probe tests/parity/runtime_registry_probe.cpp)
    target_link_libraries(teaql_runtime_registry_probe PRIVATE teaql::runtime)
    add_executable(teaql_registry_debug_scope_probe tests/parity/registry_debug_scope_probe.cpp)
    target_link_libraries(teaql_registry_debug_scope_probe PRIVATE teaql::runtime)
    add_test(NAME registry_debug_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_registry_debug_scope.py
        --cpp $<TARGET_FILE:teaql_registry_debug_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_check_tests teaql_runtime/tests/runtime_check_tests.cpp)
    target_link_libraries(teaql_runtime_check_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_check_error_contracts COMMAND teaql_runtime_check_tests)
    add_executable(teaql_error_generic_tests teaql_runtime/tests/error_generic_tests.cpp)
    target_link_libraries(teaql_error_generic_tests PRIVATE teaql::runtime)
    add_test(NAME error_generic_context_contracts COMMAND teaql_error_generic_tests)
    add_executable(teaql_sql_move_only_tests teaql_runtime/tests/sql_move_only_tests.cpp)
    target_link_libraries(teaql_sql_move_only_tests PRIVATE teaql::runtime)
    add_test(NAME sql_move_only_context_contracts COMMAND teaql_sql_move_only_tests)
    add_test(NAME runtime_error_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_error_debug.py
        --cpp $<TARGET_FILE:teaql_runtime_check_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_check_probe tests/parity/runtime_check_probe.cpp)
    target_link_libraries(teaql_runtime_check_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_context_tests teaql_runtime/tests/runtime_context_tests.cpp)
    target_link_libraries(teaql_runtime_context_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_context_resource_contracts COMMAND teaql_runtime_context_tests)
    add_executable(teaql_context_model_tests teaql_runtime/tests/context_model_tests.cpp)
    target_link_libraries(teaql_context_model_tests PRIVATE teaql::runtime)
    add_test(NAME context_model_contracts COMMAND teaql_context_model_tests)
    add_executable(teaql_context_model_probe tests/parity/context_model_probe.cpp)
    target_link_libraries(teaql_context_model_probe PRIVATE teaql::runtime)
    add_test(NAME context_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_context_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_context_probe tests/parity/runtime_context_probe.cpp)
    target_link_libraries(teaql_runtime_context_probe PRIVATE teaql::runtime)
    add_executable(teaql_provider_sqlite_business_id_tests teaql_provider_sqlite/tests/provider_sqlite_business_id_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_business_id_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME provider_sqlite_business_id_contracts COMMAND teaql_provider_sqlite_business_id_tests)
    set_tests_properties(provider_sqlite_business_id_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_provider_sqlite_id_tests teaql_provider_sqlite/tests/provider_sqlite_id_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_id_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME provider_sqlite_id_contracts COMMAND teaql_provider_sqlite_id_tests)
    set_tests_properties(provider_sqlite_id_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sqlite_transport_probe tests/parity/sqlite_transport_probe.cpp)
    add_executable(teaql_sqlite_async_lease_probe tests/parity/sqlite_async_lease_probe.cpp)
    add_executable(teaql_sqlite_async_schedule_probe tests/parity/sqlite_async_schedule_probe.cpp)
    target_link_libraries(teaql_sqlite_async_schedule_probe PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME sqlite_async_schedule_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_async_schedule_5_0_6.py
        --cpp $<TARGET_FILE:teaql_sqlite_async_schedule_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_sqlite_async_lease_probe PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME sqlite_async_lease_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_async_lease_5_0_6.py
        --cpp $<TARGET_FILE:teaql_sqlite_async_lease_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_sqlite_transport_probe PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_executable(teaql_provider_sqlite_transport_tests teaql_provider_sqlite/tests/provider_sqlite_transport_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_transport_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME provider_sqlite_transport_contracts COMMAND teaql_provider_sqlite_transport_tests)
    set_tests_properties(provider_sqlite_transport_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_provider_sqlite_decoding_tests teaql_provider_sqlite/tests/provider_sqlite_decoding_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_decoding_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME provider_sqlite_decoding_contracts COMMAND teaql_provider_sqlite_decoding_tests)
    add_executable(teaql_sqlite_decoding_probe tests/parity/sqlite_decoding_probe.cpp)
    target_link_libraries(teaql_sqlite_decoding_probe PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_executable(teaql_sqlite_bindings_probe tests/parity/sqlite_bindings_probe.cpp)
    target_link_libraries(teaql_sqlite_bindings_probe PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_executable(teaql_provider_sqlite_bindings_tests teaql_provider_sqlite/tests/provider_sqlite_bindings_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_bindings_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME provider_sqlite_binding_contracts COMMAND teaql_provider_sqlite_bindings_tests)
    add_executable(teaql_provider_sqlite_model_tests teaql_provider_sqlite/tests/provider_sqlite_model_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_model_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_model_contracts COMMAND teaql_provider_sqlite_model_tests)
    add_executable(teaql_provider_sqlite_error_tests teaql_provider_sqlite/tests/provider_sqlite_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_error_contracts COMMAND teaql_provider_sqlite_error_tests)
    add_executable(teaql_provider_sqlite_driver_error_tests teaql_provider_sqlite/tests/provider_sqlite_driver_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_driver_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_driver_error_contracts COMMAND teaql_provider_sqlite_driver_error_tests)
    add_executable(teaql_sqlite_error_model_probe tests/parity/sqlite_error_model_probe.cpp)
    target_link_libraries(teaql_sqlite_error_model_probe PRIVATE teaql::provider_sqlite)
    add_executable(teaql_sqlite_native_diagnostics_probe tests/parity/sqlite_native_diagnostics_probe.cpp)
    target_link_libraries(teaql_sqlite_native_diagnostics_probe PRIVATE teaql::provider_sqlite)
    add_executable(teaql_provider_sqlite_simple_error_tests teaql_provider_sqlite/tests/provider_sqlite_simple_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_simple_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_simple_error_contracts COMMAND teaql_provider_sqlite_simple_error_tests)
    add_executable(teaql_provider_sqlite_boxed_error_tests teaql_provider_sqlite/tests/provider_sqlite_boxed_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_boxed_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_boxed_error_contracts COMMAND teaql_provider_sqlite_boxed_error_tests)
    add_executable(teaql_provider_sqlite_from_sql_error_tests teaql_provider_sqlite/tests/provider_sqlite_from_sql_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_from_sql_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_from_sql_error_contracts COMMAND teaql_provider_sqlite_from_sql_error_tests)
    add_executable(teaql_sqlite_from_sql_error_probe tests/parity/sqlite_from_sql_error_probe.cpp)
    target_link_libraries(teaql_sqlite_from_sql_error_probe PRIVATE teaql::provider_sqlite)
    add_executable(teaql_sqlite_simple_error_probe tests/parity/sqlite_simple_error_probe.cpp)
    target_link_libraries(teaql_sqlite_simple_error_probe PRIVATE teaql::provider_sqlite)
    add_executable(teaql_sql_stream_probe tests/parity/sql_stream_probe.cpp)
    target_link_libraries(teaql_sql_stream_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_sql_stream_tests teaql_provider_sqlite/tests/sql_stream_tests.cpp)
    target_link_libraries(teaql_sql_stream_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME sql_stream_execution_contracts COMMAND teaql_sql_stream_tests)
    set_tests_properties(sql_stream_execution_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sql_transaction_tests teaql_provider_sqlite/tests/sql_transaction_tests.cpp)
    target_link_libraries(teaql_sql_transaction_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME sql_transaction_execution_contracts COMMAND teaql_sql_transaction_tests)
    set_tests_properties(sql_transaction_execution_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sql_execution_tests teaql_provider_sqlite/tests/sql_execution_tests.cpp)
    target_link_libraries(teaql_sql_execution_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME sql_mutation_execution_contracts COMMAND teaql_sql_execution_tests)
    set_tests_properties(sql_mutation_execution_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sql_execution_probe tests/parity/sql_execution_probe.cpp)
    target_link_libraries(teaql_sql_execution_probe PRIVATE teaql::sql teaql::runtime)
    add_test(NAME sql_execution_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_execution_5_0_6.py
        --cpp $<TARGET_FILE:teaql_sql_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_sql_query_probe tests/parity/sql_query_probe.cpp)
    target_link_libraries(teaql_sql_query_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_sql_query_tests teaql_sql/tests/sql_query_tests.cpp)
    target_link_libraries(teaql_sql_query_tests PRIVATE teaql::sql teaql::runtime)
    add_test(NAME sql_query_executor_contracts COMMAND teaql_sql_query_tests)
    set_tests_properties(sql_query_executor_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sql_transport_probe tests/parity/sql_transport_probe.cpp)
    target_link_libraries(teaql_sql_transport_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_sql_executor_error_probe tests/parity/sql_executor_error_probe.cpp)
    target_link_libraries(teaql_sql_executor_error_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_sql_transport_tests teaql_sql/tests/sql_transport_tests.cpp)
    target_link_libraries(teaql_sql_transport_tests PRIVATE teaql::sql teaql::runtime)
    add_test(NAME sql_transport_contracts COMMAND teaql_sql_transport_tests)
    set_tests_properties(sql_transport_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_provider_sqlite_tests teaql_provider_sqlite/tests/provider_sqlite_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_dialect_contracts COMMAND teaql_provider_sqlite_tests)
    add_executable(teaql_sql_schema_probe tests/parity/sql_schema_probe.cpp)
    target_link_libraries(teaql_sql_schema_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_sql_mutation_tests teaql_provider_sqlite/tests/sql_mutation_tests.cpp)
    target_link_libraries(teaql_sql_mutation_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME sql_mutation_contracts COMMAND teaql_sql_mutation_tests)
    add_executable(teaql_sql_mutation_probe tests/parity/sql_mutation_probe.cpp)
    target_link_libraries(teaql_sql_mutation_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_sql_dialect_tests teaql_sql/tests/sql_dialect_tests.cpp)
    target_link_libraries(teaql_sql_dialect_tests PRIVATE teaql::sql teaql::runtime)
    add_test(NAME sql_dialect_contracts COMMAND teaql_sql_dialect_tests)
    add_executable(teaql_sql_dialect_helper_probe tests/parity/sql_dialect_helper_probe.cpp)
    target_link_libraries(teaql_sql_dialect_helper_probe PRIVATE teaql::sql teaql::runtime)
    add_test(NAME sql_dialect_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_dialect_helpers.py
        --cpp $<TARGET_FILE:teaql_sql_dialect_helper_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_dialect_in_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_dialect_in.py
        --cpp $<TARGET_FILE:teaql_sql_dialect_helper_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_sql_dialect_probe tests/parity/sql_dialect_probe.cpp)
    target_link_libraries(teaql_sql_dialect_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_sql_model_tests teaql_provider_sqlite/tests/sql_model_tests.cpp)
    target_link_libraries(teaql_sql_model_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME sql_model_contracts COMMAND teaql_sql_model_tests)
    add_executable(teaql_sql_model_api_tests teaql_sql/tests/sql_model_api_tests.cpp)
    target_link_libraries(teaql_sql_model_api_tests PRIVATE teaql::sql teaql::runtime)
    add_test(NAME sql_model_api_contracts COMMAND teaql_sql_model_api_tests)
    add_executable(teaql_sql_model_api_probe tests/parity/sql_model_api_probe.cpp)
    target_link_libraries(teaql_sql_model_api_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_sql_types_probe tests/parity/sql_types_probe.cpp)
    target_link_libraries(teaql_sql_types_probe PRIVATE teaql::sql teaql::runtime)
    add_executable(teaql_async_tests teaql_async/tests/async_tests.cpp)
    target_link_libraries(teaql_async_tests PRIVATE teaql::async teaql::runtime Threads::Threads)
    add_test(NAME async_context_contracts COMMAND teaql_async_tests)
    set_tests_properties(async_context_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_executor_tests teaql_data_service/tests/executor_tests.cpp)
    target_link_libraries(teaql_executor_tests PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME executor_spi_contracts COMMAND teaql_executor_tests)
    add_executable(teaql_executor_defaults_scope_probe tests/parity/executor_defaults_scope_probe.cpp)
    target_link_libraries(teaql_executor_defaults_scope_probe PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME executor_defaults_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_executor_defaults_scope.py
        --cpp $<TARGET_FILE:teaql_executor_defaults_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    set_tests_properties(executor_spi_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_data_service_tests teaql_data_service/tests/data_service_tests.cpp)
    target_link_libraries(teaql_data_service_tests PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME data_service_contracts COMMAND teaql_data_service_tests)
    add_executable(teaql_data_service_model_tests teaql_data_service/tests/data_service_model_tests.cpp)
    target_link_libraries(teaql_data_service_model_tests PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME data_service_model_contracts COMMAND teaql_data_service_model_tests)
    add_executable(teaql_system_time_tests teaql_data_service/tests/system_time_tests.cpp)
    target_link_libraries(teaql_system_time_tests PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME system_time_contracts COMMAND teaql_system_time_tests)
    add_executable(teaql_system_time_arithmetic_scope_tests teaql_data_service/tests/system_time_arithmetic_scope_tests.cpp)
    target_link_libraries(teaql_system_time_arithmetic_scope_tests PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME system_time_arithmetic_scope_contracts COMMAND teaql_system_time_arithmetic_scope_tests)
    add_executable(teaql_system_time_arithmetic_scope_probe tests/parity/system_time_arithmetic_scope_probe.cpp)
    target_link_libraries(teaql_system_time_arithmetic_scope_probe PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME system_time_arithmetic_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_system_time_arithmetic_scope.py
        --cpp $<TARGET_FILE:teaql_system_time_arithmetic_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_system_time_probe tests/parity/system_time_probe.cpp)
    add_executable(teaql_system_time_error_scope_probe tests/parity/system_time_error_scope_probe.cpp)
    target_link_libraries(teaql_system_time_error_scope_probe PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME system_time_error_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_system_time_error_scope_5_0_6.py
        --cpp $<TARGET_FILE:teaql_system_time_error_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_system_time_probe PRIVATE teaql::data_service teaql::runtime)
    add_executable(teaql_data_service_scope_tests teaql_data_service/tests/data_service_scope_tests.cpp)
    target_link_libraries(teaql_data_service_scope_tests PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME data_service_scope_contracts COMMAND teaql_data_service_scope_tests)
    add_executable(teaql_data_service_model_scope_probe tests/parity/data_service_model_scope_probe.cpp)
    target_link_libraries(teaql_data_service_model_scope_probe PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME data_service_model_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_data_service_model_scope.py
        --cpp $<TARGET_FILE:teaql_data_service_model_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_data_service_model_probe tests/parity/data_service_model_probe.cpp)
    target_link_libraries(teaql_data_service_model_probe PRIVATE teaql::data_service teaql::runtime)
    add_executable(teaql_sql_log_probe tests/parity/sql_log_probe.cpp)
    target_link_libraries(teaql_sql_log_probe PRIVATE teaql::data_service teaql::runtime)
    add_executable(teaql_sql_projection_scope_tests teaql_data_service/tests/sql_projection_scope_tests.cpp)
    target_link_libraries(teaql_sql_projection_scope_tests PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME sql_projection_scope_contracts COMMAND teaql_sql_projection_scope_tests)
    add_executable(teaql_sql_projection_scope_probe tests/parity/sql_projection_scope_probe.cpp)
    target_link_libraries(teaql_sql_projection_scope_probe PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME sql_projection_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_projection_scope.py
        --cpp $<TARGET_FILE:teaql_sql_projection_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_decimal_tests teaql_core/tests/decimal_tests.cpp)
    target_link_libraries(teaql_decimal_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME decimal_contracts COMMAND teaql_decimal_tests)
    add_executable(teaql_decimal_inline_tests teaql_core/tests/decimal_inline_tests.cpp)
    target_link_libraries(teaql_decimal_inline_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME decimal_inline_contracts COMMAND teaql_decimal_inline_tests)
    add_executable(teaql_core_aux_debug_tests teaql_core/tests/core_aux_debug_tests.cpp)
    target_link_libraries(teaql_core_aux_debug_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME core_aux_debug_contracts COMMAND teaql_core_aux_debug_tests)
    add_executable(teaql_core_aux_debug_probe tests/parity/core_aux_debug_probe.cpp)
    target_link_libraries(teaql_core_aux_debug_probe PRIVATE teaql::core teaql::runtime)
    add_executable(teaql_decimal_inline_probe tests/parity/decimal_inline_probe.cpp)
    target_link_libraries(teaql_decimal_inline_probe PRIVATE teaql::core teaql::runtime)
    add_executable(teaql_decimal_probe tests/parity/decimal_probe.cpp)
    target_link_libraries(teaql_decimal_probe PRIVATE teaql::core teaql::runtime)
    add_executable(teaql_business_id_tests teaql_core/tests/business_id_tests.cpp)
    target_link_libraries(teaql_business_id_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME business_id_contracts COMMAND teaql_business_id_tests)
    add_executable(teaql_business_id_probe tests/parity/business_id_probe.cpp)
    target_link_libraries(teaql_business_id_probe PRIVATE teaql::core teaql::runtime)
    add_executable(teaql_presentation_tests teaql_core/tests/presentation_tests.cpp)
    target_link_libraries(teaql_presentation_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME presentation_contracts COMMAND teaql_presentation_tests)
    add_executable(teaql_presentation_probe tests/parity/presentation_probe.cpp)
    target_link_libraries(teaql_presentation_probe PRIVATE teaql::core teaql::runtime)
    add_executable(teaql_request_search_tests teaql_core/tests/request_search_tests.cpp)
    target_link_libraries(teaql_request_search_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME request_search_contracts COMMAND teaql_request_search_tests)
    add_executable(teaql_original_core_search_tests tests/parity/teaql_core/dynamic_search.cpp)
    add_executable(teaql_original_core_functional_tests tests/parity/teaql_core/functional_contracts.cpp)
    add_executable(teaql_original_core_mapping_tests tests/parity/teaql_core/entity_mapping.cpp)
    add_executable(teaql_original_core_composition_tests tests/parity/teaql_core/composition_contracts.cpp)
    add_executable(teaql_original_core_value_tests tests/parity/teaql_core/value_contracts.cpp)
    target_link_libraries(teaql_original_core_value_tests PRIVATE teaql::core)
    add_test(NAME original_core_value_contracts COMMAND teaql_original_core_value_tests)
    target_link_libraries(teaql_original_core_composition_tests PRIVATE teaql::runtime)
    add_test(NAME original_core_composition_contracts COMMAND teaql_original_core_composition_tests)
    target_link_libraries(teaql_original_core_mapping_tests PRIVATE teaql::runtime)
    add_test(NAME original_core_entity_mapping_contracts COMMAND teaql_original_core_mapping_tests)
    target_link_libraries(teaql_original_core_functional_tests PRIVATE teaql::runtime)
    add_test(NAME original_core_functional_contracts COMMAND teaql_original_core_functional_tests)
    target_link_libraries(teaql_original_core_search_tests PRIVATE teaql::core)
    add_test(NAME original_core_dynamic_search_contracts COMMAND teaql_original_core_search_tests)
    add_test(NAME original_core_dynamic_search_warning_privacy COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/check_search_warning_canary.py
        --cpp $<TARGET_FILE:teaql_original_core_search_tests>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_entity_registration_api_tests teaql_core/tests/entity_registration_api_tests.cpp)
    target_link_libraries(teaql_entity_registration_api_tests PRIVATE teaql::runtime)
    add_test(NAME entity_registration_api_contracts COMMAND teaql_entity_registration_api_tests)
    add_executable(teaql_entity_registration_api_probe tests/parity/entity_registration_api_probe.cpp)
    target_link_libraries(teaql_entity_registration_api_probe PRIVATE teaql::runtime)
    add_test(NAME entity_registration_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_registration_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_entity_registration_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_base_entity_api_tests teaql_core/tests/base_entity_api_tests.cpp)
    target_link_libraries(teaql_base_entity_api_tests PRIVATE teaql::runtime)
    add_test(NAME base_entity_api_contracts COMMAND teaql_base_entity_api_tests)
    add_executable(teaql_base_entity_api_probe tests/parity/base_entity_api_probe.cpp)
    target_link_libraries(teaql_base_entity_api_probe PRIVATE teaql::runtime)
    add_test(NAME base_entity_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_base_entity_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_base_entity_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_entity_debug_tests teaql_core/tests/entity_debug_tests.cpp)
    target_link_libraries(teaql_entity_debug_tests PRIVATE teaql::runtime)
    add_test(NAME entity_debug_contracts COMMAND teaql_entity_debug_tests)
    add_executable(teaql_entity_debug_probe tests/parity/entity_debug_probe.cpp)
    target_link_libraries(teaql_entity_debug_probe PRIVATE teaql::runtime)
    add_test(NAME entity_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_entity_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_entity_owned_api_tests teaql_core/tests/entity_owned_api_tests.cpp)
    target_link_libraries(teaql_entity_owned_api_tests PRIVATE teaql::runtime)
    add_test(NAME entity_owned_api_contracts COMMAND teaql_entity_owned_api_tests)
    add_executable(teaql_entity_defaults_api_probe tests/parity/entity_defaults_api_probe.cpp)
    target_link_libraries(teaql_entity_defaults_api_probe PRIVATE teaql::runtime)
    add_test(NAME entity_defaults_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_defaults_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_entity_defaults_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_chrono_value_tests teaql_core/tests/chrono_value_tests.cpp)
    target_link_libraries(teaql_chrono_value_tests PRIVATE teaql::runtime)
    add_test(NAME chrono_value_contracts COMMAND teaql_chrono_value_tests)
    add_executable(teaql_chrono_value_probe tests/parity/chrono_value_probe.cpp)
    target_link_libraries(teaql_chrono_value_probe PRIVATE teaql::runtime)
    add_test(NAME chrono_value_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_chrono_value.py
        --cpp $<TARGET_FILE:teaql_chrono_value_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_field_layout_probe tests/parity/field_layout_probe.cpp)
    target_link_libraries(teaql_field_layout_probe PRIVATE teaql::core teaql::runtime)
    add_test(NAME field_layout_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_field_layout.py
        --cpp $<TARGET_FILE:teaql_field_layout_probe>)
    add_executable(teaql_loaded_snapshot_probe tests/parity/loaded_snapshot_probe.cpp)
    target_link_libraries(teaql_loaded_snapshot_probe PRIVATE teaql::provider_sqlite)
    add_test(NAME loaded_snapshot_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_loaded_snapshot.py
        --cpp $<TARGET_FILE:teaql_loaded_snapshot_probe>)
    add_executable(teaql_loaded_snapshot_tests teaql_core/tests/loaded_snapshot_tests.cpp)
    target_link_libraries(teaql_loaded_snapshot_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME loaded_snapshot_contracts COMMAND teaql_loaded_snapshot_tests)
    add_executable(teaql_dynamic_fields_tests teaql_core/tests/dynamic_fields_tests.cpp)
    target_link_libraries(teaql_dynamic_fields_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME dynamic_fields_contracts COMMAND teaql_dynamic_fields_tests)
    add_executable(teaql_dynamic_carrier_tests teaql_core/tests/dynamic_carrier_tests.cpp)
    target_link_libraries(teaql_dynamic_carrier_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME dynamic_carrier_contracts COMMAND teaql_dynamic_carrier_tests)
    add_executable(teaql_commit_audit_tests teaql_runtime/tests/commit_audit_tests.cpp)
    target_link_libraries(teaql_commit_audit_tests PRIVATE teaql::runtime)
    add_test(NAME commit_audit_contracts COMMAND teaql_commit_audit_tests)
    add_executable(teaql_commit_audit_transaction_tests teaql_provider_sqlite/tests/commit_audit_transaction_tests.cpp)
    target_link_libraries(teaql_commit_audit_transaction_tests PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME commit_audit_transaction_contracts COMMAND teaql_commit_audit_transaction_tests)
    add_executable(teaql_commit_audit_error_probe tests/parity/commit_audit_error_probe.cpp)
    target_link_libraries(teaql_commit_audit_error_probe PRIVATE teaql::runtime)
    add_test(NAME commit_audit_error_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_commit_audit_error.py
        --cpp $<TARGET_FILE:teaql_commit_audit_error_probe>)
    add_executable(teaql_dynamic_relation_tests teaql_runtime/tests/dynamic_relation_tests.cpp)
    target_link_libraries(teaql_dynamic_relation_tests PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME dynamic_relation_contracts COMMAND teaql_dynamic_relation_tests)
    add_test(NAME dynamic_relation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_relation.py
        --cpp $<TARGET_FILE:teaql_dynamic_query_probe>)
    add_executable(teaql_dynamic_query_probe tests/parity/dynamic_query_probe.cpp)
    target_link_libraries(teaql_dynamic_query_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME dynamic_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_query.py
        --cpp $<TARGET_FILE:teaql_dynamic_query_probe>)
    add_executable(teaql_dynamic_query_tests teaql_runtime/tests/dynamic_query_tests.cpp)
    target_link_libraries(teaql_dynamic_query_tests PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME dynamic_query_contracts COMMAND teaql_dynamic_query_tests)
    add_executable(teaql_dynamic_provider_probe tests/parity/dynamic_provider_probe.cpp)
    target_link_libraries(teaql_dynamic_provider_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME dynamic_provider_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_provider.py
        --cpp $<TARGET_FILE:teaql_dynamic_provider_probe>)
    add_executable(teaql_dynamic_provider_tests teaql_runtime/tests/dynamic_provider_tests.cpp)
    target_link_libraries(teaql_dynamic_provider_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME dynamic_provider_contracts COMMAND teaql_dynamic_provider_tests)
    add_executable(teaql_dynamic_ledger_tests teaql_runtime/tests/dynamic_ledger_tests.cpp)
    target_link_libraries(teaql_dynamic_ledger_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME dynamic_ledger_contracts COMMAND teaql_dynamic_ledger_tests)
    add_executable(teaql_dynamic_graph_save_tests teaql_runtime/tests/dynamic_graph_save_tests.cpp)
    target_link_libraries(teaql_dynamic_graph_save_tests PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME dynamic_graph_save_contracts COMMAND teaql_dynamic_graph_save_tests)
    add_executable(teaql_dynamic_preflight_tests teaql_runtime/tests/dynamic_preflight_tests.cpp)
    target_link_libraries(teaql_dynamic_preflight_tests PRIVATE teaql::runtime)
    add_test(NAME dynamic_preflight_contracts COMMAND teaql_dynamic_preflight_tests)
    add_executable(teaql_dynamic_governance_probe tests/parity/dynamic_governance_probe.cpp)
    target_link_libraries(teaql_dynamic_governance_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME dynamic_governance_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_governance.py
        --cpp $<TARGET_FILE:teaql_dynamic_governance_probe>)
    add_executable(teaql_dynamic_graph_debug_probe tests/parity/dynamic_graph_debug_probe.cpp)
    target_link_libraries(teaql_dynamic_graph_debug_probe PRIVATE teaql::runtime)
    add_test(NAME dynamic_graph_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_graph_debug.py
        --cpp $<TARGET_FILE:teaql_dynamic_graph_debug_probe>)
    add_executable(teaql_dynamic_storage_tests teaql_data_service/tests/dynamic_storage_tests.cpp)
    target_link_libraries(teaql_dynamic_storage_tests PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME dynamic_storage_contracts COMMAND teaql_dynamic_storage_tests)
    add_executable(teaql_sqlite_dynamic_storage_tests teaql_provider_sqlite/tests/dynamic_storage_tests.cpp)
    target_link_libraries(teaql_sqlite_dynamic_storage_tests PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME sqlite_dynamic_storage_contracts COMMAND teaql_sqlite_dynamic_storage_tests)
    add_executable(teaql_sqlite_dynamic_storage_probe tests/parity/sqlite_dynamic_storage_probe.cpp)
    target_link_libraries(teaql_sqlite_dynamic_storage_probe PRIVATE teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME sqlite_dynamic_storage_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_dynamic_storage.py
        --cpp $<TARGET_FILE:teaql_sqlite_dynamic_storage_probe>)
    add_executable(teaql_dynamic_storage_probe tests/parity/dynamic_storage_probe.cpp)
    target_link_libraries(teaql_dynamic_storage_probe PRIVATE teaql::data_service teaql::runtime)
    add_test(NAME dynamic_storage_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_storage.py
        --cpp $<TARGET_FILE:teaql_dynamic_storage_probe>)
    add_executable(teaql_dynamic_ledger_probe tests/parity/dynamic_ledger_probe.cpp)
    target_link_libraries(teaql_dynamic_ledger_probe PRIVATE teaql::core teaql::runtime)
    add_test(NAME dynamic_ledger_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_ledger.py
        --cpp $<TARGET_FILE:teaql_dynamic_ledger_probe>)
    add_executable(teaql_dynamic_carrier_probe tests/parity/dynamic_carrier_probe.cpp)
    target_link_libraries(teaql_dynamic_carrier_probe PRIVATE teaql::core teaql::runtime)
    add_test(NAME dynamic_carrier_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_carrier.py
        --cpp $<TARGET_FILE:teaql_dynamic_carrier_probe>)
    add_executable(teaql_dynamic_fields_probe tests/parity/dynamic_fields_probe.cpp)
    target_link_libraries(teaql_dynamic_fields_probe PRIVATE teaql::core teaql::runtime)
    add_test(NAME dynamic_fields_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_dynamic_fields.py
        --cpp $<TARGET_FILE:teaql_dynamic_fields_probe>)
    add_test(NAME original_snapshot_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_original_snapshot.py
        --cpp $<TARGET_FILE:teaql_loaded_snapshot_probe>)
    add_test(NAME checker_provenance_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_checker_helpers.py
        --cpp $<TARGET_FILE:teaql_loaded_snapshot_probe>)
    add_executable(teaql_generated_entity_helpers_probe tests/parity/generated_entity_helpers_probe.cpp)
    target_link_libraries(teaql_generated_entity_helpers_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME generated_entity_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_entity_helpers.py
        --cpp $<TARGET_FILE:teaql_generated_entity_helpers_probe>)
    add_test(NAME generated_optional_fields_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_optional_fields.py
        --cpp $<TARGET_FILE:teaql_generated_entity_helpers_probe>)
    add_executable(teaql_generated_checker_flow_probe tests/parity/generated_checker_flow_probe.cpp)
    add_executable(teaql_generated_candidates_probe tests/parity/generated_candidates_probe.cpp)
    add_executable(teaql_generated_constant_variants_probe tests/parity/generated_constant_variants_probe.cpp)
    add_executable(teaql_generated_constant_scalar_tests tests/generator/generated_constant_scalar_tests.cpp)
    target_link_libraries(teaql_generated_constant_scalar_tests PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_constant_scalar_contracts COMMAND teaql_generated_constant_scalar_tests)
    set_tests_properties(generated_constant_scalar_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    target_link_libraries(teaql_generated_constant_variants_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_constant_variants_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_constant_variants.py
        --cpp $<TARGET_FILE:teaql_generated_constant_variants_probe>)
    set_tests_properties(generated_constant_variants_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_bootstrap_probe tests/parity/generated_bootstrap_probe.cpp)
    target_link_libraries(teaql_generated_bootstrap_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_bootstrap_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_bootstrap.py
        --cpp $<TARGET_FILE:teaql_generated_bootstrap_probe>)
    set_tests_properties(generated_bootstrap_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    target_link_libraries(teaql_generated_candidates_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_candidates_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_candidates.py
        --cpp $<TARGET_FILE:teaql_generated_candidates_probe>)
    set_tests_properties(generated_candidates_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_candidates_tests tests/generator/generated_candidates_tests.cpp)
    target_link_libraries(teaql_generated_candidates_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME generated_candidates_contracts COMMAND teaql_generated_candidates_tests)
    add_executable(teaql_generated_bootstrap_model_tests tests/generator/generated_bootstrap_model_tests.cpp)
    target_link_libraries(teaql_generated_bootstrap_model_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME generated_bootstrap_model_contracts COMMAND teaql_generated_bootstrap_model_tests)
    add_executable(teaql_generated_inheritance_probe tests/parity/generated_inheritance_probe.cpp)
    target_link_libraries(teaql_generated_inheritance_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME generated_inheritance_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_inheritance.py
        --cpp $<TARGET_FILE:teaql_generated_inheritance_probe>)
    add_executable(teaql_generated_inheritance_tests tests/generator/generated_inheritance_tests.cpp)
    target_link_libraries(teaql_generated_inheritance_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_inheritance_contracts COMMAND teaql_generated_inheritance_tests)
    set_tests_properties(generated_inheritance_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_inheritance_query_probe tests/parity/generated_inheritance_query_probe.cpp)
    target_link_libraries(teaql_generated_inheritance_query_probe PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME generated_inheritance_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_inheritance_query.py
        --cpp $<TARGET_FILE:teaql_generated_inheritance_query_probe>)
    add_executable(teaql_generated_inheritance_query_tests tests/generator/generated_inheritance_query_tests.cpp)
    target_link_libraries(teaql_generated_inheritance_query_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME generated_inheritance_query_contracts COMMAND teaql_generated_inheritance_query_tests)
    add_executable(teaql_generated_return_type_tests tests/generator/generated_return_type_tests.cpp)
    target_link_libraries(teaql_generated_return_type_tests PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_return_type_contracts COMMAND teaql_generated_return_type_tests)
    set_tests_properties(generated_return_type_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_inheritance_sqlite_probe tests/parity/generated_inheritance_sqlite_probe.cpp)
    target_link_libraries(teaql_generated_inheritance_sqlite_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_inheritance_sqlite_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_inheritance_sqlite.py
        --cpp $<TARGET_FILE:teaql_generated_inheritance_sqlite_probe>)
    set_tests_properties(generated_inheritance_sqlite_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME generated_return_type_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_return_type.py
        --cpp $<TARGET_FILE:teaql_generated_inheritance_sqlite_probe>)
    set_tests_properties(generated_return_type_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_inheritance_save_probe tests/parity/generated_inheritance_save_probe.cpp)
    target_link_libraries(teaql_generated_inheritance_save_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_inheritance_save_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_inheritance_save.py
        --cpp $<TARGET_FILE:teaql_generated_inheritance_save_probe>)
    set_tests_properties(generated_inheritance_save_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_inheritance_graph_probe tests/parity/generated_inheritance_graph_probe.cpp)
    target_link_libraries(teaql_generated_inheritance_graph_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_inheritance_graph_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_inheritance_graph.py
        --cpp $<TARGET_FILE:teaql_generated_inheritance_graph_probe>)
    set_tests_properties(generated_inheritance_graph_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_inheritance_graph_tests tests/generator/generated_inheritance_graph_tests.cpp)
    target_link_libraries(teaql_generated_inheritance_graph_tests PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_inheritance_graph_contracts COMMAND teaql_generated_inheritance_graph_tests)
    set_tests_properties(generated_inheritance_graph_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_inheritance_variants_probe tests/parity/generated_inheritance_variants_probe.cpp)
    target_link_libraries(teaql_generated_inheritance_variants_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_inheritance_variants_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_inheritance_variants.py
        --cpp $<TARGET_FILE:teaql_generated_inheritance_variants_probe>)
    set_tests_properties(generated_inheritance_variants_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_inheritance_variants_tests tests/generator/generated_inheritance_variants_tests.cpp)
    target_link_libraries(teaql_generated_inheritance_variants_tests PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_inheritance_variants_contracts COMMAND teaql_generated_inheritance_variants_tests)
    set_tests_properties(generated_inheritance_variants_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_inherited_stream_probe tests/parity/generated_inherited_stream_probe.cpp)
    target_link_libraries(teaql_generated_inherited_stream_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_inherited_stream_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_inherited_stream.py
        --cpp $<TARGET_FILE:teaql_generated_inherited_stream_probe>)
    set_tests_properties(generated_inherited_stream_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    target_link_libraries(teaql_generated_checker_flow_probe PRIVATE teaql::runtime teaql::provider_sqlite SQLite3::SQLite3)
    add_test(NAME generated_checker_flow_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_checker_flow.py
        --cpp $<TARGET_FILE:teaql_generated_checker_flow_probe>)
    set_tests_properties(generated_checker_flow_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME generated_checker_commit_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_checker_commit.py
        --cpp $<TARGET_FILE:teaql_generated_checker_flow_probe>)
    set_tests_properties(generated_checker_commit_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME generated_relations_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_relations.py
        --cpp $<TARGET_FILE:teaql_loaded_snapshot_probe>)
    add_test(NAME generated_business_ids_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_business_ids.py
        --cpp $<TARGET_FILE:teaql_loaded_snapshot_probe>)
    add_test(NAME intent_trace_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_intent_trace.py
        --cpp $<TARGET_FILE:teaql_loaded_snapshot_probe>)
    add_executable(teaql_field_layout_tests teaql_core/tests/field_layout_tests.cpp)
    target_link_libraries(teaql_field_layout_tests PRIVATE teaql::core teaql::runtime)
    add_test(NAME field_layout_contracts COMMAND teaql_field_layout_tests)
    add_executable(teaql_entity_expression_tests teaql_core/tests/entity_expression_tests.cpp)
    target_link_libraries(teaql_entity_expression_tests PRIVATE teaql::core teaql::memory teaql::runtime)
    add_test(NAME entity_expression_contracts COMMAND teaql_entity_expression_tests)
    add_executable(teaql_search_probe tests/parity/search_probe.cpp)
    target_link_libraries(teaql_search_probe PRIVATE teaql::core teaql::runtime)
    add_executable(teaql_value_probe tests/parity/value_probe.cpp)
    target_link_libraries(teaql_value_probe PRIVATE teaql::core teaql::memory teaql::runtime)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME provider_sqlite_schema_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_schema.py --cpp $<TARGET_FILE:teaql_sqlite_schema_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_base_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_base_5_0_6.py --cpp $<TARGET_FILE:teaql_runtime_base_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_base_intent_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_base_intent_5_0_6.py --cpp $<TARGET_FILE:teaql_runtime_base_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME context_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_query_5_0_6.py --cpp $<TARGET_FILE:teaql_context_query_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME context_query_intent_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_query_intent_5_0_6.py --cpp $<TARGET_FILE:teaql_context_query_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME relation_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_execution_5_0_6.py --cpp $<TARGET_FILE:teaql_relation_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME query_keys_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_query_keys.py --cpp $<TARGET_FILE:teaql_query_keys_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}"
        "--pointer-bytes=${CMAKE_SIZEOF_VOID_P}")
    add_test(NAME id_set_build_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_id_set_build.py --cpp $<TARGET_FILE:teaql_id_set_build_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME query_store_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_query_store.py --cpp $<TARGET_FILE:teaql_query_store_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME relation_enhancement_variants_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_enhancement_variants_5_0_6.py
        --cpp $<TARGET_FILE:teaql_relation_enhancement_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME relation_enhancement_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_enhancement_5_0_6.py --cpp $<TARGET_FILE:teaql_relation_enhancement_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME relation_hydration_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_hydration_5_0_6.py --cpp $<TARGET_FILE:teaql_relation_hydration_replay_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME relation_planning_extensions_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_planning_extensions_5_0_6.py
        --cpp $<TARGET_FILE:teaql_relation_planning_extensions_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_service_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_service_5_0_6.py --cpp $<TARGET_FILE:teaql_entity_service_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_query_5_0_6.py --cpp $<TARGET_FILE:teaql_entity_query_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_query_capture_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_query_capture_5_0_6.py --cpp $<TARGET_FILE:teaql_entity_query_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_query_boundary_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_query_boundaries.py
        --cpp $<TARGET_FILE:teaql_entity_query_probe>
        "--profile=$<IF:$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>,release,debug>"
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_stream_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_stream_5_0_6.py --cpp $<TARGET_FILE:teaql_entity_stream_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME id_set_page_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_id_set_page_5_0_6.py --cpp $<TARGET_FILE:teaql_id_set_page_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME enhanced_entities_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_enhanced_entities_5_0_6.py --cpp $<TARGET_FILE:teaql_enhanced_entities_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_decoder_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_decoder_5_0_6.py --cpp $<TARGET_FILE:teaql_entity_decoder_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_runtime_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_runtime.py --cpp $<TARGET_FILE:teaql_entity_runtime_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_graph_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_graph_5_0_6.py --cpp $<TARGET_FILE:teaql_runtime_graph_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME context_event_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_event.py --cpp $<TARGET_FILE:teaql_context_event_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME telemetry_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_telemetry.py --cpp $<TARGET_FILE:teaql_telemetry_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME context_logging_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_logging.py --cpp $<TARGET_FILE:teaql_context_logging_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME log_manager_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_log_manager.py --cpp $<TARGET_FILE:teaql_log_manager_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME log_formatter_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_log_formatter.py --cpp ${CMAKE_CURRENT_BINARY_DIR}
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_log_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_log.py --cpp $<TARGET_FILE:teaql_runtime_log_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME audit_event_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_audit_event.py --cpp $<TARGET_FILE:teaql_audit_event_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME schema_runtime_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_schema_runtime.py --cpp $<TARGET_FILE:teaql_schema_runtime_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME internal_id_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_internal_id_5_0_6.py --cpp $<TARGET_FILE:teaql_internal_id_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME business_service_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_business_service.py --cpp $<TARGET_FILE:teaql_business_service_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_business_id_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_business_id.py --cpp $<TARGET_FILE:teaql_runtime_business_id_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_registry_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_registry.py --cpp $<TARGET_FILE:teaql_runtime_registry_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_check_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_check.py --cpp $<TARGET_FILE:teaql_runtime_check_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_context_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_context.py --cpp $<TARGET_FILE:teaql_runtime_context_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_provider_business_id_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_business_id_5_0_6.py --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_provider_id_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_id_5_0_6.py --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_provider_transaction_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_transaction_5_0_6.py
        --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
        --cpp-async $<TARGET_FILE:teaql_sqlite_async_schedule_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_sync_transaction_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_sync_transaction_5_0_6.py
        --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_provider_transport_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_transport_5_0_6.py --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_decoding_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_decoding.py --cpp $<TARGET_FILE:teaql_sqlite_decoding_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_bindings_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_bindings.py --cpp $<TARGET_FILE:teaql_sqlite_bindings_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_error_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_error_model.py --cpp $<TARGET_FILE:teaql_sqlite_error_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_native_diagnostics_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_native_diagnostics.py --cpp $<TARGET_FILE:teaql_sqlite_native_diagnostics_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_simple_error_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_simple_error.py --cpp $<TARGET_FILE:teaql_sqlite_simple_error_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_from_sql_error_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_from_sql_error.py --cpp $<TARGET_FILE:teaql_sqlite_from_sql_error_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    if(NOT CMAKE_CROSSCOMPILING AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_test(NAME sqlite_from_sql_raw_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_from_sql_raw_debug.py
            --cpp $<TARGET_FILE:teaql_sqlite_from_sql_error_probe>)
    endif()
    add_test(NAME sql_stream_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_stream.py --cpp $<TARGET_FILE:teaql_sql_stream_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_transaction_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_transaction.py --cpp $<TARGET_FILE:teaql_sql_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_execution.py --cpp $<TARGET_FILE:teaql_sql_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_executor_kind_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_executor_kind.py
        --cpp $<TARGET_FILE:teaql_sql_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_executor_error_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_executor_error_5_0_6.py
        --cpp $<TARGET_FILE:teaql_sql_executor_error_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_query.py --cpp $<TARGET_FILE:teaql_sql_query_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_transport_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_transport.py --cpp $<TARGET_FILE:teaql_sql_transport_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_schema_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_schema.py --cpp $<TARGET_FILE:teaql_sql_schema_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_mutation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_mutation.py --cpp $<TARGET_FILE:teaql_sql_mutation_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_dialect_hooks_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_dialect_hooks.py
        --cpp $<TARGET_FILE:teaql_sql_mutation_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_dialect_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_dialect.py --cpp $<TARGET_FILE:teaql_sql_dialect_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_types_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_types_5_0_6.py --cpp $<TARGET_FILE:teaql_sql_types_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_log_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_log_5_0_6.py --cpp $<TARGET_FILE:teaql_sql_log_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME system_time_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_system_time.py
        --cpp $<TARGET_FILE:teaql_system_time_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME data_service_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_data_service_model_5_0_6.py
        --cpp $<TARGET_FILE:teaql_data_service_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_model_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_model_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_sql_model_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal.py --cpp $<TARGET_FILE:teaql_decimal_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_extended_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal_extended.py
        --cpp $<TARGET_FILE:teaql_decimal_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_inline_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal_inline.py
        --cpp $<TARGET_FILE:teaql_decimal_inline_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME core_aux_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_core_aux_debug.py
        --cpp $<TARGET_FILE:teaql_core_aux_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_scale_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal_scale.py --cpp $<TARGET_FILE:teaql_decimal_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_parsing_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal_parsing.py --cpp $<TARGET_FILE:teaql_decimal_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_numeric_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal_numeric.py --cpp $<TARGET_FILE:teaql_decimal_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_pointer_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal_pointer_scope.py
        --cpp $<TARGET_FILE:teaql_decimal_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_foundation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal_foundations.py --cpp $<TARGET_FILE:teaql_decimal_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME decimal_wire_format_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_decimal_wire_format.py --cpp $<TARGET_FILE:teaql_decimal_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME business_id_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_business_id_5_0_6.py --cpp $<TARGET_FILE:teaql_business_id_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME presentation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_presentation.py --cpp $<TARGET_FILE:teaql_presentation_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME search_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_search.py --cpp $<TARGET_FILE:teaql_search_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME value_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_values.py --cpp $<TARGET_FILE:teaql_value_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME format_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_formats.py --cpp $<TARGET_FILE:teaql_value_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_value_model_tests teaql_core/tests/value_model_tests.cpp)
    target_link_libraries(teaql_value_model_tests PRIVATE teaql::core teaql::memory teaql::runtime)
    add_test(NAME value_model_contracts COMMAND teaql_value_model_tests)
    add_executable(teaql_value_owned_api_tests teaql_core/tests/value_owned_api_tests.cpp)
    target_link_libraries(teaql_value_owned_api_tests PRIVATE teaql::runtime)
    add_test(NAME value_owned_api_contracts COMMAND teaql_value_owned_api_tests)
    add_executable(teaql_value_owned_api_probe tests/parity/value_owned_api_probe.cpp)
    target_link_libraries(teaql_value_owned_api_probe PRIVATE teaql::runtime)
    add_test(NAME value_owned_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_value_owned_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_value_owned_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_core_tests teaql_core/tests/core_tests.cpp)
    add_executable(teaql_meta_api_tests teaql_core/tests/meta_api_tests.cpp)
    add_executable(teaql_query_debug_tests teaql_core/tests/query_debug_tests.cpp)
    add_executable(teaql_mutation_debug_tests teaql_core/tests/mutation_debug_tests.cpp)
    add_executable(teaql_request_api_tests teaql_core/tests/request_api_tests.cpp)
    add_executable(teaql_request_helpers_tests teaql_core/tests/request_helpers_tests.cpp)
    add_executable(teaql_core_graph_api_tests teaql_core/tests/core_graph_api_tests.cpp)
    add_executable(teaql_presentation_api_tests teaql_core/tests/presentation_api_tests.cpp)
    target_link_libraries(teaql_presentation_api_tests PRIVATE teaql::runtime)
    add_test(NAME presentation_api_contracts COMMAND teaql_presentation_api_tests)
    add_executable(teaql_presentation_api_probe tests/parity/presentation_api_probe.cpp)
    target_link_libraries(teaql_presentation_api_probe PRIVATE teaql::runtime)
    add_test(NAME presentation_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_presentation_api.py
        --cpp $<TARGET_FILE:teaql_presentation_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_core_graph_api_tests PRIVATE teaql::runtime)
    add_test(NAME core_graph_api_contracts COMMAND teaql_core_graph_api_tests)
    add_executable(teaql_core_graph_api_probe tests/parity/core_graph_api_probe.cpp)
    target_link_libraries(teaql_core_graph_api_probe PRIVATE teaql::runtime)
    add_test(NAME core_graph_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_core_graph_api.py
        --cpp $<TARGET_FILE:teaql_core_graph_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_request_helpers_tests PRIVATE teaql::runtime)
    add_test(NAME request_helpers_contracts COMMAND teaql_request_helpers_tests)
    add_executable(teaql_request_helpers_probe tests/parity/request_helpers_probe.cpp)
    target_link_libraries(teaql_request_helpers_probe PRIVATE teaql::runtime)
    add_test(NAME request_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_request_helpers_5_0_6.py
        --cpp $<TARGET_FILE:teaql_request_helpers_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_request_api_tests PRIVATE teaql::runtime)
    add_test(NAME request_api_contracts COMMAND teaql_request_api_tests)
    add_executable(teaql_request_api_probe tests/parity/request_api_probe.cpp)
    target_link_libraries(teaql_request_api_probe PRIVATE teaql::runtime)
    add_test(NAME request_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_request_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_request_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_mutation_debug_tests PRIVATE teaql::runtime)
    add_test(NAME mutation_debug_contracts COMMAND teaql_mutation_debug_tests)
    add_executable(teaql_mutation_debug_probe tests/parity/mutation_debug_probe.cpp)
    target_link_libraries(teaql_mutation_debug_probe PRIVATE teaql::runtime)
    add_test(NAME mutation_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_mutation_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_mutation_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_query_debug_tests PRIVATE teaql::runtime)
    add_test(NAME query_debug_contracts COMMAND teaql_query_debug_tests)
    add_executable(teaql_query_debug_probe tests/parity/query_debug_probe.cpp)
    target_link_libraries(teaql_query_debug_probe PRIVATE teaql::runtime)
    add_test(NAME query_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_query_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_query_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_meta_api_tests PRIVATE teaql::runtime)
    add_test(NAME meta_api_contracts COMMAND teaql_meta_api_tests)
    add_executable(teaql_meta_api_probe tests/parity/meta_api_probe.cpp)
    target_link_libraries(teaql_meta_api_probe PRIVATE teaql::runtime)
    add_test(NAME meta_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_meta_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_meta_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_timestamp_api_tests teaql_core/tests/timestamp_api_tests.cpp)
    target_link_libraries(teaql_timestamp_api_tests PRIVATE teaql::runtime)
    add_test(NAME timestamp_api_contracts COMMAND teaql_timestamp_api_tests)
    add_executable(teaql_timestamp_api_probe tests/parity/timestamp_api_probe.cpp)
    target_link_libraries(teaql_timestamp_api_probe PRIVATE teaql::runtime)
    add_test(NAME timestamp_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_timestamp_api.py
        --cpp $<TARGET_FILE:teaql_timestamp_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_value_api_probe tests/parity/value_api_probe.cpp)
    target_link_libraries(teaql_value_api_probe PRIVATE teaql::runtime)
    add_test(NAME value_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_value_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_value_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_core_tests PRIVATE teaql::core teaql::memory teaql::runtime)
    add_test(NAME core_contracts COMMAND teaql_core_tests)
    add_executable(teaql_core_model_tests teaql_core/tests/core_model_tests.cpp)
    add_executable(teaql_eval_tests teaql_core/tests/eval_tests.cpp)
    target_link_libraries(teaql_eval_tests PRIVATE teaql::runtime)
    add_test(NAME eval_context_contracts COMMAND teaql_eval_tests)
    add_executable(teaql_eval_probe tests/parity/eval_probe.cpp)
    target_link_libraries(teaql_eval_probe PRIVATE teaql::runtime)
    add_test(NAME eval_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_eval_5_0_6.py
        --cpp $<TARGET_FILE:teaql_eval_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_mutation_model_api_tests teaql_core/tests/mutation_model_api_tests.cpp)
    add_executable(teaql_mutation_model_api_probe tests/parity/mutation_model_api_probe.cpp)
    target_link_libraries(teaql_mutation_model_api_probe PRIVATE teaql::runtime)
    add_test(NAME mutation_model_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_mutation_model_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_mutation_model_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_mutation_model_api_tests PRIVATE teaql::runtime)
    add_test(NAME mutation_model_api_contracts COMMAND teaql_mutation_model_api_tests)
    add_executable(teaql_core_api_tests teaql_core/tests/core_api_tests.cpp)
    add_executable(teaql_core_api_probe tests/parity/core_api_probe.cpp)
    target_link_libraries(teaql_core_api_probe PRIVATE teaql::runtime)
    add_test(NAME core_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_core_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_core_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_core_api_tests PRIVATE teaql::runtime)
    add_test(NAME core_api_contracts COMMAND teaql_core_api_tests)
    target_link_libraries(teaql_core_model_tests PRIVATE teaql::core teaql::memory teaql::runtime)
    add_test(NAME core_model_contracts COMMAND teaql_core_model_tests)
    add_executable(teaql_core_collection_tests teaql_core/tests/core_collection_tests.cpp)
    add_executable(teaql_smart_list_api_tests teaql_core/tests/smart_list_api_tests.cpp)
    add_executable(teaql_model_debug_tests teaql_core/tests/model_debug_tests.cpp)
    add_executable(teaql_datatype_debug_tests teaql_core/tests/datatype_debug_tests.cpp)
    target_link_libraries(teaql_datatype_debug_tests PRIVATE teaql::runtime)
    add_test(NAME datatype_debug_contracts COMMAND teaql_datatype_debug_tests)
    add_executable(teaql_datatype_debug_probe tests/parity/datatype_debug_probe.cpp)
    target_link_libraries(teaql_datatype_debug_probe PRIVATE teaql::runtime)
    add_test(NAME datatype_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_datatype_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_datatype_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_model_debug_tests PRIVATE teaql::runtime)
    add_test(NAME model_debug_contracts COMMAND teaql_model_debug_tests)
    add_executable(teaql_model_debug_probe tests/parity/model_debug_probe.cpp)
    add_executable(teaql_readonly_schema_hash_probe tests/parity/readonly_schema_hash_probe.cpp)
    target_link_libraries(teaql_readonly_schema_hash_probe PRIVATE teaql::runtime)
    add_test(NAME readonly_schema_hash_5_0_6_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_readonly_schema_hash_5_0_6.py
        --cpp $<TARGET_FILE:teaql_readonly_schema_hash_probe>)
    target_link_libraries(teaql_model_debug_probe PRIVATE teaql::runtime)
    add_test(NAME model_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_model_debug_5_0_6.py
        --cpp $<TARGET_FILE:teaql_model_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_smart_list_api_probe tests/parity/smart_list_api_probe.cpp)
    target_link_libraries(teaql_smart_list_api_probe PRIVATE teaql::runtime)
    add_test(NAME smart_list_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_smart_list_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_smart_list_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_smart_list_api_tests PRIVATE teaql::runtime)
    add_test(NAME smart_list_api_contracts COMMAND teaql_smart_list_api_tests)
    add_executable(teaql_core_shape_presentation_probe tests/parity/core_shape_presentation_probe.cpp)
    add_executable(teaql_core_shape_presentation_tests teaql_core/tests/shape_presentation_tests.cpp)
    target_link_libraries(teaql_core_shape_presentation_tests PRIVATE teaql::runtime)
    add_test(NAME core_shape_presentation_contracts COMMAND teaql_core_shape_presentation_tests)
    target_link_libraries(teaql_core_shape_presentation_probe PRIVATE teaql::runtime)
    add_test(NAME core_shape_presentation_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_core_shape_presentation.py
        --cpp $<TARGET_FILE:teaql_core_shape_presentation_probe>)
    add_executable(teaql_compact_row_api_tests teaql_core/tests/compact_row_api_tests.cpp)
    add_executable(teaql_compact_row_api_probe tests/parity/compact_row_api_probe.cpp)
    target_link_libraries(teaql_compact_row_api_probe PRIVATE teaql::runtime)
    add_test(NAME compact_row_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_compact_row_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_compact_row_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_compact_row_api_tests PRIVATE teaql::runtime)
    add_test(NAME compact_row_api_contracts COMMAND teaql_compact_row_api_tests)
    target_link_libraries(teaql_core_collection_tests PRIVATE teaql::core teaql::memory teaql::runtime)
    add_test(NAME core_collection_contracts COMMAND teaql_core_collection_tests)
    add_executable(teaql_query_model_tests teaql_core/tests/query_model_tests.cpp)
    add_executable(teaql_query_api_tests teaql_core/tests/query_api_tests.cpp)
    target_link_libraries(teaql_query_api_tests PRIVATE teaql::runtime)
    add_test(NAME query_api_contracts COMMAND teaql_query_api_tests)
    add_executable(teaql_query_api_probe tests/parity/query_api_probe.cpp)
    target_link_libraries(teaql_query_api_probe PRIVATE teaql::runtime)
    add_test(NAME query_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_query_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_query_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_query_model_tests PRIVATE teaql::core teaql::memory teaql::runtime)
    add_test(NAME query_model_contracts COMMAND teaql_query_model_tests)
    add_executable(teaql_safe_expression_api_tests teaql_core/tests/safe_expression_api_tests.cpp)
    target_link_libraries(teaql_safe_expression_api_tests PRIVATE teaql::runtime)
    add_test(NAME safe_expression_api_contracts COMMAND teaql_safe_expression_api_tests)
    add_executable(teaql_safe_expression_api_probe tests/parity/safe_expression_api_probe.cpp)
    target_link_libraries(teaql_safe_expression_api_probe PRIVATE teaql::runtime)
    add_test(NAME safe_expression_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_safe_expression_api_5_0_6.py
        --cpp $<TARGET_FILE:teaql_safe_expression_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_context_memory_tests teaql_runtime/tests/context_memory_tests.cpp)
    add_executable(teaql_data_store_tests teaql_runtime/tests/data_store_tests.cpp)
    add_executable(teaql_generated_support_tests teaql_runtime/tests/generated_support_tests.cpp)
    target_link_libraries(teaql_generated_support_tests PRIVATE teaql::runtime)
    add_test(NAME generated_support_contracts COMMAND teaql_generated_support_tests)
    add_executable(teaql_generated_support_probe tests/parity/generated_support_probe.cpp)
    target_link_libraries(teaql_generated_support_probe PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_support_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_support.py
        --cpp $<TARGET_FILE:teaql_generated_support_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME purposed_query_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_purposed_query_debug.py
        --cpp $<TARGET_FILE:teaql_generated_support_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_data_store_tests PRIVATE teaql::runtime)
    add_test(NAME data_store_contracts COMMAND teaql_data_store_tests)
    add_executable(teaql_data_store_probe tests/parity/data_store_probe.cpp)
    target_link_libraries(teaql_data_store_probe PRIVATE teaql::runtime)
    add_test(NAME data_store_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_data_store.py --cpp $<TARGET_FILE:teaql_data_store_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_context_memory_tests PRIVATE teaql::runtime)
    add_test(NAME context_memory_contracts COMMAND teaql_context_memory_tests)
    add_executable(teaql_context_reference_tests teaql_runtime/tests/context_reference_tests.cpp)
    target_link_libraries(teaql_context_reference_tests PRIVATE teaql::runtime)
    add_test(NAME context_reference_contracts COMMAND teaql_context_reference_tests)
    add_test(NAME context_early_global_close
        COMMAND teaql_context_reference_tests early-global-close)
    add_executable(teaql_sqlite_tests teaql_provider_sqlite/tests/provider_sqlite_regression_tests.cpp)
    target_link_libraries(teaql_sqlite_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME sqlite_contracts COMMAND teaql_sqlite_tests)
    add_executable(teaql_generated_api_tests tests/generator/generated_api_tests.cpp)
    add_executable(teaql_generated_matching_any_probe tests/parity/generated_matching_any_probe.cpp)
    target_link_libraries(teaql_generated_matching_any_probe PRIVATE teaql::provider_sqlite)
    target_compile_definitions(teaql_generated_matching_any_probe PRIVATE TEAQL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME generated_matching_any_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_matching_any.py
        --cpp $<TARGET_FILE:teaql_generated_matching_any_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    set_tests_properties(generated_matching_any_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME generated_matching_conversion_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_matching_conversion.py
        --cpp $<TARGET_FILE:teaql_generated_matching_any_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    set_tests_properties(generated_matching_conversion_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    target_link_libraries(teaql_generated_api_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_api_contracts COMMAND teaql_generated_api_tests)
    add_executable(teaql_generated_dynamic_field_tests tests/generator/generated_dynamic_field_tests.cpp)
    add_executable(teaql_generated_readonly_property_tests tests/generator/generated_readonly_property_tests.cpp)
    target_link_libraries(teaql_generated_readonly_property_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_readonly_property_contracts COMMAND teaql_generated_readonly_property_tests)
    add_executable(teaql_generated_readonly_properties_probe tests/parity/generated_readonly_properties_probe.cpp)
    target_link_libraries(teaql_generated_readonly_properties_probe PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_readonly_properties_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_readonly_properties.py
        --cpp $<TARGET_FILE:teaql_generated_readonly_properties_probe>)
    target_link_libraries(teaql_generated_dynamic_field_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_dynamic_field_contracts COMMAND teaql_generated_dynamic_field_tests)
    add_executable(teaql_generated_dynamic_carrier_probe tests/parity/generated_dynamic_carrier_probe.cpp)
    target_link_libraries(teaql_generated_dynamic_carrier_probe PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_dynamic_carrier_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_dynamic_carrier.py
        --cpp $<TARGET_FILE:teaql_generated_dynamic_carrier_probe>)
    set_tests_properties(generated_dynamic_field_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_checker_tests tests/generator/generated_checker_tests.cpp)
    target_link_libraries(teaql_generated_checker_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_checker_contracts COMMAND teaql_generated_checker_tests)
    add_executable(teaql_generated_relation_tests tests/generator/generated_relation_tests.cpp)
    target_link_libraries(teaql_generated_relation_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_relation_contracts COMMAND teaql_generated_relation_tests)
    add_executable(teaql_generated_optional_types_tests tests/generator/generated_optional_types_tests.cpp)
    target_link_libraries(teaql_generated_optional_types_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_optional_types_contracts COMMAND teaql_generated_optional_types_tests)
    set_tests_properties(generated_optional_types_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_business_id_tests tests/generator/generated_business_id_tests.cpp)
    target_link_libraries(teaql_generated_business_id_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_business_id_contracts COMMAND teaql_generated_business_id_tests)
    set_tests_properties(generated_business_id_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    set_tests_properties(generated_relation_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    set_tests_properties(generated_checker_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_entity_query_tests ../teaql_runtime/tests/entity_query_tests.cpp)
    target_link_libraries(teaql_entity_query_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME runtime_entity_query_contracts COMMAND teaql_entity_query_tests)
    set_tests_properties(runtime_entity_query_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_query_binding_tests ../teaql_runtime/tests/query_binding_tests.cpp)
    target_link_libraries(teaql_query_binding_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME runtime_query_binding_contracts COMMAND teaql_query_binding_tests)
    add_executable(teaql_generated_sample_data_tests ../teaql_runtime/tests/generated_sample_data_tests.cpp)
    target_link_libraries(teaql_generated_sample_data_tests PRIVATE teaql::runtime)
    add_test(NAME generated_sample_data_context_contracts COMMAND teaql_generated_sample_data_tests)
    add_executable(teaql_generated_runtime_assembly_probe tests/parity/generated_runtime_assembly_probe.cpp)
    target_link_libraries(teaql_generated_runtime_assembly_probe PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_runtime_assembly_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_runtime_assembly.py
        --cpp $<TARGET_FILE:teaql_generated_runtime_assembly_probe>)
    add_executable(teaql_generated_sample_execution_tests tests/generator/generated_sample_execution_tests.cpp)
    target_link_libraries(teaql_generated_sample_execution_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_sample_execution_contracts COMMAND teaql_generated_sample_execution_tests)
    set_tests_properties(generated_sample_execution_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_generated_sample_execution_probe tests/parity/generated_sample_execution_probe.cpp)
    target_link_libraries(teaql_generated_sample_execution_probe PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_sample_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_sample_execution.py
        --cpp $<TARGET_FILE:teaql_generated_sample_execution_probe>)
    add_test(NAME generated_sample_failures_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_sample_failures.py
        --cpp $<TARGET_FILE:teaql_generated_sample_execution_probe>)
    add_executable(teaql_generated_sample_save_async_tests tests/generator/generated_sample_save_async_tests.cpp)
    target_link_libraries(teaql_generated_sample_save_async_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_sample_save_async_contracts COMMAND teaql_generated_sample_save_async_tests)
    set_tests_properties(generated_sample_save_async_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
        add_library(teaql_sample_fixed_clock SHARED tests/support/fixed_sample_clock.cpp)
        # Load the same test clock into both Rust and C++; keep this tiny shim
        # independent of either executable's sanitizer runtime.
        target_compile_options(teaql_sample_fixed_clock PRIVATE -fno-sanitize=all)
        target_link_options(teaql_sample_fixed_clock PRIVATE -fno-sanitize=all)
        add_executable(teaql_generated_sample_fields_probe tests/parity/generated_sample_fields_probe.cpp)
        target_link_libraries(teaql_generated_sample_fields_probe PRIVATE teaql::provider_sqlite)
        add_test(NAME generated_sample_fields_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_sample_fields.py
            --cpp $<TARGET_FILE:teaql_generated_sample_fields_probe>
            --clock $<TARGET_FILE:teaql_sample_fixed_clock>)
    endif()
    add_executable(teaql_generated_sample_state_probe tests/parity/generated_sample_state_probe.cpp)
    target_link_libraries(teaql_generated_sample_state_probe PRIVATE teaql::runtime)
    add_test(NAME generated_sample_state_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_sample_state.py
        --cpp $<TARGET_FILE:teaql_generated_sample_state_probe>)
    add_executable(teaql_generated_expression_values_probe tests/parity/generated_expression_values_probe.cpp)
    target_link_libraries(teaql_generated_expression_values_probe PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_expression_values_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_expression_values.py
        --cpp $<TARGET_FILE:teaql_generated_expression_values_probe>)
    add_executable(teaql_generated_expression_diagnostics_probe tests/parity/generated_expression_diagnostics_probe.cpp)
    target_link_libraries(teaql_generated_expression_diagnostics_probe PRIVATE teaql::provider_sqlite)
    add_test(NAME generated_expression_diagnostics_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_generated_expression_diagnostics.py
        --cpp $<TARGET_FILE:teaql_generated_expression_diagnostics_probe>)
    add_executable(teaql_runtime_expression_tests ../teaql_runtime/tests/entity_expression_tests.cpp)
    target_link_libraries(teaql_runtime_expression_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME runtime_entity_expression_contracts COMMAND teaql_runtime_expression_tests)
    set_tests_properties(runtime_entity_expression_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    set_tests_properties(runtime_query_binding_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    set_tests_properties(generated_api_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_test(NAME generated_api_type_errors COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_generated_type_errors.py
        --build-dir ${CMAKE_CURRENT_BINARY_DIR})
    if(TEAQL_BUILD_EXAMPLES)
        add_test(NAME example_context_lifetime COMMAND teaql_context_lifetime)
        add_test(NAME example_sqlite_schema_crud COMMAND teaql_sqlite_schema_crud)
        add_test(NAME example_sqlite_relations_graph COMMAND teaql_sqlite_relations_graph)
        add_test(NAME example_generated_query_expression COMMAND teaql_generated_query_expression)
        set_tests_properties(example_generated_query_expression PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
        set_tests_properties(example_sqlite_schema_crud example_sqlite_relations_graph PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
        if(Python3_Interpreter_FOUND)
            add_test(NAME runtime_examples_rust_parity COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_examples_5_0_6.py
                --schema-crud $<TARGET_FILE:teaql_sqlite_schema_crud>
                --relations-graph $<TARGET_FILE:teaql_sqlite_relations_graph>
                "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
            set_tests_properties(runtime_examples_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
        endif()
    endif()
endif()
