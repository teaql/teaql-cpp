#include <teaql/provider_sqlite/model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
#include <type_traits>

using namespace teaql;

static_assert(std::is_default_constructible_v<SqliteDialect>);
static_assert(std::is_copy_constructible_v<SqliteDialect>);
static_assert(detail::domain_copyable<SqliteSchemaProvider>);
static_assert(!std::is_copy_constructible_v<SqliteSchemaProvider>);
static_assert(std::is_nothrow_move_constructible_v<SqliteSchemaProvider>);

static void check(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

static void scoped_protocols() {
    GlobalContext global;
    Ref<SqliteDialect> retained_dialect;
    Ref<SqliteSchemaProvider> retained_schema;
    Ref<SqliteSchemaProvider> retained_moved_schema;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& dialect = user->make_resource<SqliteDialect>();
        auto& schema = user->make<SqliteSchemaProvider>();
        auto& cloned = user->make<SqliteSchemaProvider>(schema);
        check(schema.memory_resource() == owner && cloned.memory_resource() == owner &&
                  debug_string(owner, schema) == debug_string(owner, cloned),
              "same-domain schema clone preserves the unit protocol and explicit owner");
        retained_dialect = user->make_global_resource<SqliteDialect>(dialect);
        retained_schema = user->make_global<SqliteSchemaProvider>(schema);
        retained_moved_schema = user->make_global<SqliteSchemaProvider>(std::move(cloned));
        auto& destination = user->get(user->rebind_global(retained_schema));
        destination = std::move(schema);
        check(destination.memory_resource() == global.memory_resource() &&
                  schema.memory_resource() == owner,
              "unit assignment preserves destination ownership without changing the source");
    }
    check(global.live_user_bytes() == 0, "unit retention keeps no source User blocks");
    auto reader = global.make_user_context();
    auto owner = reader->memory_resource();
    const auto& dialect = reader->get(reader->rebind_global(retained_dialect));
    auto& schema = reader->get(reader->rebind_global(retained_schema));
    const auto& moved = reader->get(reader->rebind_global(retained_moved_schema));
    check(schema.memory_resource() == global.memory_resource() &&
              moved.memory_resource() == global.memory_resource() &&
              debug_string(owner, schema) == "SqliteSchemaProvider" &&
              debug_string(owner, moved) == "SqliteSchemaProvider",
          "explicit copied and moved units use the destination after actual User deletion");
    check(debug_string(owner, dialect) == "SqliteDialect" &&
              dialect.kind() == DatabaseKind::Sqlite &&
              dialect.prefers_small_parent_relation_probes() &&
              !dialect.large_in_uses_array_param() && dialect.schema_setup_sqls().empty(),
          "retained dialect preserves unit diagnostics and independent compiler defaults");
    check(dialect.quote_ident(owner, "select") == "\"select\"" &&
              dialect.placeholder(owner, SIZE_MAX) == "?",
          "retained dialect keeps real provider dispatch after source reclamation");
    reader->set_schema_provider(global, schema);
    const auto& result = reader->ensure_schema().wait();
    check(
        !result && result.error().kind() == RuntimeError::Kind::Schema &&
            result.error().message() ==
                "rusqlite bind error: missing typed resource: SqliteDialect",
        "retained schema provider executes the production Context schema path with the new owner");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scoped_protocols();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite provider model and scoped clone contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
