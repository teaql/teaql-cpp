# C++ asynchronous protocol support; no executor or shared owner is introduced.
add_library(teaql_async INTERFACE)
add_library(teaql::async ALIAS teaql_async)
target_compile_features(teaql_async INTERFACE cxx_std_20)
target_include_directories(teaql_async INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(teaql_async INTERFACE teaql::memory)
