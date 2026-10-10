# Retained compatibility adapter, outside the production provider path.
add_library(teaql_legacy_sqlite EXCLUDE_FROM_ALL
    src/sqlite.cpp
    src/legacy_sqlite_compiler.cpp
)
add_library(teaql::legacy_sqlite ALIAS teaql_legacy_sqlite)
teaql_configure_package(teaql_legacy_sqlite)
target_link_libraries(teaql_legacy_sqlite PUBLIC teaql::runtime PRIVATE SQLite3::SQLite3)
