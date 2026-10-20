#include "../generated_checker_clock_scalars/include/cpp_checker_clock_scalars.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace cpp_checker_clock_scalars;

static Json observe(GlobalContext& global, std::string_view line) {
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    Json retained(global.memory_resource());
    {
        auto r = user_context.memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid scalar clock input");
        }
        Record row(r);
        row.emplace(String("id", r), Value(r, std::uint64_t(7)));
        row.emplace(String("version", r), Value(r, std::int64_t(3)));
        for (const auto& [name, data] : std::get<Json::Object>(field(*input, "values").data)) {
            row.emplace(String(name, r), value(r, data));
        }
        auto decoded = Note::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
        if (!decoded) {
            throw std::runtime_error("Scalar clock model decoding failed");
        }
        auto& entity = user_context.make<Note>(decoded.value());
        if (std::get<bool>(field(*input, "sparse").data)) {
            const std::array<std::string_view, 2> names{"id", "version"};
            entity.setLoadState(eval::LoadState::partial(r, names));
        }
        auto status_text = text(field(*input, "status"));
        auto status = status_text == "create"   ? CheckObjectStatus::Create
                      : status_text == "update" ? CheckObjectStatus::Update
                                                : CheckObjectStatus::Unknown;
        auto& checker = user_context.make<NoteChecker>();
        auto location = ObjectLocation::array_root(r, 2);
        location.member("note_info");
        CheckResults findings(r);
        checker.check_and_fix_typed(user_context, entity, status, location, findings);
        Json::Object fields(r);
        auto add = [&](std::string_view name,
                       const auto& stored,
                       const std::optional<Value>& changed) {
            Json::Object output(r);
            output.emplace(String("stored", r),
                           generated_detail::encode(r, stored).to_json_value(r));
            output.emplace(String("loaded", r), Json(r, entity.isLoaded(name)));
            output.emplace(String("changed_present", r), Json(r, bool(changed)));
            output.emplace(String("changed_value", r),
                           changed ? changed->to_json_value(r) : Json(r));
            output.emplace(String("timestamp_operand", r),
                           Json(r, changed && std::holds_alternative<Timestamp>(changed->data)));
            fields.emplace(String(name, r), Json(r, output));
        };
        add("text", entity.text(), entity.changedText());
        add("optional_text", entity.optionalText(), entity.changedOptionalText());
        add("code", entity.code(), entity.changedCode());
        add("optional_code", entity.optionalCode(), entity.changedOptionalCode());
        add("integer", entity.integer(), entity.changedInteger());
        add("optional_integer", entity.optionalInteger(), entity.changedOptionalInteger());
        add("long", entity.modelLong(), entity.changedModelLong());
        add("optional_long", entity.optionalLong(), entity.changedOptionalLong());
        add("decimal", entity.decimal(), entity.changedDecimal());
        add("optional_decimal", entity.optionalDecimal(), entity.changedOptionalDecimal());
        add("float", entity.modelFloat(), entity.changedModelFloat());
        add("optional_float", entity.optionalFloat(), entity.changedOptionalFloat());
        add("double", entity.modelDouble(), entity.changedModelDouble());
        add("optional_double", entity.optionalDouble(), entity.changedOptionalDouble());
        add("active", entity.active(), entity.changedActive());
        add("optional_active", entity.optionalActive(), entity.changedOptionalActive());
        add("date", entity.date(), entity.changedDate());
        add("optional_date", entity.optionalDate(), entity.changedOptionalDate());
        add("time", entity.time(), entity.changedTime());
        add("optional_time", entity.optionalTime(), entity.changedOptionalTime());
        add("payload", entity.payload(), entity.changedPayload());
        add("optional_payload", entity.optionalPayload(), entity.changedOptionalPayload());
        add("timestamp", entity.timestamp(), entity.changedTimestamp());
        Json::Array diagnostics(r);
        for (auto profile : {JsonFieldNamingProfile::CamelCase,
                             JsonFieldNamingProfile::SnakeCase,
                             JsonFieldNamingProfile::PascalCase}) {
            Json::Array values(r);
            for (const auto& finding : findings) {
                values.emplace_back(finding.to_wire(r, profile).to_json(r));
            }
            diagnostics.emplace_back(std::move(values));
        }
        Json::Object output(r);
        output.emplace(String("fields", r), Json(r, fields));
        output.emplace(String("diagnostics", r), Json(r, diagnostics));
        output.emplace(String("wall_millis", r), Json(r, Timestamp::now().as_millis()));
        retained = Json(global.memory_resource(), output);
    }
    user_context_owner.reset();
    if (global.live_user_bytes() != 0) {
        throw std::runtime_error("Scalar clock observations retained User storage");
    }
    return retained;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        for (std::string line; std::getline(std::cin, line);) {
            std::cout << observe(global, line).dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
