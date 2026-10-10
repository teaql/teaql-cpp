add_library(teaql_memory
    src/arena.cpp
)
add_library(teaql::memory ALIAS teaql_memory)
teaql_configure_package(teaql_memory)
target_link_libraries(teaql_memory PUBLIC Threads::Threads)
