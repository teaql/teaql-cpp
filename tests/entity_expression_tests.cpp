#include <teaql/core/safe_expression.hpp>
#include <teaql/core/entity_graph.hpp>
#include <teaql/core/web.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct NoDefaultAllocator {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~NoDefaultAllocator() {
        std::pmr::set_default_resource(previous);
    }
};

template <class T>
struct HandModel : BaseEntityDefaults<T> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Example";
    BaseEntityData data;

    HandModel(std::allocator_arg_t, Allocator a) : data(std::allocator_arg, a) {}

    HandModel(std::allocator_arg_t, Allocator a, const HandModel& other)
        : data(std::allocator_arg, a, other.data) {}

    HandModel(HandModel&&) noexcept = default;

    Memory memory_resource() const {
        return data.memory_resource();
    }

    const BaseEntityData& base() const {
        return data;
    }

    BaseEntityData& base_mut() {
        return data;
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor descriptor(std::allocator_arg, Allocator(r), ENTITY_NAME);
        descriptor.property("id", DataType::U64).id();
        descriptor.property("version", DataType::I64).version();
        return descriptor;
    }

    static Result<T, EntityError> from_compact_row(Memory r, CompactRow row) {
        using Outcome = Result<T, EntityError>;
        auto base = BaseEntityData::from_values_map(r, row.into_map(r));
        if (!base) {
            return Outcome::err(r, std::move(base).error());
        }
        T entity(std::allocator_arg, Allocator(r));
        entity.data = std::move(base).value();
        return Outcome::ok(r, std::move(entity));
    }

    MutationValues into_values() && {
        MutationValues values(memory_resource());
        values.fields = data.to_values_map(memory_resource());
        return values;
    }

    Value id_value() const {
        return Value(memory_resource(), data.id);
    }

    std::int64_t version() const {
        return data.version;
    }
};

struct Legacy : HandModel<Legacy> {
    using HandModel::HandModel;

    Legacy(std::allocator_arg_t, Allocator a, const Legacy& other)
        : HandModel(std::allocator_arg, a, other) {}

    Legacy(Legacy&&) noexcept = default;
};

struct LoadMarker {
    unsigned value = 0;
};

struct Tracked : HandModel<Tracked> {
    String comment;
    FieldSet loaded_fields, changed_fields;
    bool fields_known = false;
    unsigned marker = 0;

    Tracked(std::allocator_arg_t, Allocator a)
        : HandModel(std::allocator_arg, a), comment(a.resource()), loaded_fields(a.resource()),
          changed_fields(a.resource()) {}

    Tracked(std::allocator_arg_t, Allocator a, const Tracked& other)
        : HandModel(std::allocator_arg, a, other), comment(other.comment, a.resource()),
          loaded_fields(other.loaded_fields, a.resource()),
          changed_fields(other.changed_fields, a.resource()), fields_known(other.fields_known),
          marker(other.marker) {}

    Tracked(Tracked&&) noexcept = default;

    void set_comment(std::string_view value) {
        comment = value;
    }

    std::optional<String> get_comment() const {
        return String(comment, memory_resource());
    }

    void on_loaded(AnyRef context) {
        if (auto* value = context.get<LoadMarker>()) {
            marker = value->value;
        }
    }

    bool is_field_loaded(std::string_view field) const {
        return !fields_known || loaded_fields.contains(field);
    }

    void set_checker_loaded_fields(FieldSet fields) {
        loaded_fields = std::move(fields);
        fields_known = true;
    }

    void set_checker_dirty_fields(FieldSet fields, const MutationValues&) {
        changed_fields = std::move(fields);
    }

    std::optional<FieldSet> dirty_fields() const {
        return FieldSet(changed_fields, memory_resource());
    }
};

static_assert(Entity<Legacy> && BaseEntity<Legacy> && Entity<Tracked> && BaseEntity<Tracked>);
static_assert(IdentifiableEntity<Legacy> && VersionedEntity<Legacy>);
static_assert(ArenaObject<SafeExpression<int, int>> && ArenaObject<Audited<Tracked>>);

struct Store : EntityDescriptorStore {
    using allocator_type = Allocator;
    Vector<EntityDescriptor> entries;

    Store(std::allocator_arg_t, Allocator a) : entries(a.resource()) {}

    Memory memory_resource() const override {
        return entries.get_allocator().resource();
    }

    void register_descriptor(EntityDescriptor descriptor) override {
        entries.emplace_back(descriptor);
    }
};

static void entity_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto a = Allocator(r);
    Audited<Tracked>* retained = nullptr;
    Result<BaseEntityData, EntityError>* error = nullptr;
    {
        auto& store = global.make<Store>();
        Legacy::register_into(store);
        register_entities<Tracked>(store);
        check(store.entries.size() == 2 &&
                  store.entries[0].memory_resource() == global.memory_resource(),
              "descriptor registration chooses store domain");
        auto& base = user->make<BaseEntityData>();
        check(base.id == 0 && base.version == 0 && base.dynamic_values.empty(), "base defaults");
        base.with_id(7)
            .with_version(3)
            .with_dynamic("amount", *Decimal::parse("123.4500"))
            .with_dynamic("title", "a dynamic title long enough to allocate");
        check(base.dynamic_decimal("amount") == Decimal::parse("123.45") &&
                  base.dynamic_i64("amount") == 123,
              "dynamic conversion delegates to Value");
        check(base.dynamic_text("title") && !base.dynamic_bool("amount") &&
                  !base.dynamic("missing"),
              "dynamic accessor distinctions");
        check(base.put_dynamic("amount", true)->try_decimal() == Decimal::parse("123.45"),
              "put returns previous value");
        check(base.remove_dynamic("amount")->try_bool() == true && !base.remove_dynamic("amount"),
              "remove returns previous value");
        auto roundtrip = BaseEntityData::from_values_map(r, base.to_values_map(r));
        check(roundtrip && roundtrip.value() == base, "base map round trip");
        base.with_dynamic("id", 999);
        check(base.to_values_map(r).find("id")->second.try_i64() == 999,
              "dynamic collision wins like Rust");
        base.remove_dynamic("id");
        auto& fields = user->make<Record>();
        fields.emplace("id", Value(r, -1));
        auto invalid = BaseEntityData::from_values_map(r, fields);
        check(!invalid && invalid.error().entity == "BaseEntity" &&
                  invalid.error().message.starts_with("invalid id field:"),
              "negative base id returns typed EntityError");
        fields.find("id")->second = Value(r, TypedNull{DataType::U64});
        check(!BaseEntityData::from_values_map(r, fields),
              "TypedNull does not equal untyped Null in BaseEntity parser");
        fields.find("id")->second = Value(r);
        fields.emplace("version", Value(r, 1u));
        check(!BaseEntityData::from_values_map(r, fields), "base version accepts I64 only");
        fields.find("version")->second = Value(r);
        check(bool(BaseEntityData::from_values_map(r, fields)),
              "untyped null role values default to zero");
        auto& legacy = user->make<Legacy>();
        check(legacy.is_field_loaded("anything") && !legacy.dirty_fields() &&
                  !legacy.original_values() && !legacy.is_new() && !legacy.is_marked_as_delete(),
              "backwards compatible Entity defaults");
        legacy.mark_as_new();
        legacy.set_comment("ignored");
        check(!legacy.is_new() && !legacy.get_comment(), "default setters remain no-op as in Rust");
        LoadMarker marker{42};
        AnyRef borrow(marker);
        auto borrowed_copy = borrow;
        check(borrowed_copy.get<LoadMarker>() == &marker && !borrow.get<int>(),
              "AnyRef copies preserve original type and address");
        auto loaded = Tracked::from_compact_row_with_context(
            r, CompactRow(std::allocator_arg, a, base.to_values_map(r)), borrow);
        check(loaded && loaded.value().marker == 42 && loaded.value().id() == 7,
              "context hook follows successful materialization");
        FieldSet projected(r);
        projected.emplace("id");
        loaded.value().set_checker_loaded_fields(std::move(projected));
        MutationValues changes(r);
        changes.fields.emplace("title", Value(r, "updated"));
        FieldSet dirty(r);
        dirty.emplace("title");
        loaded.value().set_checker_dirty_fields(std::move(dirty), changes);
        check(loaded.value().is_field_loaded("id") && !loaded.value().is_field_loaded("title") &&
                  loaded.value().dirty_fields()->contains("title"),
              "loaded and dirty boundaries remain independent");
        bool rejected = false;
        try {
            user->make<Audited<Tracked>>(loaded.value(), " \t\xe3\x80\x80");
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "blank Unicode audit comment rejects");
        auto& audit = user->make<Audited<Tracked>>(loaded.value(), "  explain the change  ");
        check(audit.get_comment() == "  explain the change  " && audit.entity().comment.empty(),
              "Audited preserves comment without applying it early");
        audit.entity_mut().set_id(8);
        retained = &global.make<Audited<Tracked>>(audit);
        error = &global.make<Result<BaseEntityData, EntityError>>(invalid);
    }
    user->close();
    auto entity = std::move(*retained).into_entity();
    check(entity.comment == "  explain the change  " && entity.id() == 8,
          "Audited copy survives source close and applies comment on consume");
    check(error->error().entity.get_allocator().resource() == global.memory_resource(),
          "error payload copies into destination");
}

static void scalar_expressions() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto context = global.make_user_context();
    auto r = context->memory_resource();
    auto& expression =
        context->make<SafeExpression<int, int>>(2, [](const int& root) -> std::optional<int> {
            return root * 3;
        });
    check(expression.eval() == 6 && expression.eval_with(4) == 12, "eval_with overrides root");
    int optional_calls = 0, remaining_calls = 0;
    auto chain = SafeExpression<int, int>::value(r, 5)
                     .apply_optional([&optional_calls](int) -> std::optional<int> {
                         ++optional_calls;
                         return {};
                     })
                     .apply([&remaining_calls](int n) {
                         ++remaining_calls;
                         return n * 2;
                     });
    check(!chain.eval() && optional_calls == 1 && remaining_calls == 0,
          "missing value short circuits later mappers");
    int fallback_calls = 0;
    auto present = SafeExpression<int, int>::value(r, 7);
    check(present.or_else_with([&] {
        ++fallback_calls;
        return 9;
    }) == 7 && fallback_calls == 0,
          "present skips lazy fallback");
    check(chain.or_else_with([&] {
        ++fallback_calls;
        return 9;
    }) == 9 && fallback_calls == 1,
          "missing evaluates fallback once");
    check(present.or_else(99) == 7 && chain.or_else(99) == 99, "eager fallback");
    auto ok = present.or_else_throw([] {
        return 123;
    });
    auto error = chain.or_else_throw([] {
        return 456;
    });
    check(ok && ok.value() == 7 && !error && error.error() == 456,
          "or_else_throw returns Result, not an exception");
    int nulls = 0, values = 0;
    present.when_is_null([&] {
        ++nulls;
    });
    present.when_is_not_null([&](int n) {
        values += n;
    });
    chain.when_is_null([&] {
        ++nulls;
    });
    chain.when_is_not_null([&](int n) {
        values += n;
    });
    check(nulls == 1 && values == 7, "matching callback branches only");
    auto same = SafeExpression<int, int>(std::allocator_arg, Allocator(r), present);
    check(&same.root() == &present.root(), "same-domain clone shares immutable root");
    auto& retained = global.make<SafeExpression<int, int>>(present);
    check(&retained.root() != &present.root(), "cross-domain clone copies root");
    bool rejected = false;
    try {
        global.make<decltype(chain)>(chain);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected,
          "cross-domain opaque lambda captures reject instead of retaining borrowed source state");
    context->close();
    check(retained.eval() == 7 && context->allocation_stats().live_blocks == 0,
          "retained evaluation and full user reclamation");
}

static void empty_and_adapters() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto null = SafeExpression<Value, Value>::value(r, Value(r));
    auto typed = SafeExpression<Value, Value>::value(r, Value(r, TypedNull{DataType::Text}));
    check(null.is_not_null() && null.is_empty() && typed.is_not_empty(),
          "missing option, Value Null and TypedNull remain distinct");
    check(teaql_is_empty(std::optional<int>{}) && !teaql_is_empty(std::optional<String>(String(r))),
          "Some(empty) is not empty for Option");
    for (auto text : {"null", "\"\"", "[]", "{}"}) {
        check(teaql_is_empty(Value(r, *Json::parse(r, text))), "empty JSON cases");
    }
    for (auto text : {"0", "false", "[null]", "{\"a\":null}"}) {
        check(!teaql_is_empty(Value(r, *Json::parse(r, text))),
              "nonempty JSON scalars and collections");
    }
    auto text = SafeExpression<String, String>::value(r, String("teaql", r));
    int empties = 0, nonempty = 0;
    text.when_is_empty([&] {
        ++empties;
    });
    text.when_not_empty([&](String value) {
        check(value == "teaql", "callback value");
        ++nonempty;
    });
    auto empty = SafeExpression<String, String>::value(r, String(r));
    empty.when_is_empty([&] {
        ++empties;
    });
    empty.when_not_empty([&](String) {
        ++nonempty;
    });
    check(empties == 1 && nonempty == 1, "empty callback branches");
    auto& entity = global.make<Legacy>();
    entity.set_id(10);
    entity.set_version(3);
    auto model = SafeExpression<Legacy, Legacy>::value(r, entity);
    check(model.entity_id().eval() == 10 && model.entity_version().eval() == 3, "entity adapters");
    check(model.update_entity_id(20).eval()->id() == 20 && model.root().id() == 10,
          "entity update maps a copy and preserves root");
    auto& list = global.make<SmartList<Legacy>>();
    list.push(entity);
    auto expression = SafeExpression<SmartList<Legacy>, SmartList<Legacy>>::value(r, list);
    check(expression.size().eval() == 1 && expression.first().entity_id().eval() == 10 &&
              !expression.get(4).eval(),
          "SmartList navigation adapters");
}

struct Append {
    using allocator_type = Allocator;
    String suffix;
    unsigned* alive;

    Append(std::allocator_arg_t, Allocator a, std::string_view text, unsigned& count)
        : suffix(text, a.resource()), alive(&count) {
        ++*alive;
    }

    Append(std::allocator_arg_t, Allocator a, const Append& other)
        : suffix(other.suffix, a.resource()), alive(other.alive) {
        ++*alive;
    }

    Append(const Append&) = delete;

    Append(Append&& other) noexcept
        : suffix(std::move(other.suffix)), alive(std::exchange(other.alive, nullptr)) {}

    ~Append() {
        if (alive) {
            --*alive;
        }
    }

    String operator()(String input, Memory r) const {
        check(suffix.get_allocator().resource() == r && input.get_allocator().resource() == r,
              "callback state and input use evaluation domain");
        input += suffix;
        return input;
    }
};

struct LimitedResource : std::pmr::memory_resource {
    std::size_t remaining, live = 0;

    explicit LimitedResource(std::size_t count) : remaining(count) {}

    void* do_allocate(std::size_t n, std::size_t a) override {
        if (!remaining) {
            throw std::bad_alloc();
        }
        auto* p = std::pmr::new_delete_resource()->allocate(n, a);
        --remaining;
        ++live;
        return p;
    }

    void do_deallocate(void* p, std::size_t n, std::size_t a) override {
        --live;
        std::pmr::new_delete_resource()->deallocate(p, n, a);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return &other == this;
    }
};

static void expression_lifetimes() {
    NoDefaultAllocator guard;
    unsigned alive = 0;
    {
        GlobalContext global;
        auto user = global.make_user_context(64);
        auto r = user->memory_resource();
        auto expression =
            SafeExpression<String, String>::value(r, String(4000, 'r', r))
                .apply(Append(std::allocator_arg, Allocator(r), String(4000, 's', r), alive));
        const auto initial = alive;
        check(initial > 0, "callback state registered in arena");
        unsigned failed = 0, succeeded = 0;
        for (unsigned limit = 0; limit < 24; ++limit) {
            LimitedResource resource(limit);
            {
                GlobalContext destination(&resource, 64);
                try {
                    auto& copy = destination.make<SafeExpression<String, String>>(expression);
                    check(copy.eval()->size() == 8000, "cloned state evaluates");
                    ++succeeded;
                } catch (const std::bad_alloc&) {
                    ++failed;
                }
            }
            check(resource.live == 0 && alive == initial,
                  "partial expression graph cleans up every registered callback and arena block");
        }
        check(failed && succeeded, "allocation failures exercised before successful clone");
        auto& retained = global.make<SafeExpression<String, String>>(expression);
        {
            auto nested = SafeExpression<std::optional<String>, std::optional<String>>::value(
                r, std::optional<String>(String("nested", r)));
            check(nested.eval()->value() == "nested",
                  "optional root clones nested allocator-aware values");
            auto outcome = nested.or_else_throw([] {
                return 17;
            });
            check(outcome && outcome.value()->get_allocator().resource() == r,
                  "Result retains nested optional allocation domain");
        }
        user->close();
        auto result = retained.eval();
        check(result->size() == 8000 &&
                  result->get_allocator().resource() == global.memory_resource(),
              "root and callback survive source close only through deep copy");
        check(user->allocation_stats().live_blocks == 0,
              "all user expression arena blocks reclaimed");
    }
    check(alive == 0, "all callback state destructors run before reclamation");
}

struct Relations {
    using allocator_type = Allocator;
    Value foreign;

    Relations(std::allocator_arg_t, Allocator a, const Value& value)
        : foreign(a.resource(), value) {}

    Relations(std::allocator_arg_t, Allocator a, const Relations& other)
        : foreign(a.resource(), other.foreign) {}

    Relations(Relations&&) noexcept = default;

    static void extend_descriptor(EntityDescriptor& d) {
        d.relation("parent", "Example");
    }

    static Result<Relations, EntityError> extract_from_values(Memory r, const CompactRow& row) {
        using Outcome = Result<Relations, EntityError>;
        if (auto value = row.get("parent")) {
            return Outcome::ok(r, Relations(std::allocator_arg, Allocator(r), *value));
        }
        return Outcome::err(
            r, EntityError(std::allocator_arg, Allocator(r), "Example", "missing parent"));
    }

    void inject_into_values(Record& values) && {
        values.insert_or_assign(String("parent", values.get_allocator().resource()),
                                Value(values.get_allocator().resource(), foreign));
    }
};

static_assert(TeaqlBoxedRelations<Relations> && TeaqlBoxedRelations<BoxedRelations<Relations>>);

static void boxed_relations() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto r = global.memory_resource();
    auto& descriptor = global.make<EntityDescriptor>("Example");
    BoxedRelations<Relations>::extend_descriptor(descriptor);
    auto& row = global.make<CompactRow>();
    row.insert("parent", Value(r, 7u));
    auto result = BoxedRelations<Relations>::extract_from_values(r, row);
    check(result && result.value().get().foreign.try_u64() == 7 &&
              descriptor.find_relation("parent"),
          "boxed relations delegate descriptor and extraction");
    auto& output = global.make<Record>();
    std::move(result).value().inject_into_values(output);
    check(output.find("parent")->second.try_u64() == 7,
          "boxed injection delegates to relation type");
    auto missing = BoxedRelations<Relations>::extract_from_values(r, global.make<CompactRow>());
    check(!missing && missing.error().message == "missing parent",
          "boxed extraction preserves error");
}

static void annotated_graph() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context(64);
    auto r = user->memory_resource();
    EntityGraph* retained = nullptr;
    {
        Legacy parent(std::allocator_arg, Allocator(r));
        parent.set_id(1);
        Legacy child(std::allocator_arg, Allocator(r));
        child.set_id(2);
        child.put_dynamic(
            "payload",
            *Json::parse(r, R"({"items":["a long dynamic field requiring allocation"]})"));
        auto graph = EntityGraph::from_entity(r, std::move(parent))
                         .comment("Parent creation")
                         .child("items",
                                EntityGraph::from_entity(r, std::move(child))
                                    .comment("Child deletion")
                                    .delete_())
                         .build();
        check(graph.root.operation == EntityGraphOperation::Save &&
                  graph.root.comment == "Parent creation",
              "graph root annotation");
        check(graph.root.children.size() == 1 && graph.root.children[0].first == "items" &&
                  graph.root.children[0].second.operation == EntityGraphOperation::Delete,
              "annotated child operation");
        check(graph.root.children[0].second.comment == "Child deletion",
              "child intent stays on its own node");
        retained = &global.make<EntityGraph>(graph);
        Legacy empty(std::allocator_arg, Allocator(r));
        auto none = EntityGraph::from_entity(r, std::move(empty)).build();
        check(!none.root.comment, "graph comment starts None");
        Legacy blank(std::allocator_arg, Allocator(r));
        auto some = EntityGraph::from_entity(r, std::move(blank)).comment("").build();
        check(some.root.comment && some.root.comment->empty(),
              "graph permits explicit empty annotation, unlike Audited");
    }
    user->close();
    const auto& child = retained->root.children[0].second;
    check(child.memory_resource() == global.memory_resource() &&
              child.values.fields.find("id")->second.try_u64() == 2,
          "graph deep copy survives source close");
    check(retained->root.children[0].first.get_allocator().resource() == global.memory_resource() &&
              child.comment->get_allocator().resource() == global.memory_resource(),
          "relation names and node comments propagate allocator");
    check(std::get<Json>(child.values.fields.find("payload")->second.data).memory_resource() ==
              global.memory_resource(),
          "nested graph payload keeps destination domain");
}

static void entity_lists() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    SmartList<MutationValues>* retained = nullptr;
    {
        auto& unchanged = user->make<SmartList<Legacy>>();
        unchanged.emplace_back().set_id(1);
        check(!std::move(unchanged).into_value(),
              "unloaded unchanged entities do not invent relation intent");
        auto& loaded = user->make<SmartList<Legacy>>();
        loaded.loaded();
        auto empty = std::move(loaded).into_value();
        check(empty && empty->as_list()->empty(),
              "loaded empty relation produces explicit empty list");
        auto& tracked = user->make<SmartList<Tracked>>();
        tracked.emplace_back().set_id(2);
        auto changed = std::move(tracked).into_value();
        check(changed && changed->as_list()->size() == 1 &&
                  changed->as_list()->front().as_object()->find("id")->second.try_u64() == 2,
              "Some(empty dirty set) is still mutation intent");
        auto& list = user->make<SmartList<Legacy>>();
        list.emplace_back().set_id(7);
        list[0].set_version(4);
        list.emplace_back().set_id(7);
        list[1].set_version(5);
        list.loaded().with_total_count(19).with_summary(
            "caption", "long summary preserved across conversion and source close");
        auto ids = list.ids();
        auto versions = list.versions();
        auto by_id = list.map_by_id();
        check(ids.size() == 2 && ids[0].try_u64() == 7 && versions[0] == 4 && versions[1] == 5,
              "entity ID and version adapters");
        check(by_id.size() == 1 && by_id.at(String("u:7", r)).version() == 5,
              "ID map keeps last entity with type-qualified key");
        auto values = std::move(list).into_values();
        check(values.is_loaded && values.total_count == 19 && values.len() == 2,
              "into_values preserves list metadata and distinct MutationValues");
        retained = &global.make<SmartList<MutationValues>>(values);
    }
    user->close();
    check(retained->data[1].fields.find("version")->second.try_i64() == 5 &&
              retained->summary_value("caption")->try_text()->starts_with("long summary"),
          "entity list conversion deep copy survives source Context close");
}

static void entity_responses() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    WebResponse* retained = nullptr;
    {
        auto& list = user->make<SmartList<Legacy>>();
        list.emplace_back().set_id(17);
        list.emplace_back().set_id(19);
        list.with_total_count(99).with_summary("ignored", "not emitted as entity");
        auto& facet = user->make<SmartList<CompactRow>>();
        facet.emplace_back().insert("name", Value(r, "long facet text requiring allocation"));
        facet.with_total_count(123).with_aggregation("count", 456);
        list.with_facet("names", facet);
        auto style = WebStyle::with_font_color(r, "blue");
        style.bind_entity(list[0]);
        WebAction::view_web_action(r).bind_entity(list[0]);
        auto single = WebResponse::from_entity(r, list[0]);
        check(single.data.size() == 1 && single.record_count == 1 && list[0].id() == 17,
              "from_entity clones input without consuming it");
        auto response = WebResponse::from_smart_list(global.memory_resource(), std::move(list));
        check(response.record_count == 99 && response.data.size() == 2 &&
                  response.facets.size() == 1,
              "SmartList response preserves total count independently of payload size");
        const auto& values = std::get<Json::Array>(response.facets.begin()->second.data);
        check(values.size() == 1 && std::get<Json::Object>(values[0].data).size() == 1,
              "facet output contains rows only, not facet metadata");
        retained = &global.make<WebResponse>(std::move(response));
        auto& empty = user->make<SmartList<Legacy>>();
        empty.with_total_count(42);
        auto total = WebResponse::from_smart_list(r, std::move(empty));
        check(total.record_count == 42 && total.data.empty(),
              "empty response can carry nonzero total count");
    }
    user->close();
    check(retained->to_json_value(global.memory_resource())
                  .dump(global.memory_resource())
                  .find("long facet text") != String::npos,
          "entity response recursively survives source close");
}

int main() {
    try {
        entity_contracts();
        scalar_expressions();
        empty_and_adapters();
        expression_lifetimes();
        boxed_relations();
        annotated_graph();
        entity_lists();
        entity_responses();
        std::cout << "Entity and SafeExpression contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
