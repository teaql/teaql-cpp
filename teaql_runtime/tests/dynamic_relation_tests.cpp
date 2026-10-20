#include "../../tests/support/dynamic_hydration_fixture.hpp"
#include "../../examples/generated/include/cpp_examples.hpp"
#include <teaql/runtime/detail/data_service/relation_enhancement.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;
using dynamic_hydration_fixture::School;
using dynamic_hydration_fixture::Executor;

static void check(bool result, const char* message) {
    if (!result) {
        throw std::runtime_error(message);
    }
}

template <class T>
static T completed(AsyncOperation<T>& operation) {
    operation.wait();
    return operation.take_result();
}

static void seed(SqliteConnection& connection) {
    check(
        sqlite3_exec(
            connection.native_handle(),
            "CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);INSERT INTO school_data VALUES(7,'first',1),(8,'second',1)",
            nullptr,
            nullptr,
            nullptr) == SQLITE_OK,
        "fixture seed succeeds");
}

static SelectQuery fixture_query_request(Memory r) {
    SelectQuery q(std::allocator_arg, Allocator(r), "School");
    q.project("id").project("name").order_asc("id");
    q.comment_text.emplace("read relationships", r);
    q.purpose_text.emplace("inspect dynamic relations", r);
    return q;
}

static PurposedSelectQuery purposed(Memory r, const SelectQuery& q) {
    return PurposedSelectQuery(std::allocator_arg, Allocator(r), q, "inspect dynamic relations");
}

static void child_and_flat_loading() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection);
    seed(*connection);
    auto service = user_context->entity_data_service<Executor>("School");
    auto query = fixture_query_request(r);
    const std::array title{DynamicFieldSelection::Field{"title", DataType::Text}},
        count{DynamicFieldSelection::Field{"count", DataType::I64}};
    auto first = fixture_query_request(r);
    first.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, title)).value());
    auto second = fixture_query_request(r);
    second.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, count)).value());
    query.child_enhancements.emplace_back(first);
    query.child_enhancements.emplace_back(second);
    auto loaded = completed(service.value().get().fetch_all(purposed(r, query)));
    check(loaded && loaded.value().front().loaded_dynamic_fields()->values().size() == 2,
          "child queries merge dynamic values and selected geometry");
    auto& decoder = user_context->make<InMemoryEntityGraphDecoderRegistry>();
    decoder.register_entity<School>();
    user_context->set_entity_graph_decoder_registry(decoder);
    auto descriptor = School::entity_descriptor(r);
    auto& relation = descriptor.relation("peers", "School");
    relation.many = true;
    dynamic_cast<InMemoryMetadataStore&>(const_cast<SchemaProvider&>(executor.schema_provider))
        .register_entity(std::move(descriptor));
    RelationDescriptor specification(std::allocator_arg, Allocator(r), "peers", "School");
    specification.many = true;
    Vector<RelationLoadPlan> plans(r);
    plans.emplace_back("School", specification);
    auto selected = fixture_query_request(r);
    selected.select_dynamic_fields(DynamicFieldSelection::all(r));
    plans.front().query.emplace(std::allocator_arg, Allocator(r), selected);
    auto& root = user_context->make<EntityRuntimeState>();
    auto& graph = user_context->make<EntityGraphBuilder>();
    auto& parents = user_context->make<Vector<CompactRow>>();
    Record parent(r);
    parent.emplace(String("id", r), Value(r, std::uint64_t(7)));
    parents.emplace_back(parent);
    detail::EntityRelationFetch<Executor> fetch(
        std::allocator_arg, Allocator(r), [&](const auto& child, SelectQuery query) -> auto& {
            return child.fetch_all(purposed(r, query));
        });
    auto& work = detail::hydrate_compact_flat_plans(
        service.value().get(), parents, plans, root, graph, fetch);
    check(bool(work.wait()), "flat relation loads dynamic provider before graph decoding");
    check(bool(root.freeze_graph(std::move(graph))), "graph freezes");
    auto* peers = root.resolve_relation_list<School>("School", 7, "peers");
    check(peers && peers->data.size() == 1 && peers->data.front().dynamic_field_values() &&
              peers->data.front().is_field_loaded("#title"),
          "typed relation graph preserves extension values and availability");
}

static void checked_merge_and_saved_keys() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array fields{DynamicFieldDefinitions::Field{"title", DataType::Text}};
    auto& old_definition = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", fields)).value());
    auto& new_definition = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "v2", fields)).value());
    Vector<CompactRow> parents(r), children(r);
    for (auto* rows : {&parents, &children}) {
        Record values(r);
        values.emplace(String("id", r), Value(r, std::uint64_t(7)));
        values.emplace(String("name", r), Value(r, rows == &parents ? "original" : "overwrite"));
        rows->emplace_back(values);
    }
    Value::Object payload(r);
    payload.emplace(String("title", r), Value(r, "value"));
    parents.front().set_loaded_dynamic_fields(
        std::move(DynamicFieldValues::from_values(r, old_definition, payload)).value());
    children.front().set_loaded_dynamic_fields(
        std::move(DynamicFieldValues::from_values(r, new_definition, payload)).value());
    auto merged = detail::attach_child_query_rows(parents, children);
    check(!merged && parents.front().get("name")->try_text() == "original",
          "definition mismatch rejects child overlay before native values change");
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection);
    RelationDescriptor specification(std::allocator_arg, Allocator(r), "peers", "School");
    specification.many = true;
    RelationLoadPlan plan(std::allocator_arg, Allocator(r), "School", specification);
    children.front().insert("id", Value(r, std::uint64_t(8)));
    Vector<std::optional<Value>> keys(r);
    keys.emplace_back(Value(r, std::uint64_t(7)));
    detail::attach_relation_rows(
        *user_context, parents, plan, Vector<CompactRow>(children, r), keys);
    auto* related = std::get_if<Value::List>(&parents.front().get("peers")->data);
    check(related && related->size() == 1,
          "attachment retains fetch-time key after child row overwrite");
    (void)executor;
    (void)connection;
}

struct ChildContextMarker {};

struct ChildBatchReady {
    using allocator_type = Allocator;
    using Output = Result<DynamicFieldBatch, DynamicFieldError>;
    Output result;

    ChildBatchReady(std::allocator_arg_t, Allocator a, Output value)
        : result(detail::move_in(a.resource(), std::move(value))) {}

    std::optional<Output> poll(WakeSignal&) {
        return std::move(result);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

struct ChildProvider final : DynamicFieldsProvider {
    using allocator_type = Allocator;
    unsigned fault;
    std::size_t expected_ids = 3;
    mutable unsigned calls = 0;
    mutable String last_comment;
    mutable String last_purpose;
    mutable Vector<std::uint64_t> last_ids;

    ChildProvider(std::allocator_arg_t, Allocator a, unsigned fault)
        : fault(fault), last_comment(a.resource()), last_purpose(a.resource()),
          last_ids(a.resource()) {}

    DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext& user_context,
                                                         std::string_view owner,
                                                         std::span<const std::uint64_t> ids,
                                                         const DynamicFieldSelection& selection,
                                                         const QueryIntent& intent) const override {
        auto r = user_context.memory_resource();
        check(bool(user_context.require_resource<ChildContextMarker>()),
              "provider receives trusted Context resource");
        check(owner == "Product" && ids.size() == expected_ids &&
                  intent.comment() == "load dynamic state counterexamples" &&
                  !intent.purpose().empty(),
              "child provider receives owner IDs and validated query intent");
        ++calls;
        last_comment = intent.comment();
        last_purpose = intent.purpose();
        last_ids.assign(ids.begin(), ids.end());
        const std::array fields{DynamicFieldDefinitions::Field{"note", DataType::Text},
                                DynamicFieldDefinitions::Field{"extra", DataType::Text}};
        auto& definitions = user_context.make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(
                          r, fault == 5 ? "Platform" : owner, "extension-v1", fields))
                .value());
        DynamicStorageRows rows(r);
        for (auto id : ids) {
            auto& values = rows.try_emplace(id).first->second;
            for (std::string_view code : {"note", "extra"}) {
                if (selection.contains(code) && id != 3) {
                    values.emplace(
                        String(code, r),
                        id == 2 ? Value(r)
                                : Value(r, code == "note" ? "private-school" : "root-extra"));
                }
            }
        }
        if (fault == 1) {
            rows.erase(1);
        } else if (fault == 2) {
            rows.try_emplace(999);
        } else if (fault == 3) {
            rows.at(1).insert_or_assign(String("extra", r), Value(r, "PRIVATE-CANARY"));
        } else if (fault == 4) {
            rows.at(1).insert_or_assign(String("note", r), Value(r, false));
        }
        using Output = ChildBatchReady::Output;
        return make_operation<Output, ChildBatchReady>(
            r,
            Output::ok(r, DynamicFieldBatch(std::allocator_arg, Allocator(r), definitions, rows)));
    }
};

static void generated_child_dynamic_selection(bool root_selection, unsigned fault) {
    using Product = cpp_examples::Product;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection, false);
    auto& metadata =
        dynamic_cast<InMemoryMetadataStore&>(const_cast<SchemaProvider&>(executor.schema_provider));
    metadata.register_entity(Product::entity_descriptor(r));
    auto& registry = user_context->make<InMemoryEntityRegistry>();
    registry.register_entity("Product");
    user_context->set_entity_registry(*user_context, registry);
    auto& marker = user_context->make<ChildContextMarker>();
    user_context->insert_resource(*user_context, marker);
    auto& provider = user_context->make<ChildProvider>(fault);
    user_context->set_dynamic_fields_provider(*user_context, provider);
    check(sqlite3_exec(
              connection->native_handle(),
              "CREATE TABLE product_data(id INTEGER PRIMARY KEY,version INTEGER,name TEXT);"
              "INSERT INTO product_data VALUES(1,1,'school-1'),(2,1,'school-2'),(3,1,'school-3')",
              nullptr,
              nullptr,
              nullptr) == SQLITE_OK,
          "seed generated child-query rows");
    unsigned queries = 0;
    sqlite3_trace_v2(
        connection->native_handle(),
        SQLITE_TRACE_STMT,
        [](unsigned, void* count, void*, void*) -> int {
            ++*static_cast<unsigned*>(count);
            return 0;
        },
        &queries);
    auto select = [&](std::string_view field) {
        const std::array fields{DynamicFieldSelection::Field{field, DataType::Text}};
        return std::move(DynamicFieldSelection::fields(r, fields)).value();
    };
    SelectQuery child(std::allocator_arg, Allocator(r), "Product");
    child.project("id").project("version").project("name").limit(4);
    child.comment("load dynamic state counterexamples");
    child.select_dynamic_fields(select("note"));
    SelectQuery root(std::allocator_arg, Allocator(r), "Product");
    root.project("id").project("version").project("name").limit(4);
    root.comment("load dynamic state counterexamples");
    root.child_enhancements.emplace_back(child);
    if (root_selection) {
        root.select_dynamic_fields(select("extra"));
    }
    auto service = user_context->entity_data_service<Executor>("Product");
    check(bool(service), "generated entity resolves independent data service");
    auto result =
        completed(service.value().get().fetch_enhanced_entities<Product>(PurposedSelectQuery(
            std::allocator_arg, Allocator(r), root, "verify dynamic enhancement read metadata")));
    sqlite3_trace_v2(connection->native_handle(), 0, nullptr, nullptr);
    if (fault) {
        check(!result && provider.calls == 1, "malformed child batch returns no partial entities");
        check(result.error().to_string(r).find("PRIVATE-CANARY") == String::npos,
              "malformed child error does not reveal private values");
        return;
    }
    check(bool(result) && result.value().data.size() == 3 && queries == 2 &&
              provider.calls == (root_selection ? 2u : 1u),
          "one child and one root query preserve provider call boundaries");
    auto& rows = result.value().data;
    const std::array expected{
        DynamicFieldState::Value, DynamicFieldState::Null, DynamicFieldState::NotLoaded};
    for (std::size_t index = 0; index < rows.size(); ++index) {
        auto* fields = rows[index].dynamic_field_values();
        check(fields && fields->field("note").value().state() == expected[index] &&
                  fields->field("extra").value().state() ==
                      (root_selection ? expected[index] : DynamicFieldState::NotLoaded) &&
                  !rows[index].hasPendingDynamicMutations(),
              "child and root retain value/NULL/missing state without mutation intent");
    }
    check(rows[0].loaded_state_snapshot()->identity() ==
              rows[1].loaded_state_snapshot()->identity(),
          "equal availability shares indexed geometry across distinct payloads");
    Product copy(std::allocator_arg, Allocator(r), rows[0]);
    auto json = std::move(copy).into_json(r);
    const auto& object = std::get<Json::Object>(json.data);
    check(object.at(String("#note", r)).dump(r) == "\"private-school\"",
          "generated JSON retains child dynamic value");
    check(root_selection ? object.at(String("#extra", r)).dump(r) == "\"root-extra\""
                         : !object.contains("#extra"),
          "generated JSON emits selected root value but omits unselected dynamic fields");
}

static void generated_readonly_stream_case(unsigned mode) {
    using Product = cpp_examples::Product;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection, false);
    auto& metadata =
        dynamic_cast<InMemoryMetadataStore&>(const_cast<SchemaProvider&>(executor.schema_provider));
    metadata.register_entity(Product::entity_descriptor(r));
    auto& registry = user_context->make<InMemoryEntityRegistry>();
    registry.register_entity("Product");
    user_context->set_entity_registry(*user_context, registry);
    check(
        sqlite3_exec(
            connection->native_handle(),
            "CREATE TABLE product_data(id INTEGER PRIMARY KEY,version INTEGER,name TEXT,_count INTEGER);"
            "WITH RECURSIVE seq(x) AS (VALUES(1) UNION ALL SELECT x+1 FROM seq WHERE x<1003) "
            "INSERT INTO product_data SELECT x,1,'school-'||x,CASE WHEN x%2=0 THEN NULL ELSE 0 END FROM seq",
            nullptr,
            nullptr,
            nullptr) == SQLITE_OK,
        "seed readonly stream rows");
    unsigned queries = 0;
    sqlite3_trace_v2(
        connection->native_handle(),
        SQLITE_TRACE_STMT,
        [](unsigned, void* count, void*, void*) -> int {
            ++*static_cast<unsigned*>(count);
            return 0;
        },
        &queries);
    Vector<DynamicPropertyDefinitions::Field> fields(r);
    fields.emplace_back("_missing", DataType::I64);
    if (mode == 2) {
        fields.emplace_back("_count", DataType::I64);
    }
    auto definitions = DynamicPropertyDefinitions::from_fields(r, fields);
    check(bool(definitions), "readonly definitions validate");
    SelectQuery query(std::allocator_arg, Allocator(r), "Product");
    query.project("id").project("version").project("name").order_asc("id").limit(mode == 2 ? 1003
                                                                                           : 3);
    query.comment("load a readonly schema through query and stream");
    query.with_dynamic_property_definitions(definitions.value());
    query.stream_config = StreamConfig{mode == 2 ? 73u : 2u};
    if (mode == 2) {
        query.project_raw("_count", "_count");
    }
    if (mode == 1) {
        auto& cache = user_context->make<InMemoryAggregationCache>();
        user_context->insert_resource(*user_context, cache);
        query.enable_aggregation_cache();
    }
    PurposedSelectQuery request(std::allocator_arg,
                                Allocator(r),
                                query,
                                "verify shared geometry without sharing row values");
    auto service = user_context->entity_data_service<Executor>("Product");
    check(bool(service), "readonly generated service resolves");
    std::optional<LoadedSnapshot::Borrow> shared;
    auto inspect = [&](const Product& entity) {
        auto snapshot = entity.loaded_state_snapshot();
        check(bool(snapshot), "generated readonly row has indexed snapshot");
        if (!shared) {
            shared = snapshot;
        }
        check(shared->identity() == snapshot->identity(),
              "all compatible rows retain one availability snapshot");
        check(
            entity.dynamic_property_type("_missing") == DataType::I64 &&
                !entity.hasDynamicProperty("_missing") && !entity.dynamicProperty("_missing") &&
                !snapshot->get().is_loaded("_missing") && !entity.dynamic_field_values() &&
                !entity.dirty_fields(),
            "declared absent readonly value has type but no presence, loaded bit or mutation state");
        auto* actual = snapshot->get().dynamic_property_definitions();
        check(actual && *actual == definitions.value(), "query schema survives typed decoding");
    };
    if (mode != 2) {
        for (unsigned round = 0; round < (mode == 1 ? 2u : 1u); ++round) {
            auto result =
                completed(service.value().get().fetch_enhanced_entities<Product>(request));
            check(bool(result) && result.value().data.size() == 3, "readonly typed list succeeds");
            for (const auto& entity : result.value().data) {
                inspect(entity);
            }
        }
        check(queries == 1, "second cached typed read performs no extra SQL");
    }
    if (mode != 1) {
        Vector<Product> retained(r);
        std::size_t seen = 0, chunks = 0;
        {
            const auto& opened = service.value().get().fetch_stream(request).wait();
            check(bool(opened), "readonly cursor opens without a dynamic provider");
            auto& stream = opened.value().get();
            while (const auto* item = stream.wait_next()) {
                if (!*item) {
                    throw std::runtime_error(std::string(item->error().to_string(r)));
                }
                const auto& chunk = item->value();
                check(chunk.chunk_index == chunks++,
                      "readonly cursor chunk indexes are contiguous");
                check(chunk.rows.size() ==
                          (mode == 2 ? (chunk.is_last ? 54u : 73u) : (chunk.is_last ? 1u : 2u)),
                      "full and final short chunks retain their exact sizes");
                for (const auto& row : chunk.rows) {
                    auto result = Product::from_compact_row(
                        r, CompactRow(std::allocator_arg, Allocator(r), row));
                    check(bool(result), "generated decoder consumes streamed readonly metadata");
                    auto& entity = result.value();
                    ++seen;
                    inspect(entity);
                    check(entity.id_value().try_u64() == seen,
                          "all ordered row IDs survive streaming");
                    if (mode == 2) {
                        String expected("school-", r);
                        expected += std::to_string(seen);
                        check(
                            entity.name() == expected && entity.hasDynamicProperty("_count") &&
                                !entity.loaded_state_snapshot()->get().is_loaded("_count"),
                            "stream preserves native names and readonly count presence without a fixed bit");
                        const auto* count = entity.dynamicProperty("_count");
                        check(seen % 2 == 0 ? !count : count && count->try_i64() == 0,
                              "readonly count keeps NULL distinct from zero across chunks");
                    }
                    if (seen <= 2 || seen == 1003) {
                        retained.emplace_back(std::move(result).value());
                    }
                }
            }
            stream.cancel();
        }
        check(seen == (mode == 2 ? 1003u : 3u) && chunks == (mode == 2 ? 14u : 2u),
              "cursor consumes the complete source range and short tail");
        check(queries == (mode == 2 ? 1u : 2u), "stream issues one physical query");
        for (const auto& entity : retained) {
            inspect(entity);
            if (mode == 2) {
                check(entity.dynamic_property_type("_count") == DataType::I64 &&
                          entity.hasDynamicProperty("_count"),
                      "retained generated rows keep readonly schema after cursor close");
            }
        }
    }
    sqlite3_trace_v2(connection->native_handle(), 0, nullptr, nullptr);
}

static void generated_dynamic_ledger_case(bool composition) {
    using Product = cpp_examples::Product;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection, false);
    auto& metadata =
        dynamic_cast<InMemoryMetadataStore&>(const_cast<SchemaProvider&>(executor.schema_provider));
    metadata.register_entity(Product::entity_descriptor(r));
    auto& registry = user_context->make<InMemoryEntityRegistry>();
    registry.register_entity("Product");
    user_context->set_entity_registry(*user_context, registry);
    auto& marker = user_context->make<ChildContextMarker>();
    user_context->insert_resource(*user_context, marker);
    auto& provider = user_context->make<ChildProvider>(0);
    user_context->set_dynamic_fields_provider(*user_context, provider);
    check(sqlite3_exec(
              connection->native_handle(),
              "CREATE TABLE product_data(id INTEGER PRIMARY KEY,version INTEGER,name TEXT);"
              "INSERT INTO product_data VALUES(1,1,'school-1'),(2,1,'school-2'),(3,1,'school-3')",
              nullptr,
              nullptr,
              nullptr) == SQLITE_OK,
          "seed generated ledger rows");
    unsigned queries = 0;
    sqlite3_trace_v2(
        connection->native_handle(),
        SQLITE_TRACE_STMT,
        [](unsigned, void* count, void*, void*) -> int {
            ++*static_cast<unsigned*>(count);
            return 0;
        },
        &queries);
    SelectQuery query(std::allocator_arg, Allocator(r), "Product");
    query.project("id").project("version").project("name").order_asc("id").limit(4);
    query.comment("load dynamic state counterexamples");
    const std::array selected{DynamicFieldSelection::Field{"note", DataType::Text}};
    query.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, selected)).value());
    auto service = user_context->entity_data_service<Executor>("Product");
    check(bool(service), "generated ledger service resolves");
    auto result =
        completed(service.value().get().fetch_enhanced_entities<Product>(PurposedSelectQuery(
            std::allocator_arg, Allocator(r), query, "verify dynamic mutation ownership")));
    check(bool(result) && result.value().data.size() == 3, "load three generated ledger carriers");
    auto& rows = result.value().data;
    if (composition) {
        auto parent = rows[0].entity_runtime_state().value();
        auto null_state = rows[1].loaded_state_snapshot().value();
        auto missing_state = rows[2].loaded_state_snapshot().value();
        rows[1].attachRuntimeStateRecursive(parent);
        rows[2].attachRuntimeStateRecursive(parent);
        check(rows[1].dynamicFieldValues()->field("note").value().state() ==
                      DynamicFieldState::Null &&
                  rows[2].dynamicFieldValues()->field("note").value().state() ==
                      DynamicFieldState::NotLoaded &&
                  rows[1].loaded_state_snapshot()->identity() == null_state.identity() &&
                  rows[2].loaded_state_snapshot()->identity() == missing_state.identity(),
              "generated state attachment preserves each child's private payload and availability");
        check(rows[1].root.original_snapshot()->fields.at(String("id", r)).try_u64() == 2 &&
                  rows[2].root.original_snapshot()->fields.at(String("id", r)).try_u64() == 3 &&
                  rows[0].dynamicFieldValues()->field("note").value().value()->try_text() ==
                      "private-school",
              "generated state attachment preserves child originals and parent payload");
    } else {
        auto sibling = rows[1].loaded_state_snapshot().value();
        auto own = rows[0].loaded_state_snapshot().value();
        const std::array<FieldLayout::Index, 3> indexes{{{"id", 0}, {"name", 1}, {"version", 2}}};
        const std::array<FieldLayout::Mapping, 3> mappings{
            {{"id", "id", "id"}, {"name", "name", "name"}, {"version", "version", "version"}}};
        const std::array<std::string_view, 3> members{{"id", "name", "version"}};
        const std::array<std::string_view, 1> relations{{"order_line_list"}};
        auto& foreign = user_context->make<FieldLayout>(
            std::move(
                FieldLayout::from_generated(
                    r, "Product", "incompatible-revision", indexes, mappings, relations, members))
                .value());
        auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
        rows[0].load_state =
            std::move(eval::LoadState::fully_loaded(r).into_indexed(pool, foreign)).value();
        auto wrong_set = rows[0].updateDynamicField("note", Value(r));
        auto wrong_delete = rows[0].deleteDynamicField("note");
        check(!wrong_set && wrong_set.error().code == "DYNAMIC_FIELD_LAYOUT_MISMATCH" &&
                  !wrong_delete && wrong_delete.error().code == "DYNAMIC_FIELD_LAYOUT_MISMATCH" &&
                  !rows[0].hasPendingDynamicMutations() &&
                  rows[0].dynamicFieldValues()->field("note").value().value()->try_text() ==
                      "private-school",
              "incompatible revision rejects both mutations before changing payload or intent");
        rows[0].load_state = eval::LoadState::indexed(r, own);
        check(bool(rows[0].updateDynamicField("note", Value(r))), "generated NULL update succeeds");
        auto dirty = rows[0].dirty_fields();
        check(dirty && dirty->size() == 1 && dirty->contains("#note") &&
                  rows[0].loaded_state_snapshot()->identity() == sibling.identity(),
              "NULL update marks only its dynamic key while preserving shared availability");
        auto state = rows[0].entity_runtime_state().value();
        EntityKey key(std::allocator_arg, Allocator(r), "Product", Value(r, std::uint64_t{1}));
        auto pending = [&](const EntityChangeSet& changes) -> const DynamicFieldMutation& {
            check(changes.dynamic_changes() != nullptr, "dynamic ledger is present");
            return changes.dynamic_changes()->at(key).at(String("note", r));
        };
        auto current = state.current_change_set();
        check(pending(current).value() && pending(current).value()->is_null() &&
                  current.changes().empty() && rows[1].root.current_change_set().is_empty(),
              "dynamic NULL intent is separate from scalar changes and sibling ownership");
        auto before = state.current_change_set();
        check(!rows[0].updateDynamicField("note", Value(r, false)) &&
                  state.current_change_set() == before,
              "invalid dynamic type preserves the entire prior change set");
        check(bool(rows[0].deleteDynamicField("note")), "generated dynamic delete succeeds");
        current = state.current_change_set();
        check(pending(current).is_delete() &&
                  rows[0].dynamicFieldValues()->field("note").value().state() ==
                      DynamicFieldState::NotLoaded &&
                  rows[0].loaded_state_snapshot()->identity() ==
                      rows[2].loaded_state_snapshot()->identity(),
              "delete is distinct from NULL and reuses matching unloaded geometry");
        auto query_count = queries;
        auto graph = graph_node_from_entity(*user_context,
                                            Product(std::allocator_arg, Allocator(r), rows[0]));
        check(!graph &&
                  graph.error().to_string(r).find("DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED") !=
                      String::npos &&
                  queries == query_count,
              "graph construction rejects unbound dynamic writes before SQL");
        for (bool ledger : {false, true}) {
            Audited<Product> audited(
                std::allocator_arg, Allocator(r), rows[0], "retain dynamic intent on failed save");
            auto& operation =
                ledger ? save_audited_ledger_entity(std::move(audited), *user_context)
                       : AuditedSaveExt<Product>::save(std::move(audited), *user_context);
            const auto& saved = operation.wait();
            check(!saved && saved.error().to_string(r).find(
                                "DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED") != String::npos,
                  "both audited save entries reject an unbound dynamic write");
        }
        current = state.current_change_set();
        check(queries == query_count && pending(current).is_delete(),
              "failed saves preserve retryable delete without SQL");
        auto& composed = user_context->make<EntityRuntimeState>();
        composed.adopt_mutations_from(state);
        auto copied = composed.current_change_set();
        check(copied.dynamic_changes() && *copied.dynamic_changes() == *current.dynamic_changes() &&
                  composed.has_pending_dynamic_mutations(),
              "explicit composition retains dynamic intent");
        state.push_change_set();
        check(bool(rows[0].updateDynamicField("note", Value(r))), "inner frame records NULL");
        auto popped = state.pop_change_set();
        current = state.current_change_set();
        check(popped && pending(*popped).value() && pending(*popped).value()->is_null() &&
                  pending(current).is_delete(),
              "popping inner NULL leaves outer delete intact");
        state.clear_committed();
        check(!state.has_pending_dynamic_mutations() && state.current_change_set().is_empty() &&
                  composed.has_pending_dynamic_mutations(),
              "cleanup clears only the original ledger owner");
    }
    sqlite3_trace_v2(connection->native_handle(), 0, nullptr, nullptr);
}

static void generated_root_dynamic_boundary(unsigned mode) {
    using Product = cpp_examples::Product;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    SqliteConnection* connection = nullptr;
    auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection, false);
    auto& metadata =
        dynamic_cast<InMemoryMetadataStore&>(const_cast<SchemaProvider&>(executor.schema_provider));
    metadata.register_entity(Product::entity_descriptor(r));
    auto& registry = user_context->make<InMemoryEntityRegistry>();
    registry.register_entity("Product");
    user_context->set_entity_registry(*user_context, registry);
    auto& marker = user_context->make<ChildContextMarker>();
    user_context->insert_resource(*user_context, marker);
    auto& provider = user_context->make<ChildProvider>(mode == 6 ? 0 : mode);
    if (mode != 0) {
        user_context->set_dynamic_fields_provider(*user_context, provider);
    }
    check(sqlite3_exec(
              connection->native_handle(),
              "CREATE TABLE product_data(id INTEGER PRIMARY KEY,version INTEGER,name TEXT);"
              "INSERT INTO product_data VALUES(1,1,'school-1'),(2,1,'school-2'),(3,1,'school-3')",
              nullptr,
              nullptr,
              nullptr) == SQLITE_OK,
          "seed root dynamic boundary rows");
    unsigned queries = 0;
    sqlite3_trace_v2(
        connection->native_handle(),
        SQLITE_TRACE_STMT,
        [](unsigned, void* count, void*, void*) -> int {
            ++*static_cast<unsigned*>(count);
            return 0;
        },
        &queries);
    auto service = user_context->entity_data_service<Executor>("Product");
    check(bool(service), "root dynamic service resolves");
    SelectQuery query(std::allocator_arg, Allocator(r), "Product");
    query.project("id").project("version").project("name").order_asc("id").limit(4);
    query.comment("load dynamic state counterexamples");
    if (mode == 0) {
        auto plain = completed(service.value().get().fetch_enhanced_entities<Product>(
            PurposedSelectQuery(std::allocator_arg,
                                Allocator(r),
                                query,
                                "verify shared selection with private payloads")));
        check(bool(plain) && plain.value().data.size() == 3 && provider.calls == 0,
              "no selection needs no dynamic provider");
        for (const auto& row : plain.value().data) {
            check(!row.dynamic_field_values(), "unselected rows contain no dynamic payload");
        }
    }
    const std::array selected{DynamicFieldSelection::Field{"note", DataType::Text}};
    query.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, selected)).value());
    PurposedSelectQuery request(
        std::allocator_arg, Allocator(r), query, "verify shared selection with private payloads");
    if (mode != 6) {
        auto result = completed(service.value().get().fetch_enhanced_entities<Product>(request));
        const std::array codes{"DYNAMIC_FIELD_PROVIDER_MISSING",
                               "DYNAMIC_FIELD_BATCH_OMITTED_OWNER",
                               "DYNAMIC_FIELD_UNREQUESTED_OWNER",
                               "DYNAMIC_FIELD_UNSELECTED_VALUE",
                               "DYNAMIC_FIELD_TYPE_MISMATCH",
                               "DYNAMIC_FIELD_OWNER_MISMATCH"};
        check(!result, "malformed or absent provider cannot yield partial root entities");
        auto error = result.error().to_string(r);
        check(error.find(codes[mode]) != String::npos &&
                  error.find("PRIVATE-CANARY") == String::npos,
              "root failure preserves exact source reason without disclosing payload");
        check(provider.calls == (mode == 0 ? 0u : 1u),
              "root dynamic values use one provider batch");
    }
    if (mode == 0 || mode == 6) {
        auto before = queries;
        auto streamed = completed(service.value().get().fetch_stream(request));
        check(!streamed && queries == before && provider.calls == 0,
              "dynamic selection rejects before SQL cursor or provider access");
        if (mode == 6) {
            check(queries == 0, "configured provider does not permit opening dynamic cursor");
        }
    }
    sqlite3_trace_v2(connection->native_handle(), 0, nullptr, nullptr);
}

static void generated_typed_dynamic_batch(bool duplicate) {
    using Product = cpp_examples::Product;
    GlobalContext global;
    auto user_context = global.make_user_context();
    Ref<Product> retained;
    Ref<Product> expired;
    {
        auto r = user_context->memory_resource();
        SqliteConnection* connection = nullptr;
        auto& executor = dynamic_hydration_fixture::setup(*user_context, &connection, false);
        auto& metadata = dynamic_cast<InMemoryMetadataStore&>(
            const_cast<SchemaProvider&>(executor.schema_provider));
        metadata.register_entity(Product::entity_descriptor(r));
        auto& registry = user_context->make<InMemoryEntityRegistry>();
        registry.register_entity("Product");
        user_context->set_entity_registry(*user_context, registry);
        auto& marker = user_context->make<ChildContextMarker>();
        user_context->insert_resource(*user_context, marker);
        auto& provider = user_context->make<ChildProvider>(0);
        provider.expected_ids = duplicate ? 4u : 3u;
        user_context->set_dynamic_fields_provider(*user_context, provider);
        check(
            sqlite3_exec(
                connection->native_handle(),
                "CREATE TABLE product_data(id INTEGER,version INTEGER,name TEXT);"
                "INSERT INTO product_data VALUES(1,1,'school-1'),(2,1,'school-2'),(3,1,'school-3')",
                nullptr,
                nullptr,
                nullptr) == SQLITE_OK,
            "seed typed dynamic rows");
        if (duplicate) {
            check(sqlite3_exec(connection->native_handle(),
                               "INSERT INTO product_data VALUES(1,1,'school-1')",
                               nullptr,
                               nullptr,
                               nullptr) == SQLITE_OK,
                  "append duplicate owner row");
        }

        struct Trace {
            unsigned calls = 0;
            String sql;

            explicit Trace(Memory r) : sql(r) {}
        } trace(r);

        sqlite3_trace_v2(
            connection->native_handle(),
            SQLITE_TRACE_STMT,
            [](unsigned, void* state, void* statement, void*) -> int {
                auto& trace = *static_cast<Trace*>(state);
                ++trace.calls;
                trace.sql = sqlite3_sql(static_cast<sqlite3_stmt*>(statement));
                return 0;
            },
            &trace);
        SelectQuery query(std::allocator_arg, Allocator(r), "Product");
        query.project("id").project("version").project("name").limit(4);
        query.comment("load dynamic state counterexamples");
        const std::array selected{DynamicFieldSelection::Field{"note", DataType::Text}};
        query.select_dynamic_fields(std::move(DynamicFieldSelection::fields(r, selected)).value());
        PurposedSelectQuery request(std::allocator_arg,
                                    Allocator(r),
                                    query,
                                    "verify shared selection with private payloads");
        auto service = user_context->entity_data_service<Executor>("Product");
        check(bool(service), "typed dynamic service resolves");
        auto result =
            duplicate ? completed(service.value().get().fetch_entities<Product>(request))
                      : completed(service.value().get().fetch_enhanced_entities<Product>(request));
        check(bool(result) && result.value().data.size() == (duplicate ? 4u : 3u),
              "typed query retains all rows including duplicate owners");
        auto& rows = result.value().data;
        check(
            provider.calls == 1 && provider.last_ids.size() == (duplicate ? 4u : 3u) &&
                provider.last_comment == "load dynamic state counterexamples" &&
                provider.last_purpose == "verify shared selection with private payloads" &&
                trace.calls == 1 && trace.sql.find("note") == String::npos &&
                trace.sql.find("extra") == String::npos,
            "one physical query and one dynamic batch preserving duplicate owner IDs retain exact intent");
        for (std::size_t i = 0; i < 3; ++i) {
            check(rows[i].id() == i + 1 && !rows[i].dirty_fields(),
                  "fixture preserves ordered owner IDs and clean loaded state");
        }
        check(rows[0].loaded_state_snapshot()->identity() ==
                      rows[1].loaded_state_snapshot()->identity() &&
                  rows[1].loaded_state_snapshot()->identity() !=
                      rows[2].loaded_state_snapshot()->identity(),
              "availability shares geometry independently of private values");
        check(rows[0].dynamicFieldValues()->field("note").value().value()->try_text() ==
                      "private-school" &&
                  rows[1].dynamicFieldValues()->field("note").value().state() ==
                      DynamicFieldState::Null &&
                  rows[2].dynamicFieldValues()->field("note").value().state() ==
                      DynamicFieldState::NotLoaded &&
                  rows[0].dynamicFieldValues()->field("extra").value().state() ==
                      DynamicFieldState::NotLoaded,
              "typed dynamic carriers distinguish value NULL missing and unselected fields");
        if (duplicate) {
            check(rows[3].id() == 1 &&
                      rows[0].loaded_state_snapshot()->identity() ==
                          rows[3].loaded_state_snapshot()->identity() &&
                      rows[0].dynamicFieldValues() != rows[3].dynamicFieldValues(),
                  "duplicate rows share snapshot geometry but own separate payload carriers");
            expired = user_context->make_ref<Product>(rows[0]);
            retained = user_context->make_global<Product>(rows[0]);
            check(user_context->get(retained).dynamicFieldValues()->memory_resource() ==
                      global.memory_resource(),
                  "retained dynamic payload is explicitly copied into Global storage");
        } else {
            auto state = rows[0].entity_runtime_state().value();
            auto sibling = rows[1].entity_runtime_state().value();
            check(!(state == sibling) && state.current_change_set().changes().empty(),
                  "loaded entities have independent empty mutation ledgers");
            state.set(
                EntityKey(std::allocator_arg, Allocator(r), "Product", Value(r, std::uint64_t{1})),
                "name",
                Value(r, "local-change"));
            check(sibling.current_change_set().changes().empty(),
                  "mutation does not affect sibling ledger");
            auto original = rows[1].loaded_state_snapshot().value();
            Value::Object payload(r);
            payload.emplace(String("note", r), Value(r, "private-school"));
            payload.emplace(String("extra", r), Value(r, "private-extra"));
            auto fields = DynamicFieldValues::from_values(
                r, rows[0].dynamicFieldValues()->definitions(), payload);
            check(bool(fields), "construct complete dynamic extension payload");
            auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
            auto augmented = LoadedSnapshot::with_dynamic_fields(pool, original, fields.value());
            check(bool(augmented) && bool(rows[0].install_loaded_dynamic_fields(fields.value(),
                                                                                augmented.value())),
                  "install augmented selection through generated carrier");
            check(rows[1].loaded_state_snapshot()->identity() == original.identity() &&
                      !rows[1].is_field_loaded("#extra") && rows[0].is_field_loaded("name") &&
                      rows[1].is_field_loaded("#note") && !rows[2].is_field_loaded("#note"),
                  "augmentation preserves sibling snapshot and native/dynamic availability");
            auto values = Product(std::allocator_arg, Allocator(r), rows[0]).into_values();
            check(!values.fields.contains("#note"),
                  "loaded dynamic extension is not a native write column");
        }
        sqlite3_trace_v2(connection->native_handle(), 0, nullptr, nullptr);
    }
    if (duplicate) {
        user_context.reset();
        auto next_user_context = global.make_user_context();
        const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
        check(copy.dynamicFieldValues()->field("note").value().value()->try_text() ==
                  "private-school",
              "explicit Global copy remains readable after result and source Context destruction");
        bool rejected = false;
        try {
            (void)next_user_context->get(expired);
        } catch (const ReferenceError&) {
            rejected = true;
        }
        check(rejected, "ordinary Context reference cannot extend source lifetime");
    }
}

static void plain_carrier_readonly_defaults() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    Record fields(r);
    fields.emplace(String("id", r), Value(r, std::uint64_t{1}));
    fields.emplace(String("name", r), Value(r, "native"));
    auto row =
        example::Product::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), fields));
    check(bool(row) && row.value().id == 1 && row.value().name == "native",
          "decode original handwritten model without a readonly carrier");
    check(!example::Product::supports_dynamic_property_load(),
          "plain carrier does not advertise readonly support");
    for (std::string_view key : {"_missing", "name", "#name"}) {
        check(!row.value().dynamic_property(key) && !row.value().has_dynamic_property(key),
              "default trait hooks return no value or presence");
    }
}

static void generated_graph_read_metadata() {
    using Product = cpp_examples::Product;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array types{DynamicFieldDefinitions::Field{"note", DataType::Text}};
    auto& definitions = user_context->make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "Product", "graph-dynamic-v1", types))
            .value());
    Vector<DynamicFieldValues::Values> payloads(r);
    payloads.resize(3);
    payloads[0].emplace(String("note", r), Value(r, "private value"));
    payloads[1].emplace(String("note", r), Value(r));
    auto fields = DynamicFieldValues::from_batch(r, definitions, payloads);
    check(bool(fields), "construct value NULL and missing graph payloads");
    auto& layout = user_context->make<ColumnLayout>(
        std::initializer_list<std::string_view>{"id", "version", "name"});
    Vector<CompactRow> rows(r);
    for (std::size_t index = 0; index < 3; ++index) {
        Vector<Value> native(r);
        native.emplace_back(std::uint64_t(index + 1));
        native.emplace_back(std::int64_t{1});
        String name("row-", r);
        name += std::to_string(index);
        native.emplace_back(std::move(name));
        rows.emplace_back(layout, std::move(native));
        rows.back().set_loaded_dynamic_fields(fields.value()[index]);
        check(rows.back().len() == 3 && !rows.back().into_map(r).contains("#note"),
              "trusted dynamic sidecar does not become a native row column");
    }
    auto& root = user_context->make<EntityRuntimeState>();
    auto decoded =
        decode_compact_rows_with_read_metadata<Product>(r, Vector<CompactRow>(rows, r), root);
    check(bool(decoded) && decoded.value().size() == 3,
          "shared typed decoder accepts generated graph carriers");
    const std::array expected{
        DynamicFieldState::Value, DynamicFieldState::Null, DynamicFieldState::NotLoaded};
    auto inspect = [&](const auto& entities) {
        check(entities[0].loaded_state_snapshot()->identity() ==
                      entities[1].loaded_state_snapshot()->identity() &&
                  entities[0].loaded_state_snapshot()->identity() !=
                      entities[2].loaded_state_snapshot()->identity(),
              "graph decoding shares equal availability and separates absent selections");
        for (std::size_t index = 0; index < 3; ++index) {
            check(entities[index].dynamicFieldValues() &&
                      entities[index].dynamicFieldValues()->field("note").value().state() ==
                          expected[index] &&
                      !entities[index].hasPendingDynamicMutations(),
                  "graph decoding preserves private read states without creating mutation intent");
        }
    };
    inspect(decoded.value());
    auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
    registry.register_entity<Product>();
    auto& builder = user_context->make<EntityGraphBuilder>();
    check(
        bool(registry.decode_compact_batch("Product", Vector<CompactRow>(rows, r), root, builder)),
        "registry batch decoder retains dynamic read metadata");
    check(
        bool(registry.decode_compact_list(
            "Product", Vector<CompactRow>(rows, r), root, builder, "JsonGraphRow", 77, "children")),
        "registry list decoder retains dynamic read metadata");
    Vector<CompactRow> single(r);
    single.emplace_back(rows[0]);
    check(bool(registry.decode_compact_option(
              "Product", std::move(single), root, builder, "JsonGraphRow", 77, "single_child")),
          "registry option decoder retains dynamic read metadata");
    check(bool(registry.decode_compact(
              "Product", CompactRow(std::allocator_arg, Allocator(r), rows[0]), root, builder)),
          "registry single decoder retains dynamic read metadata");
    check(bool(root.freeze_graph(std::move(builder))), "freeze the decoded graph through runtime");
    const auto* list = root.resolve_relation_list<Product>("JsonGraphRow", 77, "children");
    check(list && list->len() == 3, "resolve all frozen list members");
    inspect(*list);
    check((*list)[0].dynamicFieldValues()->field("note").value().value()->try_text() ==
              "private value",
          "frozen list retains the actual private value");
    const auto* entity = root.resolve_entity<Product>(1);
    const auto* child = root.resolve_relation_option<Product>("JsonGraphRow", 77, "single_child");
    check(entity && child && child->has_value(), "resolve frozen entity and selected child");
    for (const auto* item : {entity, &child->value()}) {
        check(item->dynamicFieldValues()->field("note").value().value()->try_text() ==
                  "private value",
              "frozen entity and option retain private dynamic payload");
        auto json = Product(std::allocator_arg, Allocator(r), *item).into_json(r);
        check(std::get<Json::Object>(json.data).at(String("#note", r)).dump(r) ==
                  "\"private value\"",
              "generated frozen graph JSON retains selected persistent value");
    }
    auto cleared = CompactRow(std::allocator_arg, Allocator(r), rows[0]);
    cleared.clear_loaded_relations();
    check(!cleared.take_loaded_dynamic_fields(),
          "clearing trusted row read metadata clears the dynamic sidecar");
}

int main() {
    plain_carrier_readonly_defaults();
    generated_graph_read_metadata();
    std::cout << "GRAPH_READ_METADATA_CASES=1;PLAIN_CARRIER_DEFAULT_CASES=1\n";
    generated_typed_dynamic_batch(false);
    generated_typed_dynamic_batch(true);
    std::cout << "GENERATED_TYPED_DYNAMIC_BATCH_CASES=2\n";
    for (unsigned mode = 0; mode <= 6; ++mode) {
        generated_root_dynamic_boundary(mode);
    }
    std::cout << "GENERATED_ROOT_DYNAMIC_BOUNDARY_CASES=7\n";
    generated_dynamic_ledger_case(true);
    generated_dynamic_ledger_case(false);
    std::cout << "GENERATED_DYNAMIC_LEDGER_CASES=2\n";
    for (unsigned mode = 0; mode < 3; ++mode) {
        generated_readonly_stream_case(mode);
    }
    std::cout << "GENERATED_READONLY_STREAM_CASES=3;LONG_ROWS=1003;LONG_CHUNKS=14\n";
    generated_child_dynamic_selection(false, 0);
    generated_child_dynamic_selection(true, 0);
    for (unsigned fault = 1; fault <= 5; ++fault) {
        generated_child_dynamic_selection(false, fault);
    }
    std::cout << "DYNAMIC_CHILD_SOURCE_CASES=7\n";
    child_and_flat_loading();
    checked_merge_and_saved_keys();
    std::cout << "Dynamic relation contracts passed\n";
}
