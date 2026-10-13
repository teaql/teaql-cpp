#include <teaql/core/load_state.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>

using namespace teaql;
using Index = FieldLayout::Index;
using Mapping = FieldLayout::Mapping;
static constexpr std::array indexes{Index{"id", 0}, Index{"version", 1}, Index{"code", 2}};
static constexpr std::array mappings{Mapping{"id", "id", "db_id"},
                                     Mapping{"version", "version", "db_version"},
                                     Mapping{"code", "code", "db_code"}};
static constexpr std::array<std::string_view, 3> expected{"id", "version", "code"};
static constexpr std::array<std::string_view, 1> relations{"lines"};

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void valid_layout_and_retention() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    std::string entity(256, 'E'), revision(256, 'R');
    std::optional<Ref<FieldLayout>> retained;
    {
        auto result = FieldLayout::from_generated(user_context->memory_resource(),
                                                  entity,
                                                  revision,
                                                  indexes,
                                                  mappings,
                                                  relations,
                                                  expected);
        check(result.is_ok(), "valid generated layout");
        const auto& layout = result.value();
        entity[0] = 'x';
        revision[0] = 'x';
        check(layout.entity().front() == 'E' && layout.revision().front() == 'R',
              "layout owns incoming text in its Context");
        check(layout.index("db_code") == 2 && layout.index("id") == 0 &&
                  layout.index("db_version") == 1 && !layout.index("unknown") &&
                  !layout.index("#custom") && !layout.index("_derived"),
              "aliases preserve generated indexes independently of access order");
        check(layout.field_count() == 3 && layout.is_relation("lines") &&
                  !layout.is_relation("code"),
              "fixed and relation metadata stay separate");
        check(layout.shared_entity_name().data() == layout.entity().data(),
              "shared entity identity borrows immutable owner bytes");
        auto reference = user_context->make_ref<FieldLayout>(layout);
        retained = user_context->make_global<FieldLayout>(user_context->get(reference));
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    auto rebound = next_user_context->rebind_global(*retained);
    const auto& copy = next_user_context->get(rebound);
    check(copy.entity().front() == 'E' && copy.revision().front() == 'R' &&
              copy.index("db_code") == 2 && copy.is_relation("lines") &&
              copy.memory_resource() == global.memory_resource(),
          "explicit delegated Global copy retains complete metadata after User deletion");
}

static void malformed_layouts() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto rejects = [&](std::string_view entity,
                       std::string_view revision,
                       std::span<const Index> fields,
                       std::span<const Mapping> aliases,
                       std::span<const std::string_view> members,
                       std::string_view error) {
        auto result =
            FieldLayout::from_generated(r, entity, revision, fields, aliases, relations, members);
        check(result.is_err() && result.error() == error,
              "malformed model metadata must report the precise source validation failure");
        check(result.error().get_allocator().resource() == r,
              "validation error belongs to Context");
    };
    rejects("", "v1", indexes, mappings, expected, "missing entity or field-layout revision");
    rejects("Item", "", indexes, mappings, expected, "missing entity or field-layout revision");
    auto fields = indexes;
    fields[2] = {"#custom", 2};
    rejects("Item", "v1", fields, mappings, expected, "invalid fixed field/index: #custom=2");
    fields[2] = {"_derived", 2};
    rejects("Item", "v1", fields, mappings, expected, "invalid fixed field/index: _derived=2");
    fields[2] = {"code", 3};
    rejects("Item", "v1", fields, mappings, expected, "invalid fixed field/index: code=3");
    fields[2] = {"code", 1};
    rejects("Item", "v1", fields, mappings, expected, "duplicate fixed field/index: code=1");
    fields[2] = {"id", 2};
    rejects("Item", "v1", fields, mappings, expected, "duplicate fixed field/index: id=2");
    fields[1] = {"code", 1};
    fields[2] = {"other", 2};
    rejects(
        "Item", "v1", fields, mappings, expected, "generated layout must include id and version");
    auto aliases = mappings;
    aliases[2] = {"absent", "code", "db_code"};
    rejects("Item", "v1", indexes, aliases, expected, "mapping has no fixed field: absent");
    aliases[2] = {"code", "id", "db_code"};
    rejects("Item", "v1", indexes, aliases, expected, "ambiguous fixed-field alias: id");
    aliases[2] = {"code", "code", ""};
    rejects("Item", "v1", indexes, aliases, expected, "blank alias for code");
    aliases[2] = aliases[0];
    rejects("Item", "v1", indexes, aliases, expected, "duplicate field mapping: id");
    rejects("Item",
            "v1",
            indexes,
            std::span(mappings).first(2),
            expected,
            "incomplete fixed-field mappings");
    aliases = mappings;
    aliases[2] = {"db_id", "code", "db_code"};
    rejects("Item",
            "v1",
            indexes,
            aliases,
            expected,
            "mapping names an alias, not a canonical field: db_id");
    auto members = expected;
    members[2] = "absent";
    rejects("Item",
            "v1",
            indexes,
            mappings,
            members,
            "generated layout disagrees with typed entity fields");
    members[2] = "db_id";
    rejects("Item",
            "v1",
            indexes,
            mappings,
            members,
            "generated layout disagrees with typed entity fields");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        valid_layout_and_retention();
        malformed_layouts();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "FieldLayout Context and metadata contracts passed\n";
}
