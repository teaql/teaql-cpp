#pragma once
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/schema.hpp>

namespace teaql::detail {
// Model recipes describe fields and names; the runtime owns the asynchronous
// query/create/reconcile/save lifecycle and uses the existing audited save path.
template <class Recipe>
class GeneratedBootstrapModelOperation {
public:
    using allocator_type = Allocator;
    using Model = typename Recipe::Model;
    using QueryOperation =
        std::remove_reference_t<decltype(Recipe::query(std::declval<const UserContext&>()))>;
    using SaveOperation = AsyncOperation<Result<Model, RuntimeError>>;

    GeneratedBootstrapModelOperation(std::allocator_arg_t,
                                     Allocator a,
                                     const UserContext& user_context)
        : resource_(a.resource()), user_context_(user_context) {}

    std::optional<RuntimeSchemaResult> poll(WakeSignal& wake) {
        if (!is_generated_schema_bootstrap(user_context_)) {
            return RuntimeSchemaResult::err(
                resource_,
                RuntimeError::schema(resource_,
                                     "generated model bootstrap requires schema bootstrap scope"));
        }
        if (!save_) {
            if (!query_) {
                query_ = &Recipe::query(user_context_);
                require_local(*query_);
            }
            forwarding_.attach(query_->wake_signal(), wake);
            if (!query_->poll()) {
                return {};
            }
            forwarding_.reset();
            auto rows = query_->take_result();
            query_ = nullptr;
            if (!rows) {
                // Original bootstrap wraps the public generated query boundary.
                return RuntimeSchemaResult::err(
                    resource_, RuntimeError::graph(resource_, rows.error().to_string(resource_)));
            }
            bool existing = !rows.value().data.empty();
            auto& entity = existing ? user_context_.template make<Model>(rows.value().data.front())
                                    : user_context_.template make<Model>();
            if constexpr (Recipe::is_root) {
                if (existing) {
                    return install_root(entity);
                }
            }
            if (!existing) {
                entity.attachRuntimeStateRecursive(user_context_.entity_runtime_state());
                Recipe::set_id(entity);
                auto initialized = user_context_.initialize_generated_bootstrap_entity(
                    entity, Model::ENTITY_NAME, Recipe::fixed_id);
                if (!initialized) {
                    return initialized;
                }
            }
            bool changed = Recipe::configure(user_context_, entity, existing);
            if (existing && !changed) {
                return RuntimeSchemaResult::ok(resource_, {});
            }
            save_ = &AuditedSaveExt<Model>::save(
                std::move(entity).audit_as(existing ? Recipe::reconcile_reason
                                                    : Recipe::create_reason),
                user_context_);
            require_local(*save_);
        }
        forwarding_.attach(save_->wake_signal(), wake);
        if (!save_->poll()) {
            return {};
        }
        forwarding_.reset();
        auto saved = save_->take_result();
        save_ = nullptr;
        if (!saved) {
            return RuntimeSchemaResult::err(resource_, std::move(saved).error());
        }
        if constexpr (Recipe::is_root) {
            return install_root(saved.value());
        }
        return RuntimeSchemaResult::ok(resource_, {});
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (query_) {
            query_->cancel();
            query_ = nullptr;
        }
        if (save_) {
            save_->cancel();
            save_ = nullptr;
        }
    }

private:
    RuntimeSchemaResult install_root(const Model& entity) {
        return user_context_.set_generated_bootstrap_active_root(Model::ENTITY_NAME, entity.id());
    }

    template <class Operation>
    void require_local(Operation& operation) {
        if (operation.memory_resource() != resource_) {
            operation.cancel();
            throw std::invalid_argument("Generated model bootstrap must use its UserContext");
        }
    }

    Memory resource_;
    const UserContext& user_context_;
    QueryOperation* query_ = nullptr;
    SaveOperation* save_ = nullptr;
    WakeSignal::Forward forwarding_;
};
}

namespace teaql {
template <class Recipe>
GeneratedSchemaBootstrapFuture& run_generated_bootstrap_model(const UserContext& user_context) {
    return make_operation<RuntimeSchemaResult, detail::GeneratedBootstrapModelOperation<Recipe>>(
        user_context.memory_resource(), user_context);
}
}
