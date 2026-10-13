#include "../../tests/support/runtime_mutation_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <teaql/runtime/graph_debug.hpp>
#include <iostream>
using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void sqlite_ordering_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->disable_sql_log();
    auto& metadata = user->make<InMemoryMetadataStore>();
    for (auto name : {"Order", "Head", "Line"}) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), name);
        descriptor.table(name == std::string_view("Order")  ? "orders"
                         : name == std::string_view("Head") ? "heads"
                                                            : "lines");
        descriptor.property("id", DataType::U64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        if (name == std::string_view("Order")) {
            descriptor.property("head_id", DataType::U64);
            descriptor.relation("head", "Head").local("head_id").foreign("id");
            descriptor.relation("lines", "Line").local("id").foreign("order_id").to_many();
        } else if (name == std::string_view("Line")) {
            descriptor.property("order_id", DataType::U64);
        }
        metadata.register_entity(std::move(descriptor));
    }
    user->set_metadata(*user, metadata);
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (
        auto statement :
        {"PRAGMA foreign_keys=ON",
         "CREATE TABLE heads(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
         "CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT,head_id INTEGER REFERENCES heads(id))",
         "CREATE TABLE lines(id INTEGER PRIMARY KEY,version INTEGER,name TEXT,order_id INTEGER REFERENCES orders(id))",
         "CREATE TRIGGER fill_head AFTER INSERT ON heads BEGIN UPDATE heads SET name='database-head' WHERE id=NEW.id; END",
         "CREATE TRIGGER fill_order AFTER INSERT ON orders BEGIN UPDATE orders SET name='database-order' WHERE id=NEW.id; END",
         "CREATE TRIGGER fill_line AFTER INSERT ON lines BEGIN UPDATE lines SET name='database-line' WHERE id=NEW.id; END"}) {
        sql.sql = statement;
        require(bool(transport.execute(r, sql)), "SQLite schema/foreign-key/trigger setup");
    }
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& cache = user->make<SqlExecutorState>();
    auto& executor =
        user->make<SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>>(
            dialect, transport, metadata, cache);
    auto& transaction = executor.begin(r).wait().value().get();
    using Transaction = std::remove_reference_t<decltype(transaction)>;
    auto& service =
        EntityDataService<Transaction>::for_executor(*user, "Order", *user, transaction);
    require(bool(Access::bind_mutation_intent(service, "Inspect native mutation lifecycle")),
            "private fixture binds explicit mutation and readback intent");
    auto& root = user->make<GraphNode>("Order");
    root.value("id", std::uint64_t(7)).value("name", "input parent").with_comment("insert parent");
    auto& head = user->make<GraphNode>("Head");
    head.value("id", std::uint64_t(8)).value("name", "input head").with_comment("insert head");
    auto& line = user->make<GraphNode>("Line");
    line.value("id", std::uint64_t(9)).value("name", "input line").with_comment("insert line");
    root.relation("head", head).relation("lines", line);
    auto& inserted = Access::insert_graph_node_scoped(service, std::move(root), nullptr).wait();
    require(
        inserted && inserted.value().values.fields.find("head_id")->second.try_u64() == 8 &&
            inserted.value().values.fields.find("name")->second.try_text() == "database-order" &&
            inserted.value()
                    .relations.find("head")
                    ->second[0]
                    .values.fields.find("name")
                    ->second.try_text() == "database-head" &&
            inserted.value()
                    .relations.find("lines")
                    ->second[0]
                    .values.fields.find("name")
                    ->second.try_text() == "database-line" &&
            inserted.value()
                    .relations.find("lines")
                    ->second[0]
                    .values.fields.find("order_id")
                    ->second.try_u64() == 7,
        "real foreign keys require one-child-before-parent-before-many order, with authoritative trigger rereads");
    auto& update = user->make<GraphNode>("Order");
    update.value("id", std::uint64_t(7))
        .value("version", std::int64_t(1))
        .value("name", "changed parent");
    update.dirty_fields.emplace(r);
    update.dirty_fields->emplace("name");
    auto& head_reference = user->make<GraphNode>("Head");
    head_reference.reference().value("id", std::uint64_t(8));
    head_reference.value("version", std::int64_t(1));
    auto& line_reference = user->make<GraphNode>("Line");
    line_reference.reference().value("id", std::uint64_t(9));
    line_reference.value("version", std::int64_t(1));
    update.relation("head", head_reference).relation("lines", line_reference);
    auto& updated = Access::upsert_graph_node_scoped(service, std::move(update), nullptr).wait();
    require(
        updated && updated.value().values.fields.find("version")->second.try_i64() == 2 &&
            updated.value().values.fields.find("name")->second.try_text() == "changed parent" &&
            updated.value().relations.find("head")->second[0].operation ==
                GraphOperation::Reference &&
            updated.value().relations.find("lines")->second[0].operation ==
                GraphOperation::Reference,
        "recursive upsert preserves optimistic version, dirty projection and reference attachment rules");
    auto retained = user->make_global<GraphNode>(updated.value());
    auto expected = user->make_global<String>(debug_string(r, updated.value()));
    require(bool(transaction.rollback(r).wait()),
            "recursive write leaves transaction completion to its owner");
    for (auto statement :
         {"SELECT id FROM heads", "SELECT id FROM orders", "SELECT id FROM lines"}) {
        sql.sql = statement;
        auto& rows = transport.fetch_all_compact_sql(r, sql).wait();
        require(rows && rows.value().empty(), "caller rollback removes all recursive writes");
    }
    user.reset();
    require(global.live_user_bytes() == 0,
            "SQLite resources clean up before all User blocks are reclaimed");
    auto reader = global.make_user_context();
    auto& graph = reader->get(reader->rebind_global(retained));
    auto& before = reader->get(reader->rebind_global(expected));
    require(debug_string(reader->memory_resource(), graph) == before &&
                graph.memory_resource() == global.memory_resource(),
            "explicit Global copy retains full recursive snapshots after actual User deletion");
}

static void recursive_pending_cancellation() {
    GlobalContext global;
    unsigned cancellations = 0;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    user->disable_sql_log();
    auto& metadata = user->make<InMemoryMetadataStore>();
    EntityDescriptor parent(std::allocator_arg, Allocator(r), "Order");
    parent.property("id", DataType::U64).id();
    parent.relation("head", "Head");
    metadata.register_entity(std::move(parent));
    EntityDescriptor head(std::allocator_arg, Allocator(r), "Head");
    head.property("id", DataType::U64).id();
    metadata.register_entity(std::move(head));
    user->set_metadata(*user, metadata);
    auto& state = user->make<mutation_fixture::State>();
    state.allow_reads = true;
    state.read_pending = true;
    state.external_cancel = &cancellations;
    auto& executor = user->make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        *user, "Order", *user, executor);
    require(bool(Access::bind_mutation_intent(service, "Inspect native mutation lifecycle")),
            "private fixture binds explicit mutation and readback intent");
    auto& node = user->make<GraphNode>("Order");
    node.value("id", std::uint64_t(7));
    auto& reference = user->make<GraphNode>("Head");
    reference.reference().value("id", std::uint64_t(8));
    node.relation("head", reference);
    auto& work = Access::insert_graph_node_scoped(service, std::move(node), nullptr);
    require(!work.poll() && state.requests.empty() && state.child_wake,
            "recursive one-child validation can suspend before any parent write");
    auto ticket = work.wake_signal().ticket();
    state.child_wake->notify();
    require(work.wake_signal().ticket() != ticket,
            "nested validation wakes reach recursive parent");
    user.reset();
    require(cancellations == 1 && global.live_user_bytes() == 0,
            "nested recursive operations cancel once and join before User arena reclamation");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        sqlite_ordering_and_retention();
        recursive_pending_cancellation();
        std::pmr::set_default_resource(previous);
        std::cout << "Recursive graph write lifecycle and SQLite contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
