#include "../generated_checker_clock_scalars/include/cpp_checker_clock_scalars.hpp"
#include <iostream>
#include <optional>
#include <type_traits>
using namespace cpp_checker_clock_scalars;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static auto request_from_helper(UserContext& user_context) {
    auto Q = user_context.queries<query_factory>();
    return Q.notesMinimal().selectLong().unselectLong().selectLong().selectFloat().selectDouble();
}

static void scalar_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto destination = global.make_user_context();
    std::optional<NoteExpression> expired_entity;
    std::optional<RuntimeExpression<std::int64_t>> expired_scalar;
    auto retained = [&] {
        auto request = request_from_helper(*user_context);
        auto copied = request.copyTo(*destination);
        bool rejected = false;
        try {
            request.comment("Inspect scalar aliases")
                .purpose("Reject a foreign Context")
                .executeForList(*destination);
        } catch (const ReferenceError&) {
            rejected = true;
        }
        check(rejected,
              "Canonical scalar query aliases must preserve the original Context binding");
        auto& note = user_context->make<Note>();
        note.updateModelLong(42).updateModelFloat(1.25f).updateModelDouble(2.5);
        auto expression = E::note(note);
        static_assert(
            std::is_same_v<decltype(expression.getLong()), RuntimeExpression<std::int64_t>>);
        static_assert(std::is_same_v<decltype(expression.getFloat()), RuntimeExpression<float>>);
        static_assert(std::is_same_v<decltype(expression.getDouble()), RuntimeExpression<double>>);
        check(expression.getLong().unwrap() == 42 && expression.getFloat().unwrap() == 1.25f &&
                  expression.getDouble().unwrap() == 2.5,
              "Canonical keyword accessors preserve the modeled scalar types and values");
        check(expression.getModelLong().unwrap() == expression.getLong().unwrap(),
              "Existing escaped getters remain usable");
        expired_entity = expression;
        expired_scalar = expression.getLong();
        auto global_copy = user_context->make_global<Note>(note);
        return std::pair(std::move(copied), global_copy);
    }();
    user_context.reset();
    check(global.live_user_bytes() > 0,
          "The explicit destination query remains allocated in its own UserContext");
    bool entity_rejected = false, scalar_rejected = false;
    try {
        (void)expired_entity->getLong().unwrap();
    } catch (const ReferenceError&) {
        entity_rejected = true;
    }
    try {
        (void)expired_scalar->unwrap();
    } catch (const ReferenceError&) {
        scalar_rejected = true;
    }
    check(entity_rejected && scalar_rejected,
          "Canonical E wrappers must not extend source ownership");
    const auto& copied_note = destination->get(destination->rebind_global(retained.second));
    check(E::note(copied_note).getLong().unwrap() == 42,
          "Explicit Global destination copying survives source destruction");
    retained.first.unselectLong().selectLong();
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        scalar_lifetimes();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
