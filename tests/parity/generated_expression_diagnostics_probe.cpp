#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "../generated_candidates/include/cpp_candidates.hpp"
#include "../generated_inheritance/include/cpp_inheritance.hpp"
#include <iostream>
using namespace teaql;

static CompactRow row(Memory r, std::uint64_t id) {
    CompactRow result(std::allocator_arg, Allocator(r));
    result.insert("id", Value(r, id));
    result.insert("version", Value(r, std::int64_t(1)));
    return result;
}

static void generated_access(UserContext& user_context, std::string_view mode, std::uint64_t id) {
    auto r = user_context.memory_resource();
    if (mode.starts_with("constant_")) {
        auto record = cpp_candidates::Record::from_compact_row(r, row(r, id)).value();
        auto E = cpp_candidates::E::record(record);
        if (mode == "constant_draft") {
            (void)E.statusIsDraft().unwrap();
        } else if (mode == "constant_progress") {
            (void)E.statusIsInProgress().eval();
        } else if (mode == "constant_done") {
            (void)E.statusIsDone().orIfNull(false);
        } else if (mode == "constant_status") {
            (void)E.getStatus().unwrap();
        } else if (mode == "constant_phase") {
            (void)E.getPhase().unwrap();
        } else {
            throw std::logic_error("unknown constant mode");
        }
        return;
    }
    if (mode.starts_with("inherited_")) {
        auto academy = cpp_inheritance::ResearchAcademy::from_compact_row(r, row(r, id)).value();
        auto E = cpp_inheritance::E::researchAcademy(academy);
        if (mode == "inherited_name") {
            (void)E.getName().unwrap();
        } else if (mode == "inherited_campus") {
            (void)E.getCampusCode().eval();
        } else if (mode == "inherited_budget") {
            (void)E.getBudget().unwrap();
        } else {
            throw std::logic_error("unknown inherited mode");
        }
        return;
    }
    auto fields = row(r, id);
    if (mode == "loaded_platform_name") {
        Record platform(r);
        platform.emplace(String("id", r), Value(r, std::uint64_t(2)));
        platform.emplace(String("version", r), Value(r, std::int64_t(1)));
        fields.insert("platform", Value(r, platform));
    }
    auto person = cpp_dynamic_json::Person::from_compact_row(r, std::move(fields)).value();
    auto E = cpp_dynamic_json::E::person(person);
    if (mode == "name") {
        (void)E.getName().unwrap();
    } else if (mode == "name_eval") {
        (void)E.getName().eval();
    } else if (mode == "fallback") {
        (void)E.getName().orIfNull(String("fallback", r));
    } else if (mode == "fallback_callback") {
        (void)E.getName().orElseIfNull([]() -> String {
            throw std::logic_error("fallback was invoked");
        });
    } else if (mode == "fallback_default") {
        (void)E.getName().orDefaultIfNull();
    } else if (mode == "platform_name" || mode == "loaded_platform_name") {
        (void)E.getPlatform().getName().unwrap();
    } else if (mode == "platform_id") {
        (void)E.getPlatformId().unwrap();
    } else if (mode == "profile_note") {
        (void)E.getProfile().getNote().unwrap();
    } else if (mode == "orders_first_code") {
        (void)E.getOrders().first().getCode().unwrap();
    } else if (mode == "orders_get_code") {
        (void)E.getOrders().get(100).getCode().unwrap();
    } else if (mode == "orders_eval") {
        (void)E.getOrders().eval();
    } else if (mode == "orders_unwrap") {
        (void)E.getOrders().unwrap();
    } else if (mode == "list") {
        (void)E.getOrders().size().unwrap();
    } else {
        throw std::logic_error("unknown person mode");
    }
}

int main() {
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        const auto& object = std::get<Json::Object>(input->data);
        auto text = [&](std::string_view key) -> const String& {
            return std::get<String>(object.find(key)->second.data);
        };
        if (auto generated = object.find("generated"); generated != object.end()) {
            const auto& mode = std::get<String>(generated->second.data);
            auto id = std::get<std::uint64_t>(object.find("id")->second.data);
            try {
                generated_access(*user_context, mode, id);
                throw std::logic_error("Generated access did not fail closed");
            } catch (const ExpressionNotLoadedError& error) {
                std::cout << error.diagnostic().to_json(r).dump(r) << '\n';
            }
            continue;
        }
        ExpressionDiagnostic diagnostic(
            std::allocator_arg, Allocator(r), text("root"), text("node"), text("path"));
        std::cout << diagnostic.to_json(r).dump(r) << '\n';
    }
}
