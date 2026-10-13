#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/context.hpp>
#include <sqlite3.h>
#include <array>
#include <bit>
#include <cmath>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void sql(sqlite3* db, const char* text) {
    check(sqlite3_exec(db, text, nullptr, nullptr, nullptr) == SQLITE_OK, "fixture SQL succeeds");
}

static DynamicFieldDefinitions definitions(Memory r) {
    const std::array fields{DynamicFieldDefinitions::Field{"bool", DataType::Bool},
                            DynamicFieldDefinitions::Field{"i64", DataType::I64},
                            DynamicFieldDefinitions::Field{"u64", DataType::U64},
                            DynamicFieldDefinitions::Field{"f64", DataType::F64},
                            DynamicFieldDefinitions::Field{"decimal", DataType::Decimal},
                            DynamicFieldDefinitions::Field{"text", DataType::Text},
                            DynamicFieldDefinitions::Field{"large", DataType::LargeText},
                            DynamicFieldDefinitions::Field{"json", DataType::Json},
                            DynamicFieldDefinitions::Field{"date", DataType::Date},
                            DynamicFieldDefinitions::Field{"timestamp", DataType::Timestamp}};
    return std::move(DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields)).value();
}

static DynamicFieldWrite::Changes changes(Memory r) {
    DynamicFieldWrite::Changes changes(r);
    auto set = [&](std::string_view code, Value value) {
        changes.emplace(String(code, r), DynamicFieldMutation::set(r, value));
    };
    set("bool", Value(r, false));
    set("i64", Value(r, INT64_MIN));
    set("u64", Value(r, UINT64_MAX));
    set("f64", Value(r, -0.0));
    set("decimal", Value(r, *Decimal::parse("12345.6700")));
    set("text", Value(r, "雪"));
    set("large", Value(r, std::string(8192, 'x')));
    set("json", Value(r, *Json::parse(r, "{\"nested\":[null,false,7]}")));
    set("date", Value(r, *Date::parse("2026-10-11")));
    set("timestamp", Value(r, Timestamp(INT64_MIN)));
    return changes;
}

static void roundtrip_and_isolation() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& connection = user_context->make_resource<SqliteConnection>();
    auto& provider = user_context->make<SqliteMutationExecutor>(connection);
    auto& clone = user_context->make<SqliteMutationExecutor>(provider);
    auto& separate = user_context->make<SqliteMutationExecutor>(connection);
    check(provider.source_identity() == clone.source_identity() &&
              provider.binding_key() == clone.binding_key() &&
              provider.source_identity() != separate.source_identity() &&
              provider.binding_key() != separate.binding_key(),
          "clones share identity; distinct executors on one connection do not");
    check(bool(provider.ensure_schema(r).wait()),
          "dynamic schema initializes on native connection");
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    auto selection = DynamicFieldSelection::all(r);
    auto read = std::move(QueryIntent::create(r, "read dynamic fields", "verify storage")).value();
    auto reason = std::move(MutationIntent::create(r, "persist dynamic fields")).value();
    auto values = changes(r);
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back("profile", schema, UINT64_MAX, values);
    check(!provider.validate_writes(r, writes) && !provider.apply_writes(r, writes, reason).wait(),
          "ambient executor cannot mutate even with trusted values");
    auto& tx = provider.begin_sql(r).wait().value().get();
    auto* store = tx.dynamic_field_store();
    check(store && store->is_transaction_bound() &&
              store->source_identity() == provider.source_identity(),
          "native transaction exposes its bound store");
    auto& pending = store->apply_writes(r, writes, reason);
    bool rejected = false;
    try {
        tx.commit_sql(r);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    check(rejected && !tx.consumed(),
          "pending dynamic work prevents premature transaction consumption");
    check(bool(pending.wait()), "all declared variants write within transaction");
    const std::array<std::uint64_t, 2> ids{UINT64_MAX, 7};
    auto& own = store->load_values(r, "profile", schema, ids, selection, read).wait();
    check(own && own.value().at(UINT64_MAX).size() == 10 && own.value().at(7).empty(),
          "native transaction reads its writes and returns empty requested owners");
    check(std::bit_cast<std::uint64_t>(
              std::get<double>(own.value().at(UINT64_MAX).find("f64")->second.data)) ==
              std::bit_cast<std::uint64_t>(-0.0),
          "negative-zero bits survive storage");
    check(own.value().at(UINT64_MAX).find("u64")->second.try_u64() == UINT64_MAX &&
              own.value().at(UINT64_MAX).find("i64")->second.try_i64() == INT64_MIN,
          "full-width integer domains survive text owner ids and JSON operands");
    auto& outside = provider.load_values(r, "profile", schema, ids, selection, read);
    check(!outside.poll(), "ambient read waits for exclusive transaction");
    check(bool(tx.commit_sql(r).wait()), "native and dynamic transaction commits");
    check(!store->is_transaction_bound() && bool(outside.wait()),
          "terminal completion releases waiting read and invalidates store write access");
    check(outside.wait().value().at(UINT64_MAX) == own.value().at(UINT64_MAX),
          "ambient read observes committed typed payloads");
    auto& rollback = provider.begin_sql(r).wait().value().get();
    auto* rollback_store = rollback.dynamic_field_store();
    DynamicFieldWrite::Changes nulls(r);
    nulls.emplace(String("text", r),
                  DynamicFieldMutation::set(r, Value(r, TypedNull{DataType::Text})));
    nulls.emplace(String("i64", r), DynamicFieldMutation::erase(r));
    Vector<DynamicFieldWrite> updates(r);
    updates.emplace_back("profile", schema, UINT64_MAX, nulls);
    check(bool(rollback_store->apply_writes(r, updates, reason).wait()),
          "explicit NULL and Delete apply");
    auto& changed = rollback_store->load_values(r, "profile", schema, ids, selection, read).wait();
    check(changed && changed.value().at(UINT64_MAX).find("text")->second.is_null() &&
              !changed.value().at(UINT64_MAX).contains("i64"),
          "NULL retains row; Delete removes row");
    auto& blocked = provider.load_values(r, "profile", schema, ids, selection, read);
    check(!blocked.poll(), "ambient read cannot join rollback transaction");
    check(bool(rollback.rollback_sql(r).wait()) && bool(blocked.wait()) &&
              blocked.wait().value().at(UINT64_MAX).find("text")->second.try_text() == "雪",
          "rollback removes extension changes before waking ambient reader");
}

static void validation_and_errors() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& connection = user_context->make_resource<SqliteConnection>();
    auto& provider = user_context->make<SqliteMutationExecutor>(connection);
    provider.ensure_schema(r).wait();
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    auto reason = std::move(MutationIntent::create(r, "validate batch")).value();
    auto& tx = provider.begin_sql(r).wait().value().get();
    auto* store = tx.dynamic_field_store();
    DynamicFieldWrite::Changes good(r), bad(r);
    good.emplace(String("text", r), DynamicFieldMutation::set(r, Value(r, "must not persist")));
    bad.emplace(String("f64", r),
                DynamicFieldMutation::set(r, Value(r, std::numeric_limits<double>::infinity())));
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back("profile", schema, 7, good);
    writes.emplace_back("profile", schema, 8, bad);
    auto& rejected = store->apply_writes(r, writes, reason).wait();
    check(!rejected && rejected.error().code == "DYNAMIC_FIELD_NONFINITE_NUMBER",
          "entire batch rejects nonfinite operand before first statement");
    auto read = std::move(QueryIntent::create(r, "verify rejected batch", "validation")).value();
    auto selection = DynamicFieldSelection::all(r);
    const std::array<std::uint64_t, 2> ids{7, 8};
    check(
        store->load_values(r, "profile", schema, ids, selection, read).wait().value().at(7).empty(),
        "earlier valid write not partially issued");
    tx.rollback_sql(r).wait();
    sql(connection.native_handle(),
        "INSERT INTO teaql_dynamic_field_storage_v1 VALUES ('profile','Item','7','text','stale','Text',0,'\"SECRET\"')");
    auto& mismatch = provider.load_values(r, "profile", schema, ids, selection, read).wait();
    check(!mismatch && mismatch.error().code == "DYNAMIC_FIELD_DEFINITION_MISMATCH",
          "stored revision mismatch rejects");
    sql(connection.native_handle(),
        "UPDATE teaql_dynamic_field_storage_v1 SET definition_revision='v1',payload='invalid SECRET data'");
    auto& corrupt = provider.load_values(r, "profile", schema, ids, selection, read).wait();
    check(!corrupt && corrupt.error().code == "DYNAMIC_FIELD_INVALID_STORAGE" &&
              corrupt.error().to_string(r).find("SECRET") == String::npos,
          "corrupt payload diagnostics omit stored data");
    sql(connection.native_handle(), "DROP TABLE teaql_dynamic_field_storage_v1");
    auto& missing = provider.load_values(r, "profile", schema, ids, selection, read).wait();
    check(!missing && missing.error().code == "DYNAMIC_FIELD_STORAGE_FAILED" &&
              missing.error().field == "storage",
          "driver errors project to safe storage error");
}

static void cancellation_and_terminal_failure() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& connection = user_context->make_resource<SqliteConnection>();
    auto& provider = user_context->make<SqliteMutationExecutor>(connection);
    provider.ensure_schema(r).wait();
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    auto read = std::move(QueryIntent::create(r, "read", "wait cancellation")).value();
    auto selection = DynamicFieldSelection::all(r);
    const std::array<std::uint64_t, 1> ids{7};
    auto& tx = provider.begin_sql(r).wait().value().get();
    auto& wait = provider.load_values(r, "profile", schema, ids, selection, read);
    check(!wait.poll(), "read waits on owned transaction");
    tx.cancel_sql();
    check(bool(wait.wait()), "transaction abandonment wakes queued ambient work");
    sql(connection.native_handle(),
        "PRAGMA foreign_keys=ON; CREATE TABLE parent(id INTEGER PRIMARY KEY); CREATE TABLE child(id INTEGER REFERENCES parent(id) DEFERRABLE INITIALLY DEFERRED)");
    auto& failed = provider.begin_sql(r).wait().value().get();
    CompiledQuery insert(std::allocator_arg, Allocator(r));
    insert.sql = "INSERT INTO child VALUES(7)";
    check(bool(failed.execute_sql(r, insert).wait()),
          "deferred violation permits native statement");
    auto& after = provider.load_values(r, "profile", schema, ids, selection, read);
    check(!after.poll(), "read waits for failed commit");
    check(!failed.commit_sql(r).wait() && bool(after.wait()) &&
              sqlite3_get_autocommit(connection.native_handle()),
          "failed consuming commit rolls back and releases connection");
}

int main() {
    try {
        roundtrip_and_isolation();
        validation_and_errors();
        cancellation_and_terminal_failure();
        std::cout << "SQLite dynamic storage contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
