#include "../../examples/models.hpp"
#include "fixture_value.hpp"
#include "../support/context_query_fixture.hpp"
#include <teaql/runtime/purposed_query_debug.hpp>
#include <teaql/core/query_debug.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include <sqlite3.h>
#include <iostream>

using Executor = example::Executor;

static Json row(Memory r, const CompactRow& value) {
    Json::Array out(r);
    for (auto name : {"id", "version", "count"}) {
        auto* item = value.get(name);
        out.emplace_back(item && item->try_u64() ? Json(r, *item->try_u64()) : Json(r));
    }
    return Json(r, out);
}

static Json logs(UserContext& user_context) {
    auto r = user_context.memory_resource();
    Json::Array out(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object value(r);
        value.emplace("sql", Json(r, entry.sql));
        value.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        value.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        value.emplace("debug_sql", Json(r, entry.debug_sql));
        value.emplace("outcome",
                      entry.log_context.execution_outcome
                          ? Json(r, debug_string(r, *entry.log_context.execution_outcome))
                          : Json(r));
        Json::Array params(r), trace(r);
        for (const auto& item : entry.params) {
            params.emplace_back(debug_string(r, item));
        }
        for (const auto& item : entry.trace_path) {
            trace.emplace_back(debug_string(r, item));
        }
        value.emplace("params", Json(r, params));
        value.emplace("trace", Json(r, trace));
        out.emplace_back(value);
    }
    return Json(r, out);
}

static Json observe(UserContext& user_context,
                    const Json& input,
                    const PurposedSelectQuery* retained = nullptr) {
    auto r = user_context.memory_resource();
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    auto& descriptor = user_context.make<EntityDescriptor>("Order");
    descriptor.table("example_orders");
    descriptor.property("id", DataType::I64).id();
    descriptor.property("version", DataType::I64).version();
    for (auto name : {"name", "password", "status"}) {
        descriptor.property(name, DataType::Text);
    }
    descriptor.mask_fields({"name"});
    metadata.register_entity(detail::clone_in(r, descriptor));
    user_context.set_metadata(user_context, metadata);
    auto& entities = user_context.make<InMemoryEntityRegistry>();
    entities.register_entity("Order");
    user_context.set_entity_registry(user_context, entities);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor =
        user_context.make_ref<Executor>(example::value(dialect).get(), transport, metadata, state);
    user_context.register_executor(executor);
    const char* sql =
        "CREATE TABLE example_orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT,password TEXT,status TEXT); INSERT INTO example_orders VALUES(9,1,'first','credential','ACTIVE'),(10,2,'second','credential','ACTIVE'),(11,1,'other','credential','INACTIVE');";
    if (sqlite3_exec(connection.native_handle(), sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Cannot initialize diagnostic fixture");
    }
    user_context.disable_sql_log();
    if (std::get<bool>(field(input, "logging").data)) {
        user_context.enable_select_sql_log();
    }
    auto kind = text(field(input, "kind"));
    auto secret = text(field(input, "secret"));
    auto& query = user_context.make<SelectQuery>("Order");
    query.filter(Expr::eq(r, "status", "ACTIVE")).project("id").project("version").limit(1);
    if (kind == "relation") {
        auto& child = user_context.make<SelectQuery>("Order");
        child.filter(Expr::eq(r, "name", secret));
        query.relation_query("unused_relation", child);
    } else if (kind == "projection") {
        query.project_expr("discarded", Expr::eq(r, "name", secret));
    } else if (kind == "order") {
        query.order_expr_asc(Expr::eq(r, "password", secret));
    }
    String comment("count ACTIVE including ", r), purpose("page ACTIVE with ", r);
    comment += secret;
    purpose += secret;
    if (!std::get<bool>(field(input, "invalid_intent").data)) {
        query.comment(comment);
    }
    auto& local_request = user_context.make<PurposedSelectQuery>(query, purpose);
    if (kind == "facet") {
        auto& options = user_context.make<teaql::request::QueryOptions>();
        auto& child = user_context.make<SelectQuery>("Order");
        child.filter(Expr::eq(r, "name", secret));
        teaql::request::QuerySelection selection(std::allocator_arg, Allocator(r), child);
        options.facets.emplace_back("secret_facet", "unused_relation", selection, true);
        local_request.with_facet_diagnostics(options);
    } else {
        local_request.for_exact_count("count");
    }
    const auto& request = retained ? *retained : local_request;
    Json::Object out(r);
    out.emplace("wrapper", Json(r, debug_string(r, request)));
    if (std::get<bool>(field(input, "failure").data)) {
        sqlite3_exec(
            connection.native_handle(), "DROP TABLE example_orders", nullptr, nullptr, nullptr);
    }
    auto& service = example::value(user_context.entity_data_service<Executor>("Order")).get();
    Json::Array rows(r);
    Json error(r);
    auto endpoint = text(field(input, "endpoint"));
    auto append_rows = [&](const auto& result) {
        if (!result) {
            error = Json(r, result.error().to_string(r));
        } else {
            for (const auto& item : result.value()) {
                rows.emplace_back(row(r, item));
            }
        }
    };
    auto append_entities = [&](const auto& result) {
        if (!result) {
            error = Json(r, result.error().to_string(r));
        } else {
            for (const auto& item : result.value()) {
                Json::Array value(r);
                value.emplace_back(item.id);
                value.emplace_back(item.version());
                value.emplace_back();
                rows.emplace_back(value);
            }
        }
    };
    if (endpoint == "entities") {
        append_entities(service.fetch_entities<example::Order>(request).wait());
    } else if (endpoint == "enhanced") {
        append_entities(service.fetch_enhanced_entities<example::Order>(request).wait());
    } else if (endpoint == "enhanced_aggregate") {
        append_entities(
            service.fetch_enhanced_entities_with_relation_aggregates<example::Order>(request, {})
                .wait());
    } else if (endpoint == "enhanced_owned") {
        append_entities(service
                            .fetch_enhanced_entities_with_relation_aggregates_owned<example::Order>(
                                detail::clone_in(r, request), {})
                            .wait());
    } else if (endpoint == "owned") {
        append_rows(service.fetch_all_owned(detail::clone_in(r, request)).wait());
    } else if (endpoint == "list") {
        append_rows(service.fetch_smart_list(request).wait());
    } else if (endpoint == "aggregate") {
        append_rows(service.fetch_smart_list_with_relation_aggregates(request, {}).wait());
    } else if (endpoint == "stream") {
        const auto& result = service.fetch_stream(request).wait();
        if (!result) {
            error = Json(r, result.error().to_string(r));
        } else {
            auto& stream = result.value().get();
            while (auto* next = stream.wait_next()) {
                if (!*next) {
                    error = Json(r, next->error().to_string(r));
                    break;
                }
                for (const auto& item : next->value().rows) {
                    rows.emplace_back(row(r, item));
                }
            }
            stream.cancel();
        }
    } else {
        append_rows(service.fetch_all(request).wait());
    }
    out.emplace("rows", Json(r, rows));
    out.emplace("error", std::move(error));
    out.emplace("logs", logs(user_context));
    if (!std::get<bool>(field(input, "failure").data)) {
        user_context.clear_sql_logs();
        auto& independent = user_context.make<SelectQuery>("Order");
        independent.project("id").limit(1).comment(secret);
        auto& next = user_context.make<PurposedSelectQuery>(independent, "independent query");
        service.fetch_all(next).wait();
        out.emplace("independent_logs", logs(user_context));
    }
    return Json(r, out);
}

static void self_test() {
    for (bool facets : {false, true}) {
        GlobalContext global;
        PurposedSelectQuery* retained = nullptr;
        {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto& query = user_context->make<SelectQuery>("Order");
            query.filter(Expr::eq(r, "status", "ACTIVE")).project("id").project("version").limit(1);
            auto& child = user_context->make<SelectQuery>("Order");
            child.filter(Expr::eq(r, "name", "retained-canary"));
            if (!facets) {
                query.relation_query("unused_relation", child);
            }
            query.comment("count ACTIVE including retained-canary");
            auto& source =
                user_context->make<PurposedSelectQuery>(query, "page ACTIVE with retained-canary");
            if (facets) {
                auto& options = user_context->make<teaql::request::QueryOptions>();
                options.facets.emplace_back("secret_facet", "unused_relation", child, true);
                source.with_facet_diagnostics(options);
            } else {
                source.for_exact_count("count");
            }
            retained = &global.make<PurposedSelectQuery>(source);
        }
        auto next_user_context = global.make_user_context();
        auto r = next_user_context->memory_resource();
        auto input = Json::parse(
            r,
            facets
                ? R"({"kind":"facet","endpoint":"all","secret":"retained-canary","logging":true,"failure":false,"invalid_intent":false})"
                : R"({"kind":"relation","endpoint":"all","secret":"retained-canary","logging":true,"failure":false,"invalid_intent":false})");
        auto output = observe(*next_user_context, *input, retained);
        auto safe = field(output, "logs").dump(r);
        example::require(safe.find("retained-canary") == String::npos,
                         "Global diagnostic source survives actual origin User deletion");
        example::require(retained->memory_resource() == global.memory_resource(),
                         "retained wrapper uses the explicit Global allocator");
    }
    for (bool facets : {false, true}) {
        for (bool cancel : {false, true}) {
            GlobalContext global;
            auto& state = global.make<context_query_fixture::State>(
                context_query_fixture::Config{.pending = 4});
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto& metadata = user_context->make<InMemoryMetadataStore>();
            auto& descriptor = user_context->make<EntityDescriptor>("Item");
            descriptor.property("id", DataType::U64).id();
            descriptor.property("name", DataType::Text);
            metadata.register_entity(detail::clone_in(r, descriptor));
            user_context->set_metadata(*user_context, metadata);
            auto& executor = user_context->make<context_query_fixture::Mock>(state);
            auto& service = EntityDataService<context_query_fixture::Mock>::for_executor(
                *user_context, "Item", *user_context, executor);
            auto& query = user_context->make<SelectQuery>("Item");
            auto& child = user_context->make<SelectQuery>("Item");
            child.filter(Expr::eq(r, "name", String(32768, 's', r)));
            if (!facets) {
                query.relation_query("unused", child);
            }
            query.comment("qualify diagnostic-source cancellation");
            auto& request =
                user_context->make<PurposedSelectQuery>(query, "close the owning Context");
            if (facets) {
                auto& options = user_context->make<teaql::request::QueryOptions>();
                options.facets.emplace_back("secret", "unused", child, true);
                request.with_facet_diagnostics(options);
            } else {
                request.for_exact_count("count");
            }
            auto& operation = service.fetch_all(request);
            example::require(!operation.poll() && state.request,
                             "diagnostic wrapper reaches the pending executor");
            if (cancel) {
                operation.cancel();
            }
            user_context.reset();
            example::require(std::count(state.notes.begin(), state.notes.end(), "work.drop") == 1,
                             "pending derived operation stops once before User reclamation");
        }
    }
    for (bool facets : {false, true}) {
        GlobalContext global;
        auto& connection = global.make_resource<SqliteConnection>();
        auto& transport = global.make<SqliteMutationExecutor>(connection);
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        auto& descriptor = user_context->make<EntityDescriptor>("Item");
        descriptor.table("items");
        descriptor.property("id", DataType::I64).id();
        descriptor.property("name", DataType::Text);
        descriptor.mask_fields({"name"});
        metadata.register_entity(detail::clone_in(r, descriptor));
        user_context->set_metadata(*user_context, metadata);
        use_sqlite_provider(*user_context, global, transport);
        auto dialect = user_context->require_resource<SqliteDialect>();
        auto& state = user_context->make<SqlExecutorState>();
        auto executor = user_context->make_ref<Executor>(
            example::value(dialect).get(), transport, metadata, state);
        user_context->register_executor(executor);
        example::require(
            sqlite3_exec(
                connection.native_handle(),
                "CREATE TABLE items(id INTEGER PRIMARY KEY,name TEXT); INSERT INTO items VALUES(1,'first'),(2,'second');",
                nullptr,
                nullptr,
                nullptr) == SQLITE_OK,
            "initialize cursor teardown fixture");
        auto& query = user_context->make<SelectQuery>("Item");
        query.project("id").stream(1).comment("close live cursor with cursor-canary");
        auto& child = user_context->make<SelectQuery>("Item");
        child.filter(Expr::eq(r, "name", "cursor-canary"));
        if (!facets) {
            query.relation_query("unused", child);
        }
        auto& request = user_context->make<PurposedSelectQuery>(
            query, "release the cursor before reclaiming its Context");
        if (facets) {
            auto& options = user_context->make<teaql::request::QueryOptions>();
            options.facets.emplace_back("secret", "unused", child, true);
            request.with_facet_diagnostics(options);
        } else {
            request.for_exact_count("count");
        }
        auto& service = example::value(user_context->entity_data_service<Executor>("Item")).get();
        const auto& result = service.fetch_stream(request).wait();
        example::require(bool(result), "open real diagnostic-source SQLite cursor");
        auto* chunk = result.value().get().wait_next();
        example::require(chunk && *chunk, "consume one chunk while retaining the SQLite lease");
        bool busy = false;
        for (auto* statement = sqlite3_next_stmt(connection.native_handle(), nullptr); statement;
             statement = sqlite3_next_stmt(connection.native_handle(), statement)) {
            busy = busy || sqlite3_stmt_busy(statement);
        }
        example::require(busy, "teardown starts with an actual live SQLite statement");
        user_context.reset();
        for (auto* statement = sqlite3_next_stmt(connection.native_handle(), nullptr); statement;
             statement = sqlite3_next_stmt(connection.native_handle(), statement)) {
            example::require(!sqlite3_stmt_busy(statement),
                             "User teardown releases all cursor leases");
        }
    }
    std::cout << "8 diagnostic-source ownership contracts passed\n";
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc > 1 && std::string_view(argv[1]) == "--self-test") {
            self_test();
            std::pmr::set_default_resource(previous);
            return 0;
        }
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid diagnostic input");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
