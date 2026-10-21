#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <Entity T>
static Json json(Memory r, const T& value) {
    T copy(std::allocator_arg, Allocator(r), value);
    return std::move(copy).into_json(r);
}

template <Entity T>
static T decode(UserContext& user_context, std::string_view text) {
    auto input = Json::parse(user_context.memory_resource(), text);
    check(bool(input), "fixture parses");
    auto result = user_context.decode_json_entity<T>(*input);
    check(bool(result), "generated entity decodes");
    return std::move(result).value();
}

template <Entity T>
static Vector<T> decode_many(UserContext& user_context, std::string_view text) {
    auto input = Json::parse(user_context.memory_resource(), text);
    check(bool(input), "batch fixture parses");
    auto result = user_context.decode_json_entities<T>(*input);
    check(bool(result), "generated batch decodes");
    return std::move(result).value();
}

template <Entity A, Entity B>
static bool shared(const A& a, const B& b) {
    auto first = a.loaded_state_snapshot();
    auto second = b.loaded_state_snapshot();
    check(bool(first) && bool(second), "generated snapshots exist");
    return first->identity() == second->identity();
}

template <Entity T>
static Json dirty(Memory r, const T& value) {
    auto fields = value.dirty_fields();
    if (!fields) {
        return Json(r);
    }
    Json::Array result(r);
    for (const auto& name : *fields) {
        result.emplace_back(Json(r, name));
    }
    return Json(r, result);
}

static Json run(UserContext& user_context, std::string_view kind) {
    auto r = user_context.memory_resource();
    auto out = detail::model_json(r);
    if (kind == "batch") {
        auto rows = decode_many<Profile>(
            user_context,
            R"([{"id":1,"version":7,"note":null},{"version":7,"note":"private","id":1},{"id":2,"version":7}])");
        Json::Array sharing(r), before(r), loaded(r), mutations(r);
        for (const auto& a : rows) {
            Json::Array row(r);
            for (const auto& b : rows) {
                row.emplace_back(Json(r, shared(a, b)));
            }
            sharing.emplace_back(Json(r, row));
            before.emplace_back(json(r, a));
            loaded.emplace_back(Json(r, a.is_field_loaded("note")));
        }
        rows[0].entity_runtime_state()->set(
            rows[0].entity_key(), "note", Value(r, "one private mutation"));
        for (const auto& a : rows) {
            mutations.emplace_back(dirty(r, a));
        }
        detail::model_put(out, "sharing", sharing);
        detail::model_put(out, "before", before);
        detail::model_put(out, "dirty", mutations);
        detail::model_put(out, "loaded", loaded);
    } else if (kind == "owned") {
        constexpr std::string_view text =
            R"({"id":1,"version":7,"code":"parent","lines":[{"id":1,"version":7,"quantity":1,"order_id":1,"order":{"id":1,"version":7,"code":"parent"}},{"id":2,"version":7,"quantity":0,"order_id":1}]})";
        auto root = decode<Order>(user_context, text);
        auto other = decode<Order>(user_context, text);
        auto state = root.entity_runtime_state();
        auto* children = state->resolve_relation_list<Line>("Order", 1, "lines");
        check(children && children->data.size() == 2, "owned children");
        auto child_state = children->data[0].entity_runtime_state();
        auto* option = child_state->resolve_relation_option<Order>("Line", 1, "order");
        check(option && bool(*option), "owned parent");
        const auto& parent = **option;
        detail::model_put(
            out, "same_entity_address", &parent == child_state->resolve_entity<Order>(1));
        detail::model_put(out, "json", json(r, root));
        Json::Array loaded(r);
        bool clean = true;
        for (const auto& child : children->data) {
            loaded.emplace_back(Json(r, child.is_field_loaded("order")));
            clean = clean && !child.dirty_fields();
        }
        detail::model_put(out, "child_order_loaded", loaded);
        detail::model_put(out, "children_clean", clean);
        detail::model_put(out, "parent_lines_loaded", parent.is_field_loaded("lines"));
        state->set(root.entity_key(), "code", Value(r, "one root"));
        detail::model_put(out, "dirty", dirty(r, root));
        detail::model_put(out, "other_dirty", dirty(r, other));
    } else if (kind == "independent") {
        auto roots = decode_many<Order>(
            user_context,
            R"([{"id":1,"lines":[{"id":1,"quantity":1}]},{"id":1,"lines":[{"id":1,"quantity":2}]}])");
        Json::Array values(r), clean(r);
        for (const auto& root : roots) {
            auto state = root.entity_runtime_state();
            auto* rows = state->resolve_relation_list<Line>("Order", 1, "lines");
            check(rows && rows->data.size() == 1, "independent children");
            values.emplace_back(Json(r, rows->data[0].quantity()));
            clean.emplace_back(Json(r, !rows->data[0].dirty_fields()));
        }
        detail::model_put(out, "shared_snapshot", shared(roots[0], roots[1]));
        detail::model_put(out, "quantities", values);
        detail::model_put(out, "clean", clean);
    } else if (kind == "views") {
        std::optional<Order> detached;
        {
            auto root = decode<Order>(
                user_context,
                R"({"id":1,"version":7,"code":"shared","lines":[{"id":1,"order_id":1,"order":{"id":1,"version":7,"code":"shared","lines":[]}},{"id":2,"order_id":1,"order":{"id":1,"version":7}}]})");
            auto state = root.entity_runtime_state();
            auto* children = state->resolve_relation_list<Line>("Order", 1, "lines");
            check(children && children->data.size() == 2, "view children");
            auto first_state = children->data[0].entity_runtime_state();
            auto second_state = children->data[1].entity_runtime_state();
            auto* first = first_state->resolve_entity<Order>(1);
            auto* second = second_state->resolve_entity<Order>(1);
            check(first && second, "view-local lookup");
            auto first_root = first->entity_runtime_state();
            auto second_root = second->entity_runtime_state();
            auto* empty = first_root->resolve_relation_list<Line>("Order", 1, "lines");
            check(empty, "selected empty reverse list");
            detail::model_put(out, "root_first_snapshot", shared(root, *first));
            detail::model_put(out, "first_second_snapshot", shared(*first, *second));
            Json::Array loaded(r);
            loaded.emplace_back(Json(r, first->is_field_loaded("code")));
            loaded.emplace_back(Json(r, second->is_field_loaded("code")));
            detail::model_put(out, "loaded", loaded);
            detail::model_put(out, "first_empty", empty->data.empty());
            detail::model_put(out,
                              "second_missing",
                              !second_root->resolve_relation_list<Line>("Order", 1, "lines"));
            detail::model_put(out,
                              "children_clean",
                              !children->data[0].dirty_fields() &&
                                  !children->data[1].dirty_fields());
            detail::model_put(out, "json", json(r, root));
            detached.emplace(std::allocator_arg, Allocator(r), *first);
        }
        detail::model_put(out, "detached", json(r, *detached));
    } else {
        throw std::runtime_error("unknown fixture");
    }
    return out;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        module(*user_context).apply_to(*user_context);
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        check(bool(input), "case input parses");
        std::cout << run(*user_context, text(field(*input, "case"))).dump(r) << '\n';
    }
}
