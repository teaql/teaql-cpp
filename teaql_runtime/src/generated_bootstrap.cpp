#include <teaql/runtime/context.hpp>

namespace teaql {
bool generated_bootstrap_retryable_conflict(const RuntimeError& error) {
    if (error.kind() == RuntimeError::Kind::OptimisticLockConflict) {
        return true;
    }
    if (error.kind() != RuntimeError::Kind::Graph) {
        return false;
    }
    auto message = error.message();
    return (message.find("UNIQUE constraint failed:") != std::string_view::npos &&
            message.find("_data.id") != std::string_view::npos) ||
           (message.find("duplicate key value violates unique constraint") !=
                std::string_view::npos &&
            message.find("_data_pkey") != std::string_view::npos) ||
           (message.find("Duplicate entry") != std::string_view::npos &&
            message.find("PRIMARY") != std::string_view::npos);
}

namespace detail {
class GeneratedBootstrapSequence {
public:
    using allocator_type = Allocator;

    GeneratedBootstrapSequence(std::allocator_arg_t,
                               Allocator allocator,
                               const UserContext& user_context,
                               std::span<const GeneratedSchemaBootstrap> steps)
        : resource_(allocator.resource()), user_context_(user_context),
          steps_(steps.begin(), steps.end(), resource_) {
        for (auto step : steps_) {
            if (!step) {
                throw std::invalid_argument("Generated bootstrap recipe is required");
            }
        }
    }

    std::optional<RuntimeSchemaResult> poll(WakeSignal& wake) {
        if (!is_generated_schema_bootstrap(user_context_)) {
            return RuntimeSchemaResult::err(
                resource_,
                RuntimeError::schema(
                    resource_, "generated bootstrap sequence requires schema bootstrap scope"));
        }
        while (index_ < steps_.size()) {
            if (!child_) {
                child_ = &steps_[index_](user_context_);
                if (child_->memory_resource() != resource_) {
                    child_->cancel();
                    child_ = nullptr;
                    throw std::invalid_argument(
                        "Generated bootstrap recipe must use its UserContext");
                }
            }
            forwarding_.attach(child_->wake_signal(), wake);
            if (!child_->poll()) {
                return {};
            }
            forwarding_.reset();
            auto result = child_->take_result();
            child_ = nullptr;
            if (!result) {
                if (attempt_ < 4 && generated_bootstrap_retryable_conflict(result.error())) {
                    ++attempt_;
                    index_ = 0;
                    wake.notify();
                    return {};
                }
                return result;
            }
            ++index_;
        }
        return RuntimeSchemaResult::ok(resource_, {});
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
    }

private:
    Memory resource_;
    const UserContext& user_context_;
    Vector<GeneratedSchemaBootstrap> steps_;
    GeneratedSchemaBootstrapFuture* child_ = nullptr;
    std::size_t index_ = 0;
    unsigned attempt_ = 0;
    WakeSignal::Forward forwarding_;
};
}

GeneratedSchemaBootstrapFuture&
run_generated_schema_bootstrap(const UserContext& user_context,
                               std::span<const GeneratedSchemaBootstrap> recipes) {
    return make_operation<RuntimeSchemaResult, detail::GeneratedBootstrapSequence>(
        user_context.memory_resource(), user_context, recipes);
}
}
