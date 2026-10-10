#include <teaql/runtime/context.hpp>
#include <teaql/core/list.hpp>
#include <iostream>
#include <sstream>
#include "../../tests/support/smart_list_drop_fixture.hpp"

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct MoveOnly {
    using allocator_type = Allocator;
    String text;

    MoveOnly(std::allocator_arg_t, Allocator a, std::string_view value)
        : text(value, a.resource()) {}

    MoveOnly(std::allocator_arg_t, Allocator a, MoveOnly&& other) : text(a.resource()) {
        if (a.resource() != other.memory_resource()) {
            throw std::invalid_argument("move-only payload cannot change owner");
        }
        text = std::move(other.text);
    }

    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) noexcept = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;

    Memory memory_resource() const noexcept {
        return text.get_allocator().resource();
    }

    friend bool operator<(const MoveOnly& left, const MoveOnly& right) {
        return left.text < right.text;
    }
};

static_assert(!detail::domain_copyable<MoveOnly>);
static_assert(detail::domain_copyable<Value> && detail::domain_copyable<CompactRow>);

struct NoEquality {
    int value;
};

struct EqualityOnly {
    int value;
    bool operator==(const EqualityOnly&) const = default;
};

static_assert(!std::equality_comparable<SmartList<NoEquality>>);
static_assert(std::equality_comparable<SmartList<EqualityOnly>>);
static_assert(std::equality_comparable<SmartList<bool>>);
static_assert(std::equality_comparable<SmartList<float>>);
static_assert(std::equality_comparable<SmartList<Value>>);
static_assert(!detail::domain_copyable<SmartList<MoveOnly>>);
static_assert(detail::domain_copyable<SmartList<Value>>);

struct ConstructOnly : MoveOnly {
    using MoveOnly::MoveOnly;

    ConstructOnly(std::allocator_arg_t, Allocator a, ConstructOnly&& other)
        : MoveOnly(std::allocator_arg, a, static_cast<MoveOnly&&>(other)) {}

    ConstructOnly(ConstructOnly&&) noexcept = default;
    ConstructOnly(const ConstructOnly&) = delete;
    ConstructOnly& operator=(ConstructOnly&&) = delete;
};

static_assert(!std::is_move_assignable_v<ConstructOnly>);

static void construct_only_elements() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& list = user->make<SmartList<ConstructOnly>>();
    list.emplace_back("first");
    list.emplace_back("discard");
    list.emplace_back("third");
    auto prior = list.set(0, ConstructOnly(std::allocator_arg, Allocator(r), "replacement"));
    check(prior && prior->text == "first" && list[0].text == "replacement",
          "set needs construction, not an entity assignment operator");
    auto& incoming = user->make<Vector<ConstructOnly>>();
    incoming.emplace_back("third");
    incoming.emplace_back("fourth");
    list.merge_by(std::move(incoming), [r](const ConstructOnly& item) {
        return String(item.text, r);
    });
    unsigned called = 0;
    list.retain([&](const ConstructOnly& item) {
        ++called;
        return item.text != "discard";
    });
    check(called == 4 && list.len() == 3 && list[1].text == "third" && list[2].text == "fourth",
          "stable retain and keyed replacement support non-assignable elements");
}

struct MoveOnlyKey {
    int value;

    explicit MoveOnlyKey(int value) : value(value) {}

    MoveOnlyKey(const MoveOnlyKey&) = delete;
    MoveOnlyKey(MoveOnlyKey&&) = default;
    MoveOnlyKey& operator=(MoveOnlyKey&&) = default;
    auto operator<=>(const MoveOnlyKey&) const = default;
};

template <>
struct std::hash<MoveOnlyKey> {
    std::size_t operator()(const MoveOnlyKey& key) const noexcept {
        return std::hash<int>{}(key.value);
    }
};

static void input_ranges_and_move_only_keys() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    std::istringstream input("1 2 1");
    auto collected = SmartList<int>::from_range(r, std::ranges::istream_view<int>(input));
    check(collected.is_loaded && collected.len() == 3 && collected[2] == 1,
          "single-pass FromIterator input needs neither a size nor a second traversal");
    std::istringstream extension("3 4");
    collected.extend(std::ranges::istream_view<int>(extension));
    auto identities = collected.identity_map([](const int& item) {
        return MoveOnlyKey(item % 2);
    });
    auto groups = collected.group_by([](const int& item) {
        return MoveOnlyKey(item % 2);
    });
    check(identities.at(MoveOnlyKey(1)) == 3 && groups.at(MoveOnlyKey(1)).size() == 3,
          "ordered grouping and identity keys need Ord but not Clone");
    std::istringstream merges("7 8 9");
    collected.merge_by(std::ranges::istream_view<int>(merges), [](const int& item) {
        return MoveOnlyKey(item % 2);
    });
    check(collected.len() == 5 && collected[3] == 9 && collected[4] == 8,
          "single-pass keyed merge supports Eq/Hash keys without Clone");
}

static void consuming_merge_order() {
    GlobalContext global;
    auto user = global.make_user_context();
    for (bool fail : {false, true}) {
        auto& list = user->make<SmartList<int>>();
        list.emplace_back(1);
        list.emplace_back(2);
        Vector<int> trace(user->memory_resource());
        auto incoming = std::views::iota(0, 3) | std::views::transform([&](int index) {
                            int value = index == 0 ? 12 : index == 1 ? 3 : 14;
                            trace.push_back(value);
                            if (fail && value == 3) {
                                throw std::runtime_error("incoming iterator failed");
                            }
                            return value;
                        });
        bool failed = false;
        try {
            // This named view yields owned values, as Rust IntoIterator does.
            list.merge_by(incoming, [&](const int& item) {
                trace.push_back(-item);
                return item % 10;
            });
        } catch (const std::runtime_error&) {
            failed = true;
        }
        check(failed == fail, "incoming iterator failures propagate");
        const int complete[] = {-1, -2, 12, -12, 3, -3, 14, -14};
        const int interrupted[] = {-1, -2, 12, -12, 3};
        auto expected = fail ? std::span<const int>(interrupted) : std::span<const int>(complete);
        check(std::ranges::equal(trace, expected),
              "consuming iteration and merge key callbacks retain source order");
        check(list[0] == 1 && list[1] == 12 && list.len() == (fail ? 2 : 4),
              "a failed incoming iterator preserves the already applied merge prefix");
    }
}

static void iterator_and_filter_failures() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto& list = user->make<SmartList<int>>();
    list.emplace_back(1);
    list.emplace_back(2);
    auto incoming = std::views::iota(0, 3) | std::views::transform([](int index) {
                        if (index == 1) {
                            throw std::runtime_error("incoming iterator failed");
                        }
                        return 12;
                    });
    bool failed = false;
    try {
        list.extend(incoming);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    check(failed && list.len() == 3 && list[2] == 12,
          "owned extension retains items yielded before an iterator failure");
    auto& constructed = user->make<SmartList<ConstructOnly>>();
    for (const auto* item : {"1", "2", "3", "4", "5"}) {
        constructed.emplace_back(item);
    }
    unsigned calls = 0;
    failed = false;
    try {
        constructed.retain([&](const ConstructOnly& item) {
            ++calls;
            if (item.text == "4") {
                throw std::runtime_error("filter failed");
            }
            return item.text != "2";
        });
    } catch (const std::runtime_error&) {
        failed = true;
    }
    check(failed && calls == 4 && constructed.len() == 4 && constructed[0].text == "1" &&
              constructed[1].text == "3" && constructed[2].text == "4" &&
              constructed[3].text == "5",
          "filter failure keeps the visited survivors and untouched tail without assignment");
}

static void retain_cleanup_order() {
    GlobalContext global;
    auto user = global.make_user_context();
    Vector<int> events(user->memory_resource());
    events.reserve(32);
    {
        SmartList<list_fixture::DropItem> list(std::allocator_arg,
                                               Allocator(user->memory_resource()));
        for (int item : {1, 2, 3, 4, 5}) {
            list.emplace_back(item, events);
        }
        bool failed = false;
        try {
            list.retain([&](const auto& item) {
                events.push_back(item.value);
                if (item.value == 4) {
                    throw std::runtime_error("filter failed");
                }
                return item.value % 2 != 0;
            });
        } catch (const std::runtime_error&) {
            failed = true;
        }
        const int expected[] = {1, 2, -2, 3, 4};
        check(failed && std::ranges::equal(events, expected) && list.len() == 4 &&
                  list[0].value == 1 && list[1].value == 3 && list[2].value == 4 &&
                  list[3].value == 5,
              "erased ownership releases before the next filter and unwind preserves the tail");
        for (const auto& item : list) {
            check(item.owns && item.payload.size() == 4096 &&
                      item.payload.get_allocator().resource() == user->memory_resource(),
                  "survivor payloads retain their owner without cloning or assignment");
        }
    }
    const int completed[] = {1, 2, -2, 3, 4, -1, -3, -4, -5};
    check(std::ranges::equal(events, completed),
          "every original owned payload releases exactly once after failed retention");
}

static void move_only_operations() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& input = user->make<Vector<MoveOnly>>();
    input.emplace_back("first");
    auto* original = input.data();
    auto& list = user->make<SmartList<MoveOnly>>(std::move(input));
    check(list.is_loaded && input.empty() && list.data.data() == original,
          "owned vector construction requires no clone and transfers storage");
    auto produced = std::views::iota(0, 2) | std::views::transform([r](int index) {
                        return MoveOnly(std::allocator_arg,
                                        Allocator(r),
                                        index == 0 ? "produced first" : "produced second");
                    });
    auto& produced_list = user->make<SmartList<MoveOnly>>();
    produced_list.extend(produced);
    check(produced_list.len() == 2 && produced_list[0].text == "produced first" &&
              produced_list[1].text == "produced second",
          "a named value-producing extension imposes no Clone requirement");
    list.push(MoveOnly(std::allocator_arg, Allocator(r), "second"));
    auto& more = user->make<Vector<MoveOnly>>();
    more.emplace_back("third");
    more.emplace_back("fourth");
    list.extend(std::move(more));
    check(list.len() == 4 && list[3].text == "fourth",
          "owned range extension supports move-only objects");
    auto previous = list.set(1, MoveOnly(std::allocator_arg, Allocator(r), "replacement"));
    check(previous && previous->text == "second" && list[1].text == "replacement",
          "set returns the previous object without Clone");
    MoveOnly ignored(std::allocator_arg, Allocator(r), "unused");
    check(!list.set(99, std::move(ignored)) && ignored.text == "unused",
          "invalid set does not consume C++ argument");
    unsigned count = 0;
    for (auto& value : list.iter_mut()) {
        value.text += '!';
        ++count;
    }
    check(count == 4 && list.iter()[0].text == "first!", "explicit const/mutable iteration");
    auto& range = user->make<Vector<MoveOnly>>();
    range.emplace_back("from range");
    auto collected = SmartList<MoveOnly>::from_range(r, std::move(range));
    check(collected.is_loaded && collected.len() == 1 && collected[0].text == "from range",
          "FromIterator-style construction supports move-only owned input");
    auto& transferred = user->make<SmartList<MoveOnly>>(std::move(list));
    check(transferred.len() == 4 && list.is_empty(),
          "allocator-aware same-owner list movement requires no Clone");
    auto& assigned = user->make<SmartList<MoveOnly>>();
    assigned = std::move(transferred);
    check(assigned.len() == 4 && transferred.is_empty(),
          "same-owner list assignment supports move-only items");
    auto copied_outputs = assigned.to_list([r](const MoveOnly& item) {
        return MoveOnly(std::allocator_arg, Allocator(r), item.text);
    });
    auto ordered_outputs = assigned.to_set([r](const MoveOnly& item) {
        return MoveOnly(std::allocator_arg, Allocator(r), item.text);
    });
    check(copied_outputs.size() == 4 && ordered_outputs.size() == 4,
          "to_list/to_set do not require Clone for mapper-produced values");
    auto& merge_input = user->make<Vector<MoveOnly>>();
    merge_input.emplace_back("replacement!");
    merge_input.emplace_back("new!");
    merge_input.emplace_back("new!");
    assigned.merge_by(std::move(merge_input), [r](const MoveOnly& item) {
        return String(item.text, r);
    });
    check(assigned.len() == 5 && assigned[4].text == "new!",
          "owned arbitrary merge input requires no Clone and collapses incoming duplicate keys");
    bool foreign_list_rejected = false;
    try {
        (void)SmartList<MoveOnly>(
            std::allocator_arg, Allocator(global.memory_resource()), std::move(assigned));
    } catch (const std::invalid_argument&) {
        foreign_list_rejected = true;
    }
    check(foreign_list_rejected && assigned.len() == 5 && assigned[0].text == "first!",
          "foreign move-only list movement is rejected before consuming the source");
    auto mapped = std::move(assigned).map([](MoveOnly&& value) {
        return std::move(value);
    });
    check(mapped.is_loaded && mapped.len() == 5 && assigned.is_empty(),
          "move-only map preserves loaded state");
    auto moved = std::move(mapped).into_vec();
    check(mapped.is_empty() && moved[1].text == "replacement!",
          "move-only consuming vector extraction");
    auto& foreign = user->make<Vector<MoveOnly>>();
    foreign.emplace_back("still owned by User");
    bool rejected = false;
    try {
        (void)SmartList<MoveOnly>(
            std::allocator_arg, Allocator(global.memory_resource()), std::move(foreign));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && foreign[0].text == "still owned by User",
          "foreign move-only vector is rejected without consumption");
}

static void range_aliasing_and_global_retention() {
    GlobalContext global;
    Ref<SmartList<Value>> retained;
    {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& input = user->make<Vector<Value>>();
        input.emplace_back(String(4096, 'a', r));
        input.emplace_back(String(4096, 'b', r));
        auto& list = user->make<SmartList<Value>>(std::move(input));
        list.with_total_count(19).with_summary("long caption", String(4096, 's', r));
        auto reversed = list.data | std::views::reverse;
        list.extend(reversed);
        check(list.len() == 4 && list[2].try_text()->front() == 'b' &&
                  list[3].try_text()->front() == 'a' && list.total_count == 19 && list.is_loaded,
              "borrowed aliased range snapshots before vector growth without changing metadata");
        auto& facet = user->make<SmartList<CompactRow>>();
        facet.emplace_back().insert("nested", Value(r, String(4096, 'f', r)));
        facet.loaded().with_total_count(123).with_aggregation("count", 2);
        auto& child = user->make<SmartList<CompactRow>>();
        child.emplace_back().insert("child", Value(r, String(4096, 'c', r)));
        child.with_summary("caption", String(4096, 'm', r));
        facet.with_facet("child", child);
        list.with_facet("retained facet", facet);
        list.with_aggregation("payload", String(4096, 'p', r));
        retained = user->make_global<SmartList<Value>>(list);
        auto& owned = user->get(retained);
        check(owned.data.get_allocator().resource() == global.memory_resource() &&
                  std::get<String>(owned[0].data).get_allocator().resource() ==
                      global.memory_resource(),
              "complete copied list data uses destination allocator");
        auto& empty_input = user->make<Vector<Value>>();
        auto& empty = user->make<SmartList<Value>>(std::move(empty_input));
        check(empty.is_loaded && empty.is_empty(), "owned empty vector means loaded empty result");
        auto& unloaded = user->make<SmartList<Value>>();
        check(!unloaded.is_loaded, "default empty list remains unloaded");
    }
    auto next = global.make_user_context();
    const auto& list = next->get(next->rebind_global(retained));
    check(list.is_loaded && list.total_count == 19 && list[2].try_text()->size() == 4096 &&
              list.summary_value("long caption")->try_text()->size() == 4096,
          "range-created data and metadata survive actual source User deletion");
    const auto* facet = list.facet("retained facet");
    const auto* child = facet ? facet->facet("child") : nullptr;
    check(facet && facet->is_loaded && facet->total_count == 123 && child &&
              child->summary_value("caption")->try_text()->size() == 4096 &&
              child->data[0].get("child")->try_text()->size() == 4096 &&
              list.aggregation("payload")->try_text()->size() == 4096 &&
              child->data[0].shared_columns()->memory_resource() == global.memory_resource(),
          "recursive facets, immutable layouts and all metadata are retained in Global");
}

class FailingUpstream final : public std::pmr::memory_resource {
public:
    bool fail = false;
    std::size_t bytes = 0;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        if (fail) {
            throw std::bad_alloc{};
        }
        auto* result = std::pmr::new_delete_resource()->allocate(size, alignment);
        bytes += size;
        return result;
    }

    void do_deallocate(void* value, std::size_t size, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(value, size, alignment);
        bytes -= size;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void foreign_constructor_failure() {
    FailingUpstream upstream;
    {
        GlobalContext global(&upstream);
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto& input = user->make<Vector<Value>>();
        input.emplace_back(String(131072, 'x', r));
        upstream.fail = true;
        bool failed = false;
        try {
            (void)SmartList<Value>(
                std::allocator_arg, Allocator(global.memory_resource()), std::move(input));
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        upstream.fail = false;
        check(failed && input.size() == 1 && input[0].try_text()->size() == 131072,
              "failed foreign vector construction preserves all source items");
        auto list = SmartList<Value>(
            std::allocator_arg, Allocator(global.memory_resource()), std::move(input));
        check(input.empty() && list.is_loaded,
              "successful retry consumes only after complete copy");
        user.reset();
        check(list[0].try_text()->size() == 131072 &&
                  list[0].memory_resource() == global.memory_resource(),
              "foreign vector construction retains no User payload");
    }
    check(upstream.bytes == 0, "all successful and failed list blocks reclaimed");
}

static void boolean_storage_contracts() {
    static_assert(std::ranges::contiguous_range<SmartList<bool>::storage_type>);
    static_assert(std::same_as<decltype(std::declval<SmartList<bool>&>()[0]), bool&>);
    FailingUpstream upstream;
    {
        GlobalContext global(&upstream);
        Ref<SmartList<bool>> retained;
        {
            auto user = global.make_user_context();
            auto owner = user->memory_resource();
            auto& list = user->make<SmartList<bool>>();
            check(list.as_slice().empty() && !list.get(0) && !list.first() && !list.last(),
                  "empty Boolean borrows have no proxy elements");
            auto* first = &list.emplace_back(true);
            check(first == list.get_mut(0) && first == list.as_mut_slice().data(),
                  "Boolean pointers, elements and mutable slices borrow the same real object");
            *first = false;
            list.push(true);
            list.extend(list.as_slice());
            check(list.len() == 4 && !list[0] && list[1] && !list[2] && list[3],
                  "Boolean self-extension snapshots before growing contiguous storage");
            auto previous = list.set(1, false);
            check(previous == true && !list[1] && !list.set(99, true),
                  "Boolean replacement retains optional bounds and prior scalar value");
            auto pointer = list.data.data();
            auto capacity = list.data.capacity();
            upstream.fail = true;
            bool failed = false;
            try {
                list.data.reserve(131072);
            } catch (const std::bad_alloc&) {
                failed = true;
            }
            upstream.fail = false;
            check(failed && list.data.data() == pointer && list.data.capacity() == capacity &&
                      list.len() == 4 && list[3],
                  "failed Boolean growth preserves the old allocation and values");
            auto values = std::move(list).into_vec();
            check(list.is_empty() && values.data() == pointer,
                  "consuming Boolean extraction transfers the original storage");
            auto& restored = user->make<SmartList<bool>>(std::move(values));
            check(restored.is_loaded && values.empty() && restored.data.data() == pointer,
                  "Boolean owned construction transfers same-domain storage without copying");
            auto cloned = detail::clone_in(owner, restored);
            check(cloned == restored && cloned.data.data() != restored.data.data(),
                  "scoped Boolean clones have independent payloads");
            Vector<bool> packed(owner);
            packed.push_back(true);
            packed.push_back(false);
            auto& unpacked = user->make<SmartList<bool>>(std::move(packed));
            check(packed.empty() && unpacked.is_loaded && *unpacked.get(0) && !unpacked[1],
                  "standard packed C++ input converts into real Boolean objects");
            restored.with_total_count(17).with_summary("caption", String(4096, 'b', owner));
            retained = user->make_global<SmartList<bool>>(restored);
            auto& saved = user->get(retained);
            check(saved.data.get_allocator().resource() == global.memory_resource() &&
                      saved.data.data() != restored.data.data(),
                  "foreign Boolean retention explicitly copies into Global");
        }
        auto next = global.make_user_context();
        const auto& saved = next->get(next->rebind_global(retained));
        check(saved.len() == 4 && !saved[0] && saved[3] && saved.total_count == 17 &&
                  saved.summary_value("caption")->try_text()->size() == 4096,
              "Boolean data and nested metadata survive actual source User destruction");
    }
    check(upstream.bytes == 0, "all Boolean and failed-growth arena blocks return");
}

static void consumed_mapping_cleanup() {
    GlobalContext global;
    auto user = global.make_user_context();
    for (bool borrowed_parameter : {false, true}) {
        Vector<int> events(user->memory_resource());
        events.reserve(32);
        SmartList<list_fixture::DropItem> list(std::allocator_arg,
                                               Allocator(user->memory_resource()));
        for (int item : {1, 2, 3, 4, 5}) {
            list.emplace_back(item, events);
        }
        auto callback = [&](const list_fixture::DropItem& item) {
            events.push_back(item.value);
            if (item.value == 3) {
                throw std::runtime_error("mapping failed");
            }
            return item.value;
        };
        bool failed = false;
        try {
            if (borrowed_parameter) {
                (void)std::move(list).map([&](list_fixture::DropItem&& item) {
                    return callback(item);
                });
            } else {
                (void)std::move(list).map([&](list_fixture::DropItem item) {
                    return callback(item);
                });
            }
        } catch (const std::runtime_error&) {
            failed = true;
        }
        const int expected[] = {1, -1, 2, -2, 3, -3, -4, -5};
        check(failed && list.is_empty() && std::ranges::equal(events, expected),
              "consumed mapping releases processed and unvisited ownership before unwinding");
    }
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        move_only_operations();
        construct_only_elements();
        input_ranges_and_move_only_keys();
        consuming_merge_order();
        iterator_and_filter_failures();
        retain_cleanup_order();
        range_aliasing_and_global_retention();
        foreign_constructor_failure();
        boolean_storage_contracts();
        consumed_mapping_cleanup();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
