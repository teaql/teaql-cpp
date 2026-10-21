#include "../generated_sample_fields_runtime/include/cpp_sample_runtime.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace cpp_sample_runtime;

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

static Json run(UserContext& user_context, std::string_view kind) {
    auto r = user_context.memory_resource();
    auto out = detail::model_json(r);
    if (kind == "ambiguous") {
        auto input = Json::parse(
            r,
            R"({"id":1,"first_record_id":7,"first_record":{"id":7,"label":"shared"},"second_record_id":7,"second_record":{"id":7}})");
        auto row = user_context.decode_json_entity<SampleLink>(*input);
        check(bool(row), "two forward views decode");
        auto state = row.value().entity_runtime_state();
        auto* first = state->resolve_relation_option<SampleRecord>("SampleLink", 1, "first_record");
        auto* second =
            state->resolve_relation_option<SampleRecord>("SampleLink", 1, "second_record");
        check(first && *first && second && *second, "both explicit views exist");
        detail::model_put(out, "first_loaded", (**first).is_field_loaded("label"));
        detail::model_put(out, "second_loaded", (**second).is_field_loaded("label"));
        detail::model_put(out, "ambiguous", state->resolve_entity<SampleRecord>(7) == nullptr);
        detail::model_put(out, "json", json(r, row.value()));
        detail::model_put(out, "first_json", json(r, **first));
        detail::model_put(out, "second_json", json(r, **second));
    } else if (kind == "inferred_conflict") {
        auto input = Json::parse(
            r,
            R"({"id":1,"first_links":[{"id":2,"first_record_id":1,"second_record":{"id":7}},{"id":2,"first_record_id":1,"second_record":{"id":8}}]})");
        auto row = user_context.decode_json_entity<SampleRecord>(*input);
        check(!row, "inferred keys must conflict");
        detail::model_put(out, "error", row.error().to_string(r));
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
