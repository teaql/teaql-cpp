#include <teaql/runtime/detail/data_service/entity_query.hpp>
#include <iostream>

using namespace teaql;

struct Counts {
    unsigned get = 0, put = 0, query = 0;
};

struct Cache final : AggregationCacheBackend {
    using allocator_type = Allocator;
    Memory r;
    Counts& counts;
    bool hit;

    Cache(std::allocator_arg_t, Allocator a, Counts& counts, bool hit)
        : r(a.resource()), counts(counts), hit(hit) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    std::string_view name_space() const noexcept override {
        return "intent-order";
    }

    std::optional<Vector<CompactRow>>
    get(Memory target, std::string_view, std::uint64_t) const override {
        ++counts.get;
        if (hit) {
            return Vector<CompactRow>(target);
        }
        return {};
    }

    void put(std::string_view, std::span<const CompactRow>) override {
        ++counts.put;
    }

    void invalidate_namespace(std::string_view) override {}
};

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    Counts& counts;

    Executor(std::allocator_arg_t, Allocator a, Counts& counts) : r(a.resource()), counts(counts) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Ready {
        using allocator_type = Allocator;
        Memory r;

        Ready(std::allocator_arg_t, Allocator a) : r(a.resource()) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal&) {
            QueryResult result(
                std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, 0));
            return Result<QueryResult, String>::ok(r, std::move(result));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory target, QueryRequest) override {
        ++counts.query;
        return make_operation<Result<QueryResult, String>, Ready>(target);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

int main() {
    unsigned case_index = 0;
    for (std::string_view entry : {"borrowed", "owned", "compact"}) {
        for (bool hit : {false, true}) {
            for (std::string_view intent : {"missing_comment", "missing_purpose", "valid"}) {
                Counts counts;
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto& cache = user_context->make<Cache>(counts, hit);
                user_context->insert_resource(*user_context,
                                              static_cast<AggregationCacheBackend&>(cache));
                auto& executor = user_context->make<Executor>(counts);
                auto& service = EntityDataService<Executor>::for_executor(
                    *user_context, "Item", *user_context, executor);
                SelectQuery query(std::allocator_arg, Allocator(r), "Item");
                query.enable_aggregation_cache();
                if (intent != "missing_comment") {
                    query.comment_text.emplace("read cache boundary", r);
                }
                if (intent != "missing_purpose") {
                    query.purpose_text.emplace("verify prepared intent order", r);
                }
                auto& operation = make_operation<detail::RelationRowsResult<Executor>,
                                                 detail::PreparedEntityQueryOperation<Executor>>(
                    r, service, query, entry == "owned", entry == "compact");
                auto* result = operation.poll();
                if (!result) {
                    throw std::logic_error("ready fixture unexpectedly suspended");
                }
                std::cout << "{\"case\":" << case_index++ << ",\"entry\":\"" << entry
                          << "\",\"hit\":" << std::boolalpha << hit << ",\"intent\":\"" << intent
                          << "\",\"error\":";
                if (*result) {
                    std::cout << "null,\"rows\":" << result->value().size();
                } else {
                    std::cout << Json(r, result->error().to_string(r)).dump(r) << ",\"rows\":null";
                }
                std::cout << ",\"get\":" << counts.get << ",\"put\":" << counts.put
                          << ",\"query\":" << counts.query << "}\n";
            }
        }
    }
}
