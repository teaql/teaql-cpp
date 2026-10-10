// Direct pinned test-only physical schema contracts; production DDL uses UserContext.
#include "fixture.hpp"
#include <teaql/provider_sqlite/detail/schema.hpp>
#include <teaql/provider_sqlite/id.hpp>
#include <teaql/runtime/module.hpp>
#include <sqlite3.h>
#include <array>
#include <barrier>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

using namespace original_provider;

struct SchemaFixture {
    GlobalContext global;
    std::unique_ptr<UserContext> user = global.make_user_context();
    Memory resource = user->memory_resource();
    SqliteConnection& connection;
    SqliteMutationExecutor& transport;
    SqliteDialect& dialect = user->make_resource<SqliteDialect>();

    explicit SchemaFixture(std::string_view filename = ":memory:")
        : connection(user->make_resource<SqliteConnection>(filename)),
          transport(user->make<SqliteMutationExecutor>(connection)) {
        user->disable_sql_log();
    }

    void exec(std::string_view statement) {
        CompiledQuery query(std::allocator_arg, Allocator(resource));
        query.sql = statement;
        auto result = transport.execute(resource, query);
        if (!result) {
            throw std::runtime_error(std::string(result.error().to_string(resource)));
        }
    }

    Vector<CompactRow> rows(std::string_view statement) {
        CompiledQuery query(std::allocator_arg, Allocator(resource));
        query.sql = statement;
        auto result = transport.fetch_all_compact(resource, query);
        if (!result) {
            throw std::runtime_error(std::string(result.error().to_string(resource)));
        }
        return std::move(result).value();
    }

    // The Rust source entry is cfg(test)-only. Invoke the same private physical
    // kernel as the production provider, retaining the original ordered inputs.
    void ensure(std::span<const EntityDescriptor* const> descriptors) {
        detail::ensure_sqlite_physical_schema(
            resource, *connection.native_handle(), dialect, descriptors, *user);
        transport.clear_query_caches();
    }

    void one(const EntityDescriptor& descriptor) {
        const std::array<const EntityDescriptor*, 1> descriptors{&descriptor};
        try {
            ensure(descriptors);
        } catch (const MutationExecutorError& error) {
            throw std::runtime_error(std::string(error.to_string(resource)));
        }
    }

    String error(std::span<const EntityDescriptor* const> descriptors) {
        try {
            ensure(descriptors);
        } catch (const MutationExecutorError& error) {
            return error.to_string(resource);
        }
        throw std::runtime_error("original incompatible schema unexpectedly accepted");
    }

    String error(const EntityDescriptor& descriptor) {
        const std::array<const EntityDescriptor*, 1> descriptors{&descriptor};
        return error(descriptors);
    }
};

static EntityDescriptor entity(Memory owner) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "Order");
    result.table("orders");
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version().not_null();
    result.property("name", DataType::Text);
    return result;
}

static void contains(std::string_view message, std::initializer_list<std::string_view> parts) {
    for (auto part : parts) {
        check(message.find(part) != std::string_view::npos, "original complete diagnostic context");
    }
}

static void schema_compatibility_covers_boolean_and_temporal_storage_contracts() {
    SchemaFixture f;
    EntityDescriptor descriptor(std::allocator_arg, Allocator(f.resource), "HighRiskFixture");
    descriptor.table("high_risk_fixture");

    struct Case {
        DataType type;
        const char* name;
        const char* storage;
        bool accepted;
    };

    for (const auto& item : {Case{DataType::Bool, "enabled", "BOOLEAN", true},
                             Case{DataType::Bool, "enabled", "INTEGER", true},
                             Case{DataType::Bool, "enabled", "TEXT", false},
                             Case{DataType::Date, "business_date", "DATE", true},
                             Case{DataType::Date, "business_date", "TEXT", true},
                             Case{DataType::Date, "business_date", "INTEGER", false},
                             Case{DataType::Timestamp, "occurred_at", "TIMESTAMP", true},
                             Case{DataType::Timestamp, "occurred_at", "INTEGER", true},
                             Case{DataType::Timestamp, "occurred_at", "TEXT", false}}) {
        PropertyDescriptor property(f.resource, item.name, item.type);
        bool accepted = true;
        try {
            detail::ensure_sqlite_column_compatibility(
                f.resource, descriptor, property, item.storage, true);
        } catch (const MutationExecutorError&) {
            accepted = false;
        }
        check(accepted == item.accepted, "all nine original storage compatibility decisions");
    }
}

static void ensure_schema_rejects_incompatible_existing_column_type() {
    SchemaFixture f;
    f.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name INTEGER)");
    contains(f.error(entity(f.resource)),
             {"entity=Order", "table=orders", "column=name", "expected=Text", "actual=INTEGER"});
}

static EntityDescriptor shape_model(Memory owner, bool numeric) {
    EntityDescriptor result(std::allocator_arg, Allocator(owner), "ShapeFixture");
    result.table("shape_fixture");
    result.property("id", DataType::U64).id().not_null();
    result.property("version", DataType::I64).version().not_null();
    if (numeric) {
        result.property("amount", DataType::Decimal)
            .with_numeric_precision(19)
            .with_numeric_scale(7);
    } else {
        result.property("name", DataType::Text).with_max_length(100);
    }
    return result;
}

static void ensure_schema_accepts_covering_shapes_and_rejects_narrower_sqlite_columns() {
    SchemaFixture f;
    f.exec(
        "CREATE TABLE shape_fixture (id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name VARCHAR(255), amount NUMERIC(38,10))");
    auto length = shape_model(f.resource, false);
    auto numeric = shape_model(f.resource, true);
    f.one(length);
    f.one(numeric);
    f.exec("DROP TABLE shape_fixture");
    f.exec(
        "CREATE TABLE shape_fixture (id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name VARCHAR(32), amount NUMERIC(18,2))");
    contains(f.error(length), {"required_max_length=100", "actual_max_length=Some(32)"});
    contains(f.error(numeric),
             {"required_precision=19",
              "required_scale=7",
              "actual_precision=Some(18)",
              "actual_scale=Some(2)"});
}

static void sqlite_schema_ddl_uses_declared_length_and_numeric_shape() {
    SchemaFixture f;
    EntityDescriptor descriptor(std::allocator_arg, Allocator(f.resource), "ShapeFixture");
    descriptor.table("shape_fixture");
    descriptor.property("name", DataType::Text).with_max_length(100);
    descriptor.property("amount", DataType::Decimal)
        .with_numeric_precision(19)
        .with_numeric_scale(7);
    try {
        check(
            f.dialect.compile_create_table(f.resource, descriptor) ==
                "CREATE TABLE IF NOT EXISTS shape_fixture (name VARCHAR(100), amount NUMERIC(19,7))",
            "original complete shaped DDL");
    } catch (const SqlCompileError&) {
        throw std::runtime_error("original valid shaped DDL was rejected");
    }
}

static void ensure_schema_rejects_incompatible_existing_column_nullability() {
    SchemaFixture f;
    f.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name TEXT)");
    auto descriptor = entity(f.resource);
    descriptor.property("required_name", DataType::Text).not_null().column_name = "name";
    contains(f.error(descriptor),
             {"entity=Order", "column=name", "expected_nullable=false", "actual_nullable=true"});
}

static void required_column_contract(bool populated) {
    SchemaFixture f;
    f.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name TEXT)");
    if (populated) {
        f.exec("INSERT INTO orders(id, version, name) VALUES (1, 1, 'existing')");
    }
    auto descriptor = entity(f.resource);
    descriptor.property("code", DataType::Text).not_null();
    if (populated) {
        contains(f.error(descriptor),
                 {"entity=Order", "table=orders", "column=code", "migrate/backfill explicitly"});
    } else {
        f.one(descriptor);
        f.one(descriptor);
    }
    auto columns = f.transport.table_columns(f.resource, "orders");
    check(bool(columns), "original table-column reread succeeds");
    auto found = columns.value().find(std::string_view("code"));
    if (populated) {
        check(found == columns.value().end(),
              "failed required-column evolution leaves no partial column");
    } else {
        check(found != columns.value().end() && found->second.first == "VARCHAR(255)" &&
                  !found->second.second,
              "original new column has its exact declaration/nullability");
    }
}

static std::array<EntityDescriptor, 2>
foreign_key_models(Memory owner, bool reverse = false, bool composite = false) {
    EntityDescriptor parent(std::allocator_arg,
                            Allocator(owner),
                            composite ? "SqliteFkCompositeParent" : "SqliteFkParentFixture");
    parent.table(composite ? "teaql_sqlite_fk_composite_parent" : "teaql_sqlite_fk_parent_fixture");
    parent.property("id", DataType::U64).id().not_null();
    if (reverse) {
        parent.relation("children", "SqliteFkChildFixture")
            .local("id")
            .foreign("parent_id")
            .to_many();
    }
    EntityDescriptor child(std::allocator_arg,
                           Allocator(owner),
                           composite ? "SqliteFkCompositeChild" : "SqliteFkChildFixture");
    child.table(composite ? "teaql_sqlite_fk_composite_child" : "teaql_sqlite_fk_child_fixture");
    child.property("id", DataType::U64).id().not_null();
    child.property("parent_id", DataType::U64).not_null();
    child.relation("parent", parent.name).local("parent_id").foreign("id");
    return {std::move(parent), std::move(child)};
}

static void ensure_schema_creates_and_enforces_sqlite_foreign_keys() {
    SchemaFixture f;
    auto models = foreign_key_models(f.resource, true);
    const std::array<const EntityDescriptor*, 2> child_first{&models[1], &models[0]};
    const std::array<const EntityDescriptor*, 2> parent_first{&models[0], &models[1]};
    try {
        f.ensure(child_first);
        f.ensure(parent_first);
    } catch (const MutationExecutorError& error) {
        throw std::runtime_error(std::string(error.to_string(f.resource)));
    }
    auto count = f.rows(
        "SELECT COUNT(*) AS count FROM pragma_foreign_key_list('teaql_sqlite_fk_child_fixture') WHERE `from`='parent_id' AND `table`='teaql_sqlite_fk_parent_fixture' AND `to`='id'");
    check(*count[0].get("count") == Value(f.resource, std::int64_t(1)),
          "original single installed foreign-key triple");
    CompiledQuery insertion(std::allocator_arg, Allocator(f.resource));
    insertion.sql = "INSERT INTO teaql_sqlite_fk_child_fixture(id, parent_id) VALUES (1, 999)";
    check(!f.transport.execute(f.resource, insertion),
          "actual SQLite rejects original orphan insert");
}

static void ensure_schema_rejects_existing_sqlite_table_without_required_foreign_key() {
    SchemaFixture f;
    f.exec("CREATE TABLE teaql_sqlite_fk_parent_fixture (id INTEGER PRIMARY KEY NOT NULL)");
    f.exec(
        "CREATE TABLE teaql_sqlite_fk_child_fixture (id INTEGER PRIMARY KEY NOT NULL, parent_id INTEGER NOT NULL)");
    auto models = foreign_key_models(f.resource);
    const std::array<const EntityDescriptor*, 2> descriptors{&models[1], &models[0]};
    contains(f.error(descriptors),
             {"table=teaql_sqlite_fk_child_fixture",
              "column=parent_id",
              "rebuild/migrate the table explicitly"});
}

static void ensure_schema_rejects_composite_fk_that_matches_only_first_key() {
    SchemaFixture f;
    f.exec(
        "CREATE TABLE teaql_sqlite_fk_composite_parent (id INTEGER PRIMARY KEY NOT NULL, scope_id INTEGER NOT NULL, UNIQUE(id, scope_id))");
    f.exec(
        "CREATE TABLE teaql_sqlite_fk_composite_child (id INTEGER PRIMARY KEY NOT NULL, parent_id INTEGER NOT NULL, scope_id INTEGER NOT NULL, FOREIGN KEY(parent_id, scope_id) REFERENCES teaql_sqlite_fk_composite_parent(id, scope_id))");
    auto models = foreign_key_models(f.resource, false, true);
    const std::array<const EntityDescriptor*, 2> descriptors{&models[1], &models[0]};
    contains(f.error(descriptors), {"installed_key_count=2"});
    f.exec("DROP TABLE teaql_sqlite_fk_composite_child");
    f.exec(
        "CREATE TABLE teaql_sqlite_fk_composite_child (id INTEGER PRIMARY KEY NOT NULL, parent_id INTEGER NOT NULL, FOREIGN KEY(parent_id) REFERENCES teaql_sqlite_fk_composite_parent(id))");
    try {
        f.ensure(descriptors);
        f.ensure(descriptors);
    } catch (const MutationExecutorError& error) {
        throw std::runtime_error(std::string(error.to_string(f.resource)));
    }
    f.exec("DROP TABLE teaql_sqlite_fk_composite_child");
    f.exec(
        "CREATE TABLE teaql_sqlite_fk_composite_child (id INTEGER PRIMARY KEY NOT NULL, parent_id INTEGER NOT NULL, FOREIGN KEY(parent_id) REFERENCES teaql_sqlite_fk_composite_parent(id) ON DELETE CASCADE)");
    contains(f.error(descriptors), {"installed_delete=CASCADE"});
}

static void first_schema_install_creates_declared_indexes() {
    SchemaFixture f;
    f.one(entity(f.resource));
    auto rows = f.rows(
        "SELECT COUNT(1) AS count FROM sqlite_master WHERE type='index' AND name='PK_ORDERS_ID_VERSION'");
    check(*rows[0].get("count") == Value(f.resource, std::int64_t(1)),
          "original first installation creates one declared composite index");
}

static void ensure_schema_rejects_colliding_declared_index_with_wrong_shape() {
    SchemaFixture f;
    auto descriptor = entity(f.resource);
    f.exec("CREATE TABLE orders (id INTEGER NOT NULL, version INTEGER NOT NULL, name TEXT)");
    f.exec("CREATE INDEX PK_ORDERS_ID_VERSION ON orders (version, id)");
    contains(f.error(descriptor), {"PK_ORDERS_ID_VERSION", "orders"});
    for (auto definition :
         {"CREATE UNIQUE INDEX PK_ORDERS_ID_VERSION ON orders (id, version) WHERE version > 0",
          "CREATE UNIQUE INDEX PK_ORDERS_ID_VERSION ON orders (id DESC, version)",
          "CREATE UNIQUE INDEX PK_ORDERS_ID_VERSION ON orders (id, version COLLATE NOCASE)",
          "CREATE UNIQUE INDEX PK_ORDERS_ID_VERSION ON orders (id, (version + 1))"}) {
        f.exec("DROP INDEX PK_ORDERS_ID_VERSION");
        f.exec(definition);
        contains(f.error(descriptor), {"PK_ORDERS_ID_VERSION"});
    }
}

static void concurrent_schema_evolution_is_idempotent_across_connections() {
    auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    auto path = std::filesystem::temp_directory_path() /
                ("teaql-concurrent-schema-" + std::to_string(ticks) + ".db");

    struct Remove {
        std::filesystem::path path;

        ~Remove() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } remove{path};

    {
        SchemaFixture seed(path.string());
        seed.exec(
            "CREATE TABLE teaql_concurrent_schema_fixture (id INTEGER PRIMARY KEY NOT NULL, version INTEGER NOT NULL)");
    }
    std::barrier start(8);
    std::array<std::exception_ptr, 8> failures{};
    std::array<std::thread, 8> threads;
    for (std::size_t index = 0; index < threads.size(); ++index) {
        threads[index] = std::thread([&, index] {
            bool arrived = false;
            try {
                SchemaFixture worker(path.string());
#if defined(__mips__)
                // Keep all eight writers and outcomes. Allow the slow target
                // fixture more contention time; the provider default stays 5s.
                check(sqlite3_busy_timeout(worker.connection.native_handle(), 30000) == SQLITE_OK,
                      "MIPS concurrency fixture busy timeout is configured");
#endif
                EntityDescriptor descriptor(
                    std::allocator_arg, Allocator(worker.resource), "ConcurrentSchemaFixture");
                descriptor.table("teaql_concurrent_schema_fixture");
                descriptor.property("id", DataType::U64).id().not_null();
                descriptor.property("version", DataType::I64).version().not_null();
                descriptor.property("name", DataType::Text);
                descriptor.property("code", DataType::Text);
                arrived = true;
                start.arrive_and_wait();
                worker.one(descriptor);
            } catch (...) {
                if (!arrived) {
                    start.arrive_and_drop();
                }
                failures[index] = std::current_exception();
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    for (const auto& failure : failures) {
        if (failure) {
            std::rethrow_exception(failure);
        }
    }
    {
        SchemaFixture verify(path.string());
        auto count = verify.rows(
            "SELECT COUNT(*) AS count FROM pragma_table_info('teaql_concurrent_schema_fixture') WHERE name IN ('name', 'code')");
        check(*count[0].get("count") == Value(verify.resource, std::int64_t(2)),
              "original eight-connection schema evolution installs each column once");
    }
    check(std::filesystem::remove(path), "original concurrent fixture file is removed");
}

static void installing_runtime_module_does_not_create_schema() {
    Fixture f;
    EntityDescriptor descriptor(std::allocator_arg, Allocator(f.resource), "OrderLine");
    descriptor.table("order_line");
    descriptor.property("id", DataType::U64).id().not_null();
    descriptor.property("order_id", DataType::U64).not_null();
    descriptor.property("name", DataType::Text);
    auto& module = f.user->make<RuntimeModule>();
    module.descriptor(std::move(descriptor));
    module.apply_to(*f.user);
    use_sqlite_provider(*f.user, *f.user, f.transport);
    auto table_exists = [&](std::string_view table) {
        auto result = f.transport.table_exists(f.resource, table);
        check(bool(result), "source passive-install table lookup succeeds");
        return result.value();
    };
    check(!table_exists("order_line") && !table_exists(DEFAULT_ID_SPACE_TABLE),
          "original module/provider installation creates neither entity nor ID table");
    check(bool(f.user->ensure_schema().wait()) && table_exists("order_line") &&
              table_exists(DEFAULT_ID_SPACE_TABLE),
          "original explicit Context schema call creates both tables");
    check(bool(f.user->ensure_schema().wait()) && table_exists("order_line"),
          "original Context schema call remains idempotent");
}

static void sqlite_executor_ensures_schema_and_roundtrips_rows() {
    Fixture f;
    auto descriptor = entity(f.resource);
    f.metadata.register_entity(detail::clone_in(f.resource, descriptor));
    use_sqlite_provider(*f.user, *f.user, f.transport);
    check(bool(f.user->ensure_schema().wait()), "original provider schema boundary succeeds");
    InsertCommand insert(std::allocator_arg, Allocator(f.resource), "Order");
    insert.value("id", std::uint64_t(1)).value("version", std::int64_t(1)).value("name", "draft");
    auto inserted =
        f.transport.execute(f.resource, f.dialect.compile_insert(f.resource, descriptor, insert));
    check(inserted && inserted.value() == 1, "original compiled insert affects one row");
    SelectQuery query(std::allocator_arg, Allocator(f.resource), "Order");
    query.filter(Expr::eq(f.resource, "id", std::uint64_t(1))).order_asc("id");
    auto rows = f.transport.fetch_all_compact(
        f.resource, f.dialect.compile_select(f.resource, descriptor, query));
    check(rows && rows.value().size() == 1 &&
              *rows.value()[0].get("id") == Value(f.resource, std::int64_t(1)) &&
              *rows.value()[0].get("version") == Value(f.resource, std::int64_t(1)) &&
              *rows.value()[0].get("name") == Value(f.resource, "draft"),
          "original native roundtrip retains exact decoded Value variants");
}

static void physical_schema_never_interprets_generated_bootstrap_graphs() {
    SchemaFixture f;
    auto descriptor = entity(f.resource);
    auto& metadata = f.user->make<InMemoryMetadataStore>();
    metadata.register_entity(detail::clone_in(f.resource, descriptor));
    f.user->set_metadata(*f.user, metadata);
    GraphNode root(std::allocator_arg, Allocator(f.resource), "Order");
    root.value("id", std::uint64_t(1))
        .value("version", std::int64_t(1))
        .value("name", "module seed");
    f.user->set_root_graphs({&root, 1});
    f.one(descriptor);
    SelectQuery query(std::allocator_arg, Allocator(f.resource), "Order");
    query.filter(Expr::eq(f.resource, "id", std::uint64_t(1)));
    auto rows = f.transport.fetch_all_compact(
        f.resource, f.dialect.compile_select(f.resource, descriptor, query));
    check(rows && rows.value().empty(),
          "original physical schema never saves generated root graphs");
}

static void public_schema_boundary_rejects_legacy_provider_owned_bootstrap_graphs() {
    Fixture f;
    f.metadata.register_entity(entity(f.resource));
    GraphNode initial(std::allocator_arg, Allocator(f.resource), "Order");
    initial.value("id", std::uint64_t(1001)).value("version", std::int64_t(1)).value("name", "red");
    f.user->set_initial_graphs({&initial, 1});
    use_sqlite_provider(*f.user, *f.user, f.transport);
    const auto& result = f.user->ensure_schema().wait();
    check(!result, "original legacy graph schema boundary fails closed");
    contains(result.error().message(),
             {"generated root/constant bootstrap must use the typed RuntimeModule callback"});
    auto rows = f.rows("SELECT COUNT(*) AS count FROM orders");
    check(*rows[0].get("count") == Value(f.resource, std::int64_t(0)),
          "original physical schema exists but no legacy seed row was inserted");
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        schema_compatibility_covers_boolean_and_temporal_storage_contracts();
        ensure_schema_rejects_incompatible_existing_column_type();
        ensure_schema_accepts_covering_shapes_and_rejects_narrower_sqlite_columns();
        sqlite_schema_ddl_uses_declared_length_and_numeric_shape();
        ensure_schema_rejects_incompatible_existing_column_nullability();
        required_column_contract(false);
        required_column_contract(true);
        concurrent_schema_evolution_is_idempotent_across_connections();
        ensure_schema_creates_and_enforces_sqlite_foreign_keys();
        ensure_schema_rejects_existing_sqlite_table_without_required_foreign_key();
        ensure_schema_rejects_composite_fk_that_matches_only_first_key();
        first_schema_install_creates_declared_indexes();
        ensure_schema_rejects_colliding_declared_index_with_wrong_shape();
        installing_runtime_module_does_not_create_schema();
        sqlite_executor_ensures_schema_and_roundtrips_rows();
        physical_schema_never_interprets_generated_bootstrap_graphs();
        public_schema_boundary_rejects_legacy_provider_owned_bootstrap_graphs();
        std::cout << "Original seventeen SQLite schema contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
