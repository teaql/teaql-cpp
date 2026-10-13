#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static DynamicFieldDefinitions definitions(Memory r, std::string_view revision = "v1") {
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    return std::move(DynamicFieldDefinitions::from_fields(r, "Item", revision, fields)).value();
}

static QueryIntent intent(Memory r) {
    return std::move(QueryIntent::create(r, "inspect extensions", "hydrate views")).value();
}

template <class T>
static T completed(AsyncOperation<T>& operation) {
    operation.wait();
    return operation.take_result();
}

static void fixture_and_batch() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    InMemoryDynamicFieldsProvider* retained = nullptr;
    const std::array<std::uint64_t, 3> ids{7, 7, 8};
    {
        auto r = user_context->memory_resource();
        auto read = intent(r);
        auto all = DynamicFieldSelection::all(r);
        check(bool(user_context->ensure_dynamic_fields_schema().wait()),
              "missing provider schema is a no-op");
        auto missing = completed(user_context->load_dynamic_fields("Item", ids, all, read));
        check(!missing && missing.error().code == "DYNAMIC_FIELD_PROVIDER_MISSING",
              "missing provider typed error");
        auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
        DynamicStorageRows rows(r);
        rows.try_emplace(7).first->second.emplace(String("title", r), Value(r, "private"));
        rows.try_emplace(7).first->second.emplace(String("count", r), Value(r));
        const std::array owners{InMemoryDynamicFieldsProvider::Owner{&schema, &rows}};
        auto configured = InMemoryDynamicFieldsProvider::from_owners(r, owners);
        auto& provider =
            user_context->make<InMemoryDynamicFieldsProvider>(std::move(configured).value());
        user_context->set_dynamic_fields_provider(*user_context, provider);
        check(!user_context->dynamic_graph_provider(), "fixtures cannot perform graph saves");
        auto loaded = completed(user_context->load_dynamic_fields("Item", ids, all, read));
        check(loaded && loaded.value().rows.size() == 2 && loaded.value().rows.at(8).empty(),
              "missing owners have empty entries");
        auto batch = std::move(loaded).value();
        auto values = std::move(batch).into_values("Item", ids, all);
        check(values && values.value().size() == 3, "duplicate views retained");
        auto& first = user_context->make<DynamicFieldValues>(std::move(values.value()[0]));
        auto& second = user_context->make<DynamicFieldValues>(std::move(values.value()[1]));
        check(first.selection_identity() == second.selection_identity(),
              "matching views share geometry");
        check(bool(first.assign("title", Value(r, "changed"))),
              "fixture view can change independently");
        check(second.field("title").value().value()->to_json_value(r).dump(r) == "\"private\"",
              "sibling payload independent");
        const std::array selection_fields{DynamicFieldSelection::Field{"title", DataType::Text}};
        auto selection = std::move(DynamicFieldSelection::fields(r, selection_fields)).value();
        auto selected = completed(user_context->load_dynamic_fields("Item", ids, selection, read));
        check(selected && selected.value().rows.at(7).size() == 1,
              "fixture filters selected values");
        DynamicFieldBatch unselected(std::allocator_arg, Allocator(r), schema, rows);
        auto rejected = std::move(unselected).into_values("Item", ids, selection);
        check(!rejected && rejected.error().code == "DYNAMIC_FIELD_UNSELECTED_VALUE",
              "batch rejects unexpected selected value");
        DynamicFieldBatch omitted(std::allocator_arg, Allocator(r), schema, rows);
        auto omitted_result = std::move(omitted).into_values("Item", ids, all);
        check(!omitted_result && omitted_result.error().code == "DYNAMIC_FIELD_BATCH_OMITTED_OWNER",
              "batch requires every owner");
        const std::array<std::uint64_t, 1> extra{8};
        DynamicFieldBatch unsolicited(std::allocator_arg, Allocator(r), schema, rows);
        auto unsolicited_result = std::move(unsolicited).into_values("Item", extra, all);
        check(!unsolicited_result &&
                  unsolicited_result.error().code == "DYNAMIC_FIELD_UNREQUESTED_OWNER",
              "batch rejects extra owner");
        DynamicFieldBatch wrong(std::allocator_arg, Allocator(r), schema, rows);
        auto wrong_result = std::move(wrong).into_values("Other", ids, all);
        check(!wrong_result && wrong_result.error().code == "DYNAMIC_FIELD_OWNER_MISMATCH",
              "batch rejects owner type");
        const std::array duplicate{owners[0], owners[0]};
        auto duplicate_result = InMemoryDynamicFieldsProvider::from_owners(r, duplicate);
        check(!duplicate_result && duplicate_result.error().code == "DYNAMIC_FIELD_DUPLICATE_OWNER",
              "duplicate configuration fails");
        retained = &global.make<InMemoryDynamicFieldsProvider>(provider);
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    next_user_context->set_dynamic_fields_provider(global, *retained);
    auto next_r = next_user_context->memory_resource();
    auto copied = completed(next_user_context->load_dynamic_fields(
        "Item", ids, DynamicFieldSelection::all(next_r), intent(next_r)));
    check(copied && copied.value()
                            .rows.at(7)
                            .at(String("title", next_r))
                            .to_json_value(next_r)
                            .dump(next_r) == "\"private\"",
          "explicit Global fixture copy survives User deletion");
}

static void database_provider() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& schema = user_context->make<DynamicFieldDefinitions>(definitions(r));
    const std::array<const DynamicFieldDefinitions*, 1> schemas{&schema};
    using Provider = DatabaseDynamicFieldsProvider<SqliteMutationExecutor>;
    auto invalid = Provider::create(r, "\u2003", schemas);
    check(!invalid && invalid.error().code == "DYNAMIC_FIELD_NAMESPACE_REQUIRED",
          "Unicode blank namespace fails");
    auto& provider =
        user_context->make<Provider>(std::move(Provider::create(r, "profile", schemas)).value());
    user_context->set_dynamic_fields_provider(*user_context, provider);
    auto missing = provider.definitions_for_context(*user_context, "Item");
    check(!missing && missing.error().code == "DYNAMIC_FIELD_EXECUTOR_MISSING",
          "configuration requires registered executor");
    auto& connection = user_context->make_resource<SqliteConnection>();
    auto& executor = user_context->make<SqliteMutationExecutor>(connection);
    user_context->insert_resource(*user_context, executor);
    check(bool(user_context->ensure_dynamic_fields_schema().wait()),
          "context schema uses registered storage");
    auto bound = provider.definitions_for_context(*user_context, "Item");
    check(bound &&
              bound.value().get().storage_binding_matches("profile", *executor.source_identity()),
          "bound schema has trusted identity");
    auto all = DynamicFieldSelection::all(r);
    auto read = intent(r);
    auto plan = user_context->prepare_dynamic_field_stream(executor, "Item", all, read);
    check(bool(plan), "context prepares trusted stream plan");
    auto& other_executor = user_context->make<SqliteMutationExecutor>(connection);
    check(!user_context->prepare_dynamic_field_stream(other_executor, "Item", all, read),
          "different storage instance rejected");
    auto& transaction = executor.begin_sql(r).wait().value().get();
    auto* store = transaction.dynamic_field_store();
    DynamicFieldWrite::Changes changes(r);
    changes.emplace(String("title", r), DynamicFieldMutation::set(r, Value(r, "persisted")));
    check(!provider.prepare_graph_write(*user_context, executor, bound.value().get(), 7, changes),
          "ambient writes rejected");
    auto unbound = provider.prepare_graph_write(*user_context, *store, schema, 7, changes);
    check(!unbound && unbound.error().code == "DYNAMIC_FIELD_STORAGE_PROVENANCE_MISMATCH",
          "unbound metadata is not trusted");
    auto prepared =
        provider.prepare_graph_write(*user_context, *store, bound.value().get(), 7, changes);
    check(prepared && prepared.value().namespace_name == "profile",
          "write consumes trusted metadata");
    Vector<DynamicFieldWrite> writes(r);
    writes.emplace_back(std::move(prepared).value());
    auto reason = std::move(MutationIntent::create(r, "save extensions")).value();
    check(bool(store->apply_writes(r, writes, reason).wait()),
          "prepared write executes only in transaction");
    const std::array<std::uint64_t, 2> ids{7, 8};
    auto& pending = user_context->load_dynamic_fields("Item", ids, all, read);
    check(!pending.poll(), "context read waits while exclusive transaction is active");
    transaction.commit_sql(r).wait();
    auto loaded = completed(pending);
    check(loaded &&
              loaded.value().rows.at(7).at(String("title", r)).to_json_value(r).dump(r) ==
                  "\"persisted\"" &&
              loaded.value().rows.at(8).empty(),
          "context reads committed dynamic values");
    check(loaded.value().definitions.get().storage_binding_matches("profile",
                                                                   *executor.source_identity()),
          "loaded batch has storage authority");
    auto& next_transaction = executor.begin_sql(r).wait().value().get();
    auto& cancelled = user_context->load_dynamic_fields("Item", ids, all, read);
    check(!cancelled.poll(), "second load is pending");
    cancelled.cancel();
    next_transaction.rollback_sql(r).wait();
    check(bool(user_context->load_dynamic_fields("Item", ids, all, read).wait()),
          "cancelled wrapper detaches queued SQLite work");
    Value actual(r, "actual");
    auto set = DynamicFieldMutation::set(r, Value(r, "proposed"));
    check(detail::mutation_audit_value(r, set, &actual).to_json_value(r).dump(r) ==
              "{\"operation\":\"SET\",\"value\":\"actual\"}",
          "audit uses actual persisted value");
    check(detail::mutation_audit_value(r, set, nullptr).to_json_value(r).dump(r) ==
              "{\"operation\":\"SET\",\"value\":\"proposed\"}",
          "audit falls back to proposed value");
    check(detail::mutation_audit_value(r, DynamicFieldMutation::erase(r), &actual)
                  .to_json_value(r)
                  .dump(r) == "{\"operation\":\"DELETE\"}",
          "delete audit contains no operand");
}

static void context_close_quiesces_provider() {
    GlobalContext global;
    auto r = global.memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& executor = global.make<SqliteMutationExecutor>(connection);
    auto& schema = global.make<DynamicFieldDefinitions>(definitions(r));
    const std::array<const DynamicFieldDefinitions*, 1> schemas{&schema};
    using Provider = DatabaseDynamicFieldsProvider<SqliteMutationExecutor>;
    auto& provider =
        global.make<Provider>(std::move(Provider::create(r, "profile", schemas)).value());
    auto user_context = global.make_user_context();
    user_context->insert_resource(global, executor);
    user_context->set_dynamic_fields_provider(global, provider);
    check(bool(user_context->ensure_dynamic_fields_schema().wait()),
          "Global provider initializes through UserContext");
    const std::array<std::uint64_t, 1> ids{7};
    {
        auto user_r = user_context->memory_resource();
        auto& transaction = executor.begin_sql(user_r).wait().value().get();
        auto schema_result = provider.definitions_for_context(*user_context, "Item");
        DynamicFieldWrite::Changes changes(user_r);
        changes.emplace(String("title", user_r),
                        DynamicFieldMutation::set(user_r, Value(user_r, "rolled back")));
        auto prepared = provider.prepare_graph_write(*user_context,
                                                     *transaction.dynamic_field_store(),
                                                     schema_result.value().get(),
                                                     7,
                                                     changes);
        Vector<DynamicFieldWrite> writes(user_r);
        writes.emplace_back(std::move(prepared).value());
        auto reason = std::move(MutationIntent::create(user_r, "abandoned save")).value();
        check(bool(transaction.dynamic_field_store()->apply_writes(user_r, writes, reason).wait()),
              "pending transaction writes fixture");
        auto& load = user_context->load_dynamic_fields(
            "Item", ids, DynamicFieldSelection::all(user_r), intent(user_r));
        check(!load.poll(), "provider load queues behind transaction");
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    next_user_context->insert_resource(global, executor);
    next_user_context->set_dynamic_fields_provider(global, provider);
    auto next_r = next_user_context->memory_resource();
    auto loaded = completed(next_user_context->load_dynamic_fields(
        "Item", ids, DynamicFieldSelection::all(next_r), intent(next_r)));
    check(loaded && loaded.value().rows.at(7).empty(),
          "Context destruction cancels provider work and rolls back before reclaiming memory");
}

int main() {
    fixture_and_batch();
    database_provider();
    context_close_quiesces_provider();
    std::cout << "Dynamic Context provider contracts passed\n";
}
