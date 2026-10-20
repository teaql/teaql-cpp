#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace cpp_dynamic_json;

static bool flag(const Json& value, std::string_view name) {
    return std::get<bool>(field(value, name).data);
}

static CompactRow row(Memory r,
                      const Json& input,
                      RelationShapeCache& shapes,
                      std::string_view relation,
                      bool loaded) {
    Record fields(r);
    for (const auto& [name, value] : std::get<Json::Object>(input.data)) {
        fields.emplace(name, ::value(r, value));
    }
    CompactRow result(std::allocator_arg, Allocator(r), std::move(fields));
    if (loaded) {
        result.mark_relation_loaded(relation, shapes);
    }
    return result;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto out = detail::model_json(r);
    RelationShapeCache shapes(std::allocator_arg, Allocator(r));
    EntityRuntimeState root(std::allocator_arg, Allocator(r));
    auto order = Order::from_compact_row_with_context(
        r,
        row(r, field(input, "order"), shapes, "customer", flag(input, "forward_loaded")),
        AnyRef(root));
    auto person = Person::from_compact_row_with_context(
        r,
        row(r, field(input, "person"), shapes, "orders", flag(input, "reverse_loaded")),
        AnyRef(root));
    if (!order || !person) {
        detail::model_put(
            out, "error", !order ? order.error().to_string(r) : person.error().to_string(r));
        return out;
    }
    EntityGraphBuilder graph(std::allocator_arg, Allocator(r));
    if (flag(input, "table")) {
        graph.install<Person>(2, Person(std::allocator_arg, Allocator(r), person.value()));
    }
    if (flag(input, "list")) {
        SmartList<Order> list(std::allocator_arg, Allocator(r));
        if (!flag(input, "empty")) {
            list.data.emplace_back(order.value());
        }
        list.loaded();
        graph.install_relation_list("Person", 2, "orders", std::move(list));
    }
    const auto edge = text(field(input, "edge"));
    if (edge != "absent") {
        std::optional<Person> target;
        if (edge == "target") {
            target.emplace(std::allocator_arg, Allocator(r), person.value());
        }
        graph.install_relation_option<Person>("Order", 3, "customer", std::move(target));
    }
    if (!root.freeze_graph(std::move(graph))) {
        throw std::logic_error("Graph freeze failed");
    }
    detail::model_put(out, "order", std::move(order).value().into_json(r));
    detail::model_put(out, "person", std::move(person).value().into_json(r));
    return out;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        std::cout << observe(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
