#include "parity/graph_serialization_contracts/cpp/include/cpp_graph_serialization.hpp"
#include <iostream>

using namespace teaql;
using Node = cpp_graph_serialization::JsonGraphRow;
using Parent = cpp_graph_serialization::SerializationParent;
using Member = cpp_graph_serialization::SerializationMember;

static bool count_copies = false;
static std::size_t copies = 0;

// Test-only link wrappers; every generated entity allocator-copy copies its root.
#define ROOT_COPY_ARGS                                                                             \
    EntityRuntimeState *target, std::allocator_arg_t tag, Allocator a,                             \
        const EntityRuntimeState &source
#define ROOT_COPY_C1                                                                               \
    _ZN5teaql18EntityRuntimeStateC1ESt15allocator_arg_tNSt3pmr21polymorphic_allocatorISt4byteEERKS0_
#define ROOT_COPY_C2                                                                               \
    _ZN5teaql18EntityRuntimeStateC2ESt15allocator_arg_tNSt3pmr21polymorphic_allocatorISt4byteEERKS0_
#define JOIN_IMPL(a, b) a##b
#define JOIN(a, b) JOIN_IMPL(a, b)
#define ROOT_COPY_WRAPPER(symbol)                                                                  \
    extern "C" void JOIN(__real_, symbol)(ROOT_COPY_ARGS);                                         \
    extern "C" void JOIN(__wrap_, symbol)(ROOT_COPY_ARGS) {                                        \
        if (count_copies) {                                                                        \
            ++copies;                                                                              \
        }                                                                                          \
        JOIN(__real_, symbol)(target, tag, a, source);                                             \
    }
ROOT_COPY_WRAPPER(ROOT_COPY_C1)
ROOT_COPY_WRAPPER(ROOT_COPY_C2)
#undef ROOT_COPY_WRAPPER
#undef JOIN
#undef JOIN_IMPL
#undef ROOT_COPY_C2
#undef ROOT_COPY_C1
#undef ROOT_COPY_ARGS

static void check(bool result, const char* message) {
    if (!result) {
        throw std::runtime_error(message);
    }
}

static const Json& at(const Json& json, std::string_view key) {
    const auto& object = std::get<Json::Object>(json.data);
    auto found = object.find(key);
    if (found == object.end()) {
        throw std::runtime_error("expected JSON member missing: " + std::string(key) + " in " +
                                 std::string(json.dump(json.memory_resource())));
    }
    return found->second;
}

static bool has(const Json& json, std::string_view key) {
    return std::get<Json::Object>(json.data).contains(key);
}

static CompactRow row(Memory r, std::uint64_t id, bool name = true) {
    Record fields(r);
    fields.emplace(String("id", r), Value(r, id));
    fields.emplace(String("version", r), Value(r, std::int64_t{1}));
    if (name) {
        String text("node-", r);
        text += std::to_string(id);
        fields.emplace(String("name", r), Value(r, text));
    }
    return CompactRow(std::allocator_arg, Allocator(r), fields);
}

static Node node(Memory r, CompactRow input, const EntityRuntimeState& root) {
    auto result = Node::from_compact_row_with_context(r, std::move(input), AnyRef(root));
    check(bool(result), "decode generated graph node");
    return std::move(result).value();
}

static void selected(Node& entity, std::string_view key) {
    auto& pool = detail::ContextArena::from_memory(entity.memory_resource()).loaded_snapshots();
    check(bool(entity.load_state.mark_loaded(key, &pool)), "mark this view's requested edge");
}

template <class T>
static Json render(T& entity) {
    auto r = entity.memory_resource();
    EntityJsonTraversal traversal(std::allocator_arg, Allocator(r));
    copies = 0;
    count_copies = true;
    auto result = entity.borrowed_json(r, traversal);
    count_copies = false;
    check(bool(result) && copies == 0, "borrowed presentation must not clone entity runtime state");
    return std::move(*result);
}

static void forward_cycle() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& root = user_context->make<EntityRuntimeState>();
    auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
    registry.register_entity<Node>();
    auto& builder = user_context->make<EntityGraphBuilder>();
    RelationShapeCache shapes(std::allocator_arg, Allocator(r));
    for (std::uint64_t id : {1, 2}) {
        auto input = row(r, id);
        input.insert("peer_id", Value(r, std::uint64_t{3 - id}));
        input.mark_relation_loaded("peer", shapes);
        check(bool(registry.decode_compact("JsonGraphRow", std::move(input), root, builder)),
              "install cyclic graph node");
    }
    check(bool(root.freeze_graph(std::move(builder))), "freeze cycle");
    auto input = row(r, 1);
    input.insert("peer_id", Value(r, std::uint64_t{2}));
    auto entity = node(r, std::move(input), root);
    selected(entity, "peer");
    entity.updateName(String("live-name", r));
    auto snapshot = entity.loaded_state_snapshot().value();
    copies = 0;
    count_copies = true;
    Node positive(std::allocator_arg, Allocator(r), entity);
    count_copies = false;
    check(copies > 0, "copy counter observes a real generated entity copy");
    auto first = render(entity);
    check(at(first, "name").dump(r) == "\"live-name\"" &&
              at(at(first, "peer"), "name").dump(r) == "\"node-2\"" &&
              at(at(at(first, "peer"), "peer"), "name").dump(r) == "\"node-1\"",
          "borrowed traversal uses live typed values and follows frozen peers");
    const auto& terminal = at(at(at(first, "peer"), "peer"), "peer");
    check(at(terminal, "id").dump(r) == "2" && at(terminal, "version").dump(r) == "1" &&
              !has(terminal, "name") && !has(terminal, "peer"),
          "cycle terminates at identity/version projection");
    check(snapshot.identity() == entity.loaded_state_snapshot()->identity(),
          "serialization preserves loaded snapshot");
    copies = 0;
    count_copies = true;
    auto owned = std::move(entity).into_json(r);
    count_copies = false;
    check(copies == 0 && owned == first, "consuming JSON delegates without entity clones");
    std::cout << "GENERATED_GRAPH_COPY_CONTROL=passed;BORROWED_COPIES=0\n";
}

static void reverse_projection() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& root = user_context->make<EntityRuntimeState>();
    auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
    registry.register_entity<Node>();
    auto& builder = user_context->make<EntityGraphBuilder>();
    Vector<CompactRow> children(r);
    children.push_back(row(r, 11));
    children.push_back(row(r, 12));
    check(bool(registry.decode_compact_list(
              "JsonGraphRow", std::move(children), root, builder, "JsonGraphRow", 1, "child_rows")),
          "install reverse children");
    check(
        bool(registry.decode_compact_list(
            "JsonGraphRow", Vector<CompactRow>(r), root, builder, "JsonGraphRow", 2, "child_rows")),
        "install empty reverse children");
    check(bool(registry.decode_compact_option("JsonGraphRow",
                                              Vector<CompactRow>(r),
                                              root,
                                              builder,
                                              "JsonGraphRow",
                                              2,
                                              "single_child")),
          "install reverse NULL child");
    check(bool(root.freeze_graph(std::move(builder))), "freeze reverse graph");
    auto json = [&](std::uint64_t id, bool requested) {
        auto input = row(r, id);
        if (requested) {
            input.insert("child_rows", Value(r));
            if (id == 2) {
                input.insert("single_child", Value(r));
            }
        }
        auto entity = node(r, std::move(input), root);
        return render(entity);
    };
    auto parent = json(1, true);
    const auto& members = std::get<Json::Array>(at(parent, "child_rows").data);
    check(members.size() == 2 && at(members[0], "name").dump(r) == "\"node-11\"" &&
              at(members[1], "name").dump(r) == "\"node-12\"",
          "reverse list retains complete typed members");
    auto empty = json(2, true);
    check(at(empty, "child_rows").dump(r) == "[]" && at(empty, "single_child").dump(r) == "null",
          "selected reverse edges distinguish empty list and NULL object");
    for (std::uint64_t id : {1, 2, 3}) {
        auto unselected = json(id, false);
        check(!has(unselected, "child_rows") && !has(unselected, "single_child"),
              "graph edges are not implicitly selected by other views");
    }
    check(!has(members[0], "_original_values"), "reverse child JSON omits private original values");
}

static void sibling_isolation() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& root = user_context->make<EntityRuntimeState>();
    auto& registry = user_context->make<InMemoryEntityGraphDecoderRegistry>();
    registry.register_entity<Node>();
    auto& builder = user_context->make<EntityGraphBuilder>();
    auto detail = row(r, 7);
    detail.insert("name", Value(r, "visible-sibling"));
    check(bool(registry.decode_compact("JsonGraphRow", std::move(detail), root, builder)),
          "install full sibling view");
    Record id_only(r);
    id_only.emplace(String("id", r), Value(r, std::uint64_t{7}));
    Vector<CompactRow> restricted(r);
    restricted.emplace_back(id_only);
    check(bool(registry.decode_compact_option(
              "JsonGraphRow", std::move(restricted), root, builder, "JsonGraphRow", 2, "peer")),
          "install restricted peer view");
    check(bool(root.freeze_graph(std::move(builder))), "freeze sibling views");
    auto make = [&](std::uint64_t id, bool null) {
        auto input = row(r, id, false);
        input.insert("peer_id", null ? Value(r) : Value(r, std::uint64_t{7}));
        return node(r, std::move(input), root);
    };
    for (std::uint64_t id : {1, 2}) {
        auto entity = make(id, false);
        check(!has(render(entity), "peer"), "unselected peer never exposes sibling detail");
    }
    auto full = make(1, false);
    selected(full, "peer");
    check(at(at(render(full), "peer"), "name").dump(r) == "\"visible-sibling\"",
          "selected full peer resolves canonical detail");
    auto filtered = make(2, false);
    selected(filtered, "peer");
    auto filtered_json = render(filtered);
    check(at(at(filtered_json, "peer"), "id").dump(r) == "7" &&
              !has(at(filtered_json, "peer"), "name"),
          "selected restricted edge does not borrow canonical sibling detail");
    auto null = make(3, true);
    selected(null, "peer");
    check(at(render(null), "peer").dump(r) == "null", "selected null key serializes NULL");
}

template <class T>
static T nested_child(UserContext& user_context) {
    auto r = user_context.memory_resource();
    Record input(r);
    input.emplace(String("id", r), Value(r, std::uint64_t{1}));
    input.emplace(String("version", r), Value(r, std::int64_t{1}));
    input.emplace(String("name", r), Value(r, "native"));
    input.emplace(String("_name", r), Value(r, "derived"));
    Value::Object business_payload(r);
    business_payload.emplace(String("_comment", r), Value(r, "literal business data"));
    input.emplace(String("_payload", r), Value(r, std::move(business_payload)));
    auto result = T::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), input));
    check(bool(result), "decode nested generated child");
    auto child = std::move(result).value();
    const std::array types{DynamicFieldDefinitions::Field{"name", DataType::Text},
                           DynamicFieldDefinitions::Field{"nil", DataType::Text},
                           DynamicFieldDefinitions::Field{"missing", DataType::Text}};
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, T::ENTITY_NAME, "nested-json-v1", types))
            .value());
    Value::Object values(r);
    values.emplace(String("name", r), Value(r, "persistent"));
    values.emplace(String("nil", r), Value(r));
    auto fields = DynamicFieldValues::from_values(r, definitions, values);
    check(bool(fields), "construct nested dynamic read fields");
    auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
    auto state = LoadedSnapshot::with_dynamic_fields(
        pool, child.loaded_state_snapshot().value(), fields.value());
    check(bool(state) && bool(child.install_loaded_dynamic_fields(fields.value(), state.value())),
          "install nested read metadata");
    child.set_comment("private child mutation reason");
    child.mark_as_new();
    return child;
}

static Parent parent(Memory r, bool selected_edges) {
    auto input = row(r, 2, false);
    RelationShapeCache shapes(std::allocator_arg, Allocator(r));
    if (selected_edges) {
        input.insert("child", Value(r));
        input.insert("boxed_child", Value(r));
        input.insert("child_rows", Value(r, Value::List(r)));
        input.insert("boxed_children", Value(r, Value::List(r)));
        for (std::string_view edge : {"child", "boxed_child", "child_rows", "boxed_children"}) {
            input.mark_relation_loaded(edge, shapes);
        }
    }
    auto result = Parent::from_compact_row(r, std::move(input));
    check(bool(result), "decode generated parent relation projection");
    if (selected_edges) {
        result.value().childRows() =
            SmartList<Member>(std::allocator_arg, Allocator(r), Vector<Member>(r));
        result.value().boxedChildren() =
            SmartList<Member>(std::allocator_arg, Allocator(r), Vector<Member>(r));
    }
    return std::move(result).value();
}

static void nested_projection() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto child = nested_child<Node>(*user_context);
    auto member = nested_child<Member>(*user_context);
    auto entity = parent(r, true);
    entity.updateChild(child);
    entity.updateBoxedChild(child);
    entity.childRows().data.emplace_back(member);
    entity.boxedChildren().data.emplace_back(member);
    auto mutation = Parent(std::allocator_arg, Allocator(r), entity).into_values();
    const auto& nested = std::get<Value::Object>(mutation.fields.at(String("child", r)).data);
    check(nested.contains("_comment"), "nested governed mutation retains child intent");
    auto json = render(entity);
    const auto& direct = at(json, "child");
    const auto& boxed = at(json, "boxed_child");
    const auto& list = std::get<Json::Array>(at(json, "child_rows").data);
    const auto& boxed_list = std::get<Json::Array>(at(json, "boxed_children").data);
    check(list.size() == 1 && boxed_list.size() == 1, "nested list members remain present");
    for (const auto* projection : {&direct, &boxed, &list[0], &boxed_list[0]}) {
        for (std::string_view key :
             {"_comment", "_is_new", "_is_deleted", "_dirty_fields", "_original_values"}) {
            check(!has(*projection, key), "nested JSON excludes child mutation internals");
        }
        check(
            at(*projection, "name").dump(r) == "\"native\"" &&
                at(*projection, "_name").dump(r) == "\"derived\"" &&
                at(*projection, "#name").dump(r) == "\"persistent\"" &&
                at(*projection, "#nil").dump(r) == "null" && !has(*projection, "#missing") &&
                at(at(*projection, "_payload"), "_comment").dump(r) == "\"literal business data\"",
            "nested extension namespaces survive without filtering literal business payload keys");
    }
    check(json.dump(r).find("private child mutation reason") == String::npos,
          "private nested audit reason never leaks");
}

static void selected_relations() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto absent = parent(r, false);
    auto missing = render(absent);
    for (std::string_view edge : {"child", "child_rows", "boxed_child", "boxed_children"}) {
        check(!has(missing, edge), "unselected relation is omitted");
    }
    auto selected_parent = parent(r, true);
    auto loaded = render(selected_parent);
    check(at(loaded, "child").dump(r) == "null" && at(loaded, "boxed_child").dump(r) == "null" &&
              at(loaded, "child_rows").dump(r) == "[]" &&
              at(loaded, "boxed_children").dump(r) == "[]",
          "selected NULL objects and empty lists remain distinct from absent relations");
}

static void runtime_embedded_projection() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto child = nested_child<Node>(*user_context);
    auto member = nested_child<Member>(*user_context);
    auto entity = parent(r, true);
    entity.childRows().data.emplace_back(member);
    auto render_runtime = [&](bool loaded, const Node* embedded, const SmartList<Member>* list) {
        EntityJsonTraversal traversal(std::allocator_arg, Allocator(r));
        GeneratedEntityJson projection(
            r, traversal, Parent::ENTITY_NAME, &entity, entity.root, entity.id());
        projection.reverse_one<Node>("single", loaded, embedded);
        projection.reverse_many<Member>("many", loaded, list);
        return projection.finish(nullptr).value();
    };
    auto populated = render_runtime(true, &child, &entity.childRows());
    check(at(at(populated, "single"), "#name").dump(r) == "\"persistent\"" &&
              std::get<Json::Array>(at(populated, "many").data).size() == 1,
          "runtime serializes existing embedded one and many relations");
    auto empty_parent = parent(r, true);
    auto empty = render_runtime(true, nullptr, &empty_parent.childRows());
    check(at(empty, "single").dump(r) == "null" && at(empty, "many").dump(r) == "[]",
          "runtime retains selected embedded NULL and empty list");
    auto missing_parent = parent(r, false);
    auto missing = render_runtime(false, nullptr, &missing_parent.childRows());
    check(!has(missing, "single") && !has(missing, "many"),
          "runtime omits unloaded embedded relations");
    std::cout << "RUNTIME_EMBEDDED_RELATIONS=passed\n";
}

int main(int argc, char** argv) {
    runtime_embedded_projection();
    if (argc == 2 && std::string_view(argv[1]) == "--runtime-only") {
        forward_cycle();
        reverse_projection();
        sibling_isolation();
        return 0;
    }
    std::cerr << "case forward_cycle\n";
    forward_cycle();
    std::cerr << "case reverse_projection\n";
    reverse_projection();
    std::cerr << "case sibling_isolation\n";
    sibling_isolation();
    std::cerr << "case nested_projection\n";
    nested_projection();
    std::cerr << "case selected_relations\n";
    selected_relations();
    std::cout << "GENERATED_BORROWED_GRAPH_CASES=3;NESTED_RELATION_CASES=2\n";
}
