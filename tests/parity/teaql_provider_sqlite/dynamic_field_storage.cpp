// All five locked Rust 5.0.6 SQLite dynamic field storage integration tests.
#include "fixture.hpp"
#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/data_service/model_debug.hpp>
#include <array>
#include <bit>
#include <iostream>
using namespace original_provider;
using NativeTransaction = SqlDataServiceTransaction<MutationExecutorError>;

struct StorageFixture : Fixture {
    QueryIntent read =
        std::move(
            QueryIntent::create(resource, "read typed extensions", "verify storage invariants"))
            .value();
    MutationIntent reason =
        std::move(MutationIntent::create(resource, "framework graph persistence regression"))
            .value();
    DynamicFieldSelection all = DynamicFieldSelection::all(resource);
    const DynamicFieldStore& store = *executor.dynamic_field_store();

    StorageFixture() {
        user->insert_resource(*user, executor);
    }

    DynamicFieldDefinitions& define(std::string_view owner,
                                    std::initializer_list<DynamicFieldDefinitions::Field> fields,
                                    std::string_view revision = "test-v1") {
        return user->make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(
                          resource, owner, revision, std::span(fields.begin(), fields.size())))
                .value());
    }

    NativeTransaction& begin() {
        const auto& result = executor.begin(resource).wait();
        check(bool(result), "begin native data-service transaction");
        return result.value().get_as<NativeTransaction>();
    }

    DynamicFieldWrite write(const DynamicFieldDefinitions& definitions,
                            std::string_view space,
                            std::uint64_t id,
                            std::string_view code,
                            Value value) {
        DynamicFieldWrite::Changes changes(resource);
        changes.emplace(String(code, resource), DynamicFieldMutation::set(resource, value));
        return DynamicFieldWrite(
            std::allocator_arg, Allocator(resource), space, definitions, id, changes);
    }
};

static void context_schema_and_ambient() {
    StorageFixture f;
    auto r = f.resource;
    auto& definitions = f.define("School", {{"note", DataType::Text}});
    using Provider = DatabaseDynamicFieldsProvider<Executor>;
    std::array<const DynamicFieldDefinitions*, 1> schemas{&definitions};
    auto& provider =
        f.user->make<Provider>(std::move(Provider::create(r, "extension-demo", schemas)).value());
    f.user->set_dynamic_fields_provider(*f.user, provider);
    use_sqlite_provider(*f.user, *f.user, f.transport);
    check(bool(f.user->ensure_schema().wait()) && bool(f.user->ensure_schema().wait()),
          "repeatable context schema");
    check(!f.store.is_transaction_bound(), "ambient store is not transaction bound");
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back(
        f.write(definitions, "extension-demo", 1, "note", Value(r, "PRIVATE-CANARY")));
    const auto& rejected = f.store.apply_writes(r, writes, f.reason).wait();
    check(!rejected && rejected.error().code == "DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED",
          "ambient mutation rejected with exact code");
    std::array<std::uint64_t, 2> ids{1, 2};
    const auto& loaded = provider.load_values(*f.user, "School", ids, f.all, f.read).wait();
    check(bool(loaded) && loaded.value().rows.size() == 2,
          "provider returns every requested owner");
    for (const auto& [id, values] : loaded.value().rows) {
        check(values.empty(), "ambient failure leaves no values");
    }
    check(debug_string(r, writes[0]).find("PRIVATE-CANARY") == String::npos,
          "write Debug hides private payload");
}

static void native_identity_commit_rollback() {
    StorageFixture f;
    auto r = f.resource;
    check(bool(f.store.ensure_schema(r).wait()), "storage schema");
    auto& school = f.define("School", {{"note", DataType::Text}});
    auto& platform = f.define("Platform", {{"note", DataType::Text}});
    f.exec("CREATE TABLE native_probe(id INTEGER PRIMARY KEY, name TEXT)");
    auto& tx = f.begin();
    auto* storage = tx.dynamic_field_store();
    check(storage && storage->is_transaction_bound() &&
              storage->binding_key() == f.store.binding_key() && f.store.source_identity() &&
              storage->source_identity() == f.store.source_identity(),
          "native transaction shares bound identity");
    auto& clone = f.user->make<SqliteMutationExecutor>(f.transport);
    auto& separate = f.user->make<SqliteMutationExecutor>(f.connection);
    check(clone.source_identity() == f.store.source_identity() &&
              separate.source_identity() != f.store.source_identity() &&
              separate.binding_key() != storage->binding_key(),
          "clone and separate source identity");
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back(f.write(school, "first", 1, "note", Value(r, "school")));
    writes.emplace_back(f.write(platform, "first", 1, "note", Value(r, "platform")));
    writes.emplace_back(f.write(school, "second", 1, "note", Value(r, "other namespace")));
    check(bool(storage->validate_writes(r, writes)) &&
              bool(storage->apply_writes(r, writes, f.reason).wait()),
          "validated typed writes");
    CompiledQuery native(std::allocator_arg, Allocator(r));
    native.sql = "INSERT INTO native_probe(id,name) VALUES(1,?)";
    native.params.emplace_back("native");
    native.comment.emplace("native transaction rollback probe", r);
    check(bool(tx.transport.execute_sql(r, native).wait()),
          "same native transaction writes ordinary row");
    std::array<std::uint64_t, 1> ids{1};
    const auto& own = storage->load_values(r, "first", school, ids, f.all, f.read).wait();
    check(bool(own) && own.value().at(1).at(String("note", r)) == Value(r, "school"),
          "transaction reads its own dynamic write");
    check(bool(tx.rollback(r).wait()), "rollback native transaction");
    check(f.rows("SELECT COUNT(*) AS count FROM native_probe")[0].get("count")->try_i64() == 0,
          "ordinary DML rolled back");
    check(f.store.load_values(r, "first", school, ids, f.all, f.read).wait().value().at(1).empty(),
          "dynamic write rolled back");
    auto& committed = f.begin();
    check(bool(committed.dynamic_field_store()->apply_writes(r, writes, f.reason).wait()) &&
              bool(committed.commit(r).wait()),
          "commit dynamic writes");
    for (unsigned i = 0; i < 3; ++i) {
        const auto& values =
            f.store
                .load_values(
                    r, i == 2 ? "second" : "first", i == 1 ? platform : school, ids, f.all, f.read)
                .wait();
        check(bool(values) && values.value().at(1).at(String("note", r)).try_text() ==
                                  (i == 0   ? "school"
                                   : i == 1 ? "platform"
                                            : "other namespace"),
              "owner type and namespace isolation");
    }
    auto& newer = f.define("School", {{"note", DataType::Text}}, "test-v2");
    const auto& mismatch = f.store.load_values(r, "first", newer, ids, f.all, f.read).wait();
    check(!mismatch && mismatch.error().code == "DYNAMIC_FIELD_DEFINITION_MISMATCH",
          "revision mismatch rejects");
}

static void scalar_roundtrip() {
    StorageFixture f;
    auto r = f.resource;
    check(bool(f.store.ensure_schema(r).wait()), "scalar schema");
    auto& definitions = f.define("School",
                                 {{"bool", DataType::Bool},
                                  {"signed", DataType::I64},
                                  {"unsigned", DataType::U64},
                                  {"float", DataType::F64},
                                  {"negative_zero", DataType::F64},
                                  {"decimal", DataType::Decimal},
                                  {"text", DataType::Text},
                                  {"empty", DataType::LargeText},
                                  {"json", DataType::Json},
                                  {"json_null", DataType::Json},
                                  {"date", DataType::Date},
                                  {"time", DataType::Timestamp},
                                  {"sql_null", DataType::Text}});
    Record expected(r);
    auto set = [&](std::string_view name, Value value) {
        expected.emplace(String(name, r), std::move(value));
    };
    set("bool", Value(r, false));
    set("signed", Value(r, INT64_MIN));
    set("unsigned", Value(r, UINT64_MAX));
    set("float", Value(r, std::bit_cast<double>(std::uint64_t(0x3fd5555555555555))));
    set("negative_zero", Value(r, -0.0));
    set("decimal", Value(r, *Decimal::parse("12345678901234567890.12345678")));
    set("text", Value(r, "'); DROP TABLE native_probe; -- 🌍"));
    set("empty", Value(r, ""));
    set("json", Value(r, *Json::parse(r, "{\"code\":\"é\",\"count\":42}")));
    set("json_null", Value(r, Json(r)));
    set("date", Value(r, *Date::parse("1995-09-01")));
    set("time", Value(r, Timestamp(1750000000123)));
    set("sql_null", Value(r, nullptr));
    DynamicFieldWrite::Changes changes(r);
    for (const auto& [code, value] : expected) {
        changes.emplace(String(code, r), DynamicFieldMutation::set(r, value));
    }
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back("types", definitions, UINT64_MAX, changes);
    auto& tx = f.begin();
    check(bool(tx.dynamic_field_store()->apply_writes(r, writes, f.reason).wait()) &&
              bool(tx.commit(r).wait()),
          "commit all exact scalar variants");
    std::array<std::uint64_t, 1> ids{UINT64_MAX};
    const auto& loaded = f.store.load_values(r, "types", definitions, ids, f.all, f.read).wait();
    check(bool(loaded), "load all scalar variants");
    const auto& actual = loaded.value().at(UINT64_MAX);
    check(actual == expected && actual.size() == 13,
          "all thirteen original values and tags roundtrip exactly");
    for (const auto& [code, value] : expected) {
        if (auto* number = std::get_if<double>(&value.data)) {
            check(std::bit_cast<std::uint64_t>(*number) ==
                      std::bit_cast<std::uint64_t>(std::get<double>(actual.at(code).data)),
                  "float bits including negative zero preserved");
        }
    }
    auto built = DynamicFieldValues::from_values(r, definitions, actual);
    check(bool(built), "construct typed scalar carrier");
    auto& fields = f.user->make<DynamicFieldValues>(std::move(built).value());
    check(fields.field("json_null").value().state() == DynamicFieldState::Value &&
              fields.field("sql_null").value().state() == DynamicFieldState::Null,
          "JSON null is a Value while SQL NULL is Null");
}

static void states_and_prevalidation() {
    StorageFixture f;
    auto r = f.resource;
    check(bool(f.store.ensure_schema(r).wait()), "state schema");
    auto& definitions = f.define("School", {{"note", DataType::Text}, {"amount", DataType::F64}});
    auto& tx = f.begin();
    auto* store = tx.dynamic_field_store();
    Vector<DynamicFieldWrite> invalid(r);
    invalid.emplace_back(f.write(definitions, "states", 1, "note", Value(r, "must-not-write")));
    invalid.emplace_back(f.write(
        definitions, "states", 2, "amount", Value(r, std::numeric_limits<double>::quiet_NaN())));
    const auto& rejected = store->apply_writes(r, invalid, f.reason).wait();
    check(!rejected && rejected.error().code == "DYNAMIC_FIELD_NONFINITE_NUMBER",
          "NaN rejects whole batch");
    std::array<std::uint64_t, 2> ids{1, 2};
    const auto& unchanged = store->load_values(r, "states", definitions, ids, f.all, f.read).wait();
    check(bool(unchanged) && unchanged.value().size() == 2, "both requested owners present");
    for (const auto& [id, values] : unchanged.value()) {
        check(values.empty(), "no earlier write before validation failure");
    }
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back(f.write(definitions, "states", 1, "note", Value(r, nullptr)));
    writes.emplace_back(f.write(definitions, "states", 2, "note", Value(r, "second")));
    check(bool(store->apply_writes(r, writes, f.reason).wait()) && bool(tx.commit(r).wait()),
          "commit null and value");
    std::array<std::uint64_t, 4> duplicated{1, 2, 3, 1};
    const auto& rows =
        f.store.load_values(r, "states", definitions, duplicated, f.all, f.read).wait();
    check(bool(rows) && rows.value().size() == 3, "duplicate IDs deduplicate");
    Vector<Record> payloads(r);
    for (std::uint64_t id : {1, 2, 3}) {
        payloads.emplace_back(clone_record(r, rows.value().at(id)));
    }
    auto built = DynamicFieldValues::from_batch(r, definitions, payloads);
    check(bool(built) && built.value().size() == 3, "construct batch carriers");
    std::array<DynamicFieldValues*, 3> fields{};
    for (std::size_t i = 0; i < fields.size(); ++i) {
        fields[i] = &f.user->make<DynamicFieldValues>(std::move(built.value()[i]));
    }
    check(fields[0]->field("note").value().state() == DynamicFieldState::Null &&
              fields[2]->field("note").value().state() == DynamicFieldState::NotLoaded,
          "null and missing states distinct");
    check(fields[0]->selection_identity() == fields[1]->selection_identity() &&
              fields[1]->selection_identity() != fields[2]->selection_identity(),
          "same selected geometry shares identity; missing geometry differs");
    DynamicFieldWrite::Changes remove(r);
    remove.emplace(String("note", r), DynamicFieldMutation::erase(r));
    Vector<DynamicFieldWrite> deleted(r);
    deleted.emplace_back("states", definitions, 1, remove);
    auto& deletion = f.begin();
    check(bool(deletion.dynamic_field_store()->apply_writes(r, deleted, f.reason).wait()) &&
              bool(deletion.commit(r).wait()),
          "commit delete");
    std::array<std::uint64_t, 1> one{1};
    check(f.store.load_values(r, "states", definitions, one, f.all, f.read)
              .wait()
              .value()
              .at(1)
              .empty(),
          "Delete removes previously stored NULL");
}

static void dropped_transaction_large_batch() {
    StorageFixture f;
    auto r = f.resource;
    check(bool(f.store.ensure_schema(r).wait()), "large schema");
    auto& definitions = f.define("School", {{"note", DataType::Text}});
    auto& tx = f.begin();
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back(f.write(definitions, "large", 1, "note", Value(r, "abandoned")));
    check(bool(tx.dynamic_field_store()->apply_writes(r, writes, f.reason).wait()),
          "write before abandonment");
    tx.cancel();
    Vector<std::uint64_t> ids(r);
    for (std::uint64_t id = 1; id <= 1200; ++id) {
        ids.push_back(id);
    }
    const auto& rows = f.store.load_values(r, "large", definitions, ids, f.all, f.read).wait();
    check(bool(rows) && rows.value().size() == 1200,
          "bounded load returns all 1200 requested owners");
    for (const auto& [id, values] : rows.value()) {
        check(values.empty(), "abandoned transaction rolled back");
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    context_schema_and_ambient();
    native_identity_commit_rollback();
    scalar_roundtrip();
    states_and_prevalidation();
    dropped_transaction_large_batch();
    std::cout << "DYNAMIC_STORAGE_SOURCE_TESTS=5;passed\n";
}
