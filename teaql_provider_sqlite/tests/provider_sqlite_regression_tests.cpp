#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/runtime/data_service.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>

using namespace teaql;
using Executor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;
using Access = detail::EntityDataServiceAccess;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static Executor& setup(UserContext& user) {
    auto r = user.memory_resource();
    auto& metadata = user.make<InMemoryMetadataStore>();
    EntityDescriptor entity(std::allocator_arg, Allocator(r), "Order");
    entity.table("orders").mask_fields({"name"});
    entity.property("id", DataType::U64).id();
    entity.property("version", DataType::I64).version();
    entity.property("name", DataType::Text, "display_name");
    entity.property("password", DataType::Text);
    entity.property("payload", DataType::Json);
    metadata.register_entity(std::move(entity));
    user.set_metadata(user, metadata);
    auto& connection = user.make_resource<SqliteConnection>();
    auto& transport = user.make<SqliteMutationExecutor>(connection);
    auto& dialect = user.make_resource<SqliteDialect>();
    auto& ddl = user.make<CompiledQuery>();
    ddl.sql = dialect.compile_create_table(r, *metadata.get_entity("Order"));
    check(bool(transport.execute(r, ddl)), "production dialect creates the regression schema");
    auto& state = user.make<SqlExecutorState>();
    return user.make<Executor>(dialect, transport, metadata, state);
}

static const Vector<CompactRow>& fetch(const EntityDataService<Executor>& service,
                                       const SelectQuery& query) {
    PurposedSelectQuery purpose(std::allocator_arg,
                                Allocator(service.memory_resource()),
                                query,
                                "Inspect the regression database");
    auto& result = service.fetch_all(purpose).wait();
    check(bool(result), "runtime query succeeds");
    return result.value();
}

static void mutations_privacy_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& executor = setup(*user);
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    auto& insert = user->make<InsertCommand>("Order");
    insert.value("id", std::uint64_t(1))
        .value("version", std::int64_t(1))
        .value("name", "sensitive-order")
        .value("password", "secret-value")
        .audit_as("Create sensitive-order with secret-value");
    auto& written = Access::insert_internal(service, insert).wait();
    check(written && written.value() == 1, "runtime insert uses the independent provider");
    {
        auto logs = user->sql_logs();
        check(!logs.empty() && logs.back().operation == SqlLogOperation::Insert,
              "runtime retains source-aligned SQL logs");
        for (const auto& log : logs) {
            for (const auto& parameter : log.params) {
                check(parameter.try_text() != "secret-value",
                      "retained SQL parameters exclude the credential");
            }
            check(log.debug_sql.find("secret-value") == String::npos &&
                      (!log.comment || log.comment->find("secret-value") == String::npos) &&
                      (!log.audit_reason || log.audit_reason->find("secret-value") == String::npos),
                  "credential never escapes in SQL diagnostics or audit intent");
        }
    }
    auto& query = user->make<SelectQuery>("Order");
    query.filter(Expr::eq(r, "id", std::uint64_t(1)));
    const auto& rows = fetch(service, query);
    check(rows.size() == 1 && rows[0].get("name")->try_text() == "sensitive-order" &&
              rows[0].get("password")->try_text() == "secret-value",
          "privacy projection does not modify stored or returned business values");
    check(rows[0].memory_resource() == r && rows[0].get("name")->memory_resource() == r,
          "row and nested values belong to UserContext");
    auto& retained = global.make<CompactRow>(rows[0]);
    auto& duplicate = Access::insert_internal(service, insert).wait();
    check(!duplicate, "duplicate identity fails through the typed production error path");
    {
        auto logs = user->sql_logs();
        check(logs.back().log_context.execution_outcome == SqlExecutionOutcome::Failure &&
                  logs.back().debug_sql.find("secret-value") == String::npos,
              "failed SQL preserves privacy and statement outcome");
    }
    auto& update = user->make<UpdateCommand>("Order", std::uint64_t(1));
    update.value("name", "submitted").expected_version(1).audit_as("Submit the order");
    auto& changed = Access::update_internal(service, update).wait();
    check(changed && changed.value() == 1, "version-checked update");
    auto& stale = Access::update_internal(service, update).wait();
    check(!stale &&
              stale.error().runtime_error().kind() == RuntimeError::Kind::OptimisticLockConflict,
          "stale update remains a runtime optimistic lock conflict");
    auto& remove = user->make<DeleteCommand>("Order", std::uint64_t(1));
    remove.expected_version(2).audit_as("Delete the order");
    check(bool(Access::delete_internal(service, remove).wait()), "soft delete");
    check(fetch(service, query)[0].get("version")->try_i64() == -3, "soft delete version");
    auto& recover = user->make<RecoverCommand>("Order", std::uint64_t(1), -3);
    recover.audit_as("Recover the order");
    check(bool(Access::recover_internal(service, recover).wait()), "recover");
    check(fetch(service, query)[0].get("version")->try_i64() == 4, "recovery version");
    remove.expected_version(4).hard_delete().audit_as("Permanently delete the order");
    check(bool(Access::delete_internal(service, remove).wait()) && fetch(service, query).empty(),
          "hard delete");
    user.reset();
    check(retained.get("name")->try_text() == "sensitive-order" &&
              retained.get("name")->memory_resource() == global.memory_resource(),
          "explicit destination copy survives actual UserContext destruction");
}

static void bindings_and_transactions() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& executor = setup(*user);
    user->disable_sql_log();
    auto& service = EntityDataService<Executor>::for_executor(*user, "Order", *user, executor);
    constexpr std::string_view literal = "x'); DROP TABLE orders; --";
    auto& insert = user->make<InsertCommand>("Order");
    insert.value("id", std::uint64_t(2)).value("name", literal).audit_as("Insert literal text");
    check(bool(Access::insert_internal(service, insert).wait()), "injection text binds as data");
    auto& query = user->make<SelectQuery>("Order");
    query.filter(Expr::eq(r, "name", literal));
    check(fetch(service, query).size() == 1, "literal predicate does not execute SQL text");
    auto& tx = executor.begin(r).wait().value().get();
    auto& third = user->make<InsertCommand>("Order");
    third.value("id", std::uint64_t(3)).value("name", TypedNull{DataType::Text});
    check(bool(tx.mutate(r, MutationRequest(std::allocator_arg, Allocator(r), third)).wait()),
          "transaction accepts typed NULL");
    auto& failed = tx.mutate(r, MutationRequest(std::allocator_arg, Allocator(r), insert)).wait();
    check(!failed, "duplicate write fails inside a production transaction");
    check(bool(tx.rollback(r).wait()), "explicit transaction rollback");
    auto& third_query = user->make<SelectQuery>("Order");
    third_query.filter(Expr::eq(r, "id", std::uint64_t(3)));
    check(fetch(service, third_query).empty(), "rollback removes the earlier successful write");
    auto& committed = executor.begin(r).wait().value().get();
    check(
        bool(
            committed.mutate(r, MutationRequest(std::allocator_arg, Allocator(r), third)).wait()) &&
            bool(committed.commit(r).wait()),
        "production transaction commits");
    check(fetch(service, third_query)[0].get("name")->is_null(), "typed NULL round trip");
    check(user->sql_logs().empty(), "disabled runtime logging remains disabled");
    user->enable_all_sql_log();
    auto& payload = user->make<InsertCommand>("Order");
    auto json = Json::parse(r, R"({"items":[{"access_token":"nested-secret"}]})");
    check(json.has_value(), "JSON fixture");
    payload.value("id", std::uint64_t(4)).value("payload", *json).audit_as("Store JSON");
    check(bool(Access::insert_internal(service, payload).wait()),
          "production provider supports JSON unlike the retired adapter");
    auto& json_query = user->make<SelectQuery>("Order");
    json_query.filter(Expr::eq(r, "id", std::uint64_t(4)));
    check(std::get<Json>(fetch(service, json_query)[0].get("payload")->data) == *json,
          "nested JSON round trip");
    auto& object = user->make<Value::Object>();
    object.emplace(String("access_token", r), Value(r, "nested-secret"));
    auto& unsupported = user->make<InsertCommand>("Order");
    unsupported.value("id", std::uint64_t(5))
        .value("payload", object)
        .audit_as("Reject nested-secret as a raw Object binding");
    check(!Access::insert_internal(service, unsupported).wait(),
          "raw Object binding still fails instead of silently stringifying");
    auto logs = user->sql_logs();
    check(logs.back().log_context.execution_outcome == SqlExecutionOutcome::Failure,
          "unsupported structured binding retains failure metadata");
    for (const auto& log : logs) {
        check(log.debug_sql.find("nested-secret") == String::npos &&
                  (!log.comment || log.comment->find("nested-secret") == String::npos) &&
                  (!log.audit_reason || log.audit_reason->find("nested-secret") == String::npos),
              "successful JSON and failed Object diagnostics protect nested credentials");
    }
    check(unsupported.values.fields.find("payload")->second.as_object() &&
              std::holds_alternative<Json>(payload.values.fields.find("payload")->second.data),
          "privacy leaves both structured execution inputs unchanged");
    auto& overflow = user->make<InsertCommand>("Order");
    overflow.value("id", UINT64_MAX);
    auto& rejected =
        executor.mutate(r, MutationRequest(std::allocator_arg, Allocator(r), overflow)).wait();
    check(!rejected && rejected.error().kind() == Executor::Error::Kind::Transport &&
              rejected.error().transport_error().kind() == MutationExecutorError::Kind::Bind,
          "unsigned SQLite overflow remains a typed binding error");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        mutations_privacy_and_retention();
        bindings_and_transactions();
        std::pmr::set_default_resource(previous);
        std::cout << "Production SQLite regression contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
