cmake_minimum_required(VERSION 3.20)
project(teaql_cpp VERSION 0.1.0 LANGUAGES CXX)

add_library(teaql_core src/core/query_debug.cpp src/core/debug.cpp src/core/core.cpp src/core/business_id.cpp src/core/web.cpp src/core/xls.cpp src/core/entity.cpp src/core/list.cpp src/core/number_text.cpp src/core/request.cpp src/core/dynamic_search.cpp src/core/value.cpp src/core/decimal.cpp src/core/decimal_arithmetic.cpp src/core/decimal_format.cpp src/core/decimal_json.cpp src/core/json.cpp src/core/time.cpp src/core/row.cpp src/core/expr.cpp src/core/query.cpp)
add_library(teaql::core ALIAS teaql_core)
target_sources(teaql_core PRIVATE src/core/aes_gcm.cpp)
target_compile_features(teaql_core PUBLIC cxx_std_20)
target_include_directories(teaql_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(teaql_core PROPERTIES CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(teaql_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(teaql_memory src/memory/arena.cpp)
add_library(teaql::memory ALIAS teaql_memory)
target_compile_features(teaql_memory PUBLIC cxx_std_20)
target_include_directories(teaql_memory PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(teaql_memory PROPERTIES CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(teaql_memory PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(teaql_core PUBLIC teaql::memory)
find_package(Threads REQUIRED)
target_link_libraries(teaql_memory PUBLIC Threads::Threads)

add_library(teaql_data_service src/data_service/sql_log.cpp)
add_library(teaql::data_service ALIAS teaql_data_service)
target_link_libraries(teaql_data_service PUBLIC teaql::core Threads::Threads)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(teaql_data_service PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(SQLite3 3.37 REQUIRED)
add_library(teaql_sql src/sql/types.cpp src/sql/bindings.cpp src/sql/dialect.cpp src/sql/schema.cpp src/sql/mutation.cpp src/sql/query_plan.cpp src/sql/diagnostic.cpp src/sql/mutation_execution.cpp)
add_library(teaql::sql ALIAS teaql_sql)
target_link_libraries(teaql_sql PUBLIC teaql::data_service)

add_library(teaql_provider_sqlite src/provider_sqlite/schema.cpp src/provider_sqlite/schema_model.cpp src/provider_sqlite/schema_unicode.cpp src/provider_sqlite/dialect.cpp src/provider_sqlite/error.cpp src/provider_sqlite/bindings.cpp src/provider_sqlite/decoding.cpp src/provider_sqlite/provider.cpp)
add_library(teaql::provider_sqlite ALIAS teaql_provider_sqlite)
target_sources(teaql_provider_sqlite PRIVATE src/provider_sqlite/driver_error.cpp)
target_sources(teaql_provider_sqlite PRIVATE src/provider_sqlite/native_diagnostics.cpp)
target_sources(teaql_provider_sqlite PRIVATE src/provider_sqlite/simple_error.cpp)
target_sources(teaql_provider_sqlite PRIVATE src/provider_sqlite/from_sql_error.cpp)
target_link_libraries(teaql_provider_sqlite PUBLIC teaql::sql PRIVATE SQLite::SQLite3)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(teaql_provider_sqlite PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(teaql_runtime src/runtime/query_keys.cpp src/runtime/id_set_build_lock.cpp src/runtime/query_stores.cpp src/runtime/context_pagination.cpp src/runtime/relation_enhancement.cpp src/runtime/relation_installation.cpp src/runtime/relation.cpp src/runtime/data_service.cpp src/runtime/entity_graph_decoder.cpp src/runtime/context_entity.cpp src/runtime/entity_runtime.cpp src/runtime/graph.cpp src/runtime/context_graph.cpp src/runtime/context_event.cpp src/runtime/telemetry.cpp src/runtime/context_logging.cpp src/runtime/log_manager.cpp src/runtime/log_formatter.cpp src/runtime/logging.cpp src/runtime/entity_key.cpp src/runtime/event.cpp src/runtime/audit_privacy.cpp src/runtime/schema.cpp src/runtime/id.cpp src/runtime/business_clock.cpp src/runtime/business_service.cpp src/runtime/business_id.cpp src/runtime/registry.cpp src/runtime/check_result.cpp src/runtime/error.cpp)
add_library(teaql::runtime ALIAS teaql_runtime)
target_sources(teaql_runtime PRIVATE src/runtime/data_store.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/context_data_store.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/generated_support.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/round_trip_reference_types.cpp src/runtime/round_trip_reference.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/context_round_trip_reference.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/reference_crypto.cpp src/runtime/entity_reference.cpp src/runtime/context_entity_reference.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/inmemory_engine.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/context_root.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/context_locking.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/relation_graph.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/language.cpp src/runtime/i18n.cpp src/runtime/i18n_debug.cpp src/runtime/context_language.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/checker.cpp src/runtime/context_checker.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/fix_evidence.cpp src/runtime/context_fix.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/entity_save_preparation.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/ledger_plan.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/mutation_policy.cpp src/runtime/context_mutation_policy.cpp)
target_sources(teaql_runtime PRIVATE src/runtime/mutation_policy_plan.cpp)
target_link_libraries(teaql_runtime PUBLIC teaql::sql)

# Earlier C++ API: selected explicitly, outside the production runtime/provider path.
add_library(teaql_legacy_sqlite EXCLUDE_FROM_ALL
    src/compatibility/sqlite.cpp src/compatibility/legacy_sqlite_compiler.cpp)
add_library(teaql::legacy_sqlite ALIAS teaql_legacy_sqlite)
target_link_libraries(teaql_legacy_sqlite PUBLIC teaql::runtime PRIVATE SQLite::SQLite3)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(teaql_legacy_sqlite PRIVATE -Wall -Wextra -Wpedantic)
endif()
# Provider implements runtime ID/schema SPIs, as in the pinned Rust provider.
# Publish this dependency so applications linking only the provider also get
# RuntimeError's allocator-aware payload implementations.
target_link_libraries(teaql_provider_sqlite PUBLIC teaql::runtime)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(teaql_sql PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(teaql_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(TEAQL_LAYER_METADATA "")
foreach(layer IN ITEMS teaql_core teaql_memory teaql_data_service teaql_sql teaql_runtime)
    string(APPEND TEAQL_LAYER_METADATA
        "[${layer}]\nLINK=$<TARGET_PROPERTY:${layer},LINK_LIBRARIES>\nINTERFACE=$<TARGET_PROPERTY:${layer},INTERFACE_LINK_LIBRARIES>\nSOURCES=$<TARGET_PROPERTY:${layer},SOURCES>\n")
endforeach()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/teaql-layer-contracts.txt"
    CONTENT "${TEAQL_LAYER_METADATA}")

option(TEAQL_BUILD_EXAMPLES "Build TeaQL examples" ON)
if(TEAQL_BUILD_EXAMPLES)
    add_executable(teaql_context_lifetime examples/context_lifetime.cpp)
    target_link_libraries(teaql_context_lifetime PRIVATE teaql::runtime)
    add_executable(teaql_sqlite_schema_crud examples/sqlite_schema_crud.cpp)
    target_link_libraries(teaql_sqlite_schema_crud PRIVATE teaql::provider_sqlite)
    add_executable(teaql_sqlite_relations_graph examples/sqlite_relations_graph.cpp)
    target_link_libraries(teaql_sqlite_relations_graph PRIVATE teaql::provider_sqlite)
endif()

include(CTest)
# Development-only formatting; the runtime does not depend on Python or LLVM.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_custom_target(format
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/format-cpp.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Format handwritten C++ with pinned clang-format"
        VERBATIM)
    add_custom_target(format-check
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/format-cpp.py --check
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Check handwritten C++ formatting"
        VERBATIM)
endif()
if(BUILD_TESTING)
    add_executable(teaql_runtime_namespace_tests tests/runtime_namespace_tests.cpp)
    target_link_libraries(teaql_runtime_namespace_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_public_namespace_contracts COMMAND teaql_runtime_namespace_tests)
    if(Python3_Interpreter_FOUND AND CMAKE_NM)
        add_test(NAME runtime_layer_boundaries COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-layer-boundaries.py
            --runtime $<TARGET_FILE:teaql_runtime> --nm ${CMAKE_NM}
            --metadata ${CMAKE_CURRENT_BINARY_DIR}/teaql-layer-contracts.txt)
    endif()
    add_executable(teaql_entity_runtime_model_tests tests/entity_runtime_model_tests.cpp)
    target_link_libraries(teaql_entity_runtime_model_tests PRIVATE teaql::runtime)
    add_test(NAME entity_runtime_model_contracts COMMAND teaql_entity_runtime_model_tests)
    add_executable(teaql_entity_runtime_model_probe tests/parity/entity_runtime_model_probe.cpp)
    target_link_libraries(teaql_entity_runtime_model_probe PRIVATE teaql::runtime)
    add_test(NAME entity_runtime_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_runtime_models.py
        --cpp $<TARGET_FILE:teaql_entity_runtime_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_business_clock_scope_tests tests/business_clock_scope_tests.cpp)
    target_link_libraries(teaql_business_clock_scope_tests PRIVATE teaql::runtime)
    add_test(NAME business_clock_scope_contracts COMMAND teaql_business_clock_scope_tests)
    add_test(NAME business_clock_reentrant_time COMMAND teaql_business_clock_scope_tests reentrant-time)
    add_test(NAME business_clock_reentrant_date COMMAND teaql_business_clock_scope_tests reentrant-date)
    add_executable(teaql_runtime_callback_scope_tests tests/runtime_callback_scope_tests.cpp)
    target_link_libraries(teaql_runtime_callback_scope_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_callback_scope_contracts COMMAND teaql_runtime_callback_scope_tests)
    add_test(NAME runtime_callback_registry_close COMMAND teaql_runtime_callback_scope_tests registry)
    add_test(NAME runtime_callback_relation_loads_close COMMAND teaql_runtime_callback_scope_tests loads)
    add_test(NAME runtime_callback_select_behavior_close COMMAND teaql_runtime_callback_scope_tests select-behavior)
    add_test(NAME runtime_callback_select_policy_close COMMAND teaql_runtime_callback_scope_tests select-policy)
    add_test(NAME runtime_callback_business_slot_close COMMAND teaql_runtime_callback_scope_tests business-slot)
    add_test(NAME runtime_callback_business_key_close COMMAND teaql_runtime_callback_scope_tests business-key)
    add_test(NAME runtime_callback_business_allocator_close COMMAND teaql_runtime_callback_scope_tests business-allocator)
    add_executable(teaql_context_provider_borrow_tests tests/context_provider_borrow_tests.cpp)
    target_link_libraries(teaql_context_provider_borrow_tests PRIVATE teaql::runtime)
    add_test(NAME context_provider_borrow_contracts COMMAND teaql_context_provider_borrow_tests)
    add_test(NAME context_provider_reentrant_registry COMMAND teaql_context_provider_borrow_tests reentrant-registry)
    add_test(NAME context_provider_reentrant_schema COMMAND teaql_context_provider_borrow_tests reentrant-schema)
    add_executable(teaql_context_provider_dispatch_tests tests/context_provider_dispatch_tests.cpp)
    target_link_libraries(teaql_context_provider_dispatch_tests PRIVATE teaql::runtime)
    add_test(NAME context_dispatch_lifecycle_contracts COMMAND teaql_context_provider_dispatch_tests lifecycle)
    add_test(NAME context_dispatch_exception_contracts COMMAND teaql_context_provider_dispatch_tests exceptions)
    add_test(NAME context_dispatch_shutdown_contracts COMMAND teaql_context_provider_dispatch_tests shutdown)
    add_test(NAME context_dispatch_entity_close COMMAND teaql_context_provider_dispatch_tests entity)
    add_test(NAME context_dispatch_all_entities_close COMMAND teaql_context_provider_dispatch_tests all-entities)
    add_test(NAME context_dispatch_checker_close COMMAND teaql_context_provider_dispatch_tests checker)
    add_test(NAME context_dispatch_telemetry_noop_close COMMAND teaql_context_provider_dispatch_tests telemetry-noop)
    add_test(NAME context_dispatch_telemetry_start_close COMMAND teaql_context_provider_dispatch_tests telemetry-start)
    add_executable(teaql_runtime_id_model_tests tests/runtime_id_model_tests.cpp)
    target_link_libraries(teaql_runtime_id_model_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_id_model_contracts COMMAND teaql_runtime_id_model_tests)
    add_executable(teaql_runtime_id_model_probe tests/parity/runtime_id_model_probe.cpp)
    target_link_libraries(teaql_runtime_id_model_probe PRIVATE teaql::runtime)
    add_test(NAME runtime_id_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_id_models.py
        --cpp $<TARGET_FILE:teaql_runtime_id_model_probe>
        "--profile=$<IF:$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>,release,debug>"
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_context_defaults_tests tests/context_defaults_tests.cpp)
    target_link_libraries(teaql_context_defaults_tests PRIVATE teaql::runtime)
    add_test(NAME context_defaults_contracts COMMAND teaql_context_defaults_tests)
    add_executable(teaql_context_defaults_probe tests/parity/context_defaults_probe.cpp)
    target_link_libraries(teaql_context_defaults_probe PRIVATE teaql::runtime)
    add_test(NAME context_defaults_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_defaults.py
        --cpp $<TARGET_FILE:teaql_context_defaults_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_reference_model_tests tests/reference_model_tests.cpp)
    target_link_libraries(teaql_reference_model_tests PRIVATE teaql::runtime)
    add_test(NAME reference_model_contracts COMMAND teaql_reference_model_tests)
    add_executable(teaql_reference_model_probe tests/parity/reference_model_probe.cpp)
    target_link_libraries(teaql_reference_model_probe PRIVATE teaql::runtime)
    add_test(NAME reference_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_reference_models.py
        --cpp $<TARGET_FILE:teaql_reference_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_round_trip_reference_tests tests/round_trip_reference_tests.cpp)
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
    add_executable(teaql_entity_reference_tests tests/entity_reference_tests.cpp)
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
    add_executable(teaql_reference_crypto_tests tests/reference_crypto_tests.cpp)
    target_link_libraries(teaql_reference_crypto_tests PRIVATE teaql::runtime)
    add_test(NAME reference_crypto_nist_contracts COMMAND teaql_reference_crypto_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/nist_gcm_cases.json)
    add_executable(teaql_inmemory_tests tests/inmemory_tests.cpp)
    target_link_libraries(teaql_inmemory_tests PRIVATE teaql::runtime)
    add_test(NAME inmemory_context_contracts COMMAND teaql_inmemory_tests)
    add_executable(teaql_inmemory_probe tests/parity/inmemory_probe.cpp)
    target_link_libraries(teaql_inmemory_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME inmemory_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_inmemory.py --cpp $<TARGET_FILE:teaql_inmemory_probe>
            "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_locking_tests tests/locking_tests.cpp)
    target_link_libraries(teaql_locking_tests PRIVATE teaql::runtime)
    add_test(NAME context_locking_contracts COMMAND teaql_locking_tests)
    add_executable(teaql_locking_probe tests/parity/locking_probe.cpp)
    target_link_libraries(teaql_locking_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME context_locking_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_locking.py --cpp $<TARGET_FILE:teaql_locking_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_runtime_examples_tests tests/runtime_examples_tests.cpp)
    target_link_libraries(teaql_runtime_examples_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME runtime_examples_context_contracts COMMAND teaql_runtime_examples_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/runtime_examples_provenance.json)
    set_tests_properties(runtime_examples_context_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_transaction_move_only_tests tests/transaction_move_only_tests.cpp)
    target_link_libraries(teaql_transaction_move_only_tests PRIVATE teaql::runtime)
    add_test(NAME transaction_move_only_contracts COMMAND teaql_transaction_move_only_tests)
    add_executable(teaql_transaction_provider_error_tests tests/transaction_provider_error_tests.cpp)
    target_link_libraries(teaql_transaction_provider_error_tests PRIVATE teaql::runtime)
    add_test(NAME transaction_provider_error_contracts COMMAND teaql_transaction_provider_error_tests)
    add_executable(teaql_transaction_move_only_probe tests/parity/transaction_move_only_probe.cpp)
    target_link_libraries(teaql_transaction_move_only_probe PRIVATE teaql::runtime)
    add_test(NAME transaction_move_only_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_transaction_move_only.py
        --cpp $<TARGET_FILE:teaql_transaction_move_only_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_ledger_move_only_tests tests/ledger_move_only_tests.cpp)
    target_link_libraries(teaql_ledger_move_only_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME ledger_move_only_contracts COMMAND teaql_ledger_move_only_tests)
    set_tests_properties(ledger_move_only_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_ledger_move_only_probe tests/parity/ledger_move_only_probe.cpp)
    target_link_libraries(teaql_ledger_move_only_probe PRIVATE teaql::runtime)
    add_test(NAME ledger_move_only_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_ledger_move_only.py
        --cpp $<TARGET_FILE:teaql_ledger_move_only_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_policy_model_tests tests/policy_model_tests.cpp)
    target_link_libraries(teaql_policy_model_tests PRIVATE teaql::runtime)
    add_test(NAME policy_model_contracts COMMAND teaql_policy_model_tests)
    add_executable(teaql_policy_model_probe tests/parity/policy_model_probe.cpp)
    target_link_libraries(teaql_policy_model_probe PRIVATE teaql::runtime)
    add_test(NAME policy_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_policy_models.py
        --cpp $<TARGET_FILE:teaql_policy_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_event_model_tests tests/event_model_tests.cpp)
    target_link_libraries(teaql_event_model_tests PRIVATE teaql::runtime)
    add_test(NAME event_model_contracts COMMAND teaql_event_model_tests)
    add_executable(teaql_event_model_probe tests/parity/event_model_probe.cpp)
    target_link_libraries(teaql_event_model_probe PRIVATE teaql::runtime)
    add_test(NAME event_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_event_models.py
        --cpp $<TARGET_FILE:teaql_event_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_check_model_tests tests/check_model_tests.cpp)
    target_link_libraries(teaql_check_model_tests PRIVATE teaql::runtime)
    add_test(NAME check_model_contracts COMMAND teaql_check_model_tests)
    add_executable(teaql_check_model_probe tests/parity/check_model_probe.cpp)
    target_link_libraries(teaql_check_model_probe PRIVATE teaql::runtime)
    add_test(NAME check_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_check_models.py
        --cpp $<TARGET_FILE:teaql_check_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_relation_model_tests tests/relation_model_tests.cpp)
    target_link_libraries(teaql_relation_model_tests PRIVATE teaql::runtime)
    add_test(NAME relation_model_contracts COMMAND teaql_relation_model_tests)
    add_executable(teaql_relation_model_probe tests/parity/relation_model_probe.cpp)
    target_link_libraries(teaql_relation_model_probe PRIVATE teaql::runtime)
    add_test(NAME relation_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_models.py
        --cpp $<TARGET_FILE:teaql_relation_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_foreign_work_tests tests/runtime_foreign_work_tests.cpp)
    target_link_libraries(teaql_runtime_foreign_work_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_foreign_query_cleanup COMMAND teaql_runtime_foreign_work_tests query)
    add_test(NAME runtime_foreign_mutation_cleanup COMMAND teaql_runtime_foreign_work_tests mutation)
    add_executable(teaql_unified_buffer_clone_tests tests/unified_buffer_clone_tests.cpp)
    target_link_libraries(teaql_unified_buffer_clone_tests PRIVATE teaql::runtime)
    add_test(NAME unified_buffer_clone_contracts COMMAND teaql_unified_buffer_clone_tests)
    add_executable(teaql_unified_buffer_clone_probe tests/parity/unified_buffer_clone_probe.cpp)
    target_link_libraries(teaql_unified_buffer_clone_probe PRIVATE teaql::runtime)
    add_test(NAME unified_buffer_clone_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_unified_buffer_clone.py
        --cpp $<TARGET_FILE:teaql_unified_buffer_clone_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_log_model_tests tests/log_model_tests.cpp)
    target_link_libraries(teaql_log_model_tests PRIVATE teaql::runtime)
    add_test(NAME log_model_contracts COMMAND teaql_log_model_tests)
    add_executable(teaql_log_model_probe tests/parity/log_model_probe.cpp)
    target_link_libraries(teaql_log_model_probe PRIVATE teaql::runtime)
    add_test(NAME log_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_log_models.py
        --cpp $<TARGET_FILE:teaql_log_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_transaction_scope_tests tests/transaction_scope_tests.cpp)
    target_link_libraries(teaql_transaction_scope_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME transaction_scope_context_contracts COMMAND teaql_transaction_scope_tests)
    set_tests_properties(transaction_scope_context_contracts PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_transaction_scope_probe tests/parity/transaction_scope_probe.cpp)
    target_link_libraries(teaql_transaction_scope_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME transaction_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_transaction_scope.py --cpp $<TARGET_FILE:teaql_transaction_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_audited_auto_save_tests tests/audited_auto_save_tests.cpp)
    target_link_libraries(teaql_audited_auto_save_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME audited_auto_save_context_contracts COMMAND teaql_audited_auto_save_tests)
    add_executable(teaql_audited_auto_save_probe tests/parity/audited_auto_save_probe.cpp)
    target_link_libraries(teaql_audited_auto_save_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME audited_auto_save_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_audited_auto_save.py --cpp $<TARGET_FILE:teaql_audited_auto_save_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_audited_save_probe tests/parity/audited_save_probe.cpp)
    target_link_libraries(teaql_audited_save_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME audited_save_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_audited_save.py --cpp $<TARGET_FILE:teaql_audited_save_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_audited_save_tests tests/audited_save_tests.cpp)
    target_link_libraries(teaql_audited_save_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME audited_save_context_contracts COMMAND teaql_audited_save_tests)
    add_executable(teaql_graph_saver_tests tests/graph_saver_tests.cpp)
    target_link_libraries(teaql_graph_saver_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME graph_saver_context_contracts COMMAND teaql_graph_saver_tests)
    add_executable(teaql_graph_saver_probe tests/parity/graph_saver_probe.cpp)
    target_link_libraries(teaql_graph_saver_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME graph_saver_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_saver.py --cpp $<TARGET_FILE:teaql_graph_saver_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_ledger_execution_tests tests/ledger_execution_tests.cpp)
    target_link_libraries(teaql_ledger_execution_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME ledger_execution_context_contracts COMMAND teaql_ledger_execution_tests)
    add_executable(teaql_ledger_execution_probe tests/parity/ledger_execution_probe.cpp)
    target_link_libraries(teaql_ledger_execution_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME ledger_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_ledger_execution.py --cpp $<TARGET_FILE:teaql_ledger_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_entity_graph_execution_tests tests/entity_graph_execution_tests.cpp)
    target_link_libraries(teaql_entity_graph_execution_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_graph_execution_context_contracts COMMAND teaql_entity_graph_execution_tests)
    add_executable(teaql_graph_execution_probe tests/parity/graph_execution_probe.cpp)
    add_executable(teaql_recursive_graph_write_tests tests/recursive_graph_write_tests.cpp)
    target_link_libraries(teaql_recursive_graph_write_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME recursive_graph_write_context_contracts COMMAND teaql_recursive_graph_write_tests)
    add_test(NAME recursive_graph_write_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_recursive_graph_write.py
        --cpp $<TARGET_FILE:teaql_graph_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_graph_node_command_tests tests/graph_node_command_tests.cpp)
    target_link_libraries(teaql_graph_node_command_tests PRIVATE teaql::runtime)
    add_test(NAME graph_node_command_context_contracts COMMAND teaql_graph_node_command_tests)
    add_test(NAME graph_node_command_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_node_commands.py
        --cpp $<TARGET_FILE:teaql_graph_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_graph_save_adapter_tests tests/graph_save_adapter_tests.cpp)
    target_link_libraries(teaql_graph_save_adapter_tests PRIVATE teaql::runtime)
    add_test(NAME graph_save_adapter_context_contracts COMMAND teaql_graph_save_adapter_tests)
    add_test(NAME graph_save_adapter_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_save_adapters.py
        --cpp $<TARGET_FILE:teaql_graph_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_graph_execution_probe PRIVATE teaql::runtime)
    add_executable(teaql_ledger_plan_tests tests/ledger_plan_tests.cpp)
    target_link_libraries(teaql_ledger_plan_tests PRIVATE teaql::runtime)
    add_test(NAME ledger_plan_context_contracts COMMAND teaql_ledger_plan_tests)
    add_executable(teaql_ledger_plan_probe tests/parity/ledger_plan_probe.cpp)
    target_link_libraries(teaql_ledger_plan_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME ledger_plan_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_ledger_plan.py --cpp $<TARGET_FILE:teaql_ledger_plan_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    if(Python3_Interpreter_FOUND)
        add_test(NAME graph_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_execution.py --cpp $<TARGET_FILE:teaql_graph_execution_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_runtime_mutation_tests tests/runtime_mutation_tests.cpp)
    target_link_libraries(teaql_runtime_mutation_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME runtime_mutation_context_contracts COMMAND teaql_runtime_mutation_tests)
    add_test(NAME runtime_mutation_reentrant_prepare COMMAND teaql_runtime_mutation_tests reentrant-prepare)
    add_test(NAME runtime_mutation_reentrant_event COMMAND teaql_runtime_mutation_tests reentrant-event)
    add_executable(teaql_runtime_mutation_probe tests/parity/runtime_mutation_probe.cpp)
    target_link_libraries(teaql_runtime_mutation_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME runtime_mutation_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_mutation.py --cpp $<TARGET_FILE:teaql_runtime_mutation_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_entity_graph_plan_tests tests/entity_graph_plan_tests.cpp)
    target_link_libraries(teaql_entity_graph_plan_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_graph_plan_context_contracts COMMAND teaql_entity_graph_plan_tests)
    add_test(NAME internal_id_reentrant_close COMMAND teaql_internal_id_tests reentrant-close)
    add_test(NAME internal_id_reentrant_floor COMMAND teaql_internal_id_tests reentrant-floor)
    add_executable(teaql_entity_graph_plan_probe tests/parity/entity_graph_plan_probe.cpp)
    target_link_libraries(teaql_entity_graph_plan_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME entity_graph_plan_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_graph_plan.py --cpp $<TARGET_FILE:teaql_entity_graph_plan_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_policy_plan_probe tests/parity/policy_plan_probe.cpp)
    target_link_libraries(teaql_policy_plan_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME policy_plan_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_policy_plan.py --cpp $<TARGET_FILE:teaql_policy_plan_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_mutation_policy_tests tests/mutation_policy_tests.cpp)
    target_link_libraries(teaql_mutation_policy_tests PRIVATE teaql::runtime)
    add_test(NAME mutation_policy_context_contracts COMMAND teaql_mutation_policy_tests)
    add_test(NAME mutation_policy_reentrant_close COMMAND teaql_mutation_policy_tests reentrant-close)
    add_executable(teaql_mutation_policy_probe tests/parity/mutation_policy_probe.cpp)
    target_link_libraries(teaql_mutation_policy_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME mutation_policy_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_mutation_policy.py --cpp $<TARGET_FILE:teaql_mutation_policy_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_save_preparation_tests tests/save_preparation_tests.cpp)
    target_link_libraries(teaql_save_preparation_tests PRIVATE teaql::runtime)
    add_test(NAME save_preparation_context_contracts COMMAND teaql_save_preparation_tests)
    add_executable(teaql_save_preparation_probe tests/parity/save_preparation_probe.cpp)
    target_link_libraries(teaql_save_preparation_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME save_preparation_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_save_preparation.py --cpp $<TARGET_FILE:teaql_save_preparation_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_graph_fix_tests tests/graph_fix_tests.cpp)
    target_link_libraries(teaql_graph_fix_tests PRIVATE teaql::runtime)
    add_test(NAME graph_fix_context_contracts COMMAND teaql_graph_fix_tests)
    add_executable(teaql_graph_fix_probe tests/parity/graph_fix_probe.cpp)
    target_link_libraries(teaql_graph_fix_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME graph_fix_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_fix.py --cpp $<TARGET_FILE:teaql_graph_fix_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_checker_tests tests/checker_tests.cpp)
    target_link_libraries(teaql_checker_tests PRIVATE teaql::runtime)
    add_test(NAME checker_context_contracts COMMAND teaql_checker_tests)
    add_test(NAME checker_reentrant_close COMMAND teaql_checker_tests reentrant-close)
    add_test(NAME checker_registration_reentrant_close COMMAND teaql_checker_tests registration-close)
    add_test(NAME checker_typed_direct_reentrant_close COMMAND teaql_checker_tests typed-direct-close)
    add_executable(teaql_checker_probe tests/parity/checker_probe.cpp)
    target_link_libraries(teaql_checker_probe PRIVATE teaql::runtime)
    if(Python3_Interpreter_FOUND)
        add_test(NAME checker_rust_parity COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_checker.py --cpp $<TARGET_FILE:teaql_checker_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_i18n_tests tests/i18n_tests.cpp)
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
    target_link_libraries(teaql_sqlite_schema_probe PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_executable(teaql_provider_sqlite_schema_tests tests/provider_sqlite_schema_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_schema_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME provider_sqlite_schema_contracts COMMAND teaql_provider_sqlite_schema_tests)
    add_executable(teaql_original_sql_binding_tests tests/parity/teaql-sql/binding_tests.cpp)
    add_executable(teaql_original_query_execution_tests tests/parity/teaql-sql/execution_tests.cpp)
    add_executable(teaql_original_mutation_execution_tests tests/parity/teaql-sql/mutation_execution_tests.cpp)
    target_link_libraries(teaql_original_mutation_execution_tests PRIVATE teaql::runtime teaql::sql)
    add_test(NAME original_sql_mutation_execution_contracts COMMAND teaql_original_mutation_execution_tests)
    target_link_libraries(teaql_original_query_execution_tests PRIVATE teaql::runtime teaql::sql)
    add_test(NAME original_sql_query_execution_contracts COMMAND teaql_original_query_execution_tests)
    target_link_libraries(teaql_original_sql_binding_tests PRIVATE teaql::sql)
    add_test(NAME original_sql_binding_policy_contracts COMMAND teaql_original_sql_binding_tests)
    add_executable(teaql_original_sql_rendering_tests tests/parity/teaql-sql/log_rendering_tests.cpp)
    add_executable(teaql_original_sql_default_tests tests/parity/teaql-sql/default_contracts.cpp)
    add_executable(teaql_original_sql_executor_tests tests/parity/teaql-sql/executor_contracts.cpp)
    target_link_libraries(teaql_original_sql_executor_tests PRIVATE teaql::runtime)
    add_test(NAME original_sql_executor_contracts COMMAND teaql_original_sql_executor_tests)
    add_executable(teaql_original_sql_compiler_tests tests/parity/teaql-sql/compiler.cpp)
    target_link_libraries(teaql_original_sql_compiler_tests PRIVATE teaql::sql)
    add_test(NAME original_sql_compiler_contracts COMMAND teaql_original_sql_compiler_tests)
    target_link_libraries(teaql_original_sql_default_tests PRIVATE teaql::sql)
    add_test(NAME original_sql_default_contracts COMMAND teaql_original_sql_default_tests)
    add_executable(teaql_original_diagnostic_stream_tests tests/parity/teaql-sql/diagnostic_stream.cpp)
    target_link_libraries(teaql_original_diagnostic_stream_tests PRIVATE teaql::runtime)
    add_test(NAME original_sql_diagnostic_stream_contracts COMMAND teaql_original_diagnostic_stream_tests)
    target_link_libraries(teaql_original_sql_rendering_tests PRIVATE teaql::sql)
    add_test(NAME original_sql_log_rendering_contracts COMMAND teaql_original_sql_rendering_tests)
    add_executable(teaql_original_business_clock_tests tests/parity/teaql-runtime/business_clock.cpp)
    add_executable(teaql_original_entity_status_tests tests/parity/teaql-runtime/entity_status.cpp)
    target_link_libraries(teaql_original_entity_status_tests PRIVATE teaql::memory)
    add_test(NAME original_runtime_entity_status_contracts COMMAND teaql_original_entity_status_tests)
    add_executable(teaql_original_cache_relation_tests tests/parity/teaql-runtime/cache_relation.cpp)
    target_link_libraries(teaql_original_cache_relation_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_cache_relation_contracts COMMAND teaql_original_cache_relation_tests)
    add_executable(teaql_original_inmemory_tests tests/parity/teaql-runtime/inmemory_engine.cpp)
    target_link_libraries(teaql_original_inmemory_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_inmemory_contracts COMMAND teaql_original_inmemory_tests)
    add_executable(teaql_original_checker_locale_tests tests/parity/teaql-runtime/checker_locale.cpp)
    target_link_libraries(teaql_original_checker_locale_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_checker_locale_contracts COMMAND teaql_original_checker_locale_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql-runtime/locale-cases-v1.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql-runtime/builtin-i18n-v1.json)
    add_executable(teaql_original_log_telemetry_tests tests/parity/teaql-runtime/log_telemetry.cpp)
    target_link_libraries(teaql_original_log_telemetry_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_log_telemetry_contracts COMMAND teaql_original_log_telemetry_tests)
    add_executable(teaql_original_policy_graph_tests tests/parity/teaql-runtime/policy_graph.cpp)
    target_link_libraries(teaql_original_policy_graph_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_policy_graph_contracts COMMAND teaql_original_policy_graph_tests)
    add_executable(teaql_original_log_privacy_tests tests/parity/teaql-runtime/log_privacy.cpp)
    target_link_libraries(teaql_original_log_privacy_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_log_privacy_contracts COMMAND teaql_original_log_privacy_tests)
    add_executable(teaql_original_context_tests tests/parity/teaql-runtime/context.cpp)
    target_link_libraries(teaql_original_context_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_context_contracts COMMAND teaql_original_context_tests)
    if(Python3_Interpreter_FOUND)
        add_test(NAME original_runtime_file_log_privacy_contracts COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql-runtime/file_log_privacy.py
            --cpp $<TARGET_FILE:teaql_original_log_privacy_tests>
            "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    add_executable(teaql_original_masking_tests tests/parity/teaql-runtime/masking_contract_tests.cpp)
    add_executable(teaql_original_flat_graph_tests tests/parity/teaql-runtime/flat_identity_graph.cpp)
    add_executable(teaql_original_ledger_tests tests/parity/teaql-runtime/test_incremental_ledger.cpp)
    target_link_libraries(teaql_original_ledger_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_incremental_ledger_contracts COMMAND teaql_original_ledger_tests)
    target_link_libraries(teaql_original_flat_graph_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_flat_identity_graph_contracts COMMAND teaql_original_flat_graph_tests)
    target_link_libraries(teaql_original_masking_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_masking_contracts COMMAND teaql_original_masking_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql-runtime/data/masking-v1.tsv)
    target_link_libraries(teaql_original_business_clock_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_business_clock_contracts COMMAND teaql_original_business_clock_tests)
    add_executable(teaql_original_business_permutation_tests tests/parity/teaql-runtime/business_id_permutation.cpp)
    target_link_libraries(teaql_original_business_permutation_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_business_permutation_contracts COMMAND teaql_original_business_permutation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql-runtime/data/business-id-permutation-v1.csv)
    add_executable(teaql_original_provider_search_tests tests/parity/teaql-provider-sqlite/dynamic_search.cpp)
    add_executable(teaql_original_provider_storage_tests tests/parity/teaql-provider-sqlite/storage.cpp)
    target_link_libraries(teaql_original_provider_storage_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME original_provider_storage_contracts COMMAND teaql_original_provider_storage_tests)
    add_executable(teaql_original_provider_schema_tests tests/parity/teaql-provider-sqlite/schema.cpp)
    target_link_libraries(teaql_original_provider_schema_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME original_provider_schema_contracts COMMAND teaql_original_provider_schema_tests)
    add_executable(teaql_original_provider_query_tests tests/parity/teaql-provider-sqlite/query_rows.cpp)
    target_link_libraries(teaql_original_provider_query_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME original_provider_query_contracts COMMAND teaql_original_provider_query_tests)
    add_executable(teaql_original_provider_transaction_tests tests/parity/teaql-provider-sqlite/transactions.cpp)
    target_link_libraries(teaql_original_provider_transaction_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME original_provider_transaction_contracts COMMAND teaql_original_provider_transaction_tests)
    add_executable(teaql_original_entity_save_tests tests/parity/teaql-runtime/entity_save.cpp)
    target_link_libraries(teaql_original_entity_save_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_entity_save_contracts COMMAND teaql_original_entity_save_tests)
    add_executable(teaql_original_round_trip_reference_tests tests/parity/teaql-runtime/round_trip_reference.cpp)
    target_link_libraries(teaql_original_round_trip_reference_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_round_trip_reference_contracts COMMAND teaql_original_round_trip_reference_tests)
    add_executable(teaql_original_runtime_registration_tests tests/parity/teaql-runtime/registration.cpp)
    target_link_libraries(teaql_original_runtime_registration_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_registration_contracts COMMAND teaql_original_runtime_registration_tests)
    add_executable(teaql_original_runtime_service_tests tests/parity/teaql-runtime/service_boundaries.cpp)
    target_link_libraries(teaql_original_runtime_service_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_service_boundary_contracts COMMAND teaql_original_runtime_service_tests)
    add_executable(teaql_original_runtime_validation_tests tests/parity/teaql-runtime/validation.cpp)
    target_link_libraries(teaql_original_runtime_validation_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_validation_contracts COMMAND teaql_original_runtime_validation_tests)
    add_executable(teaql_original_runtime_events_relations_tests tests/parity/teaql-runtime/events_relations.cpp)
    target_link_libraries(teaql_original_runtime_events_relations_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_events_relations_contracts COMMAND teaql_original_runtime_events_relations_tests)
    add_executable(teaql_original_runtime_typed_query_tests tests/parity/teaql-runtime/typed_queries.cpp)
    target_link_libraries(teaql_original_runtime_typed_query_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_typed_query_contracts COMMAND teaql_original_runtime_typed_query_tests)
    add_executable(teaql_original_runtime_aggregate_tests tests/parity/teaql-runtime/aggregates_cache.cpp)
    target_link_libraries(teaql_original_runtime_aggregate_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_aggregate_cache_contracts COMMAND teaql_original_runtime_aggregate_tests)
    add_executable(teaql_original_runtime_pagination_tests tests/parity/teaql-runtime/pagination.cpp)
    target_link_libraries(teaql_original_runtime_pagination_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_pagination_contracts COMMAND teaql_original_runtime_pagination_tests)
    add_executable(teaql_original_runtime_memory_service_tests tests/parity/teaql-runtime/memory_service.cpp)
    target_link_libraries(teaql_original_runtime_memory_service_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_memory_service_contracts COMMAND teaql_original_runtime_memory_service_tests)
    add_executable(teaql_original_runtime_context_tail_tests tests/parity/teaql-runtime/context_tail.cpp)
    target_link_libraries(teaql_original_runtime_context_tail_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_context_tail_contracts COMMAND teaql_original_runtime_context_tail_tests)
    add_executable(teaql_original_runtime_graph_reference_tests tests/parity/teaql-runtime/graph_reference_lifetime.cpp)
    target_link_libraries(teaql_original_runtime_graph_reference_tests PRIVATE teaql::runtime)
    add_test(NAME original_runtime_graph_reference_contracts COMMAND teaql_original_runtime_graph_reference_tests)
    if(Python3_Interpreter_FOUND)
        add_test(NAME original_runtime_optimistic_conflict_output_contracts
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/teaql-runtime/optimistic_conflict_output.py
            --cpp $<TARGET_FILE:teaql_original_runtime_memory_service_tests>
            "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()
    set_tests_properties(original_provider_query_contracts PROPERTIES TIMEOUT 60)
    set_tests_properties(original_provider_schema_contracts PROPERTIES TIMEOUT 60)
    target_link_libraries(teaql_original_provider_search_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME original_provider_dynamic_search_contracts COMMAND teaql_original_provider_search_tests)
    add_executable(teaql_original_provider_guard_tests tests/parity/teaql-provider-sqlite/guarded_mutation.cpp)
    target_link_libraries(teaql_original_provider_guard_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME original_provider_guarded_mutation_contracts COMMAND teaql_original_provider_guard_tests)
    set_tests_properties(original_provider_dynamic_search_contracts original_provider_guarded_mutation_contracts
        PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
    add_executable(teaql_provider_sqlite_unicode_tests tests/provider_sqlite_unicode_tests.cpp)
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
    add_executable(teaql_query_keys_tests tests/query_keys_tests.cpp)
    target_link_libraries(teaql_query_keys_tests PRIVATE teaql::runtime)
    add_test(NAME query_keys_contracts COMMAND teaql_query_keys_tests)
    add_executable(teaql_id_set_build_probe tests/parity/id_set_build_probe.cpp)
    target_link_libraries(teaql_id_set_build_probe PRIVATE teaql::runtime)
    add_executable(teaql_id_set_build_tests tests/id_set_build_tests.cpp)
    target_link_libraries(teaql_id_set_build_tests PRIVATE teaql::runtime)
    add_test(NAME id_set_build_contracts COMMAND teaql_id_set_build_tests)
    set_tests_properties(id_set_build_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_query_store_model_tests tests/query_store_model_tests.cpp)
    target_link_libraries(teaql_query_store_model_tests PRIVATE teaql::runtime)
    add_test(NAME query_store_model_contracts COMMAND teaql_query_store_model_tests)
    add_executable(teaql_query_store_model_probe tests/parity/query_store_model_probe.cpp)
    target_link_libraries(teaql_query_store_model_probe PRIVATE teaql::runtime)
    add_test(NAME query_store_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_query_store_models.py
        --cpp $<TARGET_FILE:teaql_query_store_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_query_store_probe tests/parity/query_store_probe.cpp)
    target_link_libraries(teaql_query_store_probe PRIVATE teaql::runtime)
    add_executable(teaql_query_store_tests tests/query_store_tests.cpp)
    target_link_libraries(teaql_query_store_tests PRIVATE teaql::runtime)
    add_test(NAME query_store_contracts COMMAND teaql_query_store_tests)
    set_tests_properties(query_store_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_relation_enhancement_probe tests/parity/relation_enhancement_probe.cpp)
    target_link_libraries(teaql_relation_enhancement_probe PRIVATE teaql::runtime)
    add_executable(teaql_relation_hydration_probe tests/parity/relation_hydration_probe.cpp)
    target_link_libraries(teaql_relation_hydration_probe PRIVATE teaql::runtime)
    add_executable(teaql_relation_execution_tests tests/relation_execution_tests.cpp)
    target_link_libraries(teaql_relation_execution_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME relation_execution_contracts COMMAND teaql_relation_execution_tests)
    set_tests_properties(relation_execution_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_relation_hydration_tests tests/relation_hydration_tests.cpp)
    target_link_libraries(teaql_relation_hydration_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME relation_hydration_contracts COMMAND teaql_relation_hydration_tests)
    set_tests_properties(relation_hydration_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_relation_enhancement_tests tests/relation_enhancement_tests.cpp)
    target_link_libraries(teaql_relation_enhancement_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME relation_enhancement_contracts COMMAND teaql_relation_enhancement_tests)
    set_tests_properties(relation_enhancement_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_context_query_tests tests/context_query_tests.cpp)
    target_link_libraries(teaql_context_query_tests PRIVATE teaql::runtime)
    add_test(NAME context_query_contracts COMMAND teaql_context_query_tests)
    set_tests_properties(context_query_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_entity_service_tests tests/entity_service_tests.cpp)
    target_link_libraries(teaql_entity_service_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_service_contracts COMMAND teaql_entity_service_tests)
    add_executable(teaql_enhanced_entities_tests tests/enhanced_entities_tests.cpp)
    target_link_libraries(teaql_enhanced_entities_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME enhanced_entities_contracts COMMAND teaql_enhanced_entities_tests)
    add_executable(teaql_runtime_move_only_tests tests/runtime_move_only_tests.cpp)
    target_link_libraries(teaql_runtime_move_only_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_move_only_query_contracts COMMAND teaql_runtime_move_only_tests)
    add_executable(teaql_runtime_completion_tests tests/runtime_completion_tests.cpp)
    target_link_libraries(teaql_runtime_completion_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_generic_completion_contracts COMMAND teaql_runtime_completion_tests)
    add_executable(teaql_enhanced_entities_probe tests/parity/enhanced_entities_probe.cpp)
    target_link_libraries(teaql_enhanced_entities_probe PRIVATE teaql::provider_sqlite)
    set_tests_properties(enhanced_entities_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_entity_stream_tests tests/entity_stream_tests.cpp)
    target_link_libraries(teaql_entity_stream_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_stream_contracts COMMAND teaql_entity_stream_tests)
    add_executable(teaql_id_set_page_tests tests/id_set_page_tests.cpp)
    target_link_libraries(teaql_id_set_page_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME id_set_page_contracts COMMAND teaql_id_set_page_tests)
    add_executable(teaql_id_set_page_probe tests/parity/id_set_page_probe.cpp)
    target_link_libraries(teaql_id_set_page_probe PRIVATE teaql::runtime)
    set_tests_properties(id_set_page_contracts PROPERTIES TIMEOUT 30)
    set_tests_properties(entity_stream_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_entity_service_probe tests/parity/entity_service_probe.cpp)
    target_link_libraries(teaql_entity_service_probe PRIVATE teaql::runtime)
    add_executable(teaql_typed_aggregate_scope_probe tests/parity/typed_aggregate_scope_probe.cpp)
    target_link_libraries(teaql_typed_aggregate_scope_probe PRIVATE teaql::runtime)
    add_executable(teaql_typed_aggregate_tests tests/typed_aggregate_tests.cpp)
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
    add_executable(teaql_entity_decoder_tests tests/entity_decoder_tests.cpp)
    target_link_libraries(teaql_entity_decoder_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME entity_decoder_contracts COMMAND teaql_entity_decoder_tests)
    add_executable(teaql_entity_decoder_probe tests/parity/entity_decoder_probe.cpp)
    target_link_libraries(teaql_entity_decoder_probe PRIVATE teaql::runtime)
    add_executable(teaql_entity_runtime_probe tests/parity/entity_runtime_probe.cpp)
    target_link_libraries(teaql_entity_runtime_probe PRIVATE teaql::runtime)
    add_executable(teaql_entity_runtime_tests tests/entity_runtime_tests.cpp)
    target_link_libraries(teaql_entity_runtime_tests PRIVATE teaql::runtime)
    add_test(NAME entity_runtime_contracts COMMAND teaql_entity_runtime_tests)
    add_executable(teaql_runtime_graph_model_probe tests/parity/runtime_graph_model_probe.cpp)
    add_executable(teaql_graph_repository_probe tests/parity/graph_repository_probe.cpp)
    target_link_libraries(teaql_graph_repository_probe PRIVATE teaql::runtime)
    add_test(NAME graph_repository_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_graph_repository.py
        --cpp $<TARGET_FILE:teaql_graph_repository_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_runtime_graph_model_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_graph_model_tests tests/runtime_graph_model_tests.cpp)
    target_link_libraries(teaql_runtime_graph_model_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_graph_model_contracts COMMAND teaql_runtime_graph_model_tests)
    add_test(NAME runtime_graph_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_graph_models.py
        --cpp $<TARGET_FILE:teaql_runtime_graph_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_graph_probe tests/parity/runtime_graph_probe.cpp)
    target_link_libraries(teaql_runtime_graph_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_graph_tests tests/runtime_graph_tests.cpp)
    target_link_libraries(teaql_runtime_graph_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_graph_contracts COMMAND teaql_runtime_graph_tests)
    add_executable(teaql_context_event_probe tests/parity/context_event_probe.cpp)
    target_link_libraries(teaql_context_event_probe PRIVATE teaql::runtime)
    add_executable(teaql_context_event_tests tests/context_event_tests.cpp)
    target_link_libraries(teaql_context_event_tests PRIVATE teaql::runtime)
    add_test(NAME context_event_contracts COMMAND teaql_context_event_tests)
    add_executable(teaql_telemetry_tests tests/telemetry_tests.cpp)
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
    add_executable(teaql_context_logging_tests tests/context_logging_tests.cpp)
    target_link_libraries(teaql_context_logging_tests PRIVATE teaql::runtime)
    add_test(NAME context_logging_contracts COMMAND teaql_context_logging_tests)
    add_executable(teaql_context_logging_probe tests/parity/context_logging_probe.cpp)
    target_link_libraries(teaql_context_logging_probe PRIVATE teaql::runtime)
    add_executable(teaql_log_manager_tests tests/log_manager_tests.cpp)
    target_link_libraries(teaql_log_manager_tests PRIVATE teaql::runtime)
    add_test(NAME log_manager_contracts COMMAND teaql_log_manager_tests)
    add_executable(teaql_log_manager_probe tests/parity/log_manager_probe.cpp)
    target_link_libraries(teaql_log_manager_probe PRIVATE teaql::runtime)
    add_executable(teaql_log_formatter_tests tests/log_formatter_tests.cpp)
    target_link_libraries(teaql_log_formatter_tests PRIVATE teaql::runtime)
    add_test(NAME log_formatter_contracts COMMAND teaql_log_formatter_tests)
    add_executable(teaql_log_formatter_probe tests/parity/log_formatter_probe.cpp)
    target_link_libraries(teaql_log_formatter_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_log_tests tests/runtime_log_tests.cpp)
    target_link_libraries(teaql_runtime_log_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_log_contracts COMMAND teaql_runtime_log_tests)
    add_executable(teaql_runtime_log_probe tests/parity/runtime_log_probe.cpp)
    target_link_libraries(teaql_runtime_log_probe PRIVATE teaql::runtime)
    add_executable(teaql_audit_event_tests tests/audit_event_tests.cpp)
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
    add_executable(teaql_schema_runtime_tests tests/schema_runtime_tests.cpp)
    target_link_libraries(teaql_schema_runtime_tests PRIVATE teaql::runtime)
    add_test(NAME schema_runtime_contracts COMMAND teaql_schema_runtime_tests)
    add_executable(teaql_internal_id_tests tests/internal_id_tests.cpp)
    target_link_libraries(teaql_internal_id_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME internal_id_contracts COMMAND teaql_internal_id_tests)
    add_executable(teaql_internal_id_probe tests/parity/internal_id_probe.cpp)
    target_link_libraries(teaql_internal_id_probe PRIVATE teaql::runtime)
    add_executable(teaql_business_service_tests tests/business_service_tests.cpp)
    target_link_libraries(teaql_business_service_tests PRIVATE teaql::runtime teaql::provider_sqlite)
    add_test(NAME business_service_contracts COMMAND teaql_business_service_tests)
    add_executable(teaql_business_service_probe tests/parity/business_service_probe.cpp)
    target_link_libraries(teaql_business_service_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_business_id_tests tests/runtime_business_id_tests.cpp)
    target_link_libraries(teaql_runtime_business_id_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_business_id_contracts COMMAND teaql_runtime_business_id_tests)
    add_executable(teaql_runtime_business_id_probe tests/parity/runtime_business_id_probe.cpp)
    target_link_libraries(teaql_runtime_business_id_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_registry_tests tests/runtime_registry_tests.cpp)
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
    add_executable(teaql_runtime_check_tests tests/runtime_check_tests.cpp)
    target_link_libraries(teaql_runtime_check_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_check_error_contracts COMMAND teaql_runtime_check_tests)
    add_executable(teaql_error_generic_tests tests/error_generic_tests.cpp)
    target_link_libraries(teaql_error_generic_tests PRIVATE teaql::runtime)
    add_test(NAME error_generic_context_contracts COMMAND teaql_error_generic_tests)
    add_executable(teaql_sql_move_only_tests tests/sql_move_only_tests.cpp)
    target_link_libraries(teaql_sql_move_only_tests PRIVATE teaql::runtime)
    add_test(NAME sql_move_only_context_contracts COMMAND teaql_sql_move_only_tests)
    add_test(NAME runtime_error_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_error_debug.py
        --cpp $<TARGET_FILE:teaql_runtime_check_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_check_probe tests/parity/runtime_check_probe.cpp)
    target_link_libraries(teaql_runtime_check_probe PRIVATE teaql::runtime)
    add_executable(teaql_runtime_context_tests tests/runtime_context_tests.cpp)
    target_link_libraries(teaql_runtime_context_tests PRIVATE teaql::runtime)
    add_test(NAME runtime_context_resource_contracts COMMAND teaql_runtime_context_tests)
    add_executable(teaql_context_model_tests tests/context_model_tests.cpp)
    target_link_libraries(teaql_context_model_tests PRIVATE teaql::runtime)
    add_test(NAME context_model_contracts COMMAND teaql_context_model_tests)
    add_executable(teaql_context_model_probe tests/parity/context_model_probe.cpp)
    target_link_libraries(teaql_context_model_probe PRIVATE teaql::runtime)
    add_test(NAME context_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_model.py
        --cpp $<TARGET_FILE:teaql_context_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_runtime_context_probe tests/parity/runtime_context_probe.cpp)
    target_link_libraries(teaql_runtime_context_probe PRIVATE teaql::runtime)
    add_executable(teaql_provider_sqlite_business_id_tests tests/provider_sqlite_business_id_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_business_id_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME provider_sqlite_business_id_contracts COMMAND teaql_provider_sqlite_business_id_tests)
    set_tests_properties(provider_sqlite_business_id_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_provider_sqlite_id_tests tests/provider_sqlite_id_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_id_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME provider_sqlite_id_contracts COMMAND teaql_provider_sqlite_id_tests)
    set_tests_properties(provider_sqlite_id_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sqlite_transport_probe tests/parity/sqlite_transport_probe.cpp)
    target_link_libraries(teaql_sqlite_transport_probe PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_executable(teaql_provider_sqlite_transport_tests tests/provider_sqlite_transport_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_transport_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME provider_sqlite_transport_contracts COMMAND teaql_provider_sqlite_transport_tests)
    set_tests_properties(provider_sqlite_transport_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_provider_sqlite_decoding_tests tests/provider_sqlite_decoding_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_decoding_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME provider_sqlite_decoding_contracts COMMAND teaql_provider_sqlite_decoding_tests)
    add_executable(teaql_sqlite_decoding_probe tests/parity/sqlite_decoding_probe.cpp)
    target_link_libraries(teaql_sqlite_decoding_probe PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_executable(teaql_sqlite_bindings_probe tests/parity/sqlite_bindings_probe.cpp)
    target_link_libraries(teaql_sqlite_bindings_probe PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_executable(teaql_provider_sqlite_bindings_tests tests/provider_sqlite_bindings_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_bindings_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME provider_sqlite_binding_contracts COMMAND teaql_provider_sqlite_bindings_tests)
    add_executable(teaql_provider_sqlite_model_tests tests/provider_sqlite_model_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_model_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_model_contracts COMMAND teaql_provider_sqlite_model_tests)
    add_executable(teaql_provider_sqlite_error_tests tests/provider_sqlite_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_error_contracts COMMAND teaql_provider_sqlite_error_tests)
    add_executable(teaql_provider_sqlite_driver_error_tests tests/provider_sqlite_driver_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_driver_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_driver_error_contracts COMMAND teaql_provider_sqlite_driver_error_tests)
    add_executable(teaql_sqlite_error_model_probe tests/parity/sqlite_error_model_probe.cpp)
    target_link_libraries(teaql_sqlite_error_model_probe PRIVATE teaql::provider_sqlite)
    add_executable(teaql_sqlite_native_diagnostics_probe tests/parity/sqlite_native_diagnostics_probe.cpp)
    target_link_libraries(teaql_sqlite_native_diagnostics_probe PRIVATE teaql::provider_sqlite)
    add_executable(teaql_provider_sqlite_simple_error_tests tests/provider_sqlite_simple_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_simple_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_simple_error_contracts COMMAND teaql_provider_sqlite_simple_error_tests)
    add_executable(teaql_provider_sqlite_boxed_error_tests tests/provider_sqlite_boxed_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_boxed_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_boxed_error_contracts COMMAND teaql_provider_sqlite_boxed_error_tests)
    add_executable(teaql_provider_sqlite_from_sql_error_tests tests/provider_sqlite_from_sql_error_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_from_sql_error_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_from_sql_error_contracts COMMAND teaql_provider_sqlite_from_sql_error_tests)
    add_executable(teaql_sqlite_from_sql_error_probe tests/parity/sqlite_from_sql_error_probe.cpp)
    target_link_libraries(teaql_sqlite_from_sql_error_probe PRIVATE teaql::provider_sqlite)
    add_executable(teaql_sqlite_simple_error_probe tests/parity/sqlite_simple_error_probe.cpp)
    target_link_libraries(teaql_sqlite_simple_error_probe PRIVATE teaql::provider_sqlite)
    add_executable(teaql_sql_stream_probe tests/parity/sql_stream_probe.cpp)
    target_link_libraries(teaql_sql_stream_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_stream_tests tests/sql_stream_tests.cpp)
    target_link_libraries(teaql_sql_stream_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME sql_stream_execution_contracts COMMAND teaql_sql_stream_tests)
    set_tests_properties(sql_stream_execution_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sql_transaction_tests tests/sql_transaction_tests.cpp)
    target_link_libraries(teaql_sql_transaction_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME sql_transaction_execution_contracts COMMAND teaql_sql_transaction_tests)
    set_tests_properties(sql_transaction_execution_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sql_execution_tests tests/sql_execution_tests.cpp)
    target_link_libraries(teaql_sql_execution_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME sql_mutation_execution_contracts COMMAND teaql_sql_execution_tests)
    set_tests_properties(sql_mutation_execution_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sql_execution_probe tests/parity/sql_execution_probe.cpp)
    target_link_libraries(teaql_sql_execution_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_query_probe tests/parity/sql_query_probe.cpp)
    target_link_libraries(teaql_sql_query_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_query_tests tests/sql_query_tests.cpp)
    target_link_libraries(teaql_sql_query_tests PRIVATE teaql::sql)
    add_test(NAME sql_query_executor_contracts COMMAND teaql_sql_query_tests)
    set_tests_properties(sql_query_executor_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_sql_transport_probe tests/parity/sql_transport_probe.cpp)
    target_link_libraries(teaql_sql_transport_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_executor_error_probe tests/parity/sql_executor_error_probe.cpp)
    target_link_libraries(teaql_sql_executor_error_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_transport_tests tests/sql_transport_tests.cpp)
    target_link_libraries(teaql_sql_transport_tests PRIVATE teaql::sql)
    add_test(NAME sql_transport_contracts COMMAND teaql_sql_transport_tests)
    set_tests_properties(sql_transport_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_provider_sqlite_tests tests/provider_sqlite_tests.cpp)
    target_link_libraries(teaql_provider_sqlite_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME provider_sqlite_dialect_contracts COMMAND teaql_provider_sqlite_tests)
    add_executable(teaql_sql_schema_probe tests/parity/sql_schema_probe.cpp)
    target_link_libraries(teaql_sql_schema_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_mutation_tests tests/sql_mutation_tests.cpp)
    target_link_libraries(teaql_sql_mutation_tests PRIVATE teaql::provider_sqlite SQLite::SQLite3)
    add_test(NAME sql_mutation_contracts COMMAND teaql_sql_mutation_tests)
    add_executable(teaql_sql_mutation_probe tests/parity/sql_mutation_probe.cpp)
    target_link_libraries(teaql_sql_mutation_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_dialect_tests tests/sql_dialect_tests.cpp)
    target_link_libraries(teaql_sql_dialect_tests PRIVATE teaql::sql)
    add_test(NAME sql_dialect_contracts COMMAND teaql_sql_dialect_tests)
    add_executable(teaql_sql_dialect_helper_probe tests/parity/sql_dialect_helper_probe.cpp)
    target_link_libraries(teaql_sql_dialect_helper_probe PRIVATE teaql::sql)
    add_test(NAME sql_dialect_helpers_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_dialect_helpers.py
        --cpp $<TARGET_FILE:teaql_sql_dialect_helper_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_dialect_in_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_dialect_in.py
        --cpp $<TARGET_FILE:teaql_sql_dialect_helper_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_sql_dialect_probe tests/parity/sql_dialect_probe.cpp)
    target_link_libraries(teaql_sql_dialect_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_model_tests tests/sql_model_tests.cpp)
    target_link_libraries(teaql_sql_model_tests PRIVATE teaql::provider_sqlite)
    add_test(NAME sql_model_contracts COMMAND teaql_sql_model_tests)
    add_executable(teaql_sql_model_api_tests tests/sql_model_api_tests.cpp)
    target_link_libraries(teaql_sql_model_api_tests PRIVATE teaql::sql)
    add_test(NAME sql_model_api_contracts COMMAND teaql_sql_model_api_tests)
    add_executable(teaql_sql_model_api_probe tests/parity/sql_model_api_probe.cpp)
    target_link_libraries(teaql_sql_model_api_probe PRIVATE teaql::sql)
    add_executable(teaql_sql_types_probe tests/parity/sql_types_probe.cpp)
    target_link_libraries(teaql_sql_types_probe PRIVATE teaql::sql)
    add_executable(teaql_async_tests tests/async_tests.cpp)
    target_link_libraries(teaql_async_tests PRIVATE teaql::memory Threads::Threads)
    add_test(NAME async_context_contracts COMMAND teaql_async_tests)
    set_tests_properties(async_context_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_executor_tests tests/executor_tests.cpp)
    target_link_libraries(teaql_executor_tests PRIVATE teaql::data_service)
    add_test(NAME executor_spi_contracts COMMAND teaql_executor_tests)
    add_executable(teaql_executor_defaults_scope_probe tests/parity/executor_defaults_scope_probe.cpp)
    target_link_libraries(teaql_executor_defaults_scope_probe PRIVATE teaql::data_service)
    add_test(NAME executor_defaults_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_executor_defaults_scope.py
        --cpp $<TARGET_FILE:teaql_executor_defaults_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    set_tests_properties(executor_spi_contracts PROPERTIES TIMEOUT 30)
    add_executable(teaql_data_service_tests tests/data_service_tests.cpp)
    target_link_libraries(teaql_data_service_tests PRIVATE teaql::data_service)
    add_test(NAME data_service_contracts COMMAND teaql_data_service_tests)
    add_executable(teaql_data_service_model_tests tests/data_service_model_tests.cpp)
    target_link_libraries(teaql_data_service_model_tests PRIVATE teaql::data_service)
    add_test(NAME data_service_model_contracts COMMAND teaql_data_service_model_tests)
    add_executable(teaql_system_time_tests tests/system_time_tests.cpp)
    target_link_libraries(teaql_system_time_tests PRIVATE teaql::data_service)
    add_test(NAME system_time_contracts COMMAND teaql_system_time_tests)
    add_executable(teaql_system_time_arithmetic_scope_tests tests/system_time_arithmetic_scope_tests.cpp)
    target_link_libraries(teaql_system_time_arithmetic_scope_tests PRIVATE teaql::data_service)
    add_test(NAME system_time_arithmetic_scope_contracts COMMAND teaql_system_time_arithmetic_scope_tests)
    add_executable(teaql_system_time_arithmetic_scope_probe tests/parity/system_time_arithmetic_scope_probe.cpp)
    target_link_libraries(teaql_system_time_arithmetic_scope_probe PRIVATE teaql::data_service)
    add_test(NAME system_time_arithmetic_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_system_time_arithmetic_scope.py
        --cpp $<TARGET_FILE:teaql_system_time_arithmetic_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_system_time_probe tests/parity/system_time_probe.cpp)
    add_executable(teaql_system_time_error_scope_probe tests/parity/system_time_error_scope_probe.cpp)
    target_link_libraries(teaql_system_time_error_scope_probe PRIVATE teaql::data_service)
    add_test(NAME system_time_error_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_system_time_error_scope.py
        --cpp $<TARGET_FILE:teaql_system_time_error_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_system_time_probe PRIVATE teaql::data_service)
    add_executable(teaql_data_service_scope_tests tests/data_service_scope_tests.cpp)
    target_link_libraries(teaql_data_service_scope_tests PRIVATE teaql::data_service)
    add_test(NAME data_service_scope_contracts COMMAND teaql_data_service_scope_tests)
    add_executable(teaql_data_service_model_scope_probe tests/parity/data_service_model_scope_probe.cpp)
    target_link_libraries(teaql_data_service_model_scope_probe PRIVATE teaql::data_service)
    add_test(NAME data_service_model_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_data_service_model_scope.py
        --cpp $<TARGET_FILE:teaql_data_service_model_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_data_service_model_probe tests/parity/data_service_model_probe.cpp)
    target_link_libraries(teaql_data_service_model_probe PRIVATE teaql::data_service)
    add_executable(teaql_sql_log_probe tests/parity/sql_log_probe.cpp)
    target_link_libraries(teaql_sql_log_probe PRIVATE teaql::data_service)
    add_executable(teaql_sql_projection_scope_tests tests/sql_projection_scope_tests.cpp)
    target_link_libraries(teaql_sql_projection_scope_tests PRIVATE teaql::data_service)
    add_test(NAME sql_projection_scope_contracts COMMAND teaql_sql_projection_scope_tests)
    add_executable(teaql_sql_projection_scope_probe tests/parity/sql_projection_scope_probe.cpp)
    target_link_libraries(teaql_sql_projection_scope_probe PRIVATE teaql::data_service)
    add_test(NAME sql_projection_scope_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_projection_scope.py
        --cpp $<TARGET_FILE:teaql_sql_projection_scope_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_decimal_tests tests/decimal_tests.cpp)
    target_link_libraries(teaql_decimal_tests PRIVATE teaql::core)
    add_test(NAME decimal_contracts COMMAND teaql_decimal_tests)
    add_executable(teaql_decimal_inline_tests tests/decimal_inline_tests.cpp)
    target_link_libraries(teaql_decimal_inline_tests PRIVATE teaql::core)
    add_test(NAME decimal_inline_contracts COMMAND teaql_decimal_inline_tests)
    add_executable(teaql_core_aux_debug_tests tests/core_aux_debug_tests.cpp)
    target_link_libraries(teaql_core_aux_debug_tests PRIVATE teaql::core)
    add_test(NAME core_aux_debug_contracts COMMAND teaql_core_aux_debug_tests)
    add_executable(teaql_core_aux_debug_probe tests/parity/core_aux_debug_probe.cpp)
    target_link_libraries(teaql_core_aux_debug_probe PRIVATE teaql::core)
    add_executable(teaql_decimal_inline_probe tests/parity/decimal_inline_probe.cpp)
    target_link_libraries(teaql_decimal_inline_probe PRIVATE teaql::core)
    add_executable(teaql_decimal_probe tests/parity/decimal_probe.cpp)
    target_link_libraries(teaql_decimal_probe PRIVATE teaql::core)
    add_executable(teaql_business_id_tests tests/business_id_tests.cpp)
    target_link_libraries(teaql_business_id_tests PRIVATE teaql::core)
    add_test(NAME business_id_contracts COMMAND teaql_business_id_tests)
    add_executable(teaql_business_id_probe tests/parity/business_id_probe.cpp)
    target_link_libraries(teaql_business_id_probe PRIVATE teaql::core)
    add_executable(teaql_presentation_tests tests/presentation_tests.cpp)
    target_link_libraries(teaql_presentation_tests PRIVATE teaql::core)
    add_test(NAME presentation_contracts COMMAND teaql_presentation_tests)
    add_executable(teaql_presentation_probe tests/parity/presentation_probe.cpp)
    target_link_libraries(teaql_presentation_probe PRIVATE teaql::core)
    add_executable(teaql_request_search_tests tests/request_search_tests.cpp)
    target_link_libraries(teaql_request_search_tests PRIVATE teaql::core)
    add_test(NAME request_search_contracts COMMAND teaql_request_search_tests)
    add_executable(teaql_original_core_search_tests tests/parity/teaql-core/dynamic_search.cpp)
    add_executable(teaql_original_core_functional_tests tests/parity/teaql-core/functional_contracts.cpp)
    add_executable(teaql_original_core_mapping_tests tests/parity/teaql-core/entity_mapping.cpp)
    add_executable(teaql_original_core_composition_tests tests/parity/teaql-core/composition_contracts.cpp)
    add_executable(teaql_original_core_value_tests tests/parity/teaql-core/value_contracts.cpp)
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
    add_executable(teaql_entity_registration_api_tests tests/entity_registration_api_tests.cpp)
    target_link_libraries(teaql_entity_registration_api_tests PRIVATE teaql::runtime)
    add_test(NAME entity_registration_api_contracts COMMAND teaql_entity_registration_api_tests)
    add_executable(teaql_entity_registration_api_probe tests/parity/entity_registration_api_probe.cpp)
    target_link_libraries(teaql_entity_registration_api_probe PRIVATE teaql::runtime)
    add_test(NAME entity_registration_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_registration_api.py
        --cpp $<TARGET_FILE:teaql_entity_registration_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_base_entity_api_tests tests/base_entity_api_tests.cpp)
    target_link_libraries(teaql_base_entity_api_tests PRIVATE teaql::runtime)
    add_test(NAME base_entity_api_contracts COMMAND teaql_base_entity_api_tests)
    add_executable(teaql_base_entity_api_probe tests/parity/base_entity_api_probe.cpp)
    target_link_libraries(teaql_base_entity_api_probe PRIVATE teaql::runtime)
    add_test(NAME base_entity_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_base_entity_api.py
        --cpp $<TARGET_FILE:teaql_base_entity_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_entity_debug_tests tests/entity_debug_tests.cpp)
    target_link_libraries(teaql_entity_debug_tests PRIVATE teaql::runtime)
    add_test(NAME entity_debug_contracts COMMAND teaql_entity_debug_tests)
    add_executable(teaql_entity_debug_probe tests/parity/entity_debug_probe.cpp)
    target_link_libraries(teaql_entity_debug_probe PRIVATE teaql::runtime)
    add_test(NAME entity_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_debug.py
        --cpp $<TARGET_FILE:teaql_entity_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_entity_owned_api_tests tests/entity_owned_api_tests.cpp)
    target_link_libraries(teaql_entity_owned_api_tests PRIVATE teaql::runtime)
    add_test(NAME entity_owned_api_contracts COMMAND teaql_entity_owned_api_tests)
    add_executable(teaql_entity_defaults_api_probe tests/parity/entity_defaults_api_probe.cpp)
    target_link_libraries(teaql_entity_defaults_api_probe PRIVATE teaql::runtime)
    add_test(NAME entity_defaults_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_defaults_api.py
        --cpp $<TARGET_FILE:teaql_entity_defaults_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_chrono_value_tests tests/chrono_value_tests.cpp)
    target_link_libraries(teaql_chrono_value_tests PRIVATE teaql::runtime)
    add_test(NAME chrono_value_contracts COMMAND teaql_chrono_value_tests)
    add_executable(teaql_chrono_value_probe tests/parity/chrono_value_probe.cpp)
    target_link_libraries(teaql_chrono_value_probe PRIVATE teaql::runtime)
    add_test(NAME chrono_value_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_chrono_value.py
        --cpp $<TARGET_FILE:teaql_chrono_value_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_entity_expression_tests tests/entity_expression_tests.cpp)
    target_link_libraries(teaql_entity_expression_tests PRIVATE teaql::core teaql::memory)
    add_test(NAME entity_expression_contracts COMMAND teaql_entity_expression_tests)
    add_executable(teaql_search_probe tests/parity/search_probe.cpp)
    target_link_libraries(teaql_search_probe PRIVATE teaql::core)
    add_executable(teaql_value_probe tests/parity/value_probe.cpp)
    target_link_libraries(teaql_value_probe PRIVATE teaql::core teaql::memory)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME provider_sqlite_schema_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_schema.py --cpp $<TARGET_FILE:teaql_sqlite_schema_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_base_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_base.py --cpp $<TARGET_FILE:teaql_runtime_base_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME context_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_context_query.py --cpp $<TARGET_FILE:teaql_context_query_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME relation_execution_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_execution.py --cpp $<TARGET_FILE:teaql_relation_execution_probe>
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
    add_test(NAME relation_enhancement_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_enhancement.py --cpp $<TARGET_FILE:teaql_relation_enhancement_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME relation_hydration_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_relation_hydration.py --cpp $<TARGET_FILE:teaql_relation_hydration_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_service_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_service.py --cpp $<TARGET_FILE:teaql_entity_service_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_query_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_query.py --cpp $<TARGET_FILE:teaql_entity_query_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_query_boundary_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_query_boundaries.py
        --cpp $<TARGET_FILE:teaql_entity_query_probe>
        "--profile=$<IF:$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>,release,debug>"
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_stream_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_stream.py --cpp $<TARGET_FILE:teaql_entity_stream_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME id_set_page_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_id_set_page.py --cpp $<TARGET_FILE:teaql_id_set_page_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME enhanced_entities_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_enhanced_entities.py --cpp $<TARGET_FILE:teaql_enhanced_entities_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_decoder_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_decoder.py --cpp $<TARGET_FILE:teaql_entity_decoder_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME entity_runtime_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_entity_runtime.py --cpp $<TARGET_FILE:teaql_entity_runtime_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME runtime_graph_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_graph.py --cpp $<TARGET_FILE:teaql_runtime_graph_probe>
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_internal_id.py --cpp $<TARGET_FILE:teaql_internal_id_probe>
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_business_id.py --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_provider_id_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_id.py --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_provider_transaction_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_transaction.py --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sqlite_provider_transport_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sqlite_transport.py --cpp $<TARGET_FILE:teaql_sqlite_transport_probe>
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_executor_error.py
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_types.py --cpp $<TARGET_FILE:teaql_sql_types_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_log_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_log.py --cpp $<TARGET_FILE:teaql_sql_log_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME system_time_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_system_time.py
        --cpp $<TARGET_FILE:teaql_system_time_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME data_service_model_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_data_service_model.py
        --cpp $<TARGET_FILE:teaql_data_service_model_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_test(NAME sql_model_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_sql_model_api.py
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_business_id.py --cpp $<TARGET_FILE:teaql_business_id_probe>
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
    add_executable(teaql_value_model_tests tests/value_model_tests.cpp)
    target_link_libraries(teaql_value_model_tests PRIVATE teaql::core teaql::memory)
    add_test(NAME value_model_contracts COMMAND teaql_value_model_tests)
    add_executable(teaql_value_owned_api_tests tests/value_owned_api_tests.cpp)
    target_link_libraries(teaql_value_owned_api_tests PRIVATE teaql::runtime)
    add_test(NAME value_owned_api_contracts COMMAND teaql_value_owned_api_tests)
    add_executable(teaql_value_owned_api_probe tests/parity/value_owned_api_probe.cpp)
    target_link_libraries(teaql_value_owned_api_probe PRIVATE teaql::runtime)
    add_test(NAME value_owned_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_value_owned_api.py
        --cpp $<TARGET_FILE:teaql_value_owned_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_core_tests tests/core_tests.cpp)
    add_executable(teaql_meta_api_tests tests/meta_api_tests.cpp)
    add_executable(teaql_query_debug_tests tests/query_debug_tests.cpp)
    add_executable(teaql_mutation_debug_tests tests/mutation_debug_tests.cpp)
    add_executable(teaql_request_api_tests tests/request_api_tests.cpp)
    add_executable(teaql_request_helpers_tests tests/request_helpers_tests.cpp)
    add_executable(teaql_core_graph_api_tests tests/core_graph_api_tests.cpp)
    add_executable(teaql_presentation_api_tests tests/presentation_api_tests.cpp)
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_request_helpers.py
        --cpp $<TARGET_FILE:teaql_request_helpers_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_request_api_tests PRIVATE teaql::runtime)
    add_test(NAME request_api_contracts COMMAND teaql_request_api_tests)
    add_executable(teaql_request_api_probe tests/parity/request_api_probe.cpp)
    target_link_libraries(teaql_request_api_probe PRIVATE teaql::runtime)
    add_test(NAME request_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_request_api.py
        --cpp $<TARGET_FILE:teaql_request_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_mutation_debug_tests PRIVATE teaql::runtime)
    add_test(NAME mutation_debug_contracts COMMAND teaql_mutation_debug_tests)
    add_executable(teaql_mutation_debug_probe tests/parity/mutation_debug_probe.cpp)
    target_link_libraries(teaql_mutation_debug_probe PRIVATE teaql::runtime)
    add_test(NAME mutation_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_mutation_debug.py
        --cpp $<TARGET_FILE:teaql_mutation_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_query_debug_tests PRIVATE teaql::runtime)
    add_test(NAME query_debug_contracts COMMAND teaql_query_debug_tests)
    add_executable(teaql_query_debug_probe tests/parity/query_debug_probe.cpp)
    target_link_libraries(teaql_query_debug_probe PRIVATE teaql::runtime)
    add_test(NAME query_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_query_debug.py
        --cpp $<TARGET_FILE:teaql_query_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_meta_api_tests PRIVATE teaql::runtime)
    add_test(NAME meta_api_contracts COMMAND teaql_meta_api_tests)
    add_executable(teaql_meta_api_probe tests/parity/meta_api_probe.cpp)
    target_link_libraries(teaql_meta_api_probe PRIVATE teaql::runtime)
    add_test(NAME meta_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_meta_api.py
        --cpp $<TARGET_FILE:teaql_meta_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_timestamp_api_tests tests/timestamp_api_tests.cpp)
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_value_api.py
        --cpp $<TARGET_FILE:teaql_value_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_core_tests PRIVATE teaql::core teaql::memory)
    add_test(NAME core_contracts COMMAND teaql_core_tests)
    add_executable(teaql_core_model_tests tests/core_model_tests.cpp)
    add_executable(teaql_eval_tests tests/eval_tests.cpp)
    target_link_libraries(teaql_eval_tests PRIVATE teaql::runtime)
    add_test(NAME eval_context_contracts COMMAND teaql_eval_tests)
    add_executable(teaql_eval_probe tests/parity/eval_probe.cpp)
    target_link_libraries(teaql_eval_probe PRIVATE teaql::runtime)
    add_test(NAME eval_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_eval.py
        --cpp $<TARGET_FILE:teaql_eval_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_mutation_model_api_tests tests/mutation_model_api_tests.cpp)
    add_executable(teaql_mutation_model_api_probe tests/parity/mutation_model_api_probe.cpp)
    target_link_libraries(teaql_mutation_model_api_probe PRIVATE teaql::runtime)
    add_test(NAME mutation_model_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_mutation_model_api.py
        --cpp $<TARGET_FILE:teaql_mutation_model_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_mutation_model_api_tests PRIVATE teaql::runtime)
    add_test(NAME mutation_model_api_contracts COMMAND teaql_mutation_model_api_tests)
    add_executable(teaql_core_api_tests tests/core_api_tests.cpp)
    add_executable(teaql_core_api_probe tests/parity/core_api_probe.cpp)
    target_link_libraries(teaql_core_api_probe PRIVATE teaql::runtime)
    add_test(NAME core_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_core_api.py
        --cpp $<TARGET_FILE:teaql_core_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_core_api_tests PRIVATE teaql::runtime)
    add_test(NAME core_api_contracts COMMAND teaql_core_api_tests)
    target_link_libraries(teaql_core_model_tests PRIVATE teaql::core teaql::memory)
    add_test(NAME core_model_contracts COMMAND teaql_core_model_tests)
    add_executable(teaql_core_collection_tests tests/core_collection_tests.cpp)
    add_executable(teaql_smart_list_api_tests tests/smart_list_api_tests.cpp)
    add_executable(teaql_model_debug_tests tests/model_debug_tests.cpp)
    add_executable(teaql_datatype_debug_tests tests/datatype_debug_tests.cpp)
    target_link_libraries(teaql_datatype_debug_tests PRIVATE teaql::runtime)
    add_test(NAME datatype_debug_contracts COMMAND teaql_datatype_debug_tests)
    add_executable(teaql_datatype_debug_probe tests/parity/datatype_debug_probe.cpp)
    target_link_libraries(teaql_datatype_debug_probe PRIVATE teaql::runtime)
    add_test(NAME datatype_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_datatype_debug.py
        --cpp $<TARGET_FILE:teaql_datatype_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_model_debug_tests PRIVATE teaql::runtime)
    add_test(NAME model_debug_contracts COMMAND teaql_model_debug_tests)
    add_executable(teaql_model_debug_probe tests/parity/model_debug_probe.cpp)
    target_link_libraries(teaql_model_debug_probe PRIVATE teaql::runtime)
    add_test(NAME model_debug_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_model_debug.py
        --cpp $<TARGET_FILE:teaql_model_debug_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_smart_list_api_probe tests/parity/smart_list_api_probe.cpp)
    target_link_libraries(teaql_smart_list_api_probe PRIVATE teaql::runtime)
    add_test(NAME smart_list_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_smart_list_api.py
        --cpp $<TARGET_FILE:teaql_smart_list_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_smart_list_api_tests PRIVATE teaql::runtime)
    add_test(NAME smart_list_api_contracts COMMAND teaql_smart_list_api_tests)
    add_executable(teaql_compact_row_api_tests tests/compact_row_api_tests.cpp)
    add_executable(teaql_compact_row_api_probe tests/parity/compact_row_api_probe.cpp)
    target_link_libraries(teaql_compact_row_api_probe PRIVATE teaql::runtime)
    add_test(NAME compact_row_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_compact_row_api.py
        --cpp $<TARGET_FILE:teaql_compact_row_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_compact_row_api_tests PRIVATE teaql::runtime)
    add_test(NAME compact_row_api_contracts COMMAND teaql_compact_row_api_tests)
    target_link_libraries(teaql_core_collection_tests PRIVATE teaql::core teaql::memory)
    add_test(NAME core_collection_contracts COMMAND teaql_core_collection_tests)
    add_executable(teaql_query_model_tests tests/query_model_tests.cpp)
    add_executable(teaql_query_api_tests tests/query_api_tests.cpp)
    target_link_libraries(teaql_query_api_tests PRIVATE teaql::runtime)
    add_test(NAME query_api_contracts COMMAND teaql_query_api_tests)
    add_executable(teaql_query_api_probe tests/parity/query_api_probe.cpp)
    target_link_libraries(teaql_query_api_probe PRIVATE teaql::runtime)
    add_test(NAME query_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_query_api.py
        --cpp $<TARGET_FILE:teaql_query_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    target_link_libraries(teaql_query_model_tests PRIVATE teaql::core teaql::memory)
    add_test(NAME query_model_contracts COMMAND teaql_query_model_tests)
    add_executable(teaql_safe_expression_api_tests tests/safe_expression_api_tests.cpp)
    target_link_libraries(teaql_safe_expression_api_tests PRIVATE teaql::runtime)
    add_test(NAME safe_expression_api_contracts COMMAND teaql_safe_expression_api_tests)
    add_executable(teaql_safe_expression_api_probe tests/parity/safe_expression_api_probe.cpp)
    target_link_libraries(teaql_safe_expression_api_probe PRIVATE teaql::runtime)
    add_test(NAME safe_expression_api_rust_parity COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_safe_expression_api.py
        --cpp $<TARGET_FILE:teaql_safe_expression_api_probe>
        "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
    add_executable(teaql_context_memory_tests tests/context_memory_tests.cpp)
    add_executable(teaql_data_store_tests tests/data_store_tests.cpp)
    add_executable(teaql_generated_support_tests tests/generated_support_tests.cpp)
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
    add_executable(teaql_context_reference_tests tests/context_reference_tests.cpp)
    target_link_libraries(teaql_context_reference_tests PRIVATE teaql::runtime)
    add_test(NAME context_reference_contracts COMMAND teaql_context_reference_tests)
    add_test(NAME context_early_global_close
        COMMAND teaql_context_reference_tests early-global-close)
    add_executable(teaql_sqlite_tests tests/sqlite_tests.cpp)
    target_link_libraries(teaql_sqlite_tests PRIVATE teaql::legacy_sqlite)
    add_test(NAME sqlite_contracts COMMAND teaql_sqlite_tests)
    if(TEAQL_BUILD_EXAMPLES)
        add_test(NAME example_context_lifetime COMMAND teaql_context_lifetime)
        add_test(NAME example_sqlite_schema_crud COMMAND teaql_sqlite_schema_crud)
        add_test(NAME example_sqlite_relations_graph COMMAND teaql_sqlite_relations_graph)
        set_tests_properties(example_sqlite_schema_crud example_sqlite_relations_graph PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
        if(Python3_Interpreter_FOUND)
            add_test(NAME runtime_examples_rust_parity COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity/compare_runtime_examples.py
                --schema-crud $<TARGET_FILE:teaql_sqlite_schema_crud>
                --relations-graph $<TARGET_FILE:teaql_sqlite_relations_graph>
                "--cpp-emulator=${CMAKE_CROSSCOMPILING_EMULATOR}")
            set_tests_properties(runtime_examples_rust_parity PROPERTIES ENVIRONMENT "TEAQL_LOG_ENDPOINT=off")
        endif()
    endif()
endif()
