#include "../../examples/generated/include/cpp_examples.hpp"
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void scoped_readonly_copy() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    Ref<cpp_examples::Product> retained;
    {
        Record fields(r);
        fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
        fields.emplace(String("name", r), Value(r, "original"));
        fields.emplace(String("version", r), Value(r, std::int64_t(3)));
        fields.emplace(String("_caption", r), Value(r, String(4096, 'x', r)));
        fields.emplace(String("_empty", r), Value(r));
        fields.emplace(String("_zero", r), Value(r, std::int64_t(0)));
        fields.emplace(String("_false", r), Value(r, false));
        auto row = CompactRow(std::allocator_arg, Allocator(r), std::move(fields));
        auto result = cpp_examples::Product::from_compact_row(r, std::move(row));
        check(bool(result), "decode readonly projection into generated model");
        auto reference = user_context->make_ref<cpp_examples::Product>(std::move(result).value());
        auto& entity = user_context->get(reference);
        retained = user_context->make_global<cpp_examples::Product>(entity);
        auto& copy = user_context->get(retained);
        check(entity.hasDynamicProperty("_empty") && !entity.dynamicProperty("_empty"),
              "NULL projection is present but has no readable value");
        check(entity.dynamicProperty("_zero")->try_i64() == 0 &&
                  entity.dynamicProperty("_false")->try_bool() == false,
              "zero and false remain readable");
        check(!entity.hasDynamicProperty("name") && !entity.dynamicProperty("name"),
              "native columns do not become readonly properties");
        check(copy.dynamicProperty("_caption")->memory_resource() == global.memory_resource(),
              "explicit Global copy owns readonly value storage");
        entity.dynamic.insert_or_assign(String("_caption", r), Value(r, "changed"));
        check(copy.dynamicProperty("_caption")->try_text()->size() == 4096,
              "copied readonly map remains independent");
        check(!entity.hasPendingDynamicMutations() && !copy.hasPendingDynamicMutations(),
              "readonly access and retention do not create durable mutations");
    }
    user_context.reset();
    auto next_user_context = global.make_user_context();
    auto rebound = next_user_context->rebind_global(retained);
    auto& copy = next_user_context->get(rebound);
    check(copy.dynamicProperty("_caption")->try_text()->size() == 4096 &&
              copy.hasDynamicProperty("_empty"),
          "Global copy survives source Context destruction and explicit rebinding");
    next_user_context.reset();
    std::pmr::set_default_resource(previous);
}

int main() {
    scoped_readonly_copy();
    std::cout << "generated readonly property lifecycle contracts passed\n";
}
