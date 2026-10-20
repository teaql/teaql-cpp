#pragma once
#include <teaql/runtime/detail/data_service/dynamic_loading.hpp>
#include <teaql/runtime/detail/read_hydration.hpp>

namespace teaql::detail {
// One typed decode path; preparation decides whether persistent extensions are selected.
template <class E, Entity T>
class DynamicEntityListOperation {
public:
    using allocator_type = Allocator;
    using Error = typename E::Error;
    using Output = Result<SmartList<T>, DataServiceError<Error>>;

    DynamicEntityListOperation(std::allocator_arg_t,
                               Allocator a,
                               const EntityDataService<E>& service,
                               const SelectQuery& query)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, query),
          rows_(a.resource()) {}

    DynamicEntityListOperation(std::allocator_arg_t,
                               Allocator a,
                               const EntityDataService<E>& service,
                               SelectQuery&& query)
        : service_(std::allocator_arg, a, service), query_(std::allocator_arg, a, std::move(query)),
          rows_(a.resource()) {}

    std::optional<Output> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        const auto& user_context = EntityDataServiceAccess::context(service_);
        if (!fetched_) {
            if (!native_) {
                if (query_.dynamic_field_selection &&
                    !EntityDataServiceAccess::has_request_intent(service_)) {
                    auto scoped = EntityDataServiceAccess::query_scoped(service_, query_);
                    if (!scoped) {
                        return runtime(std::move(scoped).error());
                    }
                    service_ = std::move(scoped).value();
                }
                auto prepared = EntityDataServiceAccess::prepare_select(service_, query_);
                if (!prepared) {
                    return runtime(std::move(prepared).error());
                }
                query_ = std::move(prepared).value();
                auto intent = EntityDataServiceAccess::request_intent(service_, query_);
                if (!intent) {
                    return runtime(std::move(intent).error());
                }
                EntityDataServiceAccess::bind_query_intent(service_, intent.value());
                ensure_identity_projection(user_context, query_);
                native_ = &make_operation<RelationRowsResult<E>, PreparedEntityQueryOperation<E>>(
                    r, service_, query_, true, true);
            }
            forward_.attach(native_->wake_signal(), wake);
            auto* result = native_->poll();
            if (!result) {
                return {};
            }
            forward_.reset();
            if (!*result) {
                return Output::err(r, copy_or_take_error(*native_, *result));
            }
            rows_ = Vector<CompactRow>(result->value(), r);
            native_ = nullptr;
            fetched_ = true;
        }
        if (!rows_.empty() && query_.dynamic_field_selection) {
            if (!T::supports_dynamic_field_load()) {
                return Output::err(
                    r,
                    DataServiceError<Error>::entity(
                        r,
                        EntityError(
                            std::allocator_arg,
                            Allocator(r),
                            T::ENTITY_NAME,
                            "dynamic fields require a runtime-owned indexed entity carrier")));
            }
            if (!dynamic_) {
                auto intent = EntityDataServiceAccess::request_intent(service_, query_);
                if (!intent) {
                    return runtime(std::move(intent).error());
                }
                dynamic_ = &EntityDataServiceAccess::load_dynamic_fields_into_rows(
                    service_,
                    rows_,
                    T::ENTITY_NAME,
                    *query_.dynamic_field_selection,
                    intent.value());
            }
            forward_.attach(dynamic_->wake_signal(), wake);
            auto* result = dynamic_->poll();
            if (!result) {
                return {};
            }
            forward_.reset();
            if (!*result) {
                return Output::err(r, copy_or_take_error(*dynamic_, *result));
            }
        }
        auto& root = user_context.template make<EntityRuntimeState>();
        auto decoded = decode_compact_rows_with_read_metadata<T>(r, std::move(rows_), root);
        if (!decoded) {
            return Output::err(r, DataServiceError<Error>::entity(r, std::move(decoded).error()));
        }
        SmartList<T> list(std::allocator_arg, Allocator(r), std::move(decoded).value());
        list.loaded();
        return Output::ok(r, std::move(list));
    }

    void cancel_and_wait() noexcept {
        forward_.reset();
        if (native_) {
            native_->cancel();
            native_ = nullptr;
        }
        if (dynamic_) {
            dynamic_->cancel();
            dynamic_ = nullptr;
        }
    }

    void finish_and_wait() noexcept {
        cancel_and_wait();
    }

private:
    Output runtime(RuntimeError error) const {
        auto r = service_.memory_resource();
        return Output::err(r, DataServiceError<Error>::runtime(r, std::move(error)));
    }

    EntityDataService<E> service_;
    SelectQuery query_;
    Vector<CompactRow> rows_;
    AsyncOperation<RelationRowsResult<E>>* native_ = nullptr;
    AsyncOperation<typename DynamicRowsOperation<Error>::Output>* dynamic_ = nullptr;
    WakeSignal::Forward forward_;
    bool fetched_ = false;
};
}
