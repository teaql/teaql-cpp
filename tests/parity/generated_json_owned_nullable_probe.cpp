#include "../generated_typed_member_aliases/include/cpp_member_aliases.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/detail/json_hydration.hpp>
#include <iostream>
using namespace teaql;
using cpp_member_aliases::Platform;
using ModelRecord = cpp_member_aliases::Record;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static Json owned(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto input = Json::parse(
        r,
        R"({"id":1,"version":7,"name":"parent","record_list":[{"id":1,"version":7,"type_field":"first","platform_id":1,"platform":{"id":1,"version":7,"name":"parent"}},{"id":2,"version":7,"type_field":null,"platform_id":1}]})");
    auto first = user_context.decode_json_entity<Platform>(*input);
    auto next = user_context.decode_json_entity<Platform>(*input);
    check(bool(first) && bool(next), "generated nullable graph decodes");
    auto& root = first.value();
    auto state = root.entity_runtime_state();
    auto list = state->resolve_relation_list<ModelRecord>("Platform", 1, "record_list");
    check(list && list->data.size() == 2, "two typed children");
    auto child_state = list->data[0].entity_runtime_state();
    auto option = child_state->resolve_relation_option<Platform>("Record", 1, "platform");
    check(option && *option, "typed parent option");
    const auto& parent = **option;
    auto out = detail::model_json(r);
    Platform copy(std::allocator_arg, Allocator(r), root);
    detail::model_put(out, "json", std::move(copy).into_json(r));
    Json::Array values(r), scalar_loaded(r), parent_loaded(r);
    bool clean = true;
    for (const auto& child : list->data) {
        values.emplace_back(child.type() ? Json(r, *child.type()) : Json(r));
        scalar_loaded.emplace_back(Json(r, child.is_field_loaded("type_field")));
        parent_loaded.emplace_back(Json(r, child.is_field_loaded("platform")));
        clean = clean && !child.dirty_fields();
    }
    detail::model_put(out, "child_values", values);
    detail::model_put(out, "child_scalar_loaded", scalar_loaded);
    detail::model_put(out, "child_parent_loaded", parent_loaded);
    detail::model_put(out, "children_clean", clean);
    detail::model_put(out, "root_clean", !root.dirty_fields());
    detail::model_put(out, "parent_name", parent.name());
    detail::model_put(out, "parent_reverse_loaded", parent.is_field_loaded("record_list"));
    detail::model_put(out, "same_address", &parent == child_state->resolve_entity<Platform>(1));
    state->set(root.entity_key(), "name", Value(r, "one root"));
    auto dirty_fields = root.dirty_fields();
    check(dirty_fields && dirty_fields->size() == 1, "root mutation retained");
    Json::Array dirty(r);
    for (const auto& name : *dirty_fields) {
        dirty.emplace_back(Json(r, name));
    }
    detail::model_put(out, "root_dirty", dirty);
    check(!next.value().dirty_fields(), "independently decoded root remains clean");
    detail::model_put(out, "other_dirty", Json(r));
    return out;
}

static void shape_cache(UserContext& user_context) {
    auto r = user_context.memory_resource();
    for (std::size_t count : {1, 100, 10000}) {
        Vector<CompactRow> rows(r);
        std::optional<CompactRow> other;
        {
            detail::JsonReadSession session(user_context);
            auto descriptor = session.descriptor(ModelRecord::entity_descriptor(r));
            check(bool(descriptor), "installed descriptor");
            auto null = Json::parse(r, R"({"id":1,"version":7,"type_field":null})");
            auto value = Json::parse(r, R"({"legacy_type":"private","version":7,"id":2})");
            auto minimal = Json::parse(r, R"({"id":3,"version":7})");
            for (std::size_t index = 0; index < count; ++index) {
                std::optional<EntityRuntimeState> state;
                auto row = session.decode(
                    *descriptor.value(), index % 2 == 0 ? *null : *value, true, state);
                check(bool(row), "scalar row decodes");
                if (!rows.empty()) {
                    check(row.value().shared_layout() == rows[0].shared_layout(),
                          "one layout for aliased shape");
                }
                rows.emplace_back(std::move(row).value());
            }
            std::optional<EntityRuntimeState> state;
            auto decoded = session.decode(*descriptor.value(), *minimal, true, state);
            check(bool(decoded), "minimal row decodes");
            other.emplace(std::move(decoded).value());
            check(other->shared_layout() != rows[0].shared_layout(),
                  "second shape has distinct layout");
        }
        // The session and input JSON wrappers are gone; rows borrow Context-owned layouts.
        auto first = ModelRecord::from_compact_row(r, std::move(rows[0]));
        auto minimal = ModelRecord::from_compact_row(r, std::move(*other));
        check(bool(first) && bool(minimal), "model conversion after session destruction");
        check(first.value().is_field_loaded("type_field") && !first.value().type(),
              "loaded NULL survives");
        check(!minimal.value().is_field_loaded("type_field"), "omission survives");
        if (count > 1) {
            auto value = ModelRecord::from_compact_row(r, std::move(rows[1]));
            check(value && value.value().type() && *value.value().type() == "private",
                  "alias payload survives");
        }
        std::cout << "JSON_CACHE_LIFETIME rows=" << count << ";passed\n";
    }
}

int main(int argc, char**) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    if (argc > 1) {
        auto user_context = global.make_user_context();
        cpp_member_aliases::module(*user_context).apply_to(*user_context);
        shape_cache(*user_context);
        return 0;
    }
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        cpp_member_aliases::module(*user_context).apply_to(*user_context);
        std::cout << owned(*user_context).dump(user_context->memory_resource()) << '\n';
    }
}
