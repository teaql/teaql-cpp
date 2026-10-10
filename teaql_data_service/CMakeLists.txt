add_library(teaql_data_service
    src/sql_log.cpp
)
add_library(teaql::data_service ALIAS teaql_data_service)
teaql_configure_package(teaql_data_service)
target_link_libraries(teaql_data_service PUBLIC teaql::core teaql::async Threads::Threads)
