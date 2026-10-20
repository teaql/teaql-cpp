#include "../generated_checker_unique_string/include/cpp_checker_unique_string.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/detail/graph_fix_scope.hpp>
#include <iostream>

using namespace cpp_checker_unique_string;

static_assert(!std::invocable<decltype(&Note::updateCode), Note&, Timestamp>);

struct OptionalText {
    using allocator_type = Allocator;
    std::optional<String> value;

    OptionalText(std::allocator_arg_t, Allocator a)
        : value(std::in_place, "retained", a.resource()) {}
};

static void check(bool valid, const char* message) {
    if (!valid) {
        throw std::runtime_error(message);
    }
}

static void operand_and_evidence_contracts() {
    GlobalContext global;
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    auto r = user_context.memory_resource();
    auto& state = user_context.make<EntityRuntimeState>();
    auto& key = user_context.make<EntityKey>("Note", Value(r, std::uint64_t(7)));
    auto& stored = user_context.make<String>("retained");
    auto& optional_stored = user_context.make<OptionalText>().value;
    detail::EntityRuntimeAccess::update_text_value(
        state, key, "code", stored, Value(r, Timestamp(1234)));
    detail::EntityRuntimeAccess::update_text_value(
        state, key, "optional_code", optional_stored, Value(r, Timestamp(1234)));
    check(stored == "retained" && optional_stored && *optional_stored == "retained",
          "invalid text conversions preserve required and optional storage");
    detail::EntityRuntimeAccess::update_text_value(
        state, key, "optional_code", optional_stored, Value(r));
    check(!optional_stored, "explicit NULL clears optional text");
    detail::EntityRuntimeAccess::update_text_value(
        state, key, "code", stored, Value(r, "  text  "));
    check(stored == "text" && state.get(key, "code")->try_text() == "  text  ",
          "stored text is trimmed while the original mutation operand is retained");
    {
        auto& entity = user_context.make<Note>();
        entity.updateId(7).updateVersion(0).updateCode("retained");
        auto& checker = user_context.make<NoteChecker>();
        auto& fix_state = user_context.make<detail::GraphFixState>(Timestamp(1234));
        CheckResults findings(r);
        ObjectLocation location(std::allocator_arg, Allocator(r));
        detail::GraphFixActivation active(user_context, fix_state);
        checker.check_and_fix_typed(
            user_context, entity, CheckObjectStatus::Create, location, findings);
        check(findings.empty() && entity.code() == "retained" &&
                  entity.changedCode()->try_timestamp() == Timestamp(1234) &&
                  fix_state.evidence.size() == 1 && fix_state.evidence[0].model_path == "code" &&
                  fix_state.evidence[0].source == FixEvidenceSource::Clock &&
                  fix_state.evidence[0].source_label == "graphClock",
              "string clock fixes retain the operation clock and existing evidence contract");
    }
    user_context_owner.reset();
    check(global.live_user_bytes() == 0,
          "managed text and clock storage is reclaimed on User deletion");
}

static Json observe(GlobalContext& global, std::string_view line) {
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    Json result(global.memory_resource());
    {
        auto r = user_context.memory_resource();
        auto input = Json::parse(r, line);
        check(bool(input), "valid clock text input");
        Record row(r);
        row.emplace(String("id", r), Value(r, std::uint64_t(7)));
        row.emplace(String("version", r), Value(r, std::int64_t(3)));
        row.emplace(String("code", r), Value(r, text(field(*input, "code"))));
        auto decoded = Note::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
        auto& entity = user_context.make<Note>(decoded.value());
        if (std::get<bool>(field(*input, "sparse").data)) {
            const std::array<std::string_view, 2> fields{"id", "version"};
            entity.setLoadState(eval::LoadState::partial(r, fields));
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
        auto changed = entity.changedCode();
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
        output.emplace(String("code", r), Json(r, entity.code()));
        output.emplace(String("loaded", r), Json(r, entity.isLoaded("code")));
        output.emplace(String("changed_present", r), Json(r, bool(changed)));
        output.emplace(String("changed_value", r), changed ? changed->to_json_value(r) : Json(r));
        output.emplace(String("timestamp_operand", r),
                       Json(r, changed && changed->try_timestamp().has_value()));
        output.emplace(String("diagnostics", r), Json(r, diagnostics));
        output.emplace(String("wall_millis", r), Json(r, Timestamp::now().as_millis()));
        result = Json(global.memory_resource(), output);
    }
    user_context_owner.reset();
    check(global.live_user_bytes() == 0, "copied observations do not retain User storage");
    return result;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        operand_and_evidence_contracts();
        GlobalContext global;
        for (std::string line; std::getline(std::cin, line);) {
            std::cout << observe(global, line).dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
