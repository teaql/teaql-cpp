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
    auto& compiled = ctx->make<CompiledQuery>(LegacySqliteCompiler::compile_insert(ctx->memory_resource(), entity, create));
    check(compiled.sql == "INSERT INTO \"orders\" (\"id\", \"version\", \"display_name\", \"password\") VALUES (?, ?, ?, ?)", "insert SQL golden");
    check(compiled.log_context.parameter_policies[0] == SqlParameterLogPolicy::Plain, "id provenance");
    check(compiled.log_context.parameter_policies[2] == SqlParameterLogPolicy::Masked, "masked field provenance");
    check(compiled.log_context.parameter_policies[3] == SqlParameterLogPolicy::Credential, "credential provenance");
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
    auto& select_sql = ctx->make<CompiledQuery>(LegacySqliteCompiler::select(ctx->memory_resource(), entity, query));
    check(select_sql.sql == "SELECT \"id\", \"version\", \"display_name\" AS \"name\", \"password\" FROM \"orders\" WHERE (\"id\" = ?)", "mapped projection SQL golden");

    auto& update = ctx->make<UpdateCommand>("Order", 1u);
    update.expected_version(1).value("name", "submitted").audit_as("Submit order");
    auto& update_sql = ctx->make<CompiledQuery>(LegacySqliteCompiler::compile_update(ctx->memory_resource(), entity, update));
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
    auto& mixed_fields = ctx->make<InsertCommand>("Order");
    mixed_fields.value("name", "recognized field").value("typo", "unrecognized field");
    const auto& mixed_sql = ctx->make<CompiledQuery>(LegacySqliteCompiler::compile_insert(ctx->memory_resource(), entity, mixed_fields));
    check(mixed_sql.params.size() == 1 && mixed_sql.params.front().try_text() == "recognized field", "Rust compiler ignores unrecognized fields alongside known fields");
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
static void unsupported_query_features() {
    DefaultResourceGuard no_hidden_allocator;
    GlobalContext global;
    auto& entity = global.make<EntityDescriptor>("Order"); entity.property("id", DataType::U64).id();
    auto context = global.make_user_context();
    std::array<const EntityDescriptor*, 1> descriptors{&entity};
    auto& db = context->make_resource<SqliteSession>(*context, std::span(descriptors)); db.ensure_schema();
    using Configure = void(*)(SelectQuery&);
    const Configure cases[] = {
        [](SelectQuery& q) { q.project_expr("x", Expr::column(q.memory_resource(), "id")); },
        [](SelectQuery& q) { q.search_with_text(""); },
        [](SelectQuery& q) { q.having(Expr::gt(q.memory_resource(), "id", 0)); },
        [](SelectQuery& q) { q.partition_by("id"); },
        [](SelectQuery& q) { q.top_n_probe_parent_threshold(0); },
        [](SelectQuery& q) { q.count("total"); },
        [](SelectQuery& q) { q.group_by("id"); },
        [](SelectQuery& q) { q.relation("lines"); },
        [](SelectQuery& q) { q.enable_aggregation_cache(); },
        [](SelectQuery& q) { q.raw_sql("SELECT 1"); },
        [](SelectQuery& q) { q.add_raw_sql_search_criteria("id > 1"); },
        [](SelectQuery& q) { q.dynamic_property_raw("x", "id + 1"); },
        [](SelectQuery& q) { q.project_raw("x", "id + 1"); },
        [](SelectQuery& q) { q.object_group_by("x", "id", q); },
        [](SelectQuery& q) { q.child_enhancement(q); },
        [](SelectQuery& q) { q.stream_default(); },
        [](SelectQuery& q) { q.optimize_for_continuous_page_fetch(); },
        [](SelectQuery& q) { q.optimize_pagination_with_id_set(); },
        [](SelectQuery& q) { q.order_expr_asc(Expr::column(q.memory_resource(), "id")); },
        [](SelectQuery& q) { q.filter(Expr::sound_like(q.memory_resource(), "id", "one")); },
        [](SelectQuery& q) {
            EntityDescriptor descriptor(std::allocator_arg, Allocator(q.memory_resource()), "Order");
            descriptor.property("id", DataType::U64).id();
            q.filter(Expr::in_subquery(q.memory_resource(), "id", descriptor, q, "id"));
        },
    };
    auto logs = db.sql_logs().size();
    for (auto configure : cases) {
        auto& query = context->make<SelectQuery>("Order"); query.purpose("Check unsupported feature").comment("model versus executor capability");
        configure(query);
        bool rejected = false;
        try { db.query(query); }
        catch (const std::invalid_argument& error) { rejected = std::string_view(error.what()).starts_with("UnsupportedQueryFeature:"); }
        check(rejected, "new model fields must never be silently ignored by the old executor");
    }
    check(db.sql_logs().size() == logs, "unsupported features rejected before SQL I/O");
}
static void nested_credential_failure() {
    DefaultResourceGuard no_hidden_allocator;
    GlobalContext global;
    auto& entity = global.make<EntityDescriptor>("Payload");
    entity.property("id", DataType::U64).id();
    entity.property("payload", DataType::Text);
    entity.mask_fields({}); // Declared plain field; nested credentials must still be protected.
    auto context = global.make_user_context(); auto r = context->memory_resource();
    std::array<const EntityDescriptor*, 1> descriptors{&entity};
    auto& db = context->make_resource<SqliteSession>(*context, std::span(descriptors)); db.ensure_schema();
    auto& secret = context->make<Record>();
    secret.emplace(String("access_token", r), Value(r, "nested-token-should-never-leak"));
    auto& items = context->make<Value::List>(); items.emplace_back(secret);
    auto& payload = context->make<Record>(); payload.emplace(String("items", r), Value(r, items));
    auto& insert = context->make<InsertCommand>("Payload");
    insert.value("id", 1u).value("payload", payload).audit_as("Reject nested-token-should-never-leak as structured binding");
    const auto& sql = context->make<CompiledQuery>(LegacySqliteCompiler::compile_insert(context->memory_resource(), entity, insert));
    check(sql.log_context.parameter_policies[1] == SqlParameterLogPolicy::Plain, "outer metadata alone cannot classify nested credentials");
    rejects<std::invalid_argument>([&] { db.execute(insert); }, "SQLite does not silently stringify Object/List");
    const auto& log = db.sql_logs().back();
    check(log.outcome == SqlExecutionOutcome::Failure, "unsupported nested binding records failure");
    check(log.params[1].try_text() == "[REDACTED]", "credential inside plain object/list redacted");
    check(log.audit_reason.find("nested-token-should-never-leak") == String::npos, "nested credential removed from intent");
    check(insert.values.fields.find("payload")->second.as_object() != nullptr, "redaction does not overwrite mutation input");
    auto& select = context->make<SelectQuery>("Payload");
    select.purpose("Verify failed bind did not write").comment("unsupported structured binding");
    check(db.query(select).empty(), "unsupported binding causes no write");
    auto json = Json::parse(r, R"({"items":[{"access_token":"json-token-should-never-leak"}]})");
    auto& json_insert = context->make<InsertCommand>("Payload");
    json_insert.value("id", 2u).value("payload", *json).audit_as("Reject json-token-should-never-leak as JSON binding");
    rejects<std::invalid_argument>([&] { db.execute(json_insert); }, "JSON binding remains explicit unsupported pending provider migration");
    const auto& json_log = db.sql_logs().back();
    check(json_log.params[1].try_text() == "[REDACTED]", "nested JSON credential redacted on failure");
    check(json_log.audit_reason.find("json-token-should-never-leak") == String::npos, "JSON credential removed from intent");
    check(std::holds_alternative<Json>(json_insert.values.fields.find("payload")->second.data), "JSON input untouched by logging");
}
int main() {
    try { contracts(); nested_credential_failure(); unsupported_query_features(); std::cout << "SQLite kernel contracts passed\n"; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
