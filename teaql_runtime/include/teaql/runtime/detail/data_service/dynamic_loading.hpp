#pragma once
#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/runtime/detail/data_service/entity_service.hpp>

namespace teaql::detail {
template <class Error>
class DynamicRowsOperation {
public:
    using allocator_type = Allocator;
    using Output = Result<std::monostate, DataServiceError<Error>>;

    DynamicRowsOperation(std::allocator_arg_t,
                         Allocator a,
                         const UserContext& user_context,
                         std::span<CompactRow> rows,
                         std::string_view owner,
                         const DynamicFieldSelection& selection,
                         const QueryIntent& intent)
        : user_context_(user_context), rows_(rows), owner_(owner, a.resource()), ids_(a.resource()),
          selection_(std::allocator_arg, a, selection), intent_(std::allocator_arg, a, intent) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = owner_.get_allocator().resource();
        if (rows_.empty()) {
            return Output::ok(r, {});
        }
        if (!pending_) {
            ids_.reserve(rows_.size());
            for (const auto& row : rows_) {
                auto* value = row.get("id");
                auto id = value ? value->try_u64() : std::optional<std::uint64_t>{};
                if (!id) {
                    return Output::err(r,
                                       DataServiceError<Error>::entity(
                                           r,
                                           EntityError(std::allocator_arg,
                                                       Allocator(r),
                                                       owner_,
                                                       "dynamic-field owner ID is not loaded")));
                }
                ids_.push_back(*id);
            }
            pending_ = &user_context_.load_dynamic_fields(owner_, ids_, selection_, intent_);
        }
        forward_.attach(pending_->wake_signal(), wake);
        auto* result = pending_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return failure(result->error());
        }
        auto batch = clone_in(r, result->value());
        auto values = std::move(batch).into_values(owner_, ids_, selection_);
        if (!values) {
            return failure(values.error());
        }
        DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(r));
        for (std::size_t i = 0; i < rows_.size(); ++i) {
            auto merged = rows_[i].merge_loaded_dynamic_fields(values.value()[i], shapes);
            if (!merged) {
                return failure(merged.error());
            }
        }
        return Output::ok(r, {});
    }

    void cancel_and_wait() noexcept {
        forward_.reset();
        if (pending_) {
            pending_->cancel();
            pending_ = nullptr;
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

private:
    Output failure(const DynamicFieldError& error) const {
        auto r = owner_.get_allocator().resource();
        return Output::err(
            r, DataServiceError<Error>::runtime(r, RuntimeError::dynamic_field(r, error)));
    }

    const UserContext& user_context_;
    std::span<CompactRow> rows_;
    String owner_;
    Vector<std::uint64_t> ids_;
    DynamicFieldSelection selection_;
    QueryIntent intent_;
    DynamicStorageFuture<DynamicFieldBatch>* pending_ = nullptr;
    WakeSignal::Forward forward_;
};
}
