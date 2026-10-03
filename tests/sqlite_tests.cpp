#include <teaql/runtime/sqlite.hpp>
#include <array>
#include <iostream>
#include <limits>

using namespace teaql;
static void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
template<class E, class F> static void rejects(F&& f, const char* message) {
    bool caught = false;
    try { f(); } catch (const E&) { caught = true; }
    check(caught, message);
}
const Value& field(const Record& record, std::string_view name) { return record.find(name)->second; }
struct DefaultResourceGuard {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    ~DefaultResourceGuard() { std::pmr::set_default_resource(previous); }
};

static void contracts() {
    // Every PMR allocation must explicitly name global/context, including nested values.
    DefaultResourceGuard no_hidden_allocator;
    GlobalContext global;
    auto& entity = global.make<EntityDescriptor>("Order");
    entity.table("orders").mask_fields({"name"});
    entity.property("id", DataType::U64).id();
    entity.property("version", DataType::I64).version();
    entity.property("name", DataType::Text, "display_name");
    entity.property("password", DataType::Text);
    auto ctx = global.make_user_context(); auto r = ctx->memory_resource();
    std::array<const EntityDescriptor*, 1> models{&entity};
    auto& db = ctx->make_resource<SqliteSession>(*ctx, std::span(models));
    db.ensure_schema();
    auto& create = ctx->make<InsertCommand>("Order");
    create.value("id", 1u).value("version", 1).value("name", "sensitive-order").value("password", "secret-value")
          .audit_as("Create sensitive-order with secret-value");
    auto& compiled = SqliteDialect::mutation(*ctx, entity, MutationKind::Insert, create);
    check(compiled.sql == "INSERT INTO \"orders\" (\"id\", \"version\", \"display_name\", \"password\") VALUES (?, ?, ?, ?)", "insert SQL golden");
    check(compiled.parameter_policies[0] == SqlParameterLogPolicy::Plain, "id provenance");
    check(compiled.parameter_policies[2] == SqlParameterLogPolicy::Masked, "masked field provenance");
    check(compiled.parameter_policies[3] == SqlParameterLogPolicy::Credential, "credential provenance");
    check(db.execute(create) == 1, "insert");
    const auto& insert_log = db.sql_logs().back();
    check(std::get<String>(insert_log.params[2].data) == "[REDACTED]", "masked log parameter");
    check(std::get<String>(insert_log.params[3].data) == "[REDACTED]", "credential log parameter");
    check(insert_log.audit_reason.find("sensitive-order") == String::npos && insert_log.audit_reason.find("secret-value") == String::npos, "intent redaction");
    check(std::get<String>(create.values.fields.find("name")->second.data) == "sensitive-order", "logging preserves execution input");
    rejects<SqliteError>([&] { db.execute(create); }, "duplicate primary key fails");
    check(db.sql_logs().back().outcome == SqlExecutionOutcome::Failure, "SQL failure logged");
    check(std::get<String>(db.sql_logs().back().params[3].data) == "[REDACTED]", "failed SQL redacted");

    auto& query = ctx->make<SelectQuery>("Order");
    query.filter(Expr::eq(r, "id", 1u)).purpose("Inspect the order").comment("test order lookup");
    auto& rows = db.query(query);
    check(rows.size() == 1 && std::get<String>(field(rows[0], "name").data) == "sensitive-order", "business result unmasked");
    check(field(rows[0], "name").memory_resource() == r, "result value allocator");
    check(rows[0].get_allocator().resource() == r, "result record allocator");
    check(!query.slice, "execution keeps request unchanged");
    auto& select_sql = SqliteDialect::select(*ctx, entity, query);
    check(select_sql.sql == "SELECT \"id\", \"version\", \"display_name\" AS \"name\", \"password\" FROM \"orders\" WHERE (\"id\" = ?)", "mapped projection SQL golden");

    auto& update = ctx->make<UpdateCommand>("Order", 1u);
    update.expected_version(1).value("name", "submitted").audit_as("Submit order");
    auto& update_sql = SqliteDialect::mutation(*ctx, entity, MutationKind::Update, update);
    check(update_sql.sql == "UPDATE \"orders\" SET \"display_name\" = ?, \"version\" = ? WHERE \"id\" = ? AND \"version\" = ?", "update SQL golden");
    check(update_sql.params[1].try_i64() == 2 && update_sql.params[3].try_i64() == 1, "version bind ordering");
    db.execute(update);
    rejects<OptimisticLockError>([&] { db.execute(update); }, "stale update rejected");
    check(db.sql_logs().back().outcome == SqlExecutionOutcome::Success && db.sql_logs().back().count == 0, "statement outcome distinct from optimistic conflict");
    auto& remove = ctx->make<DeleteCommand>("Order", 1u);
    remove.expected_version(2).audit_as("Delete order"); db.execute(remove);
    check(field(db.query(query)[0], "version").try_i64() == -3, "soft delete version");
    auto& recover = ctx->make<RecoverCommand>("Order", 1u, -3);
    recover.audit_as("Recover order"); db.execute(recover);
    check(field(db.query(query)[0], "version").try_i64() == 4, "recover version");
    auto& invalid_recover = ctx->make<RecoverCommand>("Order", 1u, 4);
    invalid_recover.audit_as("invalid recover");
    rejects<std::invalid_argument>([&] { db.execute(invalid_recover); }, "recover requires negative version");
    update.expected_version(INT64_MAX);
    rejects<std::overflow_error>([&] { db.execute(update); }, "version overflow rejected");

    // SQL parameters stay data; neither identifier escaping nor parameter binding uses interpolation.
    auto& injection = ctx->make<InsertCommand>("Order");
    injection.value("id", 2u).value("version", 1).value("name", "x'); DROP TABLE orders; --").audit_as("Insert literal text");
    db.execute(injection);
    auto& literal_query = ctx->make<SelectQuery>("Order");
    literal_query.filter(Expr::eq(r, "name", "x'); DROP TABLE orders; --"))
                 .purpose("Find literal text").comment("parameter binding test");
    check(db.query(literal_query).size() == 1, "parameter injection remains data");

    auto& third = ctx->make<InsertCommand>("Order");
    third.value("id", 3u).value("version", 1).value("name", nullptr).audit_as("Create transaction row");
    rejects<SqliteError>([&] { db.transaction([&] { db.execute(third); db.execute(create); }); }, "transaction failure");
    auto& third_query = ctx->make<SelectQuery>("Order");
    third_query.filter(Expr::eq(r, "id", 3u)).purpose("Verify rollback").comment("transaction rollback");
    check(db.query(third_query).empty(), "earlier write rolled back");
    db.transaction([&] { db.execute(third); });
    check(field(db.query(third_query)[0], "name").is_null(), "typed NULL round trip");
    rejects<std::logic_error>([&] { db.transaction([&] { db.transaction([] {}); }); }, "nested transaction rejects without leaking transaction");

    auto log_count = db.sql_logs().size();
    auto& no_intent = ctx->make<SelectQuery>("Order");
    rejects<std::invalid_argument>([&] { db.query(no_intent); }, "missing purpose/comment");
    check(db.sql_logs().size() == log_count, "invalid intent performs no SQL");
    auto& no_audit = ctx->make<InsertCommand>("Order"); no_audit.value("id", 4u);
    rejects<std::invalid_argument>([&] { db.execute(no_audit); }, "missing audit");
    query.limit(10'001);
    rejects<std::invalid_argument>([&] { db.query(query); }, "hard limit enforced");
    query.limit(0); check(db.query(query).empty(), "zero limit");
    query.limit(1).offset(std::uint64_t(INT64_MAX) + 1);
    rejects<std::overflow_error>([&] { db.query(query); }, "pagination overflow");
    auto& large_id = ctx->make<InsertCommand>("Order");
    large_id.value("id", UINT64_MAX).value("version", 1).audit_as("Reject unsigned overflow");
    rejects<std::overflow_error>([&] { db.execute(large_id); }, "SQLite unsigned bound");
    auto& unknown = ctx->make<UpdateCommand>("Order", 1u);
    unknown.value("typo", "lost write").audit_as("Reject typo");
    rejects<std::invalid_argument>([&] { db.execute(unknown); }, "unknown write field rejected");
    auto& empty_in = ctx->make<SelectQuery>("Order");
    empty_in.filter(Expr::in_list(r, "id", Vector<Value>(r))).purpose("Empty set").comment("empty IN");
    rejects<std::invalid_argument>([&] { db.query(empty_in); }, "Rust empty IN behavior");

    auto& all = ctx->make<SelectQuery>("Order"); all.purpose("Read remaining rows").comment("verify database still exists");
    check(db.query(all).size() == 3, "database intact");
    remove.expected_version(4).hard_delete().audit_as("Permanently remove order");
    db.execute(remove);
    check(db.query(all).size() == 2, "hard delete");
    db.enable_sql_log(false); log_count = db.sql_logs().size(); db.query(all);
    check(db.sql_logs().size() == log_count, "logging disabled");
    // Last query result can be copied explicitly into a longer-lived domain.
    auto& retained = global.make<Value>(field(rows[0], "name"));
    ctx.reset();
    check(std::get<String>(retained.data) == "sensitive-order", "global copy survives complete context destruction");
}
int main() {
    try { contracts(); std::cout << "SQLite kernel contracts passed\n"; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
