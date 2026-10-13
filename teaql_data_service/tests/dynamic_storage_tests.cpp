#include <teaql/data_service.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Store final : DynamicFieldStore {
    std::uintptr_t key = 1;
    std::optional<std::uint64_t> identity = 7;
    bool bound = false;

    bool is_transaction_bound() const override {
        return bound;
    }

    std::uintptr_t binding_key() const override {
        return key;
    }

    std::optional<std::uint64_t> source_identity() const override {
        return identity;
    }

    DynamicStorageFuture<std::monostate>& ensure_schema(Memory) const override {
        throw std::logic_error("unexpected I/O");
    }

    DynamicStorageFuture<DynamicStorageRows>& load_values(Memory,
                                                          std::string_view,
                                                          const DynamicFieldDefinitions&,
                                                          std::span<const std::uint64_t>,
                                                          const DynamicFieldSelection&,
                                                          const QueryIntent&) const override {
        throw std::logic_error("unexpected I/O");
    }

    Result<std::monostate, DynamicFieldError>
    validate_writes(Memory r, std::span<const DynamicFieldWrite>) const override {
        return Result<std::monostate, DynamicFieldError>::ok(r, {});
    }

    DynamicStorageFuture<std::monostate>&
    apply_writes(Memory, std::span<const DynamicFieldWrite>, const MutationIntent&) const override {
        throw std::logic_error("unexpected I/O");
    }
};

struct StreamOnly final : StreamQueryExecutor<String> {
    unsigned calls = 0;

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    QueryStream<String>& query_stream(Memory, QueryRequest, std::size_t) override {
        ++calls;
        throw std::logic_error("unexpected cursor");
    }
};

static DynamicFieldDefinitions definitions(Memory r) {
    const std::array fields{DynamicFieldDefinitions::Field{"name", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    return std::move(DynamicFieldDefinitions::from_fields(r, "Item", "v1", fields)).value();
}

static void plans_and_batches() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    auto selection = DynamicFieldSelection::all(r);
    auto intent =
        std::move(QueryIntent::create(r, "read extensions", "hydrate cursor chunk")).value();
    Store store;
    auto created =
        DynamicFieldStreamPlan::create(r, "trusted-profile", schema, selection, store, intent);
    check(created && created.value().definitions().storage_binding_matches("trusted-profile", 7),
          "plan binds trusted metadata to executor source");
    auto& plan = created.value();
    check(bool(plan.validate_store(store)), "matching store validates");
    store.key = 2;
    check(!plan.validate_store(store), "different binding rejects");
    store.key = 1;
    store.identity = 8;
    check(!plan.validate_store(store),
          "same address with reused storage rejects by instance identity");
    store.identity = 7;
    store.identity.reset();
    auto missing = DynamicFieldStreamPlan::create(r, "profile", schema, selection, store, intent);
    check(!missing && missing.error().code == "DYNAMIC_FIELD_STORAGE_IDENTITY_REQUIRED",
          "missing identity fails closed");
    store.identity = 7;
    auto blank = DynamicFieldStreamPlan::create(r, "\u2003", schema, selection, store, intent);
    check(!blank && blank.error().code == "DYNAMIC_FIELD_NAMESPACE_REQUIRED",
          "Unicode blank namespace rejected");
    Vector<CompactRow> rows(r);
    for (std::uint64_t id : {7, 7, 8}) {
        Record native(r);
        native.emplace(String("id", r), Value(r, id));
        rows.emplace_back(native);
    }
    const std::array<std::uint64_t, 3> ids{7, 7, 8};
    DynamicStorageRows stored(r);
    auto& values = stored.try_emplace(7).first->second;
    values.emplace(String("name", r), Value(r, "private payload"));
    auto omitted = plan.merge(rows, ids, stored);
    check(!omitted && omitted.error().code == "DYNAMIC_FIELD_BATCH_OMITTED_OWNER" &&
              !rows[0].loaded_dynamic_fields(),
          "complete batch validates before any attachment");
    stored.try_emplace(8);
    check(bool(plan.merge(rows, ids, stored)), "duplicates and empty owners load");
    check(rows[0].loaded_dynamic_fields() != rows[1].loaded_dynamic_fields() &&
              rows[0].loaded_dynamic_fields()->selection_identity() ==
                  rows[1].loaded_dynamic_fields()->selection_identity(),
          "same owner views isolate payloads but share geometry");
    check(rows[2].loaded_dynamic_fields()->selected_codes().empty() &&
              !rows[0].contains_key("#name"),
          "absent stored fields do not become native columns");
    const std::array<std::uint64_t, 3> wrong{7, 8, 8};
    auto mismatch = plan.merge(rows, wrong, stored);
    check(!mismatch && mismatch.error().code == "DYNAMIC_FIELD_OWNER_MISMATCH",
          "owner sequence mismatch rejected");
    stored.try_emplace(9);
    auto extra = plan.merge(rows, ids, stored);
    check(!extra && extra.error().code == "DYNAMIC_FIELD_UNREQUESTED_OWNER",
          "unrequested owner rejected");
    auto row_ids = plan.owner_ids(rows);
    check(row_ids && row_ids.value().size() == 3,
          "owner ids extracted from native result projection");
    rows[0].remove("id");
    check(!plan.owner_ids(rows), "missing native owner id rejects");
    StreamOnly executor;
    SelectQuery query(std::allocator_arg, Allocator(r), "Item");
    query.comment("read");
    query.purpose("stream");
    auto request = QueryRequest::from_query(r, std::move(query));
    auto unsupported = executor.query_stream_with_dynamic_fields(
        r, std::move(request).value(), 16, detail::clone_in(r, plan), std::nullopt);
    check(!unsupported && unsupported.error().code == "DYNAMIC_FIELD_STREAM_UNSUPPORTED" &&
              executor.calls == 0,
          "unsupported enhancement fails before cursor open");
}

static void retention_and_debug() {
    GlobalContext global;
    DynamicFieldStreamPlan* retained = nullptr;
    DynamicFieldWrite* retained_write = nullptr;
    Store store;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
        const std::array fields{DynamicFieldSelection::Field{"name", DataType::Text}};
        auto selection = std::move(DynamicFieldSelection::fields(r, fields)).value();
        auto intent =
            std::move(QueryIntent::create(r, std::string(4096, 'c'), std::string(4096, 'p')))
                .value();
        auto plan = std::move(DynamicFieldStreamPlan::create(
                                  r, "SECRET-NAMESPACE", schema, selection, store, intent))
                        .value();
        retained = &global.make<DynamicFieldStreamPlan>(plan);
        DynamicFieldWrite::Changes changes(r);
        changes.emplace(String("name", r),
                        DynamicFieldMutation::set(r, Value(r, std::string(8192, 'x'))));
        auto& write =
            user_context->make<DynamicFieldWrite>("SECRET-NAMESPACE", schema, 999, changes);
        auto output = debug_string(r, write);
        check(
            output ==
                    "DynamicFieldWrite { owner_type: \"Item\", changed_fields: [\"name\"], .. }" &&
                output.find("SECRET") == String::npos && output.find("999") == String::npos,
            "write diagnostics exclude namespace, identity, owner id and operands");
        retained_write = &global.make<DynamicFieldWrite>(write);
    }
    check(retained->definitions().memory_resource() == global.memory_resource() &&
              bool(retained->validate_store(store)) && retained->selection().contains("name") &&
              !retained->selection().contains("count"),
          "Global plan retains copied definitions, selection and intent after User deletion");
    check(retained_write->changes.find("name")->second.value()->try_text()->size() == 8192 &&
              retained_write->changes.find("name")->second.memory_resource() ==
                  global.memory_resource(),
          "Global write retains deep copied pending operand");
}

int main() {
    try {
        plans_and_batches();
        retention_and_debug();
        std::cout << "Dynamic storage contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
