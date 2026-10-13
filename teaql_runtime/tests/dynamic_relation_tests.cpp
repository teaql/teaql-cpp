#include "../../tests/support/dynamic_hydration_fixture.hpp"
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

int main() {
    child_and_flat_loading();
    checked_merge_and_saved_keys();
    std::cout << "Dynamic relation contracts passed\n";
}
