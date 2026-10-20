#include "../../examples/generated/include/cpp_examples.hpp"
#include <iostream>

using namespace teaql;

static void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto next_user_context = global.make_user_context();
        {
            auto resource = user_context->memory_resource();
            auto& entity = user_context->make<cpp_examples::Product>();
            require(!entity.is_loaded("name") && !entity.changedName(), "Unexpected initial state");
            GeneratedSampleValues::unconverted_value(
                entity, "name", Value(resource, std::int64_t(8)));
            require(entity.is_loaded("name") && !entity.is_loaded("version"),
                    "The original operand must mark only its source field loaded");
            auto changed = entity.changedName();
            require(changed && changed->try_i64() == 8,
                    "The incompatible numeric operand must remain I64");
            require(entity.name().empty(), "An incompatible operand must not become display text");
            auto& independent = next_user_context->make<cpp_examples::Product>();
            bool rejected = false;
            try {
                GeneratedSampleValues::unconverted_value(
                    independent, "name", Value(resource, std::int64_t(9)));
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            require(rejected && !independent.changedName() && !independent.is_loaded("name"),
                    "A foreign operand must be rejected before mutation");
            auto copied = Value(next_user_context->memory_resource(), std::int64_t(9));
            GeneratedSampleValues::unconverted_value(independent, "name", std::move(copied));
            require(independent.changedName()->try_i64() == 9,
                    "An explicitly scoped operand must be accepted");
        }
        next_user_context.reset();
        user_context.reset();
        require(global.live_user_bytes() == 0, "Original sample operands retained User memory");
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
