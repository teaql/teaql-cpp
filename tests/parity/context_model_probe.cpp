#include <teaql/runtime/context_model_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    Json::Object output(owner);
    if (text(field(input, "mode")) == "root") {
        auto& entity = user.make<ContextEntityRef>(text(field(input, "entity")),
                                                   number<std::uint64_t>(field(input, "id")));
        auto& error = user.make<ContextRootError>(
            text(field(input, "expected")),
            std::get<bool>(field(input, "present").data) ? &entity : nullptr);
        ContextEntityRef cloned(std::allocator_arg, Allocator(owner), entity);
        ContextEntityRef changed(std::allocator_arg, Allocator(owner), entity);
        changed.id = entity.id == UINT64_MAX ? 0 : entity.id + 1;
        ContextRootError copied(std::allocator_arg, Allocator(owner), error);
        ContextRootError changed_error(std::allocator_arg, Allocator(owner), error);
        changed_error.expected_entity_type += '!';
        output.emplace("entity", formatted_debug(owner, entity));
        output.emplace("entity_clone", formatted_debug(owner, cloned));
        output.emplace("entity_clone_equal", Json(owner, entity == cloned));
        output.emplace("entity_changed_equal", Json(owner, entity == changed));
        output.emplace("error", formatted_debug(owner, error));
        output.emplace("error_clone", formatted_debug(owner, copied));
        output.emplace("error_clone_equal", Json(owner, error == copied));
        output.emplace("error_changed_equal", Json(owner, error == changed_error));
        output.emplace("display", Json(owner, error.to_string(owner)));
        output.emplace("source_none", Json(owner, error.source() == nullptr));
    } else {
        try {
            auto& evidence =
                user.make<FixEvidence>(text(field(input, "entity")),
                                       text(field(input, "path")),
                                       static_cast<FixEvidenceSource>(
                                           std::get<std::uint64_t>(field(input, "source").data)),
                                       text(field(input, "label")));
            if (auto* mutate = optional(input, "mutate"); mutate && std::get<bool>(mutate->data)) {
                evidence.entity_type.clear();
                evidence.model_path.clear();
                evidence.source_label = "token=raw";
            }
            FixEvidence cloned(std::allocator_arg, Allocator(owner), evidence);
            FixEvidence changed(std::allocator_arg, Allocator(owner), evidence);
            changed.source_label += '!';
            output.emplace("evidence", formatted_debug(owner, evidence));
            output.emplace("clone", formatted_debug(owner, cloned));
            output.emplace("clone_equal", Json(owner, evidence == cloned));
            output.emplace("changed_equal", Json(owner, evidence == changed));
            output.emplace("source", formatted_debug(owner, evidence.source));
        } catch (const std::invalid_argument& error) {
            output.emplace("panic", Json(owner, error.what()));
        }
    }
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid Context model fixture");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
