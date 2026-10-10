#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;
namespace req = teaql::request;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct NonClone {
    int value;

    explicit NonClone(int value) : value(value) {}

    NonClone(const NonClone&) = delete;

    NonClone(NonClone&& other) noexcept : value(std::exchange(other.value, -1)) {}

    bool operator==(const NonClone&) const = default;
};

static void write_debug(DebugWriter& writer, const NonClone& value) {
    writer.tuple("NonClone", [&](auto& out) {
        out.integer(value.value);
    });
}

using OwnedRange = req::DateRange<NonClone>;
static_assert(!detail::domain_copyable<OwnedRange>);
static_assert(
    !std::is_constructible_v<OwnedRange, std::allocator_arg_t, Allocator, const OwnedRange&>);
static_assert(detail::domain_copyable<req::DateRange<Value>>);

struct OwnedId {
    Value id;
    OwnedId(const OwnedId&) = delete;
    OwnedId(OwnedId&&) = default;

    explicit OwnedId(Value id) : id(std::move(id)) {}

    Value entity_id_value() && {
        return std::move(id);
    }
};

struct BorrowedId {
    Memory owner;

    Value entity_id_value() & {
        return Value(owner, std::uint64_t(7));
    }
};

static_assert(req::EntityReference<OwnedId>);
static_assert(!req::EntityReference<OwnedId&>);
static_assert(!req::EntityReference<const OwnedId&>);
static_assert(req::EntityReference<BorrowedId&>);
static_assert(!req::EntityReference<BorrowedId>);
static_assert(req::EntityReference<Value>);
static_assert(req::EntityReference<std::uint64_t>);

static void entity_reference_categories() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    OwnedId owned(Value(owner, String(4096, 'i', owner)));
    auto pointer = owned.id.try_text()->data();
    auto id = req::entity_id_value(owner, std::move(owned));
    check(id.try_text()->data() == pointer && id.try_text()->size() == 4096,
          "rvalue-only non-Clone entity reference transfers its scoped ID");
    BorrowedId borrowed{owner};
    check(req::entity_id_value(owner, borrowed).try_u64() == 7,
          "an explicitly supported borrowed receiver invokes its lvalue implementation");
}

static void consuming_range_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    NonClone first(1), second(2);
    auto& range = user->make<OwnedRange>(std::move(first), std::move(second));
    check(first.value == -1 && second.value == -1 && range.start.value == 1 && range.end.value == 2,
          "owned endpoint construction consumes non-copyable scalar values");
    auto& moved = user->make<OwnedRange>(std::move(range));
    check(range.start.value == -1 && range.end.value == -1 && moved.start.value == 1 &&
              moved.end.value == 2,
          "same-domain allocator-aware range movement needs no endpoint copy");
    try {
        (void)user->make_global<OwnedRange>(std::move(moved));
        throw std::runtime_error("Foreign non-copyable range retention was accepted");
    } catch (const std::invalid_argument&) {
    }
    check(moved.start.value == 1 && moved.end.value == 2,
          "rejected foreign retention does not consume any source endpoint");
    check(debug_string(user->memory_resource(), moved) ==
              "DateRange { start: NonClone(1), end: NonClone(2) }",
          "generic diagnostics borrow non-copyable endpoints without a Clone bound");
    auto& equal = global.make<req::DateRange<int>>(1, 2);
    auto& local = user->make<req::DateRange<int>>(1, 2);
    check(equal == local, "allocation owner is not part of source DateRange equality");
    Vector<NonClone> starts(user->memory_resource()), ends(user->memory_resource());
    starts.emplace_back(3);
    ends.emplace_back(4);
    auto& vectors =
        user->make<req::DateRange<Vector<NonClone>>>(std::move(starts), std::move(ends));
    check(debug_string(user->memory_resource(), vectors) ==
              "DateRange { start: [NonClone(3)], end: [NonClone(4)] }",
          "generic range construction and diagnostics support non-copyable vector endpoints");
}

static void retained_range_contracts() {
    GlobalContext global;
    Ref<req::DateRange<Value>> retained;
    const char* local_pointer = nullptr;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        Value first(owner, String(8192, 'a', owner));
        Value second(owner, String(8192, 'b', owner));
        local_pointer = std::get<String>(first.data).data();
        auto& range = user->make<req::DateRange<Value>>(std::move(first), std::move(second));
        check(std::get<String>(range.start.data).data() == local_pointer,
              "same-owner endpoint construction transfers nested character storage");
        retained = user->make_global<req::DateRange<Value>>(range);
        check(std::get<String>(user->get(retained).start.data).data() != local_pointer,
              "foreign range retention prepares independent payload storage");
    }
    check(global.live_user_bytes() == 0, "actual User destruction returns source range blocks");
    auto next = global.make_user_context();
    const auto& range = next->get(next->rebind_global(retained));
    auto owner = global.memory_resource();
    check(range.memory_resource() == owner && range.start.memory_resource() == owner &&
              std::get<String>(range.start.data).get_allocator().resource() == owner &&
              std::get<String>(range.end.data).get_allocator().resource() == owner &&
              range.start.try_text()->size() == 8192 && range.end.try_text()->back() == 'b',
          "both retained endpoints and all nested strings remain in Global");
}

static void retained_request_contracts() {
    GlobalContext global;
    Ref<req::QuerySelection> retained;
    Ref<String> output;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& outer = user->make<SelectQuery>("Order");
        outer.filter(Expr::eq(owner, "private", "complete diagnostic"));
        auto& selection = user->make<req::QuerySelection>(outer);
        auto& child_query = user->make<SelectQuery>("Child");
        child_query.projects({"id", "id"});
        auto& child = user->make<req::QuerySelection>(child_query);
        selection.relation_selections.emplace_back("children", child);
        selection.relation_filters.emplace_back("filtered", child);
        selection.child_enhancements.emplace_back(child);
        auto& options = selection.query_options;
        options.comment.emplace("", owner);
        options.raw_sql.emplace("raw sensitive SQL", owner);
        options.dynamic_properties.emplace_back(
            "computed", req::UnsafeRawSqlSegment::trusted(owner, "secret expression"));
        options.relation_aggregates.emplace_back("children", "amount", child, false);
        options.object_group_bys.emplace_back("group", "group_id", child);
        options.facets.emplace_back("status", "children", child, true);
        auto before = user->allocation_stats().issued_objects;
        String text(owner);
        core::DebugWriter writer(text, {.pretty = true});
        core::request::write_debug(writer, selection);
        check(text.find("RelationFilter") != String::npos &&
                  text.find("secret expression") != String::npos,
              "complete public builder diagnostics retain filter metadata and raw trusted SQL");
        check(user->allocation_stats().issued_objects == before,
              "request diagnostic scratch uses payload storage only");
        retained = user->make_global<req::QuerySelection>(selection);
        output = user->make_global<String>(text);
    }
    auto next = global.make_user_context();
    const auto& selection = next->get(next->rebind_global(retained));
    auto owner = global.memory_resource();
    check(selection.memory_resource() == owner &&
              selection.relation_filters[0].memory_resource() == owner &&
              selection.query_options.facets[0].query.memory_resource() == owner &&
              selection.query_options.dynamic_properties[0]
                      .raw_sql_segment.get_allocator()
                      .resource() == owner,
          "retained request metadata and every nested builder domain remain Global");
    check(debug_string(next->memory_resource(), selection, {.pretty = true}) ==
              next->get(next->rebind_global(output)),
          "complete builder output survives actual source User deletion");
    auto converted =
        req::QuerySelection(std::allocator_arg, Allocator(next->memory_resource()), selection)
            .into_query();
    check(converted.relations.size() == 1 && converted.child_enhancements.size() == 1 &&
              converted.object_group_bys.size() == 1 &&
              converted.raw_sql_text == "raw sensitive SQL" && converted.comment_text &&
              converted.comment_text->empty(),
          "retained builder still converts through the original independent core query layer");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        entity_reference_categories();
        consuming_range_contracts();
        retained_range_contracts();
        retained_request_contracts();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
