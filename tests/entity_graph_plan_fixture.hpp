#pragma once
#include <teaql/runtime/data_service.hpp>

namespace graph_plan_fixture {
using namespace teaql;

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory resource;
    Vector<QueryRequest> requests;
    Vector<Record> rows;
    String failure_entity;
    bool pending = false;
    unsigned cancelled = 0;
    unsigned* external_cancelled = nullptr;
    WakeSignal* child_wake = nullptr;

    Executor(std::allocator_arg_t, Allocator a)
        : resource(a.resource()), requests(resource), rows(resource), failure_entity(resource) {}

    Memory memory_resource() const {
        return resource;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Work {
        using allocator_type = Allocator;
        Memory resource;
        Executor& owner;
        String entity;

        Work(std::allocator_arg_t, Allocator a, Executor& owner, std::string_view entity)
            : resource(a.resource()), owner(owner), entity(entity, resource) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal& wake) {
            owner.child_wake = &wake;
            if (owner.pending) {
                return {};
            }
            if (entity == owner.failure_entity) {
                return Result<QueryResult, String>::err(resource,
                                                        String("lookup failed", resource));
            }
            QueryResult result(std::allocator_arg,
                               Allocator(resource),
                               ExecutionMetadata::unrecorded_query(resource, owner.rows.size()));
            for (const auto& row : owner.rows) {
                result.rows.emplace_back(row);
            }
            return Result<QueryResult, String>::ok(resource, std::move(result));
        }

        void finish_and_wait() noexcept {
            owner.child_wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            owner.child_wake = nullptr;
            ++owner.cancelled;
            if (owner.external_cancelled) {
                ++*owner.external_cancelled;
            }
        }
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        String entity(request.query.entity, r);
        requests.emplace_back(request);
        return make_operation<Result<QueryResult, String>, Work>(r, *this, entity);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("planning must not mutate");
    }
};

inline EntityDescriptor
descriptor(Memory r, std::string_view name, std::string_view id = "id", bool timestamps = false) {
    EntityDescriptor out(std::allocator_arg, Allocator(r), name);
    if (!id.empty()) {
        out.property(id, DataType::U64).id();
    }
    out.property("version", DataType::I64).version();
    out.property("name", DataType::Text);
    if (timestamps) {
        out.property("create_time", DataType::Timestamp);
        out.property("update_time", DataType::Timestamp);
    }
    out.relation("kids", "Child").local(id).foreign("parent_id").to_many();
    return out;
}
}
