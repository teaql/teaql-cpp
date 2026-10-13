add_library(teaql_provider_sqlite
    src/schema.cpp
    src/schema_model.cpp
    src/schema_unicode.cpp
    src/dialect.cpp
    src/error.cpp
    src/bindings.cpp
    src/decoding.cpp
    src/provider.cpp
    src/dynamic_fields.cpp
    src/driver_error.cpp
    src/native_diagnostics.cpp
    src/simple_error.cpp
    src/from_sql_error.cpp
)
add_library(teaql::provider_sqlite ALIAS teaql_provider_sqlite)
teaql_configure_package(teaql_provider_sqlite)
target_link_libraries(teaql_provider_sqlite PUBLIC teaql::sql teaql::runtime PRIVATE SQLite3::SQLite3)
