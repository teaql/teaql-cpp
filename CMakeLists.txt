cmake_minimum_required(VERSION 4.4)
project(teaql_cpp VERSION 0.1.0 LANGUAGES CXX)

# Existing implementation files contain no imports. Module interface file sets
# and importing consumers will enable scanning explicitly during migration.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

find_package(Threads REQUIRED)
find_package(SQLite3 3.37 REQUIRED)

# Each package publishes its own declarations. Consumers receive lower-layer
# includes through the existing target graph instead of a workspace-wide include.
function(teaql_configure_package target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_subdirectory(teaql_memory)
add_subdirectory(teaql_async)
add_subdirectory(teaql_core)
add_subdirectory(teaql_data_service)
add_subdirectory(teaql_sql)
add_subdirectory(teaql_runtime)
add_subdirectory(teaql_provider_sqlite)
add_subdirectory(compatibility/teaql_legacy_sqlite)

set(TEAQL_LAYER_METADATA "")
foreach(layer IN ITEMS teaql_core teaql_memory teaql_data_service teaql_sql teaql_runtime)
    # Read direct edges at configuration time. CMP0189 makes TARGET_PROPERTY
    # generator expressions expand link properties transitively in CMake4.1+.
    get_target_property(layer_link ${layer} LINK_LIBRARIES)
    get_target_property(layer_interface ${layer} INTERFACE_LINK_LIBRARIES)
    string(APPEND TEAQL_LAYER_METADATA
        "[${layer}]\nLINK=${layer_link}\nINTERFACE=${layer_interface}\nSOURCES=$<TARGET_PROPERTY:${layer},SOURCES>\n")
endforeach()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/teaql_layer_contracts.txt"
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
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/format_cpp.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Format handwritten C++ with pinned clang-format"
        VERBATIM)
    add_custom_target(format_check
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/format_cpp.py --check
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Check handwritten C++ formatting"
        VERBATIM)
endif()
if(BUILD_TESTING)
    include(tests/CMakeLists.txt)
endif()
