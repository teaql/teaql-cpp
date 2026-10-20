#include "../../tests/support/dynamic_json_runtime_fixture.hpp"
#include <teaql/runtime/detail/data_service/relation_installation.hpp>
#include <iostream>
#include <limits>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

static CompactRow grouped_row(Memory r, std::uint64_t owner, std::uint64_t customer) {
    Record member(r);
    member.emplace("id", Value(r, customer));
    member.emplace("version", Value(r, std::int64_t(1)));
    member.emplace("name", Value(r, "isolated customer projection"));
    Value::List people(r);
    people.emplace_back(member);
    Record platform(r);
    platform.emplace("id", Value(r, std::uint64_t(1)));
    platform.emplace("version", Value(r, std::int64_t(1)));
    platform.emplace("name", Value(r, "shared platform identity"));
    platform.emplace("person_list", Value(r, people));
    member.emplace("platform_id", Value(r, std::uint64_t(1)));
    member.emplace("platform", Value(r, platform));
    CompactRow row(std::allocator_arg, Allocator(r));
    row.insert("id", Value(r, owner));
    row.insert("version", Value(r, std::int64_t(1)));
    row.insert("customer_id", Value(r, customer));
    row.insert("customer", Value(r, member));
    return row;
}

static void null_facets(UserContext& user_context) {
    auto r = user_context.memory_resource();
    CompactRow row(std::allocator_arg, Allocator(r));
    row.insert("id", Value(r, std::uint64_t(10)));
    row.insert("version", Value(r, std::int64_t(1)));
    row.insert("customer_id", Value(r, std::uint64_t(2)));
    row.insert("customer", Value(r));
    SmartList<CompactRow> loaded(std::allocator_arg, Allocator(r));
    CompactRow facet_row(std::allocator_arg, Allocator(r));
    facet_row.insert("id", Value(r, std::uint64_t(3)));
    SmartList<CompactRow> values(std::allocator_arg, Allocator(r));
    values.data.emplace_back(std::move(facet_row));
    loaded.facets.emplace("remaining_customers", std::move(values));
    row.set_loaded_relation("customer", loaded);
    Vector<CompactRow> rows(r);
    rows.emplace_back(std::move(row));
    auto result = detail::attach_flat_relation_graph(user_context, "Order", rows);
    check(bool(result), "NULL forward row is accepted");
    auto* facet = result.value().relation_facet("Order", 10, "customer", "remaining_customers");
    check(facet && facet->data.size() == 1 && facet->data[0].get("id")->try_u64() == 3,
          "NULL forward objects retain loaded facet metadata");
    check(rows[0].get("customer")->is_null() && !rows[0].is_loaded_relation("customer"),
          "NULL forward alias retains its source availability boundary");
}

static void repeated_install(UserContext& user_context) {
    auto r = user_context.memory_resource();
    EntityRuntimeState root(std::allocator_arg, Allocator(r));
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    Vector<CompactRow> first(r), second(r);
    first.emplace_back(grouped_row(r, 10, 2));
    second.emplace_back(grouped_row(r, 11, 3));
    check(bool(detail::attach_flat_relation_graph(user_context, "Order", first, root, graph)),
          "First installation succeeds");
    check(bool(detail::attach_flat_relation_graph(user_context, "Order", second, root, graph)),
          "Second installation succeeds");
    check(bool(root.freeze_graph(std::move(graph))), "Composed graph freezes once");
    for (auto [owner, expected] : {std::pair<std::uint64_t, std::uint64_t>{10, 2}, {11, 3}}) {
        const auto* customer =
            root.resolve_relation_option<cpp_dynamic_json::Person>("Order", owner, "customer");
        check(customer && *customer && (*customer)->platform(),
              "Each owner retains its customer and platform");
        const auto& list = (*customer)->platform()->personList();
        check(list.data.size() == 1 && list.data[0].id() == expected,
              "Repeated installations preserve isolated views of a shared identity");
    }
}

static void reserved_views(UserContext& user_context) {
    auto r = user_context.memory_resource();
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    SmartList<cpp_dynamic_json::Person> list(std::allocator_arg, Allocator(r));
    graph.install_json_relation_list(100, "Platform", 1, "person_list", std::move(list));
    check(graph.allocate_read_view() > 100, "Existing relation views reserve their namespace");
    EntityGraphBuilder copied(std::allocator_arg, Allocator(r), graph);
    auto copied_view = copied.allocate_read_view();
    check(copied_view > 100, "Builder copies preserve the allocated namespace");
    EntityGraphBuilder moved(std::move(copied));
    check(moved.allocate_read_view() > copied_view, "Moves preserve allocation continuity");
    moved.reserve_read_view(std::numeric_limits<std::uint64_t>::max());
    bool exhausted = false;
    try {
        (void)moved.allocate_read_view();
    } catch (const std::overflow_error&) {
        exhausted = true;
    }
    check(exhausted, "Exhaustion cannot wrap or reuse a view id");
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        teaql::test::initialize_dynamic_json_database(*user_context);
        if (argc == 1 || std::string_view(argv[1]) == "null") {
            null_facets(*user_context);
        }
        if (argc == 1 || std::string_view(argv[1]) == "repeat") {
            repeated_install(*user_context);
        }
        if (argc == 1 || std::string_view(argv[1]) == "reserve") {
            reserved_views(*user_context);
        }
        user_context.reset();
        check(global.live_user_bytes() == 0, "Boundary checks reclaim all User storage");
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
