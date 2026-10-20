#include "../generated_relations/include/cpp_relations.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace cpp_relations;

template <class Model, class Other>
static Json run(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t(7)));
    row.emplace(String("name", r), Value(r, "stored"));
    row.emplace(String("version", r), Value(r, std::int64_t(3)));
    row.emplace(String("platform_id", r), Value(r, std::uint64_t(42)));
    auto& entity = user_context.make<Model>(
        Model::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row)).value());
    std::array<std::string_view, 1> identity{"id"};
    entity.setLoadState(text(field(input, "initial")) == "full"
                            ? eval::LoadState::fully_loaded(r)
                            : eval::LoadState::partial(r, identity));
    const std::array<std::string_view, 10> names{"id",
                                                 "name",
                                                 "version",
                                                 "platform_id",
                                                 "platform",
                                                 "school_list",
                                                 "profile",
                                                 "device_list",
                                                 "#extra",
                                                 "missing"};
    auto availability = [&] {
        Json::Array flags(r);
        for (auto name : names) {
            flags.emplace_back(entity.isLoaded(name));
        }
        return Json(r, flags);
    };
    auto before = availability();
    Vector<std::string_view> selected(r);
    for (const auto& item : std::get<Json::Array>(field(input, "fields").data)) {
        selected.emplace_back(text(item));
    }
    auto mode = text(field(input, "state"));
    auto state = mode == "none"      ? eval::LoadState::not_loaded(r)
                 : mode == "full"    ? eval::LoadState::fully_loaded(r)
                 : mode == "compact" ? eval::LoadState::partial_compact(r, selected)
                                     : eval::LoadState::partial(r, selected);
    if (mode == "columns") {
        Vector<String> names(r);
        for (auto name : selected) {
            names.emplace_back(name);
        }
        auto& columns = user_context.make<ColumnLayout>(std::move(names));
        state = eval::LoadState::shared_columns(r, columns);
    } else if (mode == "indexed" || mode == "other") {
        state =
            state
                .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                              mode == "indexed" ? Model::field_layout(r) : Other::field_layout(r))
                .value();
    }
    bool failed = false;
    try {
        entity.setLoadState(state);
    } catch (const std::exception&) {
        failed = true;
    }
    auto identifier = number<std::uint64_t>(field(input, "identifier"));
    auto value = Model::withId(user_context, identifier);
    if (value.memory_resource() != r || value.try_u64() != identifier) {
        throw std::runtime_error("withId must retain its selected domain and U64 value");
    }
    Json::Object result(r);
    result.emplace(String("id", r), value.to_json_value(r));
    result.emplace(String("before", r), before);
    result.emplace(String("failed", r), Json(r, failed));
    result.emplace(String("after", r), availability());
    result.emplace(String("name", r), Json(r, entity.name()));
    auto changed = entity.changedName();
    result.emplace(String("changed", r), changed ? changed->to_json_value(r) : Json(r));
    auto original = entity.original_values();
    const Value* name = nullptr;
    if (original) {
        auto found = original->fields.find("name");
        if (found != original->fields.end()) {
            name = &found->second;
        }
    }
    result.emplace(String("original", r), name ? name->to_json_value(r) : Json(r));
    return Json(r, result);
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            Json retained(global.memory_resource());
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("invalid entity-helper fixture");
                }
                auto model = text(field(*input, "model"));
                auto result = model == "platform" ? run<Platform, School>(*user_context, *input)
                              : model == "school" ? run<School, Platform>(*user_context, *input)
                                                  : run<Device, Platform>(*user_context, *input);
                retained = detail::clone_in(global.memory_resource(), result);
            }
            user_context.reset();
            std::cout << retained.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
