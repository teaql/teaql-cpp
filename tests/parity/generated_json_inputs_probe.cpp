#include "../generated_sample_fields_runtime/include/cpp_sample_runtime.hpp"
#include "../generated_typed_member_aliases/include/cpp_member_aliases.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
using namespace teaql;
using cpp_sample_runtime::SampleRecord;
using cpp_sample_runtime::SampleLink;

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
static String rejected(UserContext& user_context, const Json& input, Memory destination) {
    auto result = user_context.decode_json_entity<T>(input);
    check(!result, "invalid input rejected");
    return result.error().to_string(destination);
}

static Json run(GlobalContext& global, UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto out = detail::model_json(r);
    auto kind = text(field(input, "case"));
    if (kind == "combined") {
        auto value = Json::parse(
            r,
            R"({"id":1,"version":7,"optional_count":null,"quantity":0,"active":false,"height":3.5,"amount":"123.450","day":"2024-02-29","recorded_at":1700000000123,"payload":{"literal":"_comment"},"clock_text":"","_name":"derived","_nil":null,"_count":0})");
        auto result = user_context.decode_json_entity<SampleRecord>(*value);
        check(bool(result), "combined scalar input decodes");
        const auto& row = result.value();
        auto output = json(r, row);
        auto again = user_context.decode_json_entity<SampleRecord>(output);
        check(bool(again), "presentation JSON decodes again");
        detail::model_put(out, "json", output);
        detail::model_put(out, "roundtrip", json(r, again.value()));
        detail::model_put(out, "null_loaded", row.is_field_loaded("optional_count"));
        detail::model_put(out, "omitted_loaded", row.is_field_loaded("optional_active"));
        detail::model_put(out, "quantity", row.quantity());
        detail::model_put(out, "active", row.active());
        detail::model_put(out, "height", row.height());
        detail::model_put(out, "amount", row.amount().to_string(r));
        detail::model_put(out, "day", row.day().to_string(r));
        detail::model_put(out, "timestamp", row.recordedAt().millis);
        detail::model_put(out, "clock", row.clockText());
        Json dirty(r);
        if (auto fields = row.dirty_fields()) {
            Json::Array names(r);
            for (const auto& name : *fields) {
                names.emplace_back(Json(r, name));
            }
            dirty = Json(r, names);
        }
        detail::model_put(out, "dirty", dirty);
        detail::model_put(out, "new", row.is_new());
        detail::model_put(out, "deleted", row.is_marked_as_delete());
        detail::model_put(out, "dynamic_pending", row.has_pending_dynamic_mutations());
    } else if (kind == "rejections") {
        Json::Array errors(r);
        for (auto source : {R"({"unknown":"PRIVATE_LITERAL"})",
                            R"({"label":42})",
                            R"({"active":null})",
                            R"({"quantity":1.5})",
                            R"({"id":-1})",
                            R"({"day":"PRIVATE_LITERAL"})",
                            R"({"_original_values":{}})",
                            R"({"#label":"PRIVATE_LITERAL"})",
                            R"({"platform":{"id":1,"name":"PRIVATE_LITERAL"}})"}) {
            auto value = Json::parse(r, source);
            errors.emplace_back(Json(r, rejected<SampleRecord>(user_context, *value, r)));
        }
        auto missing = global.make_user_context();
        auto identity = Json::parse(r, R"({"id":1})");
        errors.emplace_back(Json(r, rejected<SampleRecord>(*missing, *identity, r)));
        auto wrong = global.make_user_context();
        auto descriptor = SampleRecord::entity_descriptor(wrong->memory_resource());
        descriptor.properties[0].data_type = DataType::Text;
        auto& metadata = wrong->make<InMemoryMetadataStore>();
        metadata.register_entity(std::move(descriptor));
        wrong->set_metadata(*wrong, metadata);
        auto private_identity = Json::parse(r, R"({"id":"PRIVATE_LITERAL"})");
        errors.emplace_back(Json(r, rejected<SampleRecord>(*wrong, *private_identity, r)));
        detail::model_put(out, "errors", errors);
    } else if (kind == "target") {
        auto descriptor = SampleLink::entity_descriptor(r);
        descriptor.properties[0].data_type = DataType::Text;
        auto& metadata = user_context.make<InMemoryMetadataStore>();
        metadata.register_entity(SampleRecord::entity_descriptor(r));
        metadata.register_entity(std::move(descriptor));
        user_context.set_metadata(user_context, metadata);
        auto value = Json::parse(r, R"({"id":1,"first_links":[]})");
        detail::model_put(out, "incompatible", rejected<SampleRecord>(user_context, *value, r));
        auto missing = global.make_user_context();
        auto& no_decoder = missing->make<InMemoryMetadataStore>();
        no_decoder.register_entity(SampleRecord::entity_descriptor(missing->memory_resource()));
        no_decoder.register_entity(SampleLink::entity_descriptor(missing->memory_resource()));
        missing->set_metadata(*missing, no_decoder);
        detail::model_put(out, "unregistered", rejected<SampleRecord>(*missing, *value, r));
    } else if (kind == "aliases") {
        auto n = std::get<std::uint64_t>(field(input, "count").data);
        Json::Array values(r);
        for (std::uint64_t i = 0; i < n; ++i) {
            auto value =
                Json::parse(r,
                            i % 2 == 0 ? R"({"id":1,"version":7,"type_field":null})"
                                       : R"({"legacy_type":"private","version":7,"id":2})");
            values.emplace_back(std::move(*value));
        }
        values.emplace_back(std::move(*Json::parse(r, R"({"id":3,"version":7})")));
        auto result = user_context.decode_json_entities<cpp_member_aliases::Record>(
            Json(r, std::move(values)));
        check(bool(result) && result.value().size() == n + 1, "alias batch decoded");
        const auto& rows = result.value();
        auto first = rows[0].loaded_state_snapshot();
        check(bool(first), "first snapshot exists");
        bool same = true;
        for (std::size_t i = 0; i < n; ++i) {
            auto snapshot = rows[i].loaded_state_snapshot();
            check(bool(snapshot), "row snapshot exists");
            same = same && first->identity() == snapshot->identity();
        }
        auto other = rows[n].loaded_state_snapshot();
        check(bool(other), "other snapshot exists");
        auto duplicate = Json::parse(r, R"({"type_field":"A","legacy_type":"B"})");
        detail::model_put(out, "count", n);
        detail::model_put(out, "same", same);
        detail::model_put(out, "distinct", first->identity() != other->identity());
        detail::model_put(out, "first_loaded", rows[0].is_field_loaded("type_field"));
        detail::model_put(out, "other_loaded", rows[n].is_field_loaded("type_field"));
        detail::model_put(out, "first", json(r, rows[0]));
        detail::model_put(out, "last", json(r, rows[n - 1]));
        detail::model_put(out, "other", json(r, rows[n]));
        detail::model_put(
            out, "duplicate", rejected<cpp_member_aliases::Record>(user_context, *duplicate, r));
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
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        check(bool(input), "case input parses");
        if (text(field(*input, "case")) == "aliases") {
            cpp_member_aliases::module(*user_context).apply_to(*user_context);
        } else {
            cpp_sample_runtime::module(*user_context).apply_to(*user_context);
        }
        std::cout << run(global, *user_context, *input).dump(r) << '\n';
    }
}
