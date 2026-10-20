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
    mutable unsigned calls = 0;

    ChildProvider(std::allocator_arg_t, Allocator, unsigned fault) : fault(fault) {}

    DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext& user_context,
                                                         std::string_view owner,
                                                         std::span<const std::uint64_t> ids,
                                                         const DynamicFieldSelection& selection,
                                                         const QueryIntent& intent) const override {
        auto r = user_context.memory_resource();
        check(bool(user_context.require_resource<ChildContextMarker>()),
              "provider receives trusted Context resource");
        check(owner == "Product" && ids.size() == 3 &&
                  intent.comment() == "load dynamic state counterexamples" &&
                  !intent.purpose().empty(),
              "child provider receives owner IDs and validated query intent");
        ++calls;
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

int main() {
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
