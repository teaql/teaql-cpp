#include <teaql/runtime/data_service.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

template <unsigned Kind>
struct Executor final : QueryExecutor<String>,
                        MutationExecutor<String>,
                        TransactionExecutor<String> {
    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("factory queried");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("factory mutated");
    }

    DataServiceOperationResult<TransactionHandle<String>, String>& begin(Memory) override {
        throw std::logic_error("factory began transaction");
    }
};

struct Registry final : EntityRegistry {
    Memory r;

    explicit Registry(Memory r) : r(r) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    bool contains(std::string_view entity) const override {
        return entity == "Payment";
    }
};

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    user_context.disable_sql_log();
    if (std::get<bool>(field(input, "metadata").data)) {
        auto& metadata = user_context.make<InMemoryMetadataStore>();
        if (std::get<bool>(field(input, "described").data)) {
            EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Payment");
            metadata.register_entity(std::move(descriptor));
        }
        user_context.set_metadata(user_context, metadata);
    }
    if (std::get<bool>(field(input, "registry").data)) {
        auto& registry = user_context.make_resource<Registry>(r);
        user_context.set_entity_registry(user_context, registry);
    }
    auto mode = text(field(input, "executor"));
    if (mode != "none") {
        auto& executor = user_context.make_resource<Executor<0>>();
        if (mode == "registered") {
            user_context.register_executor(user_context, executor);
        } else {
            user_context.insert_resource(user_context, executor);
        }
    }
    auto output = detail::model_json(r);
    auto inspect = [&]<class E>() {
        auto result = user_context.entity_data_service<E>(text(field(input, "entity")));
        if (result) {
            auto& service = result.value().get();
            using Access = detail::EntityDataServiceAccess;
            const auto& context = Access::context_service(service);
            detail::model_put(output, "entity", service.entity());
            detail::model_put(output, "trace", Access::trace(service).size());
            detail::model_put(output, "query_intent", Access::has_request_intent(service));
            detail::model_put(output, "query_privacy", Access::has_intent(service));
            detail::model_put(output, "mutation_intent", context.mutation_intent() != nullptr);
            detail::model_put(output, "mutation_privacy", context.mutation_privacy() != nullptr);
        } else {
            const auto& error = result.error();
            if (error.kind() == ContextError::Kind::MissingEntityDataService) {
                if (error.name() != text(field(input, "entity"))) {
                    throw std::logic_error("wrong missing entity");
                }
                detail::model_put(output, "error", "entity");
            } else if (error.kind() == ContextError::Kind::MissingResource) {
                if (error.name() != "metadata") {
                    throw std::logic_error("unexpected named resource");
                }
                detail::model_put(output, "error", "metadata");
            } else {
                if (error.name() != detail::resource_type_name<E>()) {
                    throw std::logic_error("wrong executor type");
                }
                detail::model_put(output, "error", "executor");
            }
        }
    };
    if (std::get<bool>(field(input, "other_type").data)) {
        inspect.template operator()<Executor<1>>();
    } else {
        inspect.template operator()<Executor<0>>();
    }
    detail::model_put(output, "executor0", user_context.get_resource<Executor<0>>() != nullptr);
    detail::model_put(output, "executor1", user_context.get_resource<Executor<1>>() != nullptr);
    detail::model_put(
        output, "saver", user_context.get_resource<detail::DynGraphSaver>() != nullptr);
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
