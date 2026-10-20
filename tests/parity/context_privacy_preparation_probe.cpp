#include "../support/runtime_mutation_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;
using Access = detail::EntityDataServiceAccess;

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    user_context.disable_sql_log();
    auto& metadata = user_context.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Payment");
    descriptor.property("secret", DataType::Text);
    descriptor.mask_fields({"secret"});
    metadata.register_entity(std::move(descriptor));
    user_context.set_metadata(user_context, metadata);
    auto& state = user_context.make<mutation_fixture::State>();
    auto& executor = user_context.make_resource<mutation_fixture::Executor>(state);
    auto& service = EntityDataService<mutation_fixture::Executor>::for_executor(
        user_context, "Payment", user_context, executor);
    auto& plan = user_context.make<GraphMutationPlan>();
    if (text(field(input, "plan")) == "secret") {
        MutationValues values(r);
        values.fields.emplace(String("secret", r), Value(r, "PRIVATE-VALUE"));
        plan.push("Payment", GraphMutationKind::Update, values, {});
        plan.rebuild_batches();
    }
    auto prepared = Access::bind_plan_privacy(service, plan);
    auto output = detail::model_json(r);
    if (!prepared) {
        detail::model_put(output, "prepare_error", prepared.error().to_string(r));
        return output;
    }
    detail::model_put(output, "prepared", true);
    detail::model_put(output, "query_privacy", Access::has_intent(service));
    auto context = detail::clone_in(r, Access::context_service(service));
    detail::model_put(output, "prepared_intent", context.mutation_intent() != nullptr);
    detail::model_put(output,
                      "prepared_privacy",
                      context.mutation_privacy() ? Json(r, context.mutation_privacy()->is_empty())
                                                 : Json(r));
    if (std::get<bool>(field(input, "clear").data)) {
        auto clear = []<class C>(C& value) {
            if constexpr (requires { value.with_mutation_privacy(nullptr); }) {
                value.with_mutation_privacy(nullptr);
            } else {
                throw std::logic_error("independent privacy clearing is unsupported");
            }
        };
        clear(context);
    }
    auto intent = MutationIntent::create(r, "persist payment");
    bool bind = std::get<bool>(field(input, "bind").data);
    auto& command = user_context.make<InsertCommand>("Payment");
    command.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    auto collect = [&](auto& work) {
        const auto& result = work.wait();
        if (result) {
            detail::model_put(output, "rows", result.value());
        } else {
            detail::model_put(output, "error", result.error().to_string(r));
        }
    };
    auto route = text(field(input, "route"));
    if (route == "context") {
        if (bind) {
            context.with_mutation_intent(intent.value());
        }
        collect(context.insert(command));
    } else {
        if (bind && route == "bind_then_forward") {
            context.with_mutation_intent(intent.value());
        }
        auto base = context.data_service();
        if (bind && route == "forward_then_bind") {
            base.with_mutation_intent(intent.value());
        }
        collect(base.insert(command));
    }
    detail::model_put(output, "requests", state.requests.size());
    Json privacy(r);
    if (!state.requests.empty() && state.requests.front().diagnostic_redactions()) {
        privacy = Json(r, state.requests.front().diagnostic_redactions()->is_empty());
    }
    detail::model_put(output, "request_privacy", privacy);
    return output;
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << evaluate(*user_context, *input).dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
