#include <teaql/runtime/detail/data_service/entity_query.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace teaql;
using Access = detail::EntityDataServiceAccess;

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    std::vector<std::string>& classified;

    Executor(std::allocator_arg_t, Allocator a, std::vector<std::string>& calls)
        : r(a.resource()), classified(calls) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    SqlIntentRedactions query_log_intent(Memory target, const SelectQuery& query) const override {
        classified.emplace_back(query.entity);
        SqlIntentRedactions intent(target);
        intent.capture_target_id(Value(target, query.entity));
        return intent;
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
        return make_operation<Result<QueryResult, String>, Ready>(target);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("classification must not mutate");
    }
};

void snapshot(const EntityDataService<Executor>& service) {
    auto r = service.memory_resource();
    std::cout << "{\"entity\":" << Json(r, service.entity()).dump(r) << ",\"privacy\":";
    auto privacy = Access::intent(service);
    if (privacy) {
        Vector<String> secrets(r);
        privacy->extend_secrets(false, secrets);
        std::sort(secrets.begin(), secrets.end());
        std::cout << '[';
        for (std::size_t i = 0; i < secrets.size(); ++i) {
            if (i) {
                std::cout << ',';
            }
            std::cout << Json(r, secrets[i]).dump(r);
        }
        std::cout << ']';
    } else {
        std::cout << "null";
    }
    SelectQuery empty(std::allocator_arg, Allocator(r), "Unused");
    auto intent = Access::request_intent(service, empty);
    std::cout << ",\"intent\":";
    if (intent) {
        std::cout << '[' << Json(r, intent.value().comment()).dump(r) << ','
                  << Json(r, intent.value().purpose()).dump(r) << ']';
    } else {
        std::cout << "null";
    }
    std::cout << '}';
}

int main() {
    unsigned case_index = 0;
    for (bool diagnostic : {false, true}) {
        for (bool capture : {false, true}) {
            for (bool bound : {false, true}) {
                std::vector<std::string> calls;
                GlobalContext global;
                auto user_context = global.make_user_context();
                user_context->set_sql_log_options(SqlLogOptions{capture, false});
                auto r = user_context->memory_resource();
                auto& executor = user_context->make<Executor>(calls);
                auto& parent = EntityDataService<Executor>::for_executor(
                    *user_context, "Parent", *user_context, executor);
                if (bound) {
                    auto intent = QueryIntent::from_optional(r, "bound comment", "bound purpose");
                    Access::bind_query_intent(parent, intent.value());
                }
                SelectQuery query(std::allocator_arg, Allocator(r), "Parent");
                query.comment_text.emplace("read parent", r);
                PurposedSelectQuery wrapped(
                    std::allocator_arg, Allocator(r), query, "verify aggregate scope");
                if (diagnostic) {
                    wrapped.for_exact_count("count");
                }
                Vector<RelationAggregate> aggregates(r);
                SelectQuery child(std::allocator_arg, Allocator(r), "AggregateSecret");
                aggregates.emplace_back("children", "total", child, true);
                auto& result =
                    parent.fetch_smart_list_with_relation_aggregates(wrapped, aggregates).wait();
                std::cout << "{\"case\":" << case_index++ << ",\"diagnostic\":" << std::boolalpha
                          << diagnostic << ",\"capture\":" << capture << ",\"bound\":" << bound
                          << ",\"error\":";
                if (result) {
                    std::cout << "null";
                } else {
                    std::cout << Json(r, result.error().to_string(r)).dump(r);
                }
                std::cout << ",\"parent\":";
                snapshot(parent);
                std::cout << ",\"classified\":[";
                for (std::size_t i = 0; i < calls.size(); ++i) {
                    if (i) {
                        std::cout << ',';
                    }
                    std::cout << Json(r, calls[i]).dump(r);
                }
                std::cout << "]}\n";
            }
        }
    }
}
