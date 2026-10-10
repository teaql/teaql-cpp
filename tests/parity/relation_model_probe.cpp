#include <teaql/runtime.hpp>
#include <teaql/runtime/relation_model_debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include "debug_modes.hpp"
#include <iostream>

static RelationLoadPlan plan(Memory owner, const Json& input) {
    RelationDescriptor relation(std::allocator_arg,
                                Allocator(owner),
                                text(field(input, "name")),
                                text(field(input, "target")));
    relation.local_key = text(field(input, "local"));
    relation.foreign_key = text(field(input, "foreign"));
    relation.many = std::get<bool>(field(input, "many").data);
    RelationLoadPlan result(
        std::allocator_arg, Allocator(owner), text(field(input, "parent")), relation);
    result.path = text(field(input, "path"));
    if (std::holds_alternative<Json::Object>(field(input, "query").data)) {
        const auto& query_input = field(input, "query");
        result.query.emplace(
            std::allocator_arg, Allocator(owner), text(field(query_input, "entity")));
        auto& query = *result.query;
        query.and_filter(Expr::eq(owner, "name", value(owner, field(query_input, "value"))));
        query.comment(text(field(query_input, "comment")));
        query.order_by.emplace_back(OrderBy::asc(owner, "id"));
        if (std::holds_alternative<String>(field(query_input, "limit").data)) {
            query.limit(number<std::uint64_t>(field(query_input, "limit")));
        }
        if (std::holds_alternative<String>(field(query_input, "offset").data)) {
            query.offset(number<std::uint64_t>(field(query_input, "offset")));
        }
        if (!std::holds_alternative<std::monostate>(field(query_input, "threshold").data)) {
            query.top_n_probe_parent_threshold(
                detail::model_integer<std::size_t>(field(query_input, "threshold"), "threshold"));
        }
    }
    for (const auto& child : detail::model_array(field(input, "children"), "children")) {
        result.children.emplace_back(plan(owner, child));
    }
    return result;
}

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto value = plan(owner, input);
    auto clone = detail::clone_in(owner, value);
    auto other = detail::clone_in(owner, value);
    auto changed = text(field(input, "changed"));
    if (changed == "parent") {
        other.parent_entity += '!';
    } else if (changed == "name") {
        other.relation_name += '!';
    } else if (changed == "path") {
        other.path += '!';
    } else if (changed == "target") {
        other.target_entity += '!';
    } else if (changed == "local") {
        other.local_key += '!';
    } else if (changed == "foreign") {
        other.foreign_key += '!';
    } else if (changed == "many") {
        other.many = !other.many;
    } else if (changed == "query") {
        if (other.query) {
            other.query.reset();
        } else {
            other.query.emplace(std::allocator_arg, Allocator(owner), "Other");
        }
    } else {
        RelationDescriptor relation(std::allocator_arg, Allocator(owner), "empty", "Other");
        relation.local_key = "id";
        relation.foreign_key = "owner_id";
        other.children.emplace_back("Child", relation);
    }
    auto output = detail::model_json(owner);
    detail::model_put(output, "debug", formatted_debug(owner, value));
    detail::model_put(output, "clone", formatted_debug(owner, clone));
    detail::model_put(output, "clone_equal", value == clone);
    detail::model_put(output, "changed_equal", value == other);
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid relation plan model fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
