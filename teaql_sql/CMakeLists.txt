add_library(teaql_sql
    src/types.cpp
    src/bindings.cpp
    src/dialect.cpp
    src/schema.cpp
    src/mutation.cpp
    src/query_plan.cpp
    src/diagnostic.cpp
    src/mutation_execution.cpp
)
add_library(teaql::sql ALIAS teaql_sql)
teaql_configure_package(teaql_sql)
target_link_libraries(teaql_sql PUBLIC teaql::data_service)
