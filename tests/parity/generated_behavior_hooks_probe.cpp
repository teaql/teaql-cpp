#include "../generated_checkers/include/cpp_checker_fixture.hpp"
#include "../generated_inheritance_variants/include/cpp_inheritance_variants.hpp"
#include "../generated_constants_numeric/include/cpp_constants_numeric.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/core/mutation_debug.hpp>
#include <iostream>

using namespace teaql;

struct Settings {
    using allocator_type = Allocator;
    String text;
    mutable Vector<String> hooks;

    Settings(std::allocator_arg_t, Allocator a, std::string_view value)
        : text(value, a.resource()), hooks(a.resource()) {}
};

template <class Base>
class Custom final : public Base {
public:
    using allocator_type = Allocator;
    Memory resource;
    String mode, prefix;

    Custom(std::allocator_arg_t, Allocator a, std::string_view kind, std::string_view tag)
        : Base(std::allocator_arg, a), resource(a.resource()), mode(kind, resource),
          prefix(tag, resource) {}

    RuntimeHookResult
    apply(Memory r, const UserContext& user_context, std::string_view hook, String& entity) const {
        const auto& settings = user_context.require_resource<Settings>().value().get();
        settings.hooks.emplace_back(hook);
        entity += prefix;
        entity += settings.text;
        if (mode == "deny") {
            return RuntimeHookResult::err(r, RuntimeError::graph(r, "business behavior denied"));
        }
        return RuntimeHookResult::ok(r, {});
    }

#define BUSINESS_HOOK(method, type, label)                                                         \
    RuntimeHookResult method(Memory r, const UserContext& user_context, type& command)             \
        const override {                                                                           \
        auto result = Base::method(r, user_context, command);                                      \
        if (!result) {                                                                             \
            return result;                                                                         \
        }                                                                                          \
        return apply(r, user_context, label, command.entity);                                      \
    }
    BUSINESS_HOOK(before_select, SelectQuery, "select")
    BUSINESS_HOOK(before_insert, InsertCommand, "insert")
    BUSINESS_HOOK(before_update, UpdateCommand, "update")
    BUSINESS_HOOK(before_delete, DeleteCommand, "delete")
    BUSINESS_HOOK(before_recover, RecoverCommand, "recover")
#undef BUSINESS_HOOK

    Vector<String> relation_loads(Memory r, const UserContext& user_context) const override {
        const auto& settings = user_context.require_resource<Settings>().value().get();
        settings.hooks.emplace_back("relations");
        auto paths = Base::relation_loads(r, user_context);
        String path(prefix, r);
        path += settings.text;
        paths.emplace_back(std::move(path));
        return paths;
    }
};

#define BUSINESS_MODULE(domain, model, behavior)                                                   \
    [&]() -> RuntimeModule& {                                                                      \
        auto& module = module_mode == "plain"     ? domain::module(user_context)                   \
                       : module_mode == "checker" ? domain::moduleWithCheckers(user_context)       \
                       : module_mode == "behavior"                                                 \
                           ? domain::moduleWithBehaviors(user_context)                             \
                           : domain::moduleWithBehaviorsAndCheckers(user_context);                 \
        if (mode != "default") {                                                                   \
            auto& first = user_context.make<Custom<domain::behavior>>(mode, "first:");             \
            module.entity_with_behavior<domain::model>(user_context, first);                       \
            if (replacement) {                                                                     \
                auto& next = user_context.make<Custom<domain::behavior>>(mode, "next:");           \
                module.entity_with_behavior<domain::model>(user_context, next);                    \
            }                                                                                      \
        }                                                                                          \
        return module;                                                                             \
    }()

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto mode = text(field(input, "mode")), module_mode = text(field(input, "module"));
    auto model = text(field(input, "model"));
    bool replacement = std::get<bool>(field(input, "replacement").data);
    auto& module = model == "note" ? BUSINESS_MODULE(cpp_checker_fixture, Note, NoteBehavior)
                   : model == "invoice"
                       ? BUSINESS_MODULE(cpp_inheritance_variants, Invoice, InvoiceBehavior)
                       : BUSINESS_MODULE(cpp_constants_numeric, Priority, PriorityBehavior);
    module.apply_to(user_context);
    auto& settings = user_context.make<Settings>(text(field(input, "text")));
    user_context.insert_resource(user_context, settings);
    std::string_view name = model == "note" ? "Note" : model == "invoice" ? "Invoice" : "Priority";
    auto behavior = user_context.entity_data_service_behavior(name);
    auto operation = text(field(input, "operation"));
    Json command_text(r), error(r);
    Vector<String> paths(r);
    auto invoke = [&](auto command, auto method) {
        if (behavior) {
            auto result = (behavior->get().*method)(r, user_context, command);
            if (!result) {
                error = Json(r, result.error().to_string(r));
            }
        }
        command_text = Json(r, debug_string(r, command));
    };
    if (operation == "select") {
        invoke(SelectQuery(std::allocator_arg, Allocator(r), name),
               &EntityDataServiceBehavior::before_select);
    } else if (operation == "insert") {
        auto command = InsertCommand(std::allocator_arg, Allocator(r), name);
        command.value("name", Value(r, settings.text));
        invoke(std::move(command), &EntityDataServiceBehavior::before_insert);
    } else if (operation == "update") {
        auto command = UpdateCommand(std::allocator_arg, Allocator(r), name, std::uint64_t(7));
        command.value("name", Value(r, settings.text));
        invoke(std::move(command), &EntityDataServiceBehavior::before_update);
    } else if (operation == "delete") {
        invoke(DeleteCommand(std::allocator_arg, Allocator(r), name, std::uint64_t(7)),
               &EntityDataServiceBehavior::before_delete);
    } else if (operation == "recover") {
        invoke(RecoverCommand(std::allocator_arg, Allocator(r), name, std::uint64_t(7), -3),
               &EntityDataServiceBehavior::before_recover);
    } else if (behavior) {
        paths = behavior->get().relation_loads(r, user_context);
    }
    Json::Object result(r);
    result.emplace(String("present", r), Json(r, bool(behavior)));
    result.emplace(String("command", r), std::move(command_text));
    Json::Array path_values(r), hook_values(r);
    for (const auto& path : paths) {
        path_values.emplace_back(path);
    }
    for (const auto& hook : settings.hooks) {
        hook_values.emplace_back(hook);
    }
    result.emplace(String("paths", r), Json(r, path_values));
    result.emplace(String("error", r), std::move(error));
    result.emplace(String("hooks", r), Json(r, hook_values));
    return Json(r, result);
}

#undef BUSINESS_MODULE

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
                    throw std::runtime_error("invalid business-behavior fixture");
                }
                auto output = observe(*user_context, *input);
                retained = detail::clone_in(global.memory_resource(), output);
            }
            user_context.reset();
            std::cout << retained.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
