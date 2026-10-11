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

static void scalar_types_intent_and_copies() {
    using namespace cpp_types;
    GlobalContext global;
    auto user = global.make_user_context();
    auto executor_ref = setup(*user);
    auto r = user->memory_resource();
    auto& person = Q::people(*user)
                       .comment("Create a person with typed fields")
                       .purpose("Prepare typed scalar fixture")
                       .new_entity(*user);
    person.update_email("long.person@example.test")
        .update_active(true)
        .update_nickname(std::nullopt)
        .update_age(20)
        .update_balance(*Decimal::parse("12.50"))
        .update_height(1.75)
        .update_ratio(0.5f)
        .update_birthday(*Date::from_ymd(2024, 2, 29))
        .update_updated_at(Timestamp(12345));
    {
        auto json = Json::parse(r, R"({"items":["long nested context-owned payload"]})");
        check(json.has_value(), "JSON fixture");
        std::optional<Json> payload(std::in_place, std::allocator_arg, Allocator(r), *json);
        person.update_payload(payload);
    }
    auto id = person.id();
    require_value(
        AuditedSaveExt<Person>::save(std::move(person).audit_as("Create the typed person"), *user)
            .wait());
    auto& request = Q::people(*user)
                        .comment("Typed human predicate, boolean and numeric range")
                        .whose_email_contains("@example.test")
                        .who_are_active()
                        .whose_age_between(18, 30)
                        .purpose("Inspect typed fields")
                        .order_by_id_asc();
    auto& rows = require_value(request.execute_for_entity_list(*user).wait());
    check(rows.len() == 1 && rows[0].id() == id && rows[0].age() == 20 && rows[0].active(),
          "typed field predicates run through SQLite");
    check(rows[0].email().get_allocator().resource() == r && rows[0].payload() &&
              rows[0].payload()->memory_resource() == r,
          "generated scalar and nested allocations belong to UserContext");
    check(E::person(rows[0]).get_nickname().is_null(), "loaded optional NULL is legitimate Null");
    check(E::person(rows[0]).get_balance().unwrap() == *Decimal::parse("12.50"),
          "Decimal typed expression");
    const auto& one = require_value(request.execute(*user).wait());
    check(one && one->id() == id, "single-row execution is pollable and typed");
    {
        auto& executor = user->get(executor_ref);
        auto* descriptor = executor.schema_provider.get_entity("Person");
        check(descriptor != nullptr, "generated metadata resolves the stream fixture");
        auto& compiled = user->make<CompiledQuery>(
            executor.dialect.compile_select(r, *descriptor, request.query()));
        auto& stream = executor.transport.stream_sql(r, std::move(compiled), 1);
        check(stream.wait_next() != nullptr, "native cursor holds the shared connection");
        auto& pending = request.execute(*user);
        check(!pending.poll(), "generated single result waits behind a live native cursor");
        pending.cancel();
        stream.cancel();
        check(require_value(request.execute(*user).wait()).has_value(),
              "generated cancellation joins its child and leaves the connection usable");
    }
    auto& empty = Q::people(*user)
                      .whose_id_is(id + 100)
                      .purpose("Find absent entity")
                      .comment("Missing identity");
    check(!require_value(empty.execute(*user).wait()), "absent single result is None");
    auto& missing_comment = Q::people(*user).purpose("No comment");
    rejects(
        [&] {
            missing_comment.execute_for_list(*user);
        },
        "missing comment fails closed");
    auto& empty_purpose = Q::people(*user).comment("Comment present").purpose(" \t ");
    rejects(
        [&] {
            empty_purpose.execute_for_list(*user);
        },
        "blank purpose fails closed");
    auto& unicode_comment =
        Q::people(*user).purpose("Unicode whitespace check").comment("\xc2\xa0\xe3\x80\x80");
    rejects(
        [&] {
            unicode_comment.execute_for_list(*user);
        },
        "Rust Unicode whitespace is not a meaningful comment");
    auto other = global.make_user_context();
    rejects(
        [&] {
            request.execute_for_list(*other);
        },
        "query cannot silently cross Contexts");
    auto& partial =
        Q::people_minimal(*user).purpose("Inspect load state").comment("Only identity loaded");
    auto& partial_rows = require_value(partial.execute_for_list(*user).wait());
    rejects(
        [&] {
            E::person(partial_rows[0]).get_email().or_default_if_null();
        },
        "NotLoaded cannot be hidden by a scalar fallback");
    auto& retained = global.make<Person>(rows[0]);
    user.reset();
    check(retained.email() == "long.person@example.test" &&
              retained.email().get_allocator().resource() == global.memory_resource() &&
              retained.payload() &&
              retained.payload()->memory_resource() == global.memory_resource(),
          "explicit generated model copy survives User destruction");
}

static void relation_subqueries_and_load_state() {
    using namespace cpp_examples;
    GlobalContext global;
    auto user = global.make_user_context();
    setup(*user);
    auto& product =
        Q::products(*user).comment("Create product").purpose("Prepare graph").new_entity(*user);
    product.update_name("keyboard");
    auto product_id = product.id();
    require_value(
        AuditedSaveExt<Product>::save(std::move(product).audit_as("Create the product"), *user)
            .wait());
    auto& order =
        Q::orders(*user).comment("Create order").purpose("Prepare graph").new_entity(*user);
    order.update_name("typed-order");
    auto& line =
        Q::order_lines(*user).comment("Create child").purpose("Prepare graph").new_entity(*user);
    line.update_name("typed-line").update_order_id(order.id()).update_product_id(product_id);
    order.lines().data.emplace_back(std::move(line));
    order.lines().loaded();
    require_value(
        AuditedSaveExt<Order>::save(std::move(order).audit_as("Create the complete graph"), *user)
            .wait());
    auto& products = Q::products(*user).with_name_is("keyboard");
    auto& lines = Q::order_lines(*user).with_product(products);
    auto& request = Q::orders(*user)
                        .with_lines(lines)
                        .select_lines_with(lines)
                        .purpose("Query related graph")
                        .comment("Two subquery levels");
    auto& rows = require_value(request.execute_for_list(*user).wait());
    check(rows.len() == 1 && E::order(rows[0]).get_lines().size().unwrap() == 1,
          "generated reverse and forward relation subqueries use descriptor keys");
    rejects(
        [&] {
            E::order(rows[0]).get_lines().first().get_product().unwrap();
        },
        "filtering a relation does not implicitly load it");
    lines.select_product_with(products);
    auto& loaded_request = Q::orders(*user)
                               .with_lines(lines)
                               .select_lines_with(lines)
                               .purpose("Read explicitly loaded product")
                               .comment("Fresh nested selection");
    auto& enhanced = require_value(loaded_request.execute_for_list(*user).wait());
    check(enhanced.len() == 1, "enhanced generated query retains its root");
    check(enhanced[0].lines().len() == 1, "enhanced generated query retains its child");
    check(enhanced[0].lines()[0].product() != nullptr,
          "explicit generated product relation must be present");
    check(E::order(enhanced[0]).get_lines().first().get_product().get_name().unwrap() == "keyboard",
          "nested generated expression traverses an explicitly loaded relation");
    check(E::order(enhanced[0]).get_lines().get(9).is_null(), "out-of-range list item is Null");
    auto& retained = global.make<Order>(enhanced[0]);
    user.reset();
    check(E::order(retained).get_lines().first().get_product().get_name().unwrap() == "keyboard",
          "nested relation copies survive actual UserContext destruction");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scalar_types_intent_and_copies();
        relation_subqueries_and_load_state();
        std::pmr::set_default_resource(previous);
        std::cout << "Generated C++ API contracts passed\n";
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
