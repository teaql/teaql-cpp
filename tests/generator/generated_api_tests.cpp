#include "../../examples/generated/include/cpp_examples.hpp"
#include "../generated_types/include/cpp_types.hpp"
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class F>
static void rejects(F&& action, const char* message) {
    bool rejected = false;
    try {
        action();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, message);
}

static void indexed_model_loading() {
    using namespace cpp_examples;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& fresh = user_context->make<Product>();
    check(fresh.load_state.kind() == eval::LoadState::Kind::Indexed && !fresh.is_loaded("id") &&
              !fresh.is_loaded("name") && !fresh.is_loaded("version"),
          "fresh generated models have no loaded fields");
    check(E::product(fresh).getName().result().is_not_loaded(),
          "a default scalar does not conceal a missing load");
    check(fresh.evalName().is_not_loaded() && !fresh.changedName(),
          "model evaluation and changed helpers preserve missing load and absent mutation");
    check(fresh.loaded_state_snapshot() && fresh.loaded_state_snapshot()->get().bits() == 0,
          "generated entities expose their immutable Indexed snapshot through the core hook");
    fresh.updateName("");
    fresh.set_comment("only this entity");
    fresh.root.set_comment("receiving request reason");
    check(fresh.get_comment() == "only this entity" &&
              fresh.root.get_comment() == "receiving request reason",
          "generated entity annotations remain separate from the receiving request reason");
    check(fresh.is_loaded("name") && !fresh.is_loaded("version") &&
              E::product(fresh).getName().unwrap().empty(),
          "setters make explicit empty values available without widening other fields");
    check(fresh.changedName()->try_text()->empty() && fresh.evalName().get().empty(),
          "explicit empty updates remain values in both model helper surfaces");
    check(&Product::field_layout(r) ==
                  &Product::field_layout(next_user_context->memory_resource()) &&
              Product::field_layout(r).memory_resource() == global.memory_resource(),
          "generated type metadata is shared in the application Global Context");
    auto& columns =
        user_context->make<ColumnLayout>(std::initializer_list<std::string_view>{"id", "name"});
    Vector<Value> first_values(r), second_values(r);
    first_values.emplace_back(std::uint64_t{7});
    first_values.emplace_back("first");
    second_values.emplace_back(std::uint64_t{7});
    second_values.emplace_back("second");
    auto first = Product::from_compact_row(
        r, CompactRow(std::allocator_arg, Allocator(r), columns, std::move(first_values)));
    auto second = Product::from_compact_row(
        r, CompactRow(std::allocator_arg, Allocator(r), columns, std::move(second_values)));
    check(first && second && first.value().is_loaded("name") &&
              !first.value().is_loaded("version") &&
              first.value().load_state.indexed_snapshot()->identity() ==
                  second.value().load_state.indexed_snapshot()->identity(),
          "generated hydration shares only the immutable actual projection");
    auto& alias = user_context->make<Product>(first.value());
    first.value().updateVersion(0).updateName("changed");
    check(alias.name() == "changed" && alias.changedName()->try_text() == "changed" &&
              alias.evalName().get() == "changed" &&
              alias.original_values()->fields.at(String("name", r)).try_text() == "first",
          "required getters observe shared changes while originals stay immutable");
    check(!second.value().is_loaded("version") && second.value().name() == "second",
          "same-identity model views keep independent availability and values");
    auto& retained = next_user_context->make<Product>(second.value());
    check(&retained.load_state.indexed_snapshot()->get().layout() == &Product::field_layout(r),
          "explicit model copies preserve canonical Global layout identity");
    auto& person = user_context->make<cpp_types::Person>();
    check(person.evalActive().is_not_loaded() && person.evalNickname().is_not_loaded(),
          "false and optional defaults are not implicitly loaded");
    person.updateActive(false).updateNickname(std::nullopt);
    check(person.evalActive().is_value() && !person.evalActive().get() &&
              person.evalNickname().is_null() && person.changedNickname()->is_null(),
          "false and explicit optional NULL preserve distinct evaluation and mutation values");
}

class SparseProductChecker final : public TypedChecker<cpp_examples::Product> {
public:
    using allocator_type = Allocator;

    SparseProductChecker(std::allocator_arg_t, Allocator a, bool fix)
        : resource_(a.resource()), fix_(fix) {}

    Memory memory_resource() const {
        return resource_;
    }

    void check_and_fix_typed(const UserContext& user_context,
                             cpp_examples::Product& product,
                             CheckObjectStatus,
                             const ObjectLocation&,
                             CheckResults&) const override {
        check(product.memory_resource() == user_context.memory_resource() &&
                  product.is_loaded("id") && !product.is_loaded("name"),
              "typed Checker retains the actual sparse Indexed projection");
        check(product.evalName().is_not_loaded(), "Checker defaults do not imply loaded name");
        if (fix_) {
            check(product.changedVersion()->try_i64() == 1,
                  "dirty fields reconstruct mutation-aware access");
            product.updateName("fixed by typed checker");
        } else {
            check(!detail::EntityRuntimeAccess::has_mutation_context(product.root),
                  "readonly reconstruction leaves the mutation ledger uninitialized");
        }
    }

private:
    Memory resource_;
    bool fix_;
};

static void sparse_indexed_checker_reconstruction() {
    using namespace cpp_examples;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& readonly = user_context->make<SparseProductChecker>(false);
    auto& fix = user_context->make<SparseProductChecker>(true);
    auto& readonly_adapter = user_context->make<TypedEntityChecker<Product, SparseProductChecker>>(
        *user_context, readonly);
    auto& fixing_adapter =
        user_context->make<TypedEntityChecker<Product, SparseProductChecker>>(*user_context, fix);
    auto make_values = [&](bool dirty) {
        EntityValues values(std::allocator_arg, Allocator(r));
        values.fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
        values.fields.emplace(String("version", r), Value(r, std::int64_t{1}));
        Value::List loaded(r);
        loaded.emplace_back("id");
        loaded.emplace_back("version");
        values.fields.emplace(String("_loaded_fields", r), Value(r, std::move(loaded)));
        if (dirty) {
            Value::List fields(r);
            fields.emplace_back("version");
            values.fields.emplace(String("_dirty_fields", r), Value(r, std::move(fields)));
        }
        return values;
    };
    ObjectLocation location(std::allocator_arg, Allocator(r));
    CheckResults results(r);
    auto values = make_values(false);
    readonly_adapter.check_and_fix(*user_context, values, location, results);
    check(!values.fields.contains("name") && results.size() == 1 &&
              results[0].rule == CheckRule::Required && results[0].location.model_path(r) == "name",
          "readonly Checker preserves omission and reports the source-required nonnullable field");
    values = make_values(true);
    results.clear();
    fixing_adapter.check_and_fix(*user_context, values, location, results);
    const auto& dirty = std::get<Value::List>(values.fields.at(String("_dirty_fields", r)).data);
    check(results.empty() &&
              values.fields.at(String("name", r)).try_text() == "fixed by typed checker" &&
              !values.fields.contains("_loaded_fields") && dirty.size() == 2 &&
              dirty[0].try_text() == "name" && dirty[1].try_text() == "version",
          "typed fixes merge changed values and regenerate actual dirty metadata");
}

static void scalar_types_intent_and_copies() {
    using namespace cpp_types;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto executor_ref = setup(*user_context);
    auto r = user_context->memory_resource();
    auto Q = user_context->queries<query_factory>();
    auto& person = Q.people()
                       .comment("Create a person with typed fields")
                       .purpose("Prepare typed scalar fixture")
                       .newEntity(*user_context);
    person.updateEmail("long.person@example.test")
        .updateActive(true)
        .updateNickname(std::nullopt)
        .updateAge(20)
        .updateBalance(*Decimal::parse("12.50"))
        .updateHeight(1.75)
        .updateRatio(0.5f)
        .updateBirthday(*Date::from_ymd(2024, 2, 29))
        .updateUpdatedAt(Timestamp(12345));
    {
        auto json = Json::parse(r, R"({"items":["long nested context-owned payload"]})");
        check(json.has_value(), "JSON fixture");
        std::optional<Json> payload(std::in_place, std::allocator_arg, Allocator(r), *json);
        person.updatePayload(payload);
    }
    auto id = person.id();
    require_value(AuditedSaveExt<Person>::save(
                      std::move(person).audit_as("Create the typed person"), *user_context)
                      .wait());
    auto request = Q.people()
                       .comment("Typed human predicate, boolean and numeric range")
                       .whoseEmailContains("@example.test")
                       .whoAreActive()
                       .whoseAgeBetween(18, 30)
                       .purpose("Inspect typed fields")
                       .orderByIdAsc();
    auto& rows = require_value(request.executeForEntityList(*user_context).wait());
    check(rows.len() == 1 && rows[0].id() == id && rows[0].age() == 20 && rows[0].active(),
          "typed field predicates run through SQLite");
    check(rows[0].email().get_allocator().resource() == r && rows[0].payload() &&
              rows[0].payload()->memory_resource() == r,
          "generated scalar and nested allocations belong to UserContext");
    check(E::person(rows[0]).getNickname().isNull(), "loaded optional NULL is legitimate Null");
    check(E::person(rows[0]).getBalance().unwrap() == *Decimal::parse("12.50"),
          "Decimal typed expression");
    const auto& one = require_value(request.execute(*user_context).wait());
    check(one && one->id() == id, "single-row execution is pollable and typed");
    {
        auto& executor = user_context->get(executor_ref);
        auto* descriptor = executor.schema_provider.get_entity("Person");
        check(descriptor != nullptr, "generated metadata resolves the stream fixture");
        auto& compiled = user_context->make<CompiledQuery>(
            executor.dialect.compile_select(r, *descriptor, request.query()));
        auto& stream = executor.transport.stream_sql(r, std::move(compiled), 1);
        check(stream.wait_next() != nullptr, "native cursor holds the shared connection");
        auto& pending = request.execute(*user_context);
        check(!pending.poll(), "generated single result waits behind a live native cursor");
        pending.cancel();
        stream.cancel();
        check(require_value(request.execute(*user_context).wait()).has_value(),
              "generated cancellation joins its child and leaves the connection usable");
    }
    auto empty =
        Q.people().whoseIdIs(id + 100).purpose("Find absent entity").comment("Missing identity");
    check(!require_value(empty.execute(*user_context).wait()), "absent single result is None");
    auto missing_comment = Q.people().purpose("No comment");
    rejects(
        [&] {
            missing_comment.executeForList(*user_context);
        },
        "missing comment fails closed");
    auto empty_purpose = Q.people().comment("Comment present").purpose(" \t ");
    rejects(
        [&] {
            empty_purpose.executeForList(*user_context);
        },
        "blank purpose fails closed");
    auto unicode_comment =
        Q.people().purpose("Unicode whitespace check").comment("\xc2\xa0\xe3\x80\x80");
    rejects(
        [&] {
            unicode_comment.executeForList(*user_context);
        },
        "Rust Unicode whitespace is not a meaningful comment");
    auto aggregates = Q.peopleMinimal()
                          .groupByAge()
                          .sumHeightAs("total_height")
                          .comment("Grouped generated projection")
                          .purpose("Inspect aggregate rows");
    const auto& aggregate_rows = require_value(aggregates.executeForRows(*user_context).wait());
    check(aggregate_rows.len() == 1 && aggregate_rows[0].get("total_height"),
          "generated grouping and aggregates execute through the runtime rows facade");
    auto other = global.make_user_context();
    rejects(
        [&] {
            request.executeForList(*other);
        },
        "query cannot silently cross Contexts");
    auto partial = Q.peopleMinimal().purpose("Inspect load state").comment("Only identity loaded");
    auto& partial_rows = require_value(partial.executeForList(*user_context).wait());
    rejects(
        [&] {
            E::person(partial_rows[0]).getEmail().orDefaultIfNull();
        },
        "NotLoaded cannot be hidden by a scalar fallback");
    auto& retained = global.make<Person>(rows[0]);
    user_context.reset();
    check(retained.email() == "long.person@example.test" &&
              retained.email().get_allocator().resource() == global.memory_resource() &&
              retained.payload() &&
              retained.payload()->memory_resource() == global.memory_resource(),
          "explicit generated model copy survives User destruction");
}

static void relation_subqueries_and_load_state() {
    using namespace cpp_examples;
    GlobalContext global;
    auto user_context = global.make_user_context();
    setup(*user_context);
    auto Q = user_context->queries<query_factory>();
    auto& product =
        Q.products().comment("Create product").purpose("Prepare graph").newEntity(*user_context);
    product.updateName("keyboard");
    auto product_id = product.id();
    require_value(AuditedSaveExt<Product>::save(std::move(product).audit_as("Create the product"),
                                                *user_context)
                      .wait());
    auto& order =
        Q.orders().comment("Create order").purpose("Prepare graph").newEntity(*user_context);
    order.updateName("typed-order");
    auto& line =
        Q.orderLines().comment("Create child").purpose("Prepare graph").newEntity(*user_context);
    line.updateName("typed-line").updateOrderId(order.id()).updateProductId(product_id);
    order.lines().data.emplace_back(std::move(line));
    order.lines().loaded();
    require_value(AuditedSaveExt<Order>::save(
                      std::move(order).audit_as("Create the complete graph"), *user_context)
                      .wait());
    auto products = Q.products().withNameIs("keyboard");
    auto lines = Q.orderLines().withProduct(products);
    auto request = Q.orders()
                       .withLines(lines)
                       .selectLinesWith(lines)
                       .purpose("Query related graph")
                       .comment("Two subquery levels");
    auto& rows = require_value(request.executeForList(*user_context).wait());
    check(rows.len() == 1 && E::order(rows[0]).getLines().size().unwrap() == 1,
          "generated reverse and forward relation subqueries use descriptor keys");
    rejects(
        [&] {
            E::order(rows[0]).getLines().first().getProduct().unwrap();
        },
        "filtering a relation does not implicitly load it");
    lines.selectProductWith(products);
    auto next_user_context = global.make_user_context();
    auto next_Q = next_user_context->queries<query_factory>();
    auto foreign_products = next_Q.products();
    rejects(
        [&] {
            lines.selectProductWith(foreign_products);
        },
        "generated relation adapters reject foreign construction contexts");
    auto copied_products = foreign_products.copyTo(*user_context);
    lines.selectProductWith(copied_products);
    auto loaded_request = Q.orders()
                              .withLines(lines)
                              .selectLinesWith(lines)
                              .purpose("Read explicitly loaded product")
                              .comment("Fresh nested selection");
    auto& enhanced = require_value(loaded_request.executeForList(*user_context).wait());
    check(enhanced.len() == 1, "enhanced generated query retains its root");
    check(enhanced[0].lines().len() == 1, "enhanced generated query retains its child");
    check(enhanced[0].lines()[0].product() != nullptr,
          "explicit generated product relation must be present");
    check(E::order(enhanced[0]).getLines().first().getProduct().getName().unwrap() == "keyboard",
          "nested generated expression traverses an explicitly loaded relation");
    check(E::order(enhanced[0]).getLines().get(9).isNull(), "out-of-range list item is Null");
    auto& retained = global.make<Order>(enhanced[0]);
    user_context.reset();
    check(E::order(retained).getLines().first().getProduct().getName().unwrap() == "keyboard",
          "nested relation copies survive actual UserContext destruction");
}

static void recursive_runtime_attachment() {
    using namespace cpp_examples;
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& product = user_context->make<Product>();
    product.updateId(41).updateName("attached product");
    product.mark_as_new();
    auto& line = user_context->make<OrderLine>();
    line.updateId(42).updateProduct(product);
    auto& target = user_context->make<EntityRuntimeState>();
    line.attachRuntimeStateRecursive(target);
    check(target.get(product.entity_key(), "name")->try_text() == "attached product" &&
              target.is_new(product.entity_key()),
          "recursive attachment adopts a stored forward object's pending values and new key");
    target.set(product.entity_key(), "name", Value(r, "shared update"));
    check(line.product() && line.product()->name() == "shared update",
          "stored forward objects retain the receiving ledger after recursive attachment");
    auto& retained = global.make<OrderLine>(line);
    user_context.reset();
    check(retained.product() && retained.product()->name() == "shared update" &&
              retained.product()->memory_resource() == global.memory_resource(),
          "recursive attached objects survive only through an explicit destination copy");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scalar_types_intent_and_copies();
        indexed_model_loading();
        sparse_indexed_checker_reconstruction();
        relation_subqueries_and_load_state();
        recursive_runtime_attachment();
        std::pmr::set_default_resource(previous);
        std::cout << "Generated C++ API contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
