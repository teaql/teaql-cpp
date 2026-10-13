#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include <iostream>
using namespace cpp_dynamic_json;

static CompactRow row(Memory r, std::uint64_t id) {
    CompactRow result(std::allocator_arg, Allocator(r));
    result.insert("id", Value(r, id));
    result.insert("version", Value(r, std::int64_t(1)));
    return result;
}

static Profile profile(Memory r, std::uint64_t id, const Json& note) {
    auto fields = row(r, id);
    auto* string = std::get_if<String>(&note.data);
    fields.insert("note", string ? Value(r, *string) : Value(r));
    return Profile::from_compact_row(r, std::move(fields)).value();
}

static Json optional_text(Memory r, const std::optional<String>& value) {
    return value ? Json(r, *value) : Json(r);
}

static Json note_summary(Memory r, const RuntimeExpression<std::optional<String>>& expression) {
    auto evaluation = expression.eval();
    int callbacks = 0;
    auto fallback = expression.orElseIfNull([&] {
        ++callbacks;
        return std::optional<String>(std::in_place, "fallback", r);
    });
    bool rejected = false;
    try {
        (void)expression.unwrap();
    } catch (const std::logic_error&) {
        rejected = true;
    }
    auto result = detail::model_json(r);
    detail::model_put(result, "eval_present", evaluation.has_value());
    detail::model_put(result, "eval_value", evaluation ? optional_text(r, *evaluation) : Json(r));
    detail::model_put(result, "unwrap_rejected", rejected);
    detail::model_put(
        result,
        "fallback",
        optional_text(r, expression.orIfNull(std::optional<String>(std::in_place, "fallback", r))));
    detail::model_put(result, "callback_result", optional_text(r, fallback));
    detail::model_put(result, "callbacks", callbacks);
    detail::model_put(result, "default", optional_text(r, expression.orDefaultIfNull()));
    return result;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto id = std::get<std::uint64_t>(field(input, "id").data);
    auto mode = text(field(input, "mode"));
    if (mode == "note") {
        auto entity = profile(r, id, field(input, "note"));
        auto before = note_summary(r, E::profile(entity).getNote());
        if (std::get<bool>(field(input, "change").data)) {
            auto* next = std::get_if<String>(&field(input, "next").data);
            entity.updateNote(next ? std::optional<String>(std::in_place, *next, r) : std::nullopt);
        }
        auto result = detail::model_json(r);
        detail::model_put(result, "before", before);
        detail::model_put(result, "after", note_summary(r, E::profile(entity).getNote()));
        return result;
    }
    auto person = Person::from_compact_row(r, row(r, id)).value();
    EntityRuntimeState root(std::allocator_arg, Allocator(r));
    person.attachRuntimeStateRecursive(root);
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    if (mode == "profile") {
        std::optional<Profile> child;
        if (!std::get<bool>(field(input, "empty").data)) {
            child.emplace(profile(r, 2, field(input, "note")));
        }
        graph.install_relation_option("Person", id, "profile", std::move(child));
        root.freeze_graph(std::move(graph)).value();
        auto expression = E::person(person).getProfile();
        auto result = detail::model_json(r);
        detail::model_put(result, "present", expression.eval().has_value());
        detail::model_put(result, "note", note_summary(r, expression.getNote()));
        return result;
    }
    SmartList<Order> list(std::allocator_arg, Allocator(r));
    auto count = std::get<std::uint64_t>(field(input, "count").data);
    for (std::uint64_t n = 0; n < count; ++n) {
        auto fields = row(r, n + 1);
        String code("code-", r);
        code += std::to_string(n);
        fields.insert("code", Value(r, code));
        list.data.emplace_back(Order::from_compact_row(r, std::move(fields)).value());
    }
    list.is_loaded = std::get<bool>(field(input, "loaded_flag").data);
    graph.install_relation_list("Person", id, "orders", std::move(list));
    root.freeze_graph(std::move(graph)).value();
    auto expression = E::person(person).getOrders();
    auto item = expression.get(std::get<std::uint64_t>(field(input, "index").data)).getCode();
    int callbacks = 0;
    auto fallback = item.orElseIfNull([&] {
        ++callbacks;
        return String("fallback", r);
    });
    auto result = detail::model_json(r);
    detail::model_put(result, "present", expression.eval().has_value());
    detail::model_put(result, "size", expression.size().unwrap());
    detail::model_put(result, "first", optional_text(r, expression.first().getCode().eval()));
    detail::model_put(result, "item", optional_text(r, item.eval()));
    detail::model_put(result, "fallback", fallback);
    detail::model_put(result, "callbacks", callbacks);
    detail::model_put(result, "default", item.orDefaultIfNull());
    return result;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        try {
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        } catch (const ExpressionNotLoadedError& error) {
            auto result = detail::model_json(r);
            detail::model_put(result, "unexpected_missing", error.diagnostic().to_json(r));
            std::cout << result.dump(r) << '\n';
        }
    }
    std::pmr::set_default_resource(previous);
}
