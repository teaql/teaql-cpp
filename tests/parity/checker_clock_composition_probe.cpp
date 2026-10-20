#include "../generated_checker_clock_composition/include/cpp_checker_clock_composition.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace cpp_checker_clock_composition;

static Json observe(GlobalContext& global, std::string_view line) {
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    Json retained(global.memory_resource());
    {
        auto r = user_context.memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid clock composition input");
        }
        auto* root = optional(*input, "root");
        if (root && !std::holds_alternative<std::monostate>(root->data)) {
            user_context.with_active_root("Tenant", std::get<std::uint64_t>(root->data));
        }
        auto foreign_id = std::get<std::uint64_t>(field(*input, "foreign_id").data);
        teaql::Record row(r);
        row.emplace(String("id", r), Value(r, std::uint64_t(7)));
        row.emplace(String("version", r), Value(r, std::int64_t(3)));
        constexpr char code[] = "  界\0parent  ";
        row.emplace(String("code", r), Value(r, std::string_view(code, sizeof(code) - 1)));
        row.emplace(String("count", r), Value(r, std::int64_t(17)));
        auto payload = Json::parse(r, R"({"nested":[null,true,"界\u0000"]})");
        row.emplace(String("payload", r), Value(r, *payload));
        row.emplace(String("owner_id", r), Value(r, foreign_id));
        row.emplace(String("linked_id", r), Value(r, foreign_id));
        row.emplace(String("optional_linked_id", r),
                    std::get<bool>(field(*input, "null_optional").data) ? Value(r)
                                                                        : Value(r, foreign_id));
        auto decoded = Note::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
        if (!decoded) {
            throw std::runtime_error("Clock composition model decoding failed");
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
        add("code", entity.code(), entity.changedCode());
        add("count", entity.count(), entity.changedCount());
        add("payload", entity.payload(), entity.changedPayload());
        add("owner_id", entity.ownerId(), entity.changedOwnerId());
        add("linked_id", entity.linkedId(), entity.changedLinkedId());
        add("optional_linked_id", entity.optionalLinkedId(), entity.changedOptionalLinkedId());
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
        throw std::runtime_error("Clock composition observations retained User storage");
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
