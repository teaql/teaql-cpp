#include "../../examples/generated/include/cpp_examples.hpp"
#include <array>
#include <iostream>
#include <optional>
#include <string>

using namespace teaql;
using cpp_examples::Product;
using cpp_examples::Order;
using cpp_examples::OrderLine;
using cpp_examples::E;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& action, const char* message) {
    bool failed = false;
    try {
        action();
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, message);
}
template <class T>
concept TemporaryBinding = requires {
    entity_expression(T(std::allocator_arg, Allocator(std::pmr::new_delete_resource())));
};
static_assert(!TemporaryBinding<Product>);
template <class T>
concept TemporaryNamedBinding = requires {
    entity_expression(T(std::allocator_arg, Allocator(std::pmr::new_delete_resource())),
                      StaticString{"product"},
                      StaticString{"ModelProduct"});
};
static_assert(!TemporaryNamedBinding<Product>);
static_assert(std::is_trivially_destructible_v<RuntimeExpression<String>>);
static_assert(std::is_nothrow_move_constructible_v<EntityRuntimeState>);

static auto name_in_helper(const Product& product) {
    return E::product(product).getName();
}

static void explicit_model_identity() {
    GlobalContext global;
    auto source = global.make_user_context();
    auto destination = global.make_user_context();
    std::optional<RuntimeExpression<String>> expired;
    std::optional<RuntimeExpression<String>> retained;
    std::optional<Ref<ExpressionNotLoadedError>> retained_error;
    {
        auto r = source->memory_resource();
        auto& product = source->make<Product>();
        product.updateId(77);
        using OriginalFactory = RuntimeExpression<EntityBorrow<Product>> (*)(const Product&);
        using EntryFactory =
            RuntimeExpression<EntityBorrow<Product>> (*)(const Product&, StaticString);
        OriginalFactory original_factory = &entity_expression<Product>;
        EntryFactory entry_factory = &entity_expression<Product>;
        check(original_factory(product).rootDescription() == "Product(id=77)" &&
                  entry_factory(product, StaticString{"product"}).rootDescription() ==
                      "Product(id=77)",
              "Both exact legacy entity factory signatures retain their root contract");
        auto root =
            entity_expression(product, StaticString{"nativeProduct"}, StaticString{"ModelProduct"});
        product.updateId(88);
        check(root.rootDescription() == "ModelProduct(id=77)" &&
                  std::string_view(Product::ENTITY_NAME) == "Product",
              "Explicit canonical identity captures the ID without changing the descriptor");
        auto missing = root.andThen("related", [r](EntityBorrow<Product>) {
            return RuntimeExpression<String>::not_loaded(r, "name", "name");
        });
        expired = missing;
        retained = missing.copyTo(*destination);
        try {
            (void)missing.unwrap();
        } catch (const ExpressionNotLoadedError& error) {
            retained_error = source->make_global<ExpressionNotLoadedError>(error);
        }
    }
    source.reset();
    rejects(
        [&] {
            (void)expired->unwrap();
        },
        "Explicit model metadata must not keep its source Context alive");
    check(bool(retained_error), "A missing field must retain a diagnostic");
    auto inspect = [](const ExpressionDiagnostic& diagnostic) {
        check(diagnostic.root == "ModelProduct(id=77)" &&
                  diagnostic.original_expr_with_broken_point ==
                      "E::nativeProduct(id=77).getRelated().getName()<broken>" &&
                  diagnostic.attempted_path == "related.name",
              "Canonical root and native entry survive traversal and explicit destination copies");
    };
    bool copied_missing = false;
    try {
        (void)retained->unwrap();
    } catch (const ExpressionNotLoadedError& error) {
        inspect(error.diagnostic());
        copied_missing = true;
    }
    check(copied_missing, "The explicit scalar copy must retain its NotLoaded state");
    inspect(destination->get(destination->rebind_global(*retained_error)).diagnostic());
    retained.reset();
    destination.reset();
    check(global.live_user_bytes() == 0, "Explicit root names do not retain UserContext blocks");
}

static void snapshots_and_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    std::optional<cpp_examples::ProductExpression> dead;
    std::optional<RuntimeExpression<String>> snapshot;
    {
        Product local(std::allocator_arg, Allocator(r));
        local.updateName("original long enough string to allocate");
        dead.emplace(E::product(local));
        snapshot.emplace(name_in_helper(local));
        local.updateName("changed name");
        {
            auto value = snapshot->unwrap();
            check(value == "original long enough string to allocate", "eager string snapshot");
        }
        auto before = user_context->allocation_stats().live_bytes;
        check(snapshot->isNotNull() && !snapshot->isNull(), "null predicates inspect state");
        check(user_context->allocation_stats().live_bytes == before,
              "null checks do not copy string payload");
        Product moved(std::move(local));
        rejects(
            [&] {
                dead->getName();
            },
            "old borrow invalid after entity move");
        auto current = E::product(moved);
        {
            auto name = current.getName().unwrap();
            check(name == "changed name", "new borrow follows moved entity");
        }
    }
    rejects(
        [&] {
            dead->unwrap();
        },
        "destroyed stack entity borrow rejected");
    {
        auto value = snapshot->unwrap();
        check(value.find("original") == 0, "owning scalar survives source entity destruction");
    }
    auto managed = user_context->make_ref<Product>();
    auto binding = E::product(user_context->get(managed));
    user_context->destroy(managed);
    rejects(
        [&] {
            binding.getName();
        },
        "explicit destruction rejected");
    user_context->close();
    rejects(
        [&] {
            snapshot->isNull();
        },
        "snapshot access rejected after close");
    user_context.reset();
    rejects(
        [&] {
            dead->getName();
        },
        "binding rejected after actual Context deletion");
    rejects(
        [&] {
            snapshot->unwrap();
        },
        "snapshot rejected before accessing deleted arena");
    snapshot.reset();
    dead.reset();
    check(global.live_user_bytes() == 0, "surviving wrappers do not retain Context blocks");
}

static void lists_and_composition() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& order = user_context->make<Order>();
    auto& line = user_context->make<OrderLine>();
    line.updateName("line");
    auto& product = user_context->make<Product>();
    product.updateName("nested product");
    line.updateProduct(product);
    order.lines().data.emplace_back(std::move(line));
    order.lines().loaded();
    auto list = E::order(order).getLines();
    auto item = list.first();
    {
        auto value = item.getProduct().getName().unwrap();
        check(value == "nested product", "nested entity/list composition");
    }
    check(list.size().unwrap() == 1 && list.get(8).isNull(), "size and out-of-range Null");
    auto original_name = item.getName();
    order.lines().data.reserve(order.lines().data.capacity() + 100);
    rejects(
        [&] {
            item.getName();
        },
        "element borrow rejected after vector relocation");
    rejects(
        [&] {
            list.first();
        },
        "list view rejected after structural change");
    auto replacement = E::order(order).getLines().first();
    auto* replaced = &order.lines().data[0];
    std::destroy_at(replaced);
    std::construct_at(replaced, std::allocator_arg, Allocator(r));
    rejects(
        [&] {
            replacement.getName();
        },
        "element replacement rejects old borrow");
    {
        auto name = original_name.unwrap();
        check(name == "line", "snapshot independent of collection replacement");
    }
    auto null = RuntimeExpression<String>::null(r);
    {
        auto json_default = RuntimeExpression<Json>::null(r).orDefaultIfNull();
        check(json_default.memory_resource() == r &&
                  std::holds_alternative<std::monostate>(json_default.data),
              "allocator-aware JSON default stays in the expression domain");
    }
    auto& without_product = user_context->make<OrderLine>();
    check(E::orderLine(without_product).getProduct().getName().result().is_not_loaded(),
          "an uninitialized generated relation remains NotLoaded without SQL");
    const std::array<std::string_view, 1> just_id{"id"};
    without_product.load_state = eval::LoadState::partial(r, just_id);
    auto omitted = E::orderLine(without_product).getProduct().getName();
    check(omitted.result().is_not_loaded() &&
              omitted.result().missing().attempted_path == "product",
          "omitted relation remains NotLoaded through nested accessors");
    unsigned callbacks = 0;
    auto null_map = RuntimeExpression<int>::null(r).map([&](int value) {
        ++callbacks;
        return value;
    });
    auto missing_map = RuntimeExpression<int>::not_loaded(r, "x", "x").map([&](int value) {
        ++callbacks;
        return value;
    });
    check(null_map.isNull() && missing_map.result().is_not_loaded() && callbacks == 0,
          "map does not execute callbacks for Null or NotLoaded");
    bool invoked = false;
    {
        auto value = null.orElseIfNull([&] {
            invoked = true;
            return String("fallback", r);
        });
        check(value == "fallback" && invoked, "legitimate Null fallback");
    }
    auto missing = RuntimeExpression<String>::not_loaded(r, "name", "name");
    rejects(
        [&] {
            auto value = missing.orElseIfNull([&] {
                invoked = false;
                return String(r);
            });
            (void)value;
        },
        "NotLoaded cannot be hidden by fallback");
    check(invoked, "missing path does not invoke fallback");
    auto nested_missing = entity_expression(order).andThen("lines", [r](EntityBorrow<Order>) {
        return RuntimeExpression<int>::not_loaded(r, "name", "product.name");
    });
    check(nested_missing.result().missing().attempted_path == "lines.product.name",
          "source missing-path propagation");
    auto other = global.make_user_context();
    rejects(
        [&] {
            entity_expression(order).andThen("foreign", [&](EntityBorrow<Order>) {
                return RuntimeExpression<int>::value(other->memory_resource(), 3);
            });
        },
        "foreign composition rejected");
    auto mapped = RuntimeExpression<int>::value(r, 4).map([](int value) {
        return value * 2;
    });
    check(mapped.unwrap() == 8, "typed scalar map composition");
}

static void global_copy() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto& original = user_context->make<Product>();
    original.updateName("retained product");
    auto retained = user_context->make_global<Product>(original);
    user_context.reset();
    auto access = global.make_user_context();
    auto rebound = access->rebind_global(retained);
    auto expression = E::product(access->get(rebound));
    access.reset();
    {
        auto name = expression.getName().unwrap();
        check(name == "retained product" &&
                  name.get_allocator().resource() == global.memory_resource(),
              "explicit Global entity copy supports independent E binding");
    }
}

static void explicit_snapshot_copy_and_owner_destruction() {
    GlobalContext global;
    auto source_context = global.make_user_context();
    auto destination = global.make_user_context();
    auto& product = source_context->make<Product>();
    product.updateName("independent allocated snapshot text");
    auto source = E::product(product).getName();
    auto copied = source.copyTo(*destination);
    auto source_borrow = EntityBorrow<Product>(product);
    rejects(
        [&] {
            RuntimeExpression<EntityBorrow<Product>>::value(destination->memory_resource(),
                                                            source_borrow);
        },
        "borrows cannot be retained in a foreign expression domain");
    source_context.reset();
    {
        auto text = copied.unwrap();
        check(text == "independent allocated snapshot text" &&
                  text.get_allocator().resource() == destination->memory_resource(),
              "explicit scalar snapshot copy survives actual source deletion");
    }
    auto owner = destination->make_ref<Order>();
    auto& order = destination->get(owner);
    order.lines().loaded();
    auto list = E::order(order).getLines();
    destination->destroy(owner);
    rejects(
        [&] {
            list.size();
        },
        "list rejects access after owning entity destruction");
    destination.reset();
    rejects(
        [&] {
            list.first();
        },
        "list wrapper rejects actual Context deletion before touching list storage");
}

static void installed_list_view_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& owner = user_context->make<Order>();
    owner.updateId(7);
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    SmartList<OrderLine> stored(std::allocator_arg, Allocator(r));
    check(!stored.is_loaded, "default list payload remains unmarked");
    graph.install_relation_list("Order", 7, "lines", std::move(stored));
    owner.root.freeze_graph(std::move(graph)).value();
    auto expression = E::order(owner).getLines();
    check(expression.size().unwrap() == 0 && expression.eval().has_value() &&
              !std::as_const(owner).lines().is_loaded,
          "installed empty view is a Value without changing its payload flag");
    auto& unrelated = user_context->make<SmartList<OrderLine>>();
    auto missing = GeneratedRelationAccess::list(owner, unrelated, "lines");
    check(missing.is_not_loaded(), "availability applies to the installed view identity");
    auto empty_item = expression.first().getProduct().getName();
    bool fallback_called = false;
    check(empty_item.orElseIfNull([&] {
        fallback_called = true;
        return String("fallback", r);
    }) == "fallback" &&
              fallback_called,
          "loaded empty relation propagates Null through entity and scalar traversal");
    user_context.reset();
    rejects(
        [&] {
            (void)expression.eval();
        },
        "installed view borrow expires on actual Context deletion");
}

static void structured_diagnostics() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& product = user_context->make<Product>();
    product.updateId(42);
    auto root = E::product(product);
    product.updateId(99);
    auto missing = root.getName();
    check(missing.rootDescription() == "Product(id=42)", "root identity is an eager snapshot");
    std::optional<ExpressionNotLoadedError> retained;
    try {
        (void)missing.unwrap();
    } catch (const ExpressionNotLoadedError& error) {
        const auto& d = error.diagnostic();
        check(d.root == "Product(id=42)" && d.failed_node == "name" && d.access_path.size() == 1 &&
                  d.missing_preload[0] == "name" && d.suggested_fix == "selectName()" &&
                  !d.nested_fix &&
                  d.original_expr_with_broken_point == "E::product(id=42).getName()<broken>",
              "generated missing scalar has complete source diagnostic fields");
        auto encoded = d.to_json(r).dump(r);
        check(encoded.find("error") != String::npos, "structured diagnostic serializes");
        retained.emplace(error);
    }
    check(retained.has_value(), "NotLoaded throws the typed diagnostic");
    auto nested = root.andThen("lines", [r](EntityBorrow<Product>) {
        return RuntimeExpression<int>::not_loaded(r, "product", "product.display_name");
    });
    auto mapped = nested.map([](int n) {
        return n + 1;
    });
    auto destination = global.make_user_context();
    auto copied = mapped.copyTo(*destination);
    bool nested_rejected = false;
    try {
        (void)copied.isNull();
    } catch (const ExpressionNotLoadedError& error) {
        nested_rejected = true;
        const auto& d = error.diagnostic();
        check(d.root == "Product(id=42)" && d.access_path.size() == 3 &&
                  d.suggested_fix == "selectProduct()" && d.nested_fix &&
                  *d.nested_fix ==
                      "selectProductWith(<generated Q entry for product>.selectDisplayName())",
              "map, nested composition and explicit copy retain root provenance");
    }
    check(nested_rejected, "copied missing snapshot rejects null inspection");
    auto global_error = user_context->make_global<ExpressionNotLoadedError>(*retained);
    user_context.reset();
    check(std::string_view(retained->what()) == "Expression requires preloading",
          "what remains static after Context destruction");
    rejects(
        [&] {
            (void)retained->diagnostic();
        },
        "diagnostic borrow rejects actual Context deletion");
    auto access = global.make_user_context();
    check(access->get(access->rebind_global(global_error)).diagnostic().root == "Product(id=42)",
          "explicit diagnostic copy survives source Context deletion");
    bool copied_rejected = false;
    try {
        (void)copied.unwrap();
    } catch (const ExpressionNotLoadedError& error) {
        copied_rejected = true;
        check(error.diagnostic().root == "Product(id=42)",
              "copied missing snapshot is independent");
    }
    check(copied_rejected, "copied missing snapshot still rejects unwrap");
}

static void typed_member_alias_lookup(bool observe = false) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array indexes{FieldLayout::Index{"id", 0},
                             FieldLayout::Index{"version", 1},
                             FieldLayout::Index{"business_name", 2}};
    const std::array mappings{FieldLayout::Mapping{"id", "id", "id"},
                              FieldLayout::Mapping{"version", "version", "version"},
                              FieldLayout::Mapping{"business_name", "display_name", "legacy_url"}};
    const std::array<std::string_view, 3> members{"id", "version", "display_name"};
    auto& layout = user_context->make<FieldLayout>(
        std::move(FieldLayout::from_generated(
                      r, "ThreeNameRow", "three-name-v1", indexes, mappings, {}, members))
            .value());
    ArenaBorrow<FieldLayout> binding(std::allocator_arg, Allocator(r), layout);
    auto& pool = user_context->make<SnapshotPool>();
    const std::array<std::string_view, 4> aliases{
        "business_name", "display_name", "legacy_url", "business_nmae"};
    std::array<std::size_t, 4> order{};
    auto visit = [&](auto&& self, std::size_t count, unsigned used) -> void {
        for (bool null_value : {false, true}) {
            Vector<String> columns(r);
            columns.emplace_back("id");
            columns.emplace_back("version");
            Vector<Value> values(r);
            values.emplace_back(std::uint64_t{1});
            values.emplace_back(std::int64_t{1});
            std::optional<std::string_view> expected;
            for (std::size_t i = 0; i < count; ++i) {
                auto name = aliases[order[i]];
                columns.emplace_back(name);
                if (null_value) {
                    values.emplace_back(nullptr);
                } else {
                    values.emplace_back(name);
                }
                if (name != "business_nmae") {
                    expected = name;
                }
            }
            auto& shape = user_context->make<ColumnLayout>(std::move(columns));
            CompactRow row(std::allocator_arg, Allocator(r), shape, std::move(values));
            auto loaded = row.indexed_load_state(pool, layout);
            auto found = GeneratedRelationAccess::lookupField(
                r, row, "ThreeNameRow", "business_name", "legacy_url", false, binding);
            check(found && !found.value().alias_present && !found.value().object_alias,
                  "scalar aliases do not imply a loaded relation");
            const auto* value = found.value().value;
            check(bool(value) == expected.has_value(),
                  "only known canonical/member/physical aliases decode a payload");
            if (value) {
                check(null_value ? value->is_null() : value->try_text() == expected,
                      "last known alias in row iteration order supplies the value, including NULL");
            }
            for (auto name : {"business_name", "display_name", "legacy_url"}) {
                check(loaded.is_loaded(name) == expected.has_value(),
                      "all aliases share availability without loading a typo");
            }
            if (observe) {
                std::cout << (value ? (value->is_null() ? "NULL" : *value->try_text()) : "MISSING")
                          << ' ' << loaded.is_loaded("business_name") << ' '
                          << loaded.is_loaded("display_name") << ' '
                          << loaded.is_loaded("legacy_url") << '\n';
            }
        }
        for (std::size_t i = 0; i < aliases.size(); ++i) {
            if ((used & (1U << i)) == 0) {
                order[count] = i;
                self(self, count + 1, used | (1U << i));
            }
        }
    };
    visit(visit, 0, 0);
    CompactRow empty(std::allocator_arg, Allocator(r));
    auto& global_layout = global.make<FieldLayout>(layout);
    ArenaBorrow<FieldLayout> global_binding(std::allocator_arg, Allocator(r), global_layout);
    check(GeneratedRelationAccess::lookupField(
              r, empty, "ThreeNameRow", "business_name", "legacy_url", false, global_binding)
              .is_ok(),
          "Global metadata may lend to a binding in this UserContext");
    check(!GeneratedRelationAccess::lookupField(
              r, empty, "AnotherRow", "business_name", "legacy_url", false, binding),
          "metadata cannot silently decode another entity");
    check(!GeneratedRelationAccess::lookupField(
              r, empty, "ThreeNameRow", "business_name", "version", false, binding),
          "canonical and physical names must refer to the same validated slot");
    auto next_user_context = global.make_user_context();
    rejects(
        [&] {
            GeneratedRelationAccess::lookupField(next_user_context->memory_resource(),
                                                 empty,
                                                 "ThreeNameRow",
                                                 "business_name",
                                                 "legacy_url",
                                                 false,
                                                 binding);
        },
        "metadata binding cannot silently cross UserContexts");
}

static void typed_alias_binding_lifetime() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& layout =
        user_context->make<FieldLayout>(std::move(Product::create_field_layout(r)).value());
    ArenaBorrow<FieldLayout> expired(std::allocator_arg, Allocator(r), layout);
    user_context.reset();
    auto global_resource = global.memory_resource();
    CompactRow empty(std::allocator_arg, Allocator(global_resource));
    rejects(
        [&] {
            GeneratedRelationAccess::lookupField(
                global_resource, empty, "Product", "name", "name", false, expired);
        },
        "expired metadata is rejected before its freed layout or allocator is accessed");
}

static void typed_alias_forward_object_rules() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const std::array indexes{FieldLayout::Index{"id", 0},
                             FieldLayout::Index{"version", 1},
                             FieldLayout::Index{"business_parent", 2}};
    const std::array mappings{FieldLayout::Mapping{"id", "id", "id"},
                              FieldLayout::Mapping{"version", "version", "version"},
                              FieldLayout::Mapping{"business_parent", "parent_id", "parent"}};
    const std::array<std::string_view, 3> members{"id", "version", "parent_id"};
    const std::array<std::string_view, 1> relations{"parent"};
    auto& layout = user_context->make<FieldLayout>(
        std::move(FieldLayout::from_generated(
                      r, "ParentLink", "parent-v1", indexes, mappings, relations, members))
            .value());
    ArenaBorrow<FieldLayout> binding(std::allocator_arg, Allocator(r), layout);
    for (auto name : {"business_parent", "parent_id", "parent", "parent_typo"}) {
        for (bool with_id : {false, true}) {
            Value::Object object(r);
            if (with_id) {
                object.emplace(String("id", r), Value(r, std::uint64_t{42}));
            }
            Record record(r);
            record.emplace(String(name, r), Value(r, std::move(object)));
            CompactRow row(std::allocator_arg, Allocator(r), std::move(record));
            auto found = GeneratedRelationAccess::lookupField(
                r, row, "ParentLink", "business_parent", "parent", true, binding);
            if (std::string_view(name) == "parent" && !with_id) {
                check(!found, "physical forward object without id remains an error");
            } else {
                check(found.is_ok(), "nonphysical aliases are not parsed as reference objects");
                const auto& value = found.value();
                if (std::string_view(name) == "parent") {
                    check(value.alias_present && value.object_alias && value.value &&
                              value.value->try_u64() == 42,
                          "only the physical forward alias unwraps its object's id");
                } else {
                    check(!value.alias_present && !value.object_alias &&
                              (std::string_view(name) == "parent_typo"
                                   ? value.value == nullptr
                                   : value.value && value.value->as_object()),
                          "canonical/member slots remain raw values; unknown names stay absent");
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--alias-probe") {
            typed_member_alias_lookup(true);
            std::pmr::set_default_resource(previous);
            return 0;
        }
        typed_member_alias_lookup();
        typed_alias_binding_lifetime();
        typed_alias_forward_object_rules();
        installed_list_view_lifetimes();
        structured_diagnostics();
        explicit_model_identity();
        snapshots_and_lifetimes();
        lists_and_composition();
        global_copy();
        explicit_snapshot_copy_and_owner_destruction();
    } catch (...) {
        std::pmr::set_default_resource(previous);
        throw;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Runtime expression binding and snapshot contracts passed\n";
}
