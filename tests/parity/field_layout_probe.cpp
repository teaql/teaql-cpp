#include <teaql/core/load_state.hpp>
#include <teaql/core/json.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static const Json& field(const Json& value, std::string_view name) {
    return std::get<Json::Object>(value.data).find(name)->second;
}

static std::string_view text(const Json& value) {
    return std::get<String>(value.data);
}

static Vector<std::string_view> names(Memory owner, const Json& value) {
    Vector<std::string_view> result(owner);
    for (const auto& name : std::get<Json::Array>(value.data)) {
        result.push_back(text(name));
    }
    return result;
}

static Json evaluate(Memory owner, const Json& input) {
    Vector<FieldLayout::Index> indexes(owner);
    Vector<FieldLayout::Mapping> mappings(owner);
    for (const auto& item : std::get<Json::Array>(field(input, "indexes").data)) {
        const auto& pair = std::get<Json::Array>(item.data);
        indexes.emplace_back(text(pair[0]), std::size_t(std::get<std::uint64_t>(pair[1].data)));
    }
    for (const auto& item : std::get<Json::Array>(field(input, "mappings").data)) {
        const auto& triple = std::get<Json::Array>(item.data);
        mappings.emplace_back(text(triple[0]), text(triple[1]), text(triple[2]));
    }
    auto relations = names(owner, field(input, "relations"));
    auto expected = names(owner, field(input, "expected_members"));
    auto result = FieldLayout::from_generated(owner,
                                              text(field(input, "entity")),
                                              text(field(input, "revision")),
                                              indexes,
                                              mappings,
                                              relations,
                                              expected);
    Json::Object output(owner);
    if (!result) {
        output.emplace(String("error", owner), Json(owner, result.error()));
    } else {
        const auto& layout = result.value();
        output.emplace(String("entity", owner), Json(owner, layout.entity()));
        output.emplace(String("revision", owner), Json(owner, layout.revision()));
        output.emplace(String("count", owner), Json(owner, std::uint64_t(layout.field_count())));
        output.emplace(String("shared_identity", owner),
                       Json(owner, layout.shared_entity_name().data() == layout.entity().data()));
        Json::Array positions(owner), relation_flags(owner);
        for (auto name : names(owner, field(input, "lookups"))) {
            auto index = layout.index(name);
            positions.emplace_back(index ? Json(owner, std::uint64_t(*index)) : Json(owner));
            relation_flags.emplace_back(layout.is_relation(name));
        }
        output.emplace(String("indexes", owner), Json(owner, std::move(positions)));
        output.emplace(String("relations", owner), Json(owner, std::move(relation_flags)));
    }
    return Json(owner, std::move(output));
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto owner = user_context->memory_resource();
            auto input = Json::parse(owner, line);
            if (!input) {
                throw std::runtime_error("Invalid FieldLayout probe input JSON");
            }
            std::cout << evaluate(owner, *input).dump(owner) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
