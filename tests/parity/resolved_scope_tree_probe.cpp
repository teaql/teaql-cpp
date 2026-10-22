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

SelectQuery leaf(Memory r, std::string_view name) {
    SelectQuery query(std::allocator_arg, Allocator(r), name);
    String nested(name, r);
    nested += "Nested";
    query.child_enhancements.emplace_back(nested);
    return query;
}

SelectQuery tree(Memory r, std::string_view shape) {
    SelectQuery query(std::allocator_arg, Allocator(r), "RootDiagnostic");
    if (shape == "relation" || shape == "mixed") {
        query.relations.emplace_back("Unqueried");
        query.relations.emplace_back("edge", leaf(r, "Relation"));
    }
    if (shape == "child" || shape == "mixed") {
        query.child_enhancements.emplace_back(leaf(r, "Child"));
    }
    if (shape == "aggregate" || shape == "mixed") {
        query.relation_aggregates.emplace_back("edge", "total", leaf(r, "Aggregate"), false);
    }
    if (shape == "group" || shape == "mixed") {
        query.object_group_bys.emplace_back("object", "id", leaf(r, "Group"));
    }
    if (shape == "facet" || shape == "mixed") {
        query.facets.emplace_back("facet", "edge", leaf(r, "Facet"), false);
    }
    return query;
}

int main() {
    unsigned case_index = 0;
    for (std::string_view shape :
         {"empty", "relation", "child", "aggregate", "group", "facet", "mixed"}) {
        for (bool capture : {false, true}) {
            for (bool valid : {false, true}) {
                std::vector<std::string> calls;
                GlobalContext global;
                auto user_context = global.make_user_context();
                user_context->set_sql_log_options(SqlLogOptions{capture, false});
                auto r = user_context->memory_resource();
                auto& executor = user_context->make<Executor>(calls);
                auto& parent = EntityDataService<Executor>::for_executor(
                    *user_context, "Parent", *user_context, executor);
                Access::capture_readback_target(parent, Value(r, "parent-secret"));
                auto query = tree(r, shape);
                if (valid) {
                    query.comment_text.emplace("tree comment", r);
                    query.purpose_text.emplace("tree purpose", r);
                }
                auto original = detail::clone_in(r, query);
                auto scoped = Access::query_scoped(parent, query);
                std::cout << "{\"case\":" << case_index++ << ",\"shape\":" << Json(r, shape).dump(r)
                          << ",\"capture\":" << std::boolalpha << capture << ",\"valid\":" << valid
                          << ",\"parent\":";
                snapshot(parent);
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
                std::cout << ",\"unchanged\":" << (query == original) << ",\"classified\":[";
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
