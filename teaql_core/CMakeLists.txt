add_library(teaql_core
    src/query_debug.cpp
    src/debug.cpp
    src/core.cpp
    src/business_id.cpp
    src/web.cpp
    src/xls.cpp
    src/entity.cpp
    src/list.cpp
    src/number_text.cpp
    src/request.cpp
    src/dynamic_search.cpp
    src/value.cpp
    src/decimal.cpp
    src/decimal_arithmetic.cpp
    src/decimal_format.cpp
    src/decimal_json.cpp
    src/json.cpp
    src/time.cpp
    src/row.cpp
    src/expr.cpp
    src/query.cpp
    src/aes_gcm.cpp
)
add_library(teaql::core ALIAS teaql_core)
teaql_configure_package(teaql_core)
target_link_libraries(teaql_core PUBLIC teaql::memory)
