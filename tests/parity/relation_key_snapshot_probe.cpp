#include <teaql/runtime/detail/data_service/relation_enhancement.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include <iostream>

using namespace teaql;

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    const auto& object = std::get<Json::Object>(input.data);
    auto field = [&](std::string_view name) -> const Json& {
        return object.find(name)->second;
    };
    auto parent_count = std::get<std::uint64_t>(field("parents").data);
    auto key_count = std::get<std::uint64_t>(field("keys").data);
    auto single = std::get<bool>(field("single").data);
    auto overwritten = std::get<bool>(field("overwritten").data);
    Vector<CompactRow> parents(r), keys(r), rows(r);
    auto row = [&](std::string_view key, std::uint64_t n) {
        Record fields(r);
        fields.emplace(String(key, r), Value(r, n));
        return CompactRow(std::allocator_arg, Allocator(r), std::move(fields));
    };
    for (std::uint64_t i = 0; i < parent_count; ++i) {
        auto parent = row("id", overwritten ? 100 + i : i);
        parent.insert("agg", Value(r, std::int64_t(-1)));
        parents.emplace_back(std::move(parent));
    }
    for (std::uint64_t i = 0; i < key_count; ++i) {
        keys.emplace_back(row("id", i));
    }
    for (std::uint64_t i = 0; i < 3; ++i) {
        auto child = row("parent", i);
        child.insert("value", Value(r, 10 + i));
        rows.emplace_back(std::move(child));
    }
    RelationDescriptor relation(std::allocator_arg, Allocator(r), "items", "Child");
    relation.local_key = "id";
    relation.foreign_key = "parent";
    relation.many = true;
    RelationLoadPlan plan(std::allocator_arg, Allocator(r), "Parent", relation);
    SelectQuery query(std::allocator_arg, Allocator(r), "Child");
    RelationAggregate aggregate(std::allocator_arg, Allocator(r), "items", "agg", query, single);
    auto output = detail::model_json(r);
    detail::model_put(output, "parents", parent_count);
    detail::model_put(output, "keys", key_count);
    detail::model_put(output, "single", single);
    detail::model_put(output, "overwritten", overwritten);
    std::optional<String> failure;
    try {
        detail::attach_relation_aggregate_rows(parents, keys, plan, aggregate, std::move(rows));
    } catch (const std::exception& error) {
        failure.emplace(error.what(), r);
    }
    detail::model_put(output, "error", failure ? Json(r, *failure) : Json(r));
    Json::Array result(r);
    for (const auto& parent : parents) {
        auto item = detail::model_json(r);
        for (const auto& key : parent.keys()) {
            detail::model_put(item, key, detail::debug_value(r, *parent.get(key)));
        }
        result.emplace_back(std::move(item));
    }
    detail::model_put(output, "rows", result);
    return output;
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << evaluate(*user_context, *input).dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
