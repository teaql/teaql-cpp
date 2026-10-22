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

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("classification must not execute SQL");
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
    for (bool seed : {false, true}) {
        for (bool bound : {false, true}) {
            for (bool valid : {false, true}) {
                std::vector<std::string> calls;
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto& executor = user_context->make<Executor>(calls);
                auto& parent = EntityDataService<Executor>::for_executor(
                    *user_context, "Parent", *user_context, executor);
                if (seed) {
                    Access::capture_readback_target(parent, Value(r, "seed-secret"));
                }
                if (bound) {
                    auto intent = QueryIntent::from_optional(r, "bound comment", "bound purpose");
                    Access::bind_query_intent(parent, intent.value());
                }
                auto child = Access::scoped(parent, "Child");
                auto sibling = Access::scoped(parent, "Sibling");
                Access::capture_readback_target(parent, Value(r, "parent-late-secret"));
                Access::capture_readback_target(child, Value(r, "child-only-secret"));
                SelectQuery query(std::allocator_arg, Allocator(r), "RootDiagnostic");
                if (valid) {
                    query.comment_text.emplace("query comment", r);
                    query.purpose_text.emplace("query purpose", r);
                }
                query.child_enhancements.emplace_back("FirstChild");
                query.child_enhancements.emplace_back("LastChild");
                auto scoped = Access::query_scoped(parent, query);
                if (scoped) {
                    Access::capture_readback_target(scoped.value(), Value(r, "query-only-secret"));
                }
                std::cout << "{\"case\":" << case_index++ << ",\"seed\":" << std::boolalpha << seed
                          << ",\"bound\":" << bound << ",\"valid\":" << valid << ",\"parent\":";
                snapshot(parent);
                std::cout << ",\"child\":";
                snapshot(child);
                std::cout << ",\"sibling\":";
                snapshot(sibling);
                std::cout << ",\"query_scope\":";
                if (scoped) {
                    snapshot(scoped.value());
                } else {
                    std::cout << "null";
                }
                std::cout << ",\"error\":";
                if (scoped) {
                    std::cout << "null";
                } else {
                    std::cout << Json(r, scoped.error().to_string(r)).dump(r);
                }
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
