// Controlled original queue/capturing fixtures; no service logic.
#pragma once
#include "root_fixture.hpp"

namespace original_runtime_queue {
using namespace original_runtime_root;

struct QueueExecutor final : QueryExecutor<StubError>, MutationExecutor<StubError> {
    using allocator_type = Allocator;
    Memory owner;
    Vector<Vector<Record>> batches;
    Vector<SelectQuery> queries;
    bool repeat = false;
    std::size_t next = 0;

    QueueExecutor(std::allocator_arg_t, Allocator a, bool repeat = false)
        : owner(a.resource()), batches(owner), queries(owner), repeat(repeat) {}

    Memory memory_resource() const noexcept {
        return owner;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, StubError>& query(Memory target,
                                                              QueryRequest request) override {
        queries.emplace_back(request.query);
        const auto now = std::chrono::system_clock::now();
        ExecutionMetadata metadata(
            std::allocator_arg, Allocator(target), DataServiceOperation::Query, now, now);
        metadata.backend = repeat ? "capture" : "queue";
        metadata.result_count = repeat && !batches.empty() ? batches[0].size() : 0;
        QueryResult result(std::allocator_arg, Allocator(target), std::move(metadata));
        auto index = repeat ? 0 : next++;
        if (index < batches.size()) {
            for (const auto& row : batches[index]) {
                result.rows.emplace_back(row);
            }
        }
        return make_operation<Result<QueryResult, StubError>, Ready<QueryResult>>(
            target, std::move(result));
    }

    DataServiceOperationResult<MutationResult, StubError>& mutate(Memory target,
                                                                  MutationRequest) override {
        const auto now = std::chrono::system_clock::now();
        ExecutionMetadata metadata(
            std::allocator_arg, Allocator(target), DataServiceOperation::Update, now, now);
        metadata.backend = "queue";
        metadata.affected_rows = 1;
        MutationResult result(std::allocator_arg, Allocator(target), 1, std::move(metadata));
        return make_operation<Result<MutationResult, StubError>, Ready<MutationResult>>(
            target, std::move(result));
    }
};

static Record record(Memory owner,
                     std::initializer_list<std::pair<std::string_view, Value>> fields) {
    Record result(owner);
    for (const auto& [field, value] : fields) {
        result.emplace(String(field, owner), Value(owner, value));
    }
    return result;
}

}
