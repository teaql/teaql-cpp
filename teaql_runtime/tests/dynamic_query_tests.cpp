#include "../../tests/support/dynamic_hydration_fixture.hpp"
#include <teaql/runtime/entity_query.hpp>
#include <teaql/runtime/detail/data_service/audited_auto_save.hpp>
#include <teaql/runtime/detail/data_service/typed_aggregate.hpp>
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
    ::sql(
        *connection,
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
    auto aggregate_query = description.clone(r);
    aggregate_query.purpose("Hydrate the private typed aggregate result");
    auto aggregate = completed(
        detail::EntityDataServiceAccess::fetch_entities_with_relation_aggregates<Executor, School>(
            service.value().get(), aggregate_query, {}));
    check(aggregate && aggregate.value().data.size() == 2 &&
              aggregate.value().data.front().is_field_loaded("#title") &&
              aggregate.value().data.front().is_field_loaded("#count") &&
              aggregate.value().data.front().dynamic_field_values()->values().size() == 2 &&
              aggregate.value().data.front().root.current_change_set().is_empty(),
          "private typed aggregate hydrates persistent extensions through shared decoding");
    check(aggregate.value().data[0].loaded_state_snapshot()->identity() !=
                  aggregate.value().data[1].loaded_state_snapshot()->identity() &&
              !aggregate.value().data[1].is_field_loaded("#title"),
          "typed aggregate preserves distinct extension availability for each row");
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
    using School = dynamic_hydration_fixture::SourceSchool;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const auto& layout = School::field_layout(r);
    check(layout.field_count() == 3 && layout.index("id") == 0 && layout.index("version") == 1 &&
              layout.index("name") == 2 && layout.revision() == "dynamic-school-sqlite-v1",
          "SQLite stream source cases use the exact original three-field indexed geometry");
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection, false, true);
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                            DynamicFieldDefinitions::Field{"count", DataType::I64},
                            DynamicFieldDefinitions::Field{"extra", DataType::Text},
                            DynamicFieldDefinitions::Field{"amount", DataType::F64}};
    auto& definitions = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
    using Provider = DatabaseDynamicFieldsProvider<Executor>;
    const std::array<const DynamicFieldDefinitions*, 1> configured{&definitions};
    auto& provider =
        user_context->make<Provider>(std::move(Provider::create(r, "profile", configured)).value());
    user_context->set_dynamic_fields_provider(*user_context, provider);
    use_sqlite_provider(*user_context, *user_context, executor.transport);
    user_context->register_executor(*user_context, executor);
    check(bool(user_context->ensure_schema().wait()),
          "modeled native and extension schema initializes before audited stream seeds");
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    const std::array names{"alpha", "beta", "gamma"};
    for (std::uint64_t id : {7, 8, 9}) {
        School row(std::allocator_arg, Allocator(r));
        row.id = id;
        row.name = names[id - 7];
        row.mark_as_new();
        row.root.set(row.entity_key(), "name", Value(r, row.name));
        row.load_state =
            eval::LoadState::fully_loaded(r).into_indexed(pool, School::field_layout(r)).value();
        const auto& saved =
            AuditedSaveExt<School>::save(
                Audited<School>(
                    std::allocator_arg, Allocator(r), row, "seed ordinary root records"),
                *user_context)
                .wait();
        check(saved && saved.value().root.get_original_version(saved.value().entity_key()) == 1,
              "stream root bootstrap uses audited Save and version one");
    }
    auto bootstrap_service = user_context->entity_data_service<Executor>("School");
    check(bool(bootstrap_service), "stream bootstrap service resolves");
    SelectQuery seed_query(std::allocator_arg, Allocator(r), "School");
    seed_query.project("id").project("version").project("name").order_asc("id").limit(3);
    seed_query.select_dynamic_fields(DynamicFieldSelection::all(r));
    seed_query.comment("load stream extension seed owners");
    const auto& seed_rows = bootstrap_service.value()
                                .get()
                                .fetch_enhanced_entities<School>(purposed(r, seed_query))
                                .wait();
    check(seed_rows && seed_rows.value().data.size() == 3, "audited stream seed roots load");
    for (std::size_t index = 0; index < 2; ++index) {
        auto row = detail::clone_in(r, seed_rows.value().data[index]);
        check(bool(row.updateDynamicField("title", index == 0 ? Value(r, "stored") : Value(r))) &&
                  bool(row.updateDynamicField("extra", Value(r, "unselected-private"))),
              "stream fixture stages Value/NULL and unselected private text through ledger");
        if (index == 1) {
            check(bool(row.updateDynamicField("count", Value(r, std::int64_t(0)))),
                  "retain numeric unselected-value error-precedence control");
        }
        const auto& saved =
            AuditedSaveExt<School>::save(
                Audited<School>(std::allocator_arg,
                                Allocator(r),
                                row,
                                "seed stream value and explicit null through audited mutation"),
                *user_context)
                .wait();
        check(saved && saved.value().root.get_original_version(saved.value().entity_key()) == 2,
              "audited extension seeding advances native version before cursor execution");
    }
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
    auto& other_transport = user_context->make<SqliteMutationExecutor>(*connection);
    CompiledQuery source_query(std::allocator_arg, Allocator(r));
    source_query.sql = "SELECT id FROM school_data";
    source_query.comment.emplace("what: cursor binding negative test; why: reject before executing",
                                 r);
    auto foreign_plan = other_transport.stream_sql_with_dynamic_fields(
        r, detail::clone_in(r, source_query), 2, detail::clone_in(r, plan.value()));
    check(!foreign_plan && foreign_plan.error().code == "DYNAMIC_FIELD_STORAGE_PROVENANCE_MISMATCH",
          "another executor on the same connection cannot consume a trusted stream plan");
    auto& after_rejection = executor.transport.fetch_all_compact_sql(r, source_query);
    const auto* available = after_rejection.poll();
    check(available && *available && available->value().size() == 3,
          "provenance rejection leaves the native connection lease immediately available");
    for (std::size_t chunk_size : {1u, 2u, 3u, 73u, 8192u}) {
        auto bounded = detail::clone_in(r, request);
        bounded.project("version").limit(3);
        bounded.stream_config = StreamConfig{chunk_size};
        auto opened = completed(service.value().get().fetch_stream(purposed(r, bounded)));
        check(bool(opened), "selected dynamic cursor opens for every original batch limit");
        auto& cursor = opened.value().get();
        auto& root = user_context->make<EntityRuntimeState>();
        std::optional<LoadedSnapshot::Borrow> first_snapshot;
        Vector<School> retained(r);
        std::size_t seen = 0, chunks = 0;
        while (const auto* next = cursor.wait_next()) {
            check(bool(*next), "selected dynamic batch succeeds");
            const auto& chunk = next->value();
            check(chunk.chunk_index == chunks++ && !chunk.rows.empty() &&
                      chunk.rows.size() <= chunk_size,
                  "bounded ordered dynamic chunks");
            if (chunk_size == 2) {
                check(chunk.rows.size() == (chunk.chunk_index == 0 ? 2 : 1) &&
                          chunk.is_last == (chunk.chunk_index == 1),
                      "dynamic stream preserves full first chunk and final short tail");
                if (chunk.chunk_index == 0) {
                    auto& blocked = executor.transport.fetch_all_compact_sql(r, source_query);
                    check(!blocked.poll(), "dynamic cursor retains native lease between chunks");
                    blocked.cancel();
                }
            }
            auto decoded = decode_compact_rows_with_read_metadata<School>(
                r, detail::clone_in(r, chunk.rows), root);
            check(bool(decoded), "dynamic stream rows decode with indexed read metadata");
            for (const auto& row : decoded.value()) {
                const std::array names{"alpha", "beta", "gamma"};
                const std::array states{DynamicFieldState::Value,
                                        DynamicFieldState::Null,
                                        DynamicFieldState::NotLoaded};
                check(seen < 3 && row.id == seen + 7 && row.name == names[seen],
                      "stream preserves native row identities and names");
                const auto* fields = row.dynamic_field_values();
                check(fields && fields->field("title").value().state() == states[seen] &&
                          fields->field("count").value().state() == DynamicFieldState::NotLoaded &&
                          fields->field("extra").value().state() == DynamicFieldState::NotLoaded &&
                          !row.dirty_fields(),
                      "stream preserves selected value/null/missing and clean state");
                if (seen == 0) {
                    first_snapshot = row.loaded_state_snapshot();
                    check(fields->field("title").value().value()->try_text() == "stored",
                          "stream keeps selected payload unchanged");
                } else if (seen == 1 && chunk_size >= 2) {
                    check(first_snapshot && row.loaded_state_snapshot() &&
                              first_snapshot->identity() == row.loaded_state_snapshot()->identity(),
                          "value and NULL rows share their indexed availability geometry");
                }
                retained.emplace_back(row);
                ++seen;
            }
        }
        check(seen == 3 && chunks == (3 + chunk_size - 1) / chunk_size,
              "dynamic stream drains exactly the complete root range");
        auto& next_query = executor.transport.fetch_all_compact_sql(r, source_query);
        const auto* idle = next_query.poll();
        check(idle && *idle && idle->value().size() == 3,
              "EOF releases native lease without explicit cancellation");
        cursor.cancel();
        if (chunk_size == 2) {
            auto followup = detail::clone_in(r, bounded);
            followup.stream_config.reset();
            const auto& typed_followup =
                service.value().get().fetch_enhanced_entities<School>(purposed(r, followup)).wait();
            check(typed_followup && typed_followup.value().data.size() == 3,
                  "completed stream releases the transaction lease for typed enhanced queries");
        }
        check(retained.size() == 3, "all decoded entities survive cursor completion");
        const std::array expected_states{
            DynamicFieldState::Value, DynamicFieldState::Null, DynamicFieldState::NotLoaded};
        for (std::size_t index = 0; index < retained.size(); ++index) {
            const auto* fields = retained[index].dynamic_field_values();
            check(fields && fields->field("title").value().state() == expected_states[index] &&
                      fields->field("count").value().state() == DynamicFieldState::NotLoaded &&
                      fields->field("extra").value().state() == DynamicFieldState::NotLoaded &&
                      !retained[index].dirty_fields(),
                  "retained entities keep private dynamic state after cursor close");
        }
        check(retained[0].dynamic_field_values()->field("title").value().value()->try_text() ==
                  "stored",
              "retained streamed value remains readable in its live Context");
    }
    auto empty_request = detail::clone_in(r, request);
    empty_request.and_filter(Expr::eq(r, "id", Value(r, std::uint64_t(404))));
    empty_request.select_dynamic_fields(DynamicFieldSelection::all(r));
    auto empty_stream = completed(service.value().get().fetch_stream(purposed(r, empty_request)));
    check(empty_stream && !empty_stream.value().get().wait_next(),
          "empty selected dynamic cursor completes without publishing a chunk");
    auto zero_request_public = detail::clone_in(r, request);
    zero_request_public.stream_config = StreamConfig{0};
    auto zero_public =
        completed(service.value().get().fetch_stream(purposed(r, zero_request_public)));
    check(!zero_public && zero_public.error().to_string(r).find(
                              "DYNAMIC_FIELD_STREAM_CHUNK_REQUIRED") != String::npos,
          "public dynamic stream rejects a zero chunk size");
    auto& after_empty = executor.transport.fetch_all_compact_sql(r, source_query);
    const auto* empty_idle = after_empty.poll();
    check(empty_idle && *empty_idle, "empty and rejected cursors leave native lease available");
    auto selected_plan = provider.prepare_stream_read(
        *user_context,
        *executor.dynamic_field_store(),
        "School",
        std::move(DynamicFieldSelection::fields(r, selected_fields)).value(),
        read);
    check(bool(selected_plan), "selected stream batch plan is bound to native storage");
    auto duplicate_rows = [&] {
        Vector<CompactRow> rows(r);
        for (unsigned index = 0; index < 2; ++index) {
            Record row(r);
            row.emplace(String("id", r), Value(r, std::uint64_t(7)));
            rows.emplace_back(row);
        }
        return rows;
    };
    const std::array expected_errors{"DYNAMIC_FIELD_OWNER_MISMATCH",
                                     "DYNAMIC_FIELD_BATCH_OMITTED_OWNER",
                                     "DYNAMIC_FIELD_UNREQUESTED_OWNER",
                                     "DYNAMIC_FIELD_UNSELECTED_VALUE",
                                     "DYNAMIC_FIELD_TYPE_MISMATCH",
                                     "DYNAMIC_FIELD_UNSELECTED_VALUE"};
    for (unsigned mode = 0; mode < expected_errors.size(); ++mode) {
        auto rows = duplicate_rows();
        std::array<std::uint64_t, 2> ids{7, mode == 0 ? 8u : 7u};
        DynamicStorageRows stored(r);
        if (mode >= 2) {
            Value::Object values(r);
            if (mode == 3) {
                values.emplace(String("count", r), Value(r, "PRIVATE-CANARY"));
            } else if (mode == 4) {
                values.emplace(String("title", r), Value(r, true));
            } else if (mode == 5) {
                values.emplace(String("extra", r), Value(r, "PRIVATE-CANARY"));
            }
            stored.emplace(mode == 2 ? 8 : 7, std::move(values));
        }
        auto result = selected_plan.value().merge(rows, ids, stored);
        check(!result && result.error().code == expected_errors[mode],
              "complete stream batch validation preserves exact error precedence");
        check(result.error().to_string(r).find("PRIVATE-CANARY") == String::npos,
              "batch errors do not expose unselected values");
        for (const auto& row : rows) {
            check(!row.loaded_dynamic_fields(),
                  "failed batch publishes no partial dynamic sidecar");
        }
    }
    auto duplicates = duplicate_rows();
    const std::array<std::uint64_t, 2> duplicate_ids{7, 7};
    DynamicStorageRows stored(r);
    Value::Object values(r);
    values.emplace(String("title", r), Value(r, "duplicate"));
    stored.emplace(7, std::move(values));
    check(bool(selected_plan.value().merge(duplicates, duplicate_ids, stored)),
          "valid duplicate-owner stream batch merges");
    const auto* first_fields = duplicates[0].loaded_dynamic_fields();
    const auto* second_fields = duplicates[1].loaded_dynamic_fields();
    check(first_fields && second_fields &&
              &first_fields->definitions() == &second_fields->definitions() &&
              first_fields->selection_identity() == second_fields->selection_identity(),
          "duplicate stream owners share definitions and actual selected-code identity");
    for (const auto& row : duplicates) {
        check(row.loaded_dynamic_fields()->values().at(String("title", r)).try_text() ==
                  "duplicate",
              "each duplicate view retains the original value");
    }
    auto early_request = detail::clone_in(r, request);
    early_request.project("version").limit(3);
    early_request.stream_config = StreamConfig{1};
    auto early_opened = completed(service.value().get().fetch_stream(purposed(r, early_request)));
    check(bool(early_opened), "early-close cursor opens");
    auto& early_cursor = early_opened.value().get();
    const auto* early_chunk = early_cursor.wait_next();
    check(early_chunk && *early_chunk && early_chunk->value().rows.size() == 1,
          "early-close cursor exposes one row before completion");
    auto& early_root = user_context->make<EntityRuntimeState>();
    auto early_rows = decode_compact_rows_with_read_metadata<School>(
        r, detail::clone_in(r, early_chunk->value().rows), early_root);
    check(early_rows && early_rows.value().size() == 1, "held streamed entity decodes");
    auto& held = early_rows.value().front();
    auto& blocked_save_connection = executor.transport.fetch_all_compact_sql(r, source_query);
    check(!blocked_save_connection.poll(), "early cursor owns native lease before close");
    blocked_save_connection.cancel();
    early_cursor.cancel();
    auto& released_save_connection = executor.transport.fetch_all_compact_sql(r, source_query);
    const auto* released = released_save_connection.poll();
    check(released && *released, "early cancellation releases native lease before Save");
    check(bool(held.updateDynamicField("title", Value(r, "after-drop"))),
          "held entity stages dynamic update after cursor close");
    user_context->register_executor(*user_context, executor);
    Audited<School> audited(std::allocator_arg,
                            Allocator(r),
                            held,
                            "save a held streamed entity after early cursor close");
    const auto& saved = AuditedSaveExt<School>::save(std::move(audited), *user_context).wait();
    if (!saved) {
        throw std::runtime_error(std::string(saved.error().to_string(r)));
    }
    const auto& readback =
        service.value().get().fetch_enhanced_entities<School>(purposed(r, request)).wait();
    check(readback && readback.value().data.size() == 3 &&
              readback.value()
                      .data[0]
                      .dynamic_field_values()
                      ->field("title")
                      .value()
                      .value()
                      ->try_text() == "after-drop",
          "audited held-stream Save persists and reads back through the normal provider");
    ::sql(
        *connection,
        "UPDATE teaql_dynamic_field_storage_v1 SET data_type='I64',is_null=0,payload='PRIVATE-CANARY' WHERE owner_id='8' AND code='title'");
    request.stream_config = StreamConfig{2};
    auto corrupted = completed(service.value().get().fetch_stream(purposed(r, request)));
    check(bool(corrupted), "corrupted storage is detected at batch load");
    const auto* failed = corrupted.value().get().wait_next();
    check(failed && !*failed, "corrupt second row rejects the complete first batch");
    auto error = failed->error().to_string(r);
    check(error.find("DYNAMIC_FIELD_DEFINITION_MISMATCH") != String::npos &&
              error.find("PRIVATE-CANARY") == String::npos,
          "stored type failure preserves exact code without leaking payload");
    check(!corrupted.value().get().wait_next(), "failed dynamic cursor is terminal");
    auto& after_failure = executor.transport.fetch_all_compact_sql(r, source_query);
    const auto* failure_idle = after_failure.poll();
    check(failure_idle && *failure_idle && failure_idle->value().size() == 3,
          "failed dynamic cursor releases native lease");
    SelectQuery native_only(std::allocator_arg, Allocator(r), "School");
    native_only.project("id").project("name").project("version").limit(3).comment(
        "read native rows after a failed extension stream");
    const auto& recovered = service.value()
                                .get()
                                .fetch_enhanced_entities<School>(PurposedSelectQuery(
                                    std::allocator_arg,
                                    Allocator(r),
                                    native_only,
                                    "prove cursor failure releases the transaction lease"))
                                .wait();
    check(recovered && recovered.value().data.size() == 3,
          "typed native reads succeed after corrupted dynamic batch");
    check(sqlite3_get_autocommit(connection->native_handle()) != 0,
          "stream cleanup leaves no transaction");
}

struct PendingDynamicCounts {
    unsigned loads = 0;
    unsigned cancelled = 0;
    unsigned destroyed = 0;
    std::size_t joined_ids = 0;
    std::uint64_t joined_id_sum = 0;
    unsigned delivered = 0;
    bool released = false;
    bool fail = false;
    WakeSignal* wake = nullptr;
};

struct PendingDynamicBatch {
    using allocator_type = Allocator;
    PendingDynamicCounts& counts;
    Vector<std::uint64_t> ids;
    const DynamicFieldDefinitions& definitions;
    bool joined = false;

    PendingDynamicBatch(std::allocator_arg_t,
                        Allocator a,
                        PendingDynamicCounts& counts,
                        std::span<const std::uint64_t> ids,
                        const DynamicFieldDefinitions& definitions)
        : counts(counts), ids(ids.begin(), ids.end(), a.resource()), definitions(definitions) {}

    ~PendingDynamicBatch() {
        ++counts.destroyed;
    }

    std::optional<Result<DynamicFieldBatch, DynamicFieldError>> poll(WakeSignal& wake) {
        counts.wake = &wake;
        if (!counts.released) {
            return {};
        }
        ++counts.delivered;
        auto r = ids.get_allocator().resource();
        using Output = Result<DynamicFieldBatch, DynamicFieldError>;
        if (counts.fail) {
            return Output::err(r, DynamicFieldError(r, "FIXTURE_RESUME_ERROR", "School"));
        }
        DynamicStorageRows rows(r);
        for (auto id : ids) {
            rows.try_emplace(id).first->second.emplace(String("title", r),
                                                       id == 7 ? Value(r, "resumed") : Value(r));
        }
        return Output::ok(r,
                          DynamicFieldBatch(std::allocator_arg, Allocator(r), definitions, rows));
    }

    void cancel_and_wait() noexcept {
        if (!joined) {
            counts.joined_ids = ids.size();
            for (auto id : ids) {
                counts.joined_id_sum += id;
            }
            counts.wake = nullptr;
            ++counts.cancelled;
            joined = true;
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }
};

struct PendingDynamicProvider final : DynamicFieldsProvider {
    using allocator_type = Allocator;
    PendingDynamicCounts& counts;

    PendingDynamicProvider(std::allocator_arg_t, Allocator, PendingDynamicCounts& counts)
        : counts(counts) {}

    DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext& user_context,
                                                         std::string_view,
                                                         std::span<const std::uint64_t> ids,
                                                         const DynamicFieldSelection&,
                                                         const QueryIntent&) const override {
        ++counts.loads;
        auto r = user_context.memory_resource();
        const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text},
                                DynamicFieldDefinitions::Field{"count", DataType::I64}};
        auto& definitions = user_context.make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
        return make_operation<Result<DynamicFieldBatch, DynamicFieldError>, PendingDynamicBatch>(
            r, counts, ids, definitions);
    }
};

enum class DynamicReadEntry { Ordinary, TypedAggregate, Enhanced, EnhancedContinuous };

enum class DynamicOutcome { DeleteContext, Cancel, Resume, Fail };

static void typed_dynamic_cleanup() {
    for (auto entry : {DynamicReadEntry::Ordinary,
                       DynamicReadEntry::TypedAggregate,
                       DynamicReadEntry::Enhanced,
                       DynamicReadEntry::EnhancedContinuous}) {
        for (auto outcome : {DynamicOutcome::DeleteContext,
                             DynamicOutcome::Cancel,
                             DynamicOutcome::Resume,
                             DynamicOutcome::Fail}) {
            GlobalContext global;
            PendingDynamicCounts counts;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            SqliteConnection* connection = nullptr;
            auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection, false);
            ::sql(
                *connection,
                "CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);INSERT INTO school_data VALUES(7,'alpha',1),(8,'beta',2)");
            auto& provider = user_context->make<PendingDynamicProvider>(counts);
            user_context->set_dynamic_fields_provider(*user_context, provider);
            auto& service = EntityDataService<Executor>::for_executor(
                *user_context, "School", *user_context, executor);
            auto& description = user_context->make<SelectQuery>(query(r));
            description.purpose("Verify typed dynamic-work cleanup");
            if (entry == DynamicReadEntry::EnhancedContinuous) {
                description.page(0, 2).optimize_for_continuous_page_fetch();
            }
            auto& work = [&]() -> DataServiceOperationResult<SmartList<School>,
                                                             DataServiceError<Executor::Error>>& {
                if (entry == DynamicReadEntry::TypedAggregate) {
                    return detail::EntityDataServiceAccess::
                        fetch_entities_with_relation_aggregates<Executor, School>(
                            service, description, {});
                }
                auto request = purposed(r, description);
                if (entry == DynamicReadEntry::Ordinary) {
                    return service.fetch_entities<School>(request);
                }
                return service.fetch_enhanced_entities<School>(request);
            }();
            check(!work.poll() && counts.loads == 1,
                  "typed entry reaches the pending dynamic provider after loading native rows");
            if (outcome == DynamicOutcome::Resume || outcome == DynamicOutcome::Fail) {
                auto ticket = work.wake_signal().ticket();
                check(counts.wake != nullptr, "pending provider exposes its wake signal");
                counts.released = true;
                counts.fail = outcome == DynamicOutcome::Fail;
                counts.wake->notify();
                check(work.wake_signal().ticket() != ticket,
                      "provider notification reaches the typed caller");
                const auto& result = work.wait();
                check(counts.loads == 1 && counts.delivered == 1,
                      "resumption neither reloads the provider nor duplicates delivery");
                if (counts.fail) {
                    check(!result && result.error().runtime_error().dynamic_field_error().code ==
                                         "FIXTURE_RESUME_ERROR",
                          "resumed provider failure reaches the caller unchanged");
                } else {
                    check(result && result.value().data.size() == 2,
                          "resumed typed query preserves both native rows");
                    const auto& rows = result.value().data;
                    check(rows[0].id == 7 && rows[1].id == 8 &&
                              rows[0].dynamic_field_values()->field("title").value().state() ==
                                  DynamicFieldState::Value &&
                              rows[1].dynamic_field_values()->field("title").value().state() ==
                                  DynamicFieldState::Null &&
                              !rows[0].is_field_loaded("#count") &&
                              !rows[1].is_field_loaded("#count"),
                          "resumption preserves identity, value, NULL and absent dynamic fields");
                    check(rows[0].loaded_state_snapshot()->identity() ==
                              rows[1].loaded_state_snapshot()->identity(),
                          "equal multirow availability shares its loaded-state snapshot");
                }
                check(work.poll() == &result && counts.delivered == 1,
                      "repeated terminal polling does not decode or fetch again");
            }
            if (outcome == DynamicOutcome::Cancel) {
                work.cancel();
                check(counts.cancelled == 1 && counts.joined_ids == 2 && counts.joined_id_sum == 15,
                      "typed entry cancellation joins its dynamic provider before releasing rows");
            }
            user_context.reset();
            check(
                counts.cancelled == 1 && counts.destroyed == 1 && counts.joined_ids == 2 &&
                    counts.joined_id_sum == 15 && global.live_user_bytes() == 0,
                "actual Context deletion joins and destroys dynamic work before arena reclamation");
        }
    }
}

int main() {
    root_queries_and_carriers();
    decode_metadata_and_batch_validation();
    database_stream_and_cleanup();
    typed_dynamic_cleanup();
    std::cout << "Dynamic query hydration contracts passed\n";
}
