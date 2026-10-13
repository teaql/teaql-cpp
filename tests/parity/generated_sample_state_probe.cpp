#include <teaql/runtime.hpp>
#include "fixture_value.hpp"
#include <iostream>

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto scale_name = text(field(input, "scale"));
    auto scale = scale_name == "Tiny"    ? SampleDataScale::Tiny
                 : scale_name == "Small" ? SampleDataScale::Small
                                         : SampleDataScale::Medium;
    auto& state = user_context.make<SampleDataState>(
        SampleDataPlan{scale, std::get<std::uint64_t>(field(input, "seed").data)});
    for (const auto& item : std::get<Json::Array>(field(input, "references").data)) {
        const auto& pair = std::get<Json::Array>(item.data);
        state.addReference(text(pair[0]), std::get<std::uint64_t>(pair[1].data));
    }
    for (const auto& item : std::get<Json::Array>(field(input, "generated").data)) {
        state.recordGenerated(text(item));
    }
    for (const auto& item : std::get<Json::Array>(field(input, "skipped").data)) {
        const auto& pair = std::get<Json::Array>(item.data);
        state.recordSkipped(text(pair[0]), text(pair[1]));
    }
    Json::Array picks(r);
    for (const auto& item : std::get<Json::Array>(field(input, "picks").data)) {
        auto entity = text(field(item, "entity"));
        auto salt = std::get<std::uint64_t>(field(item, "salt").data);
        std::pmr::unordered_set<std::uint64_t> used(r);
        for (const auto& id : std::get<Json::Array>(field(item, "used").data)) {
            used.insert(std::get<std::uint64_t>(id.data));
        }
        auto out = detail::model_json(r);
        Json::Array ids(r);
        for (auto id : state.ids(entity)) {
            ids.emplace_back(id);
        }
        detail::model_put(out, "ids", ids);
        auto chosen = state.pickId(entity, salt);
        auto unused = state.pickUnusedId(entity, salt, used);
        detail::model_put(out, "pick", chosen ? Json(r, *chosen) : Json(r));
        detail::model_put(out, "unused", unused ? Json(r, *unused) : Json(r));
        picks.emplace_back(std::move(out));
    }
    auto references = detail::model_json(r);
    for (const auto& [entity, ids] : state.references) {
        Json::Array values(r);
        for (auto id : ids) {
            values.emplace_back(id);
        }
        detail::model_put(references, entity, values);
    }
    auto report = std::move(state).intoReport();
    auto generated = detail::model_json(r);
    Json::Array skipped(r), conversions(r);
    for (const auto& [entity, count] : report.generated) {
        detail::model_put(generated, entity, count);
    }
    for (const auto& entry : report.skipped) {
        auto out = detail::model_json(r);
        detail::model_put(out, "entity", entry.entity);
        detail::model_put(out, "reason", entry.reason);
        skipped.emplace_back(std::move(out));
    }
    for (const auto& item : std::get<Json::Array>(field(input, "values").data)) {
        if (std::holds_alternative<std::monostate>(item.data)) {
            conversions.emplace_back(sample_reference_id(nullptr));
        } else {
            auto converted = value(r, item);
            conversions.emplace_back(sample_reference_id(&converted));
        }
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "scale", scale_name);
    detail::model_put(out, "seed", state.plan.seed);
    detail::model_put(out, "references", references);
    detail::model_put(out, "picks", picks);
    detail::model_put(out, "generated", generated);
    detail::model_put(out, "skipped", skipped);
    detail::model_put(out, "converted", conversions);
    return out;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << observe(*user_context, *input).dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
