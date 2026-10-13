#pragma once
#include <teaql/runtime/detail/data_service/entity_query.hpp>

namespace teaql::detail {
template <class E>
class GraphCurrentRowOperation {
    using R = Result<std::optional<CompactRow>, DataServiceError<typename E::Error>>;

public:
    using allocator_type = Allocator;

    GraphCurrentRowOperation(std::allocator_arg_t,
                             Allocator a,
                             const EntityDataService<E>& service,
                             std::string_view entity,
                             std::string_view id_property,
                             const Value& id,
                             std::span<const TraceNode> trace)
        : service_(std::allocator_arg, a, EntityDataServiceAccess::scoped(service, entity)),
          query_(std::allocator_arg, a, entity) {
        query_.filter(Expr::eq(a.resource(), id_property, id));
        for (const auto& frame : trace) {
            query_.trace_chain.emplace_back(frame);
        }
        EntityDataServiceAccess::capture_readback_target(service_, id);
    }

    std::optional<R> poll(WakeSignal& wake) {
        auto r = service_.memory_resource();
        if (!child_) {
            child_ = &EntityDataServiceAccess::fetch_all(service_, query_);
        }
        forwarding_.attach(child_->wake_signal(), wake);
        auto* result = child_->poll();
        if (!result) {
            return {};
        }
        if (!*result) {
            return R::err(r, copy_or_take_error(*child_, *result));
        }
        std::optional<CompactRow> row;
        if (!result->value().empty()) {
            const auto& selected = result->value().back();
            auto metadata =
                EntityDataServiceAccess::context(service_).require_entity(query_.entity);
            if (!metadata) {
                return R::err(
                    r,
                    DataServiceError<typename E::Error>::runtime(r, std::move(metadata).error()));
            }
            const auto& descriptor = metadata.value().get();
            bool ordered_complete = selected.len() == descriptor.properties.size();
            if (ordered_complete) {
                for (std::size_t i = 0; i < descriptor.properties.size(); ++i) {
                    if (selected.keys()[i] != descriptor.properties[i].name) {
                        ordered_complete = false;
                        break;
                    }
                }
            }
            if (!ordered_complete) {
                for (const auto& property : descriptor.properties) {
                    if (!selected.contains_key(property.name)) {
                        String message("authoritative readback missing mapped field ", r);
                        message += query_.entity;
                        message += '.';
                        message += property.name;
                        return R::err(r,
                                      DataServiceError<typename E::Error>::runtime(
                                          r, RuntimeError::graph(r, message)));
                    }
                }
            }
            row.emplace(std::allocator_arg, Allocator(r), selected);
        }
        return R::ok(r, std::move(row));
    }

    void finish_and_wait() noexcept {
        forwarding_.reset();
        child_ = nullptr;
    }

    void cancel_and_wait() noexcept {
        forwarding_.reset();
        if (child_) {
            child_->cancel();
            child_ = nullptr;
        }
    }

private:
    EntityDataService<E> service_;
    SelectQuery query_;
    AsyncOperation<RelationRowsResult<E>>* child_ = nullptr;
    WakeSignal::Forward forwarding_;
};

template <class E>
auto EntityDataServiceAccess::fetch_graph_current_row_internal(const EntityDataService<E>& service,
                                                               std::string_view entity,
                                                               std::string_view property,
                                                               const Value& id,
                                                               std::span<const TraceNode> trace)
    -> AsyncOperation<Result<std::optional<CompactRow>, DataServiceError<typename E::Error>>>& {
    using R = Result<std::optional<CompactRow>, DataServiceError<typename E::Error>>;
    return make_operation<R, GraphCurrentRowOperation<E>>(
        service.memory_resource(), service, entity, property, id, trace);
}

}
