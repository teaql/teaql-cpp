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

static void row_metadata_and_retention() {
    GlobalContext global;
    Ref<CompactRow> retained;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    {
        const std::array schema{DynamicPropertyDefinitions::Field{"_computed", DataType::Text}};
        auto& properties = user_context->make<DynamicPropertyDefinitions>(
            std::move(DynamicPropertyDefinitions::from_fields(r, schema)).value());
        auto& names = user_context->make<Vector<String>>();
        names.emplace_back("id");
        names.emplace_back("version");
        names.emplace_back("_computed");
        auto& relations = user_context->make<Vector<String>>();
        relations.emplace_back("children");
        auto& columns = user_context->make<ColumnLayout>(std::move(names), relations, &properties);
        auto& values = user_context->make<Vector<Value>>();
        values.emplace_back(std::uint64_t(7));
        values.emplace_back(std::int64_t(3));
        values.emplace_back("computed-value");
        auto& row = user_context->make<CompactRow>(columns, std::move(values));
        const std::array indexes{FieldLayout::Index{"id", 0}, FieldLayout::Index{"version", 1}};
        const std::array mappings{FieldLayout::Mapping{"id", "id", "id"},
                                  FieldLayout::Mapping{"version", "version", "version"}};
        const std::array<std::string_view, 2> expected{"id", "version"};
        const std::array<std::string_view, 1> edges{"children"};
        auto& first = user_context->make<FieldLayout>(
            std::move(
                FieldLayout::from_generated(r, "First", "v1", indexes, mappings, edges, expected))
                .value());
        auto& second = user_context->make<FieldLayout>(
            std::move(
                FieldLayout::from_generated(r, "Second", "v1", indexes, mappings, edges, expected))
                .value());
        auto& pool = user_context->make<SnapshotPool>();
        auto before = debug_string(r, row);
        check(before.find("primary: OnceLock(<uninit>)") != String::npos &&
                  before.find("DynamicPropertyDefinitions") != String::npos,
              "uncached row diagnostics include readonly schema without initializing availability");
        (void)row.indexed_load_state(pool, first);
        (void)row.indexed_load_state(pool, second);
        auto& children = user_context->make<SmartList<CompactRow>>();
        row.set_loaded_relation("children", children);
        const std::array codes{DynamicFieldDefinitions::Field{"extension", DataType::Text}};
        auto& definitions = user_context->make<DynamicFieldDefinitions>(
            std::move(DynamicFieldDefinitions::from_fields(r, "First", "v1", codes)).value());
        Record dynamic(r);
        dynamic.emplace(String("extension", r), Value(r, "dynamic-value"));
        auto fields = std::move(DynamicFieldValues::from_values(r, definitions, dynamic)).value();
        row.set_loaded_dynamic_fields(fields);
        auto snapshot = row.indexed_load_state(pool, first).indexed_snapshot()->identity();
        auto objects = user_context->allocation_stats().issued_objects;
        auto output = debug_string(r, row, DebugOptions{.pretty = true});
        check(
            output.find("First") != String::npos && output.find("Second") != String::npos &&
                output.find("LoadedRelationResults") != String::npos &&
                output.find("children") != String::npos &&
                output.find("dynamic-value") != String::npos &&
                output.find("computed-value") != String::npos &&
                row.indexed_load_state(pool, first).indexed_snapshot()->identity() == snapshot &&
                user_context->allocation_stats().issued_objects == objects,
            "diagnostics expose real cached geometry and independent sidecars without managed allocations");
        retained = user_context->make_global<CompactRow>(row);
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    const auto& row = next_user_context->get(next_user_context->rebind_global(retained));
    auto output = debug_string(next_user_context->memory_resource(), row);
    check(
        output.find("dynamic-value") != String::npos && output.find("_computed") != String::npos &&
            output.find("children") != String::npos &&
            row.shared_layout()->memory_resource() == global.memory_resource(),
        "retained row diagnostics use copied Global layout/schema/read metadata after source deletion");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        ownership_and_callbacks();
        generic_writing_and_limits();
        row_metadata_and_retention();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
