// Retain the existing application SPI fixtures and their complete bodies.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#define main retained_schema_driver
#include "schema_runtime_probe.cpp"
#undef main
#pragma clang diagnostic pop
#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/runtime/error_debug.hpp>

using DynamicResult = Result<std::monostate, DynamicFieldError>;

struct DynamicStep : Step {
    using allocator_type = Allocator;

    DynamicStep(std::allocator_arg_t,
                Allocator a,
                const UserContext& user_context,
                std::uint64_t delay,
                bool fail)
        : Step(std::allocator_arg, a, user_context, "dynamic", delay, fail) {}

    std::optional<DynamicResult> poll(WakeSignal& wake) {
        note(".poll");
        if (remaining) {
            --remaining;
            wake.notify();
            return {};
        }
        if (fail) {
            return DynamicResult::err(r, DynamicFieldError(r, "DYNAMIC_SCHEMA_FAILED", "fixture"));
        }
        return DynamicResult::ok(r, {});
    }
};

struct DynamicProvider : DynamicFieldsProvider {
    DynamicStorageFuture<std::monostate>&
    ensure_schema(const UserContext& user_context) const override {
        auto& shared = state(user_context);
        shared.note(user_context, "dynamic.create");
        return make_operation<DynamicResult, DynamicStep>(
            user_context.memory_resource(),
            user_context,
            std::get<std::uint64_t>(field(shared.config, "dynamic_delay").data),
            text(field(shared.config, "fail")) == "dynamic");
    }

    DynamicStorageFuture<DynamicFieldBatch>& load_values(const UserContext&,
                                                         std::string_view,
                                                         std::span<const std::uint64_t>,
                                                         const DynamicFieldSelection&,
                                                         const QueryIntent&) const override {
        throw std::logic_error("Schema fixture must not load dynamic values");
    }
};

static Json observe_dynamic(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    auto& shared = user_context.make<Shared>(input);
    user_context.insert_resource(user_context, shared);
    auto& module = user_context.make<RuntimeModule>();
    const GeneratedSchemaBootstrap callbacks[]{first, second, third};
    for (std::size_t i = 0; i < std::get<Json::Array>(field(input, "delays").data).size(); ++i) {
        module.generated_schema_bootstrap(callbacks[i]);
    }
    module.apply_to(user_context);
    if (std::get<bool>(field(input, "provider").data)) {
        auto& provider = user_context.make_resource<Provider>();
        user_context.set_schema_provider(user_context, provider);
    }
    if (std::get<bool>(field(input, "business").data)) {
        auto& business = user_context.make_resource<Business>();
        user_context.set_business_id_infrastructure(user_context, business);
    }
    if (std::get<bool>(field(input, "dynamic").data)) {
        auto& dynamic = user_context.make_resource<DynamicProvider>();
        user_context.set_dynamic_fields_provider(user_context, dynamic);
    }
    shared.note(user_context, "before");
    auto& operation = user_context.ensure_schema();
    Json::Object result(owner);
    result.emplace("cancelled", Json(owner, true));
    for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(input, "polls").data); ++i) {
        auto* ready = operation.poll();
        if (!ready) {
            shared.note(user_context, "outside");
            continue;
        }
        result.clear();
        if (*ready) {
            result.emplace("ok", Json(owner, true));
        } else {
            result.emplace("debug", Json(owner, debug_string(owner, ready->error())));
            result.emplace("display", Json(owner, ready->error().to_string(owner)));
        }
        break;
    }
    operation.cancel();
    shared.note(user_context, "after");
    Json::Array calls(owner);
    for (const auto& call : shared.calls) {
        calls.emplace_back(call);
    }
    Json::Object out(owner);
    out.emplace("calls", Json(owner, std::move(calls)));
    out.emplace("result", Json(owner, std::move(result)));
    return Json(owner, std::move(out));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid schema lifecycle fixture");
            }
            std::cout
                << observe_dynamic(*user_context, *input).dump(user_context->memory_resource())
                << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
