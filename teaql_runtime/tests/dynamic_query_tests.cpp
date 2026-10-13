#include "../../tests/support/dynamic_hydration_fixture.hpp"
#include <teaql/runtime/entity_query.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;
using dynamic_hydration_fixture::School;
using dynamic_hydration_fixture::Executor;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class T>
static T completed(AsyncOperation<T>& operation) {
    operation.wait();
    return operation.take_result();
}

static void sql(SqliteConnection& connection, const char* sql) {
    check(sqlite3_exec(connection.native_handle(), sql, nullptr, nullptr, nullptr) == SQLITE_OK,
          "fixture SQL succeeds");
}

static SelectQuery query(Memory r) {
    SelectQuery query(std::allocator_arg, Allocator(r), "School");
    query.project("name").order_asc("id").select_dynamic_fields(DynamicFieldSelection::all(r));
    query.comment_text.emplace("inspect dynamic schools", r);
    return query;
}

static PurposedSelectQuery purposed(Memory r, const SelectQuery& query) {
    return PurposedSelectQuery(std::allocator_arg, Allocator(r), query, "hydrate schools");
}

struct PlainSchool : example::Model<PlainSchool> {
    using Base = example::Model<PlainSchool>;
    static constexpr std::string_view ENTITY_NAME = "School";

    PlainSchool(std::allocator_arg_t, Allocator a) : Base(std::allocator_arg, a) {}

    PlainSchool(std::allocator_arg_t, Allocator a, const PlainSchool& other)
        : Base(std::allocator_arg, a, other) {}

    PlainSchool(std::allocator_arg_t, Allocator a, PlainSchool&& other)
        : PlainSchool(detail::move_to(a.resource(), std::move(other))) {}

    PlainSchool(PlainSchool&&) noexcept = default;

    PlainSchool& operator=(PlainSchool&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        return School::entity_descriptor(r);
    }

    static Result<PlainSchool, EntityError> from_compact_row(Memory r, CompactRow row) {
        PlainSchool entity(std::allocator_arg, Allocator(r));
        auto id = example::decode_field<std::uint64_t>(r, row, ENTITY_NAME, "id");
        if (!id) {
            return Result<PlainSchool, EntityError>::err(r, std::move(id).error());
        }
        entity.id = id.value();
        entity.root.set_original_compact_row(ENTITY_NAME, std::move(row));
        return Result<PlainSchool, EntityError>::ok(r, std::move(entity));
    }

    MutationValues into_values() && {
        return scalar_values();
    }
};

static void root_queries_and_carriers() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection);
    sql(*connection,
        "CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);INSERT INTO school_data VALUES(7,'alpha',1),(8,'beta',2)");
    auto service = user_context->entity_data_service<Executor>("School");
    check(bool(service), "service resolves");
    auto description = query(r);
    auto basic = completed(service.value().get().fetch_entities<School>(purposed(r, description)));
    check(basic && basic.value().data.size() == 2, "root query loads entities");
    auto& first = basic.value().data.front();
    check(first.dynamic_field_values() && first.dynamic_field_values()->values().size() == 2 &&
              first.is_field_loaded("#title") && first.is_field_loaded("#count"),
          "persistent extensions install typed availability");
    check(first.is_field_loaded("id") && first.is_field_loaded("version") &&
              !first.is_field_loaded("address"),
          "native sparse state preserved");
    check(first.dynamic_field_values()->values().at(String("count", r)).is_null(),
          "explicit NULL is loaded");
    check(first.root.current_change_set().is_empty(), "hydration creates no ledger mutations");
    check(!first.root.original_snapshot()->fields.contains("#title"),
          "extensions are absent from native originals");
    auto enhanced =
        completed(service.value().get().fetch_enhanced_entities<School>(purposed(r, description)));
    check(enhanced && enhanced.value().data.front().is_field_loaded("#title"),
          "enhanced root uses shared hydration");
    auto unsupported =
        completed(service.value().get().fetch_entities<PlainSchool>(purposed(r, description)));
    check(!unsupported &&
              unsupported.error().kind() == decltype(unsupported.error().kind())::Entity &&
              unsupported.error().entity_error().message ==
                  "dynamic fields require a runtime-owned indexed entity carrier",
          "ordinary carrier rejects extensions");
    auto empty = query(r);
    empty.filter(Expr::eq(r, "id", Value(r, std::uint64_t(999))));
    auto empty_result =
        completed(service.value().get().fetch_entities<PlainSchool>(purposed(r, empty)));
    check(empty_result && empty_result.value().data.empty(),
          "empty query skips unsupported carrier and provider loading");
    const std::array fields{DynamicFieldSelection::Field{"title", DataType::Text}};
    auto selected = std::move(DynamicFieldSelection::fields(r, fields)).value();
    description.select_dynamic_fields(selected);
    auto partial =
        completed(service.value().get().fetch_entities<School>(purposed(r, description)));
    check(partial && partial.value().data.front().is_field_loaded("#title") &&
              !partial.value().data.front().is_field_loaded("#count"),
          "selected extension geometry remains sparse");
    auto different = description.clone(r);
    different.select_dynamic_fields(DynamicFieldSelection::all(r));
    check(description != different && description == description.clone(r),
          "query selection uses semantic equality across clones");
    auto& retained = global.make<SelectQuery>(description);
    check(retained.dynamic_field_selection->memory_resource() == global.memory_resource(),
          "query selection propagates destination allocator");
    auto facade = EntityQuery<School, Executor>(*user_context)
                      .select("name")
                      .selectDynamicFields(selected)
                      .comment("typed root")
                      .purpose("hydrate");
    check(bool(facade.executeForList(*user_context).wait()),
          "runtime application facade executes selected extensions");
    auto& stream = service.value().get().fetch_stream(purposed(r, description));
    auto rejected = completed(stream);
    check(!rejected && rejected.error().runtime_error().dynamic_field_error().code ==
                           "DYNAMIC_FIELD_STREAM_UNSUPPORTED",
          "fixture provider cannot prepare cursor storage");
    (void)executor;
}

static void decode_metadata_and_batch_validation() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& root = user_context->make<EntityRuntimeState>();
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text}};
    auto& schema = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
    Vector<DynamicFieldValues::Values> payloads(r);
    for (std::string_view value : {"left", "right"}) {
        payloads.emplace_back();
        payloads.back().emplace(String("title", r), Value(r, value));
    }
    auto values = std::move(DynamicFieldValues::from_batch(r, schema, payloads)).value();
    Vector<CompactRow> rows(r);
    for (std::size_t i = 0; i < 2; ++i) {
        Record native(r);
        native.emplace(String("id", r), Value(r, std::uint64_t(7)));
        native.emplace(String("name", r), Value(r, "alpha"));
        rows.emplace_back(native);
        rows.back().set_loaded_dynamic_fields(values[i]);
    }
    auto decoded =
        decode_compact_rows_with_read_metadata<School>(r, Vector<CompactRow>(rows, r), root);
    check(decoded && decoded.value()[0].loaded_state_snapshot()->identity() ==
                         decoded.value()[1].loaded_state_snapshot()->identity(),
          "duplicate views reuse augmented load geometry");
    auto changed = decoded.value()[0].root.update_dynamic_field(
        decoded.value()[0].entity_key(), "title", Value(r, "changed"));
    check(
        changed &&
            decoded.value()[1].dynamic_field_values()->values().at(String("title", r)).try_text() ==
                "right",
        "decoded views retain independent extension values");
    check(!root.loaded_dynamic_fields(),
          "decoder does not attach one row payload to shared graph root");
    auto ordinary =
        decode_compact_rows_with_read_metadata<PlainSchool>(r, Vector<CompactRow>(rows, r), root);
    check(!ordinary && ordinary.error().message ==
                           "dynamic fields require a runtime-owned indexed entity carrier",
          "central decoder enforces carrier contract");
    auto& graph = user_context->make<EntityGraphBuilder>();
    auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
    registry.register_entity<School>();
    check(bool(registry.decode_compact_batch("School", Vector<CompactRow>(rows, r), root, graph)),
          "type-erased graph decoder preserves metadata");
}

static void database_stream_and_cleanup() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection, false);
    sql(*connection,
        "CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);INSERT INTO school_data VALUES(7,'alpha',1),(8,'beta',2),(9,'gamma',3)");
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto& definitions = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
    using Provider = DatabaseDynamicFieldsProvider<Executor>;
    const std::array<const DynamicFieldDefinitions*, 1> configured{&definitions};
    auto& provider =
        user_context->make<Provider>(std::move(Provider::create(r, "profile", configured)).value());
    user_context->set_dynamic_fields_provider(*user_context, provider);
    check(bool(user_context->ensure_dynamic_fields_schema().wait()),
          "database extension schema initialized");
    auto bound = provider.definitions_for_context(*user_context, "School");
    auto& tx = executor.transport.begin_sql(r).wait().value().get();
    Vector<DynamicFieldWrite> writes(r);
    for (std::uint64_t id : {7, 8}) {
        DynamicFieldWrite::Changes changes(r);
        if (id == 7) {
            changes.emplace(String("title", r), DynamicFieldMutation::set(r, Value(r, "stored")));
        } else {
            changes.emplace(String("count", r),
                            DynamicFieldMutation::set(r, Value(r, std::int64_t(0))));
        }
        auto prepared = provider.prepare_graph_write(
            *user_context, *tx.dynamic_field_store(), bound.value().get(), id, changes);
        writes.emplace_back(std::move(prepared).value());
    }
    auto reason = std::move(MutationIntent::create(r, "seed dynamic stream")).value();
    check(bool(tx.dynamic_field_store()->apply_writes(r, writes, reason).wait()),
          "transaction seeds extension rows");
    tx.commit_sql(r).wait();
    auto service = user_context->entity_data_service<Executor>("School");
    auto request = query(r);
    request.purpose_text.emplace("hydrate schools", r);
    request.project("id");
    request.stream_config = StreamConfig{2};
    auto loaded = completed(service.value().get().fetch_stream(purposed(r, request)));
    check(bool(loaded), "database provider prepares real native cursor");
    auto& stream = loaded.value().get();
    auto* first = stream.wait_next();
    check(first && *first && first->value().rows.size() == 2 &&
              first->value()
                      .rows.front()
                      .loaded_dynamic_fields()
                      ->values()
                      .at(String("title", r))
                      .try_text() == "stored",
          "cursor loads extensions inside native lease before publishing chunk");
    auto ordinary_request = QueryRequest::from_query(r, request, false, false);
    auto& waiting = executor.query(r, std::move(ordinary_request).value());
    check(!waiting.poll(), "ambient query waits while enhanced cursor owns lease");
    stream.cancel();
    check(bool(waiting.wait()), "early cursor cancellation releases queued native work");
    request.stream_config = StreamConfig{1};
    auto typed = completed(service.value().get().fetch_entity_stream<School>(purposed(r, request)));
    check(bool(typed), "typed stream starts");
    auto& typed_stream = typed.value().get();
    auto* entity = typed_stream.wait_next();
    check(entity && *entity && entity->value().is_field_loaded("#title") &&
              !entity->value().is_field_loaded("#count"),
          "typed stream preserves sparse dynamic state");
    typed_stream.cancel();
    auto selection = DynamicFieldSelection::all(r);
    auto read = std::move(QueryIntent::create(r, "inspect stream", "hydrate schools")).value();
    auto plan = provider.prepare_stream_read(
        *user_context, *executor.dynamic_field_store(), "School", selection, read);
    auto zero_request = QueryRequest::from_query(r, request, false, false);
    auto zero = executor.query_stream_with_dynamic_fields(
        r, std::move(zero_request).value(), 0, detail::clone_in(r, plan.value()), {});
    check(!zero && zero.error().code == "DYNAMIC_FIELD_STREAM_CHUNK_REQUIRED",
          "zero chunk rejects before opening cursor");
    const std::array selected_fields{DynamicFieldSelection::Field{"title", DataType::Text}};
    request.select_dynamic_fields(
        std::move(DynamicFieldSelection::fields(r, selected_fields)).value());
    auto mismatch_request = QueryRequest::from_query(r, request, false, false);
    auto mismatch = executor.query_stream_with_dynamic_fields(
        r, std::move(mismatch_request).value(), 1, detail::clone_in(r, plan.value()), {});
    check(!mismatch && mismatch.error().code == "DYNAMIC_FIELD_STREAM_SELECTION_MISMATCH",
          "query and trusted plan must select the same fields");
    check(sqlite3_get_autocommit(connection->native_handle()) != 0,
          "stream cleanup leaves no transaction");
}

int main() {
    root_queries_and_carriers();
    decode_metadata_and_batch_validation();
    database_stream_and_cleanup();
    std::cout << "Dynamic query hydration contracts passed\n";
}
