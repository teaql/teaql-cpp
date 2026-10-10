#include <teaql/core/debug.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Item {
    using allocator_type = Allocator;
    String text;

    Item(std::allocator_arg_t, Allocator a, std::string_view text) : text(text, a.resource()) {}

    Item(const Item&) = delete;

    Item(std::allocator_arg_t, Allocator a, Item&& other)
        : text(std::move(other.text), a.resource()) {}

    Item(Item&&) noexcept = default;
    Item& operator=(Item&&) = delete;
};

static void ownership_and_callbacks() {
    GlobalContext global;
    Ref<String> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto& list = user->make<SmartList<Item>>();
        list.emplace_back("first");
        list.emplace_back(String(4096, 's', owner));
        auto before = user->allocation_stats().issued_objects;
        unsigned called = 0;
        DebugOptions options;
        options.pretty = true;
        auto text = debug_string(
            owner,
            list,
            [&](DebugWriter& writer, const Item& item) {
                ++called;
                writer.structure("Item", [&](auto& fields) {
                    fields.field("text", [&](auto& out) {
                        out.text(item.text);
                    });
                });
            },
            options);
        check(
            called == 2 && text.find("Item {\n            text: \"first\"") != String::npos &&
                text.get_allocator().resource() == owner && list.data[0].text == "first" &&
                user->allocation_stats().issued_objects == before,
            "Debug borrows non-copyable/non-assignable items and allocates owner-local payload only");
        retained = user->make_global<String>(text);
    }
    auto next = global.make_user_context();
    auto& text = next->get(next->rebind_global(retained));
    check(text.find(String(4096, 's', global.memory_resource())) != String::npos &&
              text.get_allocator().resource() == global.memory_resource(),
          "explicit Global diagnostics survive actual User destruction");
}

static void generic_writing_and_limits() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    String text(owner);
    DebugOptions options;
    options.pretty = true;
    DebugWriter writer(text, options);
    writer.structure("Empty", [](auto&) {});
    writer.raw(" ");
    writer.tuple("", [](auto& out) {
        out.raw("raw\ncontinued");
    });
    check(text == "Empty (\n    raw\n    continued,\n)",
          "custom multiline output inherits structural indentation without owning callbacks");
    auto& integers = user->make<SmartList<int>>();
    integers.push(42);
    check(debug_string(owner, integers).find("data: [42]") != String::npos &&
              debug_string(owner, "literal") == "\"literal\"" &&
              debug_string(owner, std::int64_t(-42)) == "-42",
          "default generic element diagnostics support primitive scalars and literal text");
    options.fill = 0xd800;
    bool invalid = false;
    try {
        (void)debug_string(owner, Value(owner), options);
    } catch (const std::invalid_argument&) {
        invalid = true;
    }
    check(invalid, "invalid Unicode formatting options are rejected");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        ownership_and_callbacks();
        generic_writing_and_limits();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
