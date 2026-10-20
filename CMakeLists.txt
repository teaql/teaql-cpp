cmake_minimum_required(VERSION 4.4)
project(teaql_cpp VERSION 0.1.0 LANGUAGES CXX)

# Existing implementation files contain no imports. Module interface file sets
# and importing consumers will enable scanning explicitly during migration.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
# Default only to the exact native configuration accepted in the release audit.
# Explicit presets/overrides continue to select their frontend independently.
set(TEAQL_MODULES_DEFAULT OFF)
if(NOT CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
   CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_GENERATOR STREQUAL "Ninja" AND
   CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 23.1.3 AND
   CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES MATCHES "/gcc/[^/]+/16[.]2[.]0/include/c[+][+]")
    set(TEAQL_MODULES_DEFAULT ON)
endif()
option(TEAQL_ENABLE_MODULES "Build the qualified C++20 named-module frontend" ${TEAQL_MODULES_DEFAULT})
if(TEAQL_ENABLE_MODULES)
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "Production TEAQL modules are qualified for native builds; use the header frontend for cross compilation")
    endif()
    if(NOT CMAKE_GENERATOR MATCHES "^Ninja")
        message(FATAL_ERROR "TEAQL modules require the qualified Ninja generator")
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
       CMAKE_CXX_COMPILER_VERSION VERSION_LESS 23 OR
       NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 24)
        message(FATAL_ERROR "The production module frontend is qualified on native Clang23")
    endif()
endif()

find_package(Threads REQUIRED)
find_package(SQLite3 3.37 REQUIRED)
include(GNUInstallDirs)

# Each package publishes its own declarations. Consumers receive lower-layer
# includes through the existing target graph instead of a workspace-wide include.
function(teaql_configure_package target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
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
    add_executable(teaql_generated_query_expression examples/generated_query_expression.cpp)
    target_link_libraries(teaql_generated_query_expression PRIVATE teaql::provider_sqlite)
    add_executable(teaql_custom_checker_behavior examples/custom_checker_behavior.cpp)
    target_link_libraries(teaql_custom_checker_behavior PRIVATE teaql::provider_sqlite)
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
if(TEAQL_ENABLE_MODULES)
    add_subdirectory(modules)
endif()
include(cmake/install.cmake)
