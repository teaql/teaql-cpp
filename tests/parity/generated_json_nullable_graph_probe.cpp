#include "../generated_typed_member_aliases/include/cpp_member_aliases.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace teaql;

static Json run(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto out = detail::model_json(r);
    if (text(field(input, "entity")) == "Holder") {
        auto result =
            user_context.decode_json_entity<cpp_member_aliases::Holder>(field(input, "value"));
        detail::model_put(out, "ok", bool(result));
        if (!result) {
            detail::model_put(out, "error", result.error().to_string(r));
            return out;
        }
        auto& row = result.value();
        auto state = row.entity_runtime_state();
        auto option = state->resolve_relation_option<cpp_member_aliases::Record>(
            "Holder", row.id(), "record");
        detail::model_put(out, "relation", !option ? "not_loaded" : *option ? "some" : "null");
        Json same(r);
        if (option && *option) {
            same = Json(
                r, &**option == state->resolve_entity<cpp_member_aliases::Record>((**option).id()));
        }
        detail::model_put(out, "same_address", same);
        detail::model_put(out, "foreign_key", row.recordId() ? Json(r, *row.recordId()) : Json(r));
        detail::model_put(out, "foreign_key_loaded", row.is_field_loaded("record_id"));
        detail::model_put(out, "relation_loaded", row.is_field_loaded("record"));
        detail::model_put(out, "clean", !row.dirty_fields());
        detail::model_put(out, "json", std::move(row).into_json(r));
    } else {
        auto result =
            user_context.decode_json_entity<cpp_member_aliases::Record>(field(input, "value"));
        detail::model_put(out, "ok", bool(result));
        if (!result) {
            detail::model_put(out, "error", result.error().to_string(r));
            return out;
        }
        auto& row = result.value();
        auto children =
            row.entity_runtime_state()->resolve_relation_list<cpp_member_aliases::Holder>(
                "Record", row.id(), "holder_list");
        detail::model_put(
            out, "count", children ? Json(r, std::uint64_t(children->data.size())) : Json(r));
        detail::model_put(out, "relation_loaded", row.is_field_loaded("holder_list"));
        detail::model_put(out, "clean", !row.dirty_fields());
        detail::model_put(out, "json", std::move(row).into_json(r));
    }
    return out;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        cpp_member_aliases::module(*user_context).apply_to(*user_context);
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("invalid fixture");
        }
        std::cout << run(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
}
