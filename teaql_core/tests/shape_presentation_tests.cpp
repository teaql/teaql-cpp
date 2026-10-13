#include <array>
#include <iostream>
#include <memory_resource>
#include <optional>
#include <type_traits>
#ifdef TEAQL_USE_MODULES
import teaql_runtime;
#else
#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#endif

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void context_layouts() {
    static_assert(std::same_as<CompactRowLayout, ColumnLayout>);
    static_assert(std::same_as<core::CompactRowLayout, CompactRowLayout>);
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto source_user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto source_r = source_user_context->memory_resource();
    const std::array<DynamicPropertyDefinitions::Field, 1> fields{{{"_total", DataType::I64}}};
    auto result = DynamicPropertyDefinitions::from_fields(source_r, fields);
    check(static_cast<bool>(result), "source schema creation failed");
    auto& definitions =
        source_user_context->make<DynamicPropertyDefinitions>(std::move(result.value()));
    auto& base = user_context->make<CompactRowLayout>(
        std::initializer_list<std::string_view>{"id", "_total"});
    const auto& shaped = base.with_dynamic_property_definitions(definitions);
    check(&shaped != &base && !(shaped == base), "schema changes layout identity and equality");
    check(shaped.dynamic_property_definitions()->memory_resource() == r &&
              &shaped.with_dynamic_property_definitions(definitions) == &shaped,
          "foreign schema must copy into the layout Context; equal schemas reuse the layout");
    auto retained = user_context->make_global<CompactRowLayout>(shaped);
    source_user_context.reset();
    check(shaped.dynamic_property_definitions()->data_type("_total") == DataType::I64,
          "schema source deletion must not invalidate the explicit destination copy");
    {
        auto& traversal = user_context->make<EntityJsonTraversal>();
        String transient(128, 'x', r);
        check(traversal.enter(transient, 0), "fresh typed address must expand");
        transient.assign(128, 'y');
        check(!traversal.enter(String(128, 'x', r), 0), "traversal retains entity-name values");
        traversal.leave();
        check(traversal.enter("Other", 0), "different types at the same address must expand");
        traversal.leave();
        traversal.leave();
        check(traversal.enter(String(128, 'x', r), 0),
              "a sibling visit must not be rejected by a global visited set");
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    const auto& global_copy = next_user_context->get(next_user_context->rebind_global(retained));
    check(global_copy.memory_resource() == global.memory_resource() &&
              global_copy.dynamic_property_definitions()->memory_resource() ==
                  global.memory_resource() &&
              global_copy.names().size() == 2,
          "global retention requires a deep layout and schema copy");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        context_layouts();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Core shape/presentation Context contracts passed\n";
}
