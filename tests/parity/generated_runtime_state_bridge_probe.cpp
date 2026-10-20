#include "../generated_typed_member_aliases/include/cpp_member_aliases.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <Entity T>
static T decode(UserContext& user_context) {
    auto r = user_context.memory_resource();
    Record values(r);
    values.emplace("id", Value(r, std::uint64_t{1}));
    values.emplace("version", Value(r, std::int64_t{1}));
    auto result = T::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), values));
    check(result.is_ok(), "Generated decoder must accept the identity projection");
    return std::move(result).value();
}

template <Entity T>
static Json observe(UserContext& user_context, std::size_t size) {
    auto r = user_context.memory_resource();
    auto row = decode<T>(user_context);
    auto first = row.runtime_state_any();
    auto second = row.runtime_state_any();
    auto out = detail::model_json(r);
    detail::model_put(out, "present", first.has_value());
    if (!first) {
        return out;
    }
    auto* state = first->template get<EntityRuntimeState>();
    check(state != nullptr, "Generated core hook must expose EntityRuntimeState");
    detail::model_put(out, "stable_borrow", state == second->template get<EntityRuntimeState>());
    detail::model_put(out, "wrong_type_rejected", first->template get<int>() == nullptr);
    detail::model_put(
        out, "comment_before", state->get_comment() ? Json(r, *state->get_comment()) : Json(r));
    String payload(size, 'x', r);
    auto typed = row.entity_runtime_state();
    state->set_comment(payload);
    detail::model_put(out, "typed_sees_borrow", typed->get_comment() == payload);
    payload += "-typed";
    typed->set_comment(payload);
    auto automatic = detail::borrow_entity_runtime_state(row);
    detail::model_put(
        out, "automatic_sees_typed", automatic && automatic->get_comment() == payload);
    row.updateType(std::optional<String>(std::in_place, "edited", r));
    detail::model_put(out, "dirty", row.dirty_fields().has_value());
    detail::model_put(out, "value", *row.type());
    detail::model_put(out, "comment_after", *state->get_comment());
    return out;
}

template <Entity T>
static void lifetimes() {
    GlobalContext global;
    Ref<EntityRuntimeState> retained;
    std::optional<detail::EntityLifetimeAnchor::Borrow> expired;
    {
        auto user_context = global.make_user_context();
        auto row = decode<T>(*user_context);
        auto erased = row.runtime_state_any();
        check(erased.has_value(), "Generated model must override the empty core hook");
        const auto* state = erased->template get<EntityRuntimeState>();
        check(state == &row.root, "Generated hook must borrow the existing root member");
        auto borrowed = detail::borrow_entity_runtime_state(row);
        check(borrowed && borrowed->memory_resource() == user_context->memory_resource(),
              "Runtime adapter must preserve the generated model's Context");
        borrowed->set_comment(String(2048, 'r', user_context->memory_resource()));
        check(row.root.get_comment()->size() == 2048,
              "Generated and runtime adapters must share the existing ledger");
        expired.emplace(borrowed->borrow_lifetime());
        retained = user_context->make_global<EntityRuntimeState>(*borrowed);
    }
    check(global.live_user_bytes() == 0, "Generated borrows must not retain User blocks");
    bool rejected = false;
    try {
        expired->check();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Checked state token must reject access after actual User destruction");
    auto next_user_context = global.make_user_context();
    const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
    auto comment = copy.get_comment();
    check(comment && comment->size() == 2048 && comment->front() == 'r' &&
              comment->get_allocator().resource() == global.memory_resource(),
          "Explicit Global copy must retain its independent allocator and payload");
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            lifetimes<cpp_member_aliases::Record>();
            lifetimes<cpp_member_aliases::ChildRecord>();
        } else {
            std::string line;
            while (std::getline(std::cin, line)) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto input = Json::parse(r, line);
                auto size = std::get<std::uint64_t>(field(*input, "size").data);
                auto result = text(field(*input, "model")) == "record"
                                  ? observe<cpp_member_aliases::Record>(*user_context, size)
                                  : observe<cpp_member_aliases::ChildRecord>(*user_context, size);
                std::cout << result.dump(r) << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
