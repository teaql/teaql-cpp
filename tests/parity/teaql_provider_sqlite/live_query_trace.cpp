// Locked Rust live-query isolation, using Context owner-thread cooperative polling.
#include "fixture.hpp"
#include <iostream>
#include <teaql/runtime/data_service.hpp>
using namespace original_provider;

struct Observer final : QueryExecutor<Executor::Error>, MutationExecutor<Executor::Error> {
    using allocator_type = Allocator;
    Memory resource;
    Executor& inner;

    struct Fact {
        String entity, comment, purpose;
        TraceChain trace;
        ExecutionMetadata metadata;
    };

    Vector<Fact> facts;
    unsigned held = 0;
    bool released = false;

    Observer(std::allocator_arg_t, Allocator a, Executor& inner)
        : resource(a.resource()), inner(inner), facts(resource) {}

    DataServiceCapabilities capabilities() const override {
        return inner.capabilities();
    }

    SqlIntentRedactions query_log_intent(Memory r, const SelectQuery& query) const override {
        return inner.query_log_intent(r, query);
    }

    struct Work {
        using allocator_type = Allocator;
        using Output = Result<QueryResult, Executor::Error>;
        Observer& observer;
        QueryResult result;

        Work(std::allocator_arg_t, Allocator a, Observer& observer, QueryResult result)
            : observer(observer), result(std::allocator_arg, a, std::move(result)) {}

        std::optional<Output> poll(WakeSignal&) {
            if (!observer.released) {
                return {};
            }
            return Output::ok(observer.resource, std::move(result));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    DataServiceOperationResult<QueryResult, Executor::Error>& query(Memory r,
                                                                    QueryRequest request) override {
        auto entity = String(request.query.entity, r);
        auto comment = String(request.intent.comment(), r);
        auto purpose = String(request.intent.purpose(), r);
        auto trace = request.execution_trace_chain();
        const auto& actual = inner.query(r, std::move(request)).wait();
        check(bool(actual), "real SQLite query succeeds");
        facts.push_back(Fact{std::move(entity),
                             std::move(comment),
                             std::move(purpose),
                             std::move(trace),
                             detail::clone_in(r, actual.value().metadata)});
        auto result = detail::clone_in(r, actual.value());
        if (facts.back().entity == "LiveRoot") {
            ++held;
            return make_operation<Work::Output, Work>(r, *this, std::move(result));
        }
        return make_operation<Work::Output, Work>(r, *this, std::move(result));
    }

    DataServiceOperationResult<MutationResult, Executor::Error>&
    mutate(Memory r, MutationRequest request) override {
        return inner.mutate(r, std::move(request));
    }
};

static void run(bool logging) {
    Fixture fixture;
    auto& user_context = *fixture.user;
    auto r = fixture.resource;
    constexpr std::string_view names[] = {"LiveRoot", "LiveChild", "LiveDetail", "LiveLeaf"};
    constexpr std::string_view tables[] = {"liveroot", "livechild", "livedetail", "liveleaf"};
    for (unsigned i = 0; i < 4; ++i) {
        EntityDescriptor entity(std::allocator_arg, Allocator(r), names[i]);
        entity.table(tables[i]);
        entity.property("id", DataType::I64).id();
        entity.property("version", DataType::I64).version();
        entity.property("parent_id", DataType::I64);
        if (i < 3) {
            auto& relation = entity.relation("children", names[i + 1]);
            relation.many = true;
            relation.local_key = "id";
            relation.foreign_key = "parent_id";
        }
        fixture.metadata.register_entity(std::move(entity));
    }
    use_sqlite_provider(user_context, user_context, fixture.transport);
    check(bool(user_context.ensure_schema().wait()), "native schema bootstrap");
    for (auto name : names) {
        InsertCommand insert(std::allocator_arg, Allocator(r), name);
        insert.value("id", std::int64_t(1))
            .value("version", std::int64_t(1))
            .value("parent_id", std::int64_t(1));
        MutationRequest request(
            std::allocator_arg, Allocator(r), std::move(insert), "seed native live-query fixture");
        check(bool(fixture.executor.mutate(r, std::move(request)).wait()), "seed native rows");
    }
    if (logging) {
        user_context.enable_all_sql_log();
    }
    user_context.clear_sql_logs();
    auto& observer = user_context.make<Observer>(fixture.executor);
    auto& service =
        EntityDataService<Observer>::for_executor(user_context, "LiveRoot", user_context, observer);
    auto request = [&](std::string_view label) {
        SelectQuery query(std::allocator_arg, Allocator(r), names[3]);
        query.limit(1);
        for (int i = 2; i >= 0; --i) {
            SelectQuery parent(std::allocator_arg, Allocator(r), names[i]);
            parent.limit(1).relation_query("children", query);
            query = std::move(parent);
        }
        query.comment(String("load ", r) + String(label, r) + " graph");
        return PurposedSelectQuery(std::allocator_arg,
                                   Allocator(r),
                                   query,
                                   String("render ", r) + String(label, r) + " graph");
    };
    auto alpha = request("alpha"), beta = request("beta");
    auto& first = service.fetch_all(alpha);
    auto& second = service.fetch_all(beta);
    check(!first.poll() && !second.poll(), "both real root results are held");
    check(observer.held == 2 && observer.facts.size() == 2 && user_context.sql_logs().empty(),
          "no relation or Context log before release");
    observer.released = true;
    auto check_rows = [&](const auto& result) {
        check(bool(result) && result.value().size() == 1, "one real root");
        const Value* children = result.value()[0].get("children");
        for (unsigned depth = 0; depth < 3; ++depth) {
            check(children && children->as_list() && children->as_list()->size() == 1,
                  "real child list");
            const auto* row = children->as_list()->front().as_object();
            check(row && row->at(String("id", r)).try_i64() == 1, "real nested identity");
            children = depth == 2 ? nullptr : &row->at(String("children", r));
        }
    };
    check_rows(second.wait());
    check_rows(first.wait());
    check(observer.facts.size() == 8, "eight physical reads");
    auto logs = user_context.sql_logs();
    check(logs.size() == (logging ? 8 : 0), "logging switch preserves query work");
    for (auto label : {"alpha", "beta"}) {
        auto comment = String("load ", r) + label + " graph";
        auto purpose = String("render ", r) + label + " graph";
        unsigned depth = 0;
        for (const auto& fact : observer.facts) {
            if (fact.comment != comment) {
                continue;
            }
            check(depth < 4 && fact.entity == names[depth] && fact.purpose == purpose,
                  "isolated request intent and entity order");
            unsigned edge = 0;
            for (const auto& node : fact.trace) {
                if (node.kind != TraceKind::Relation) {
                    continue;
                }
                check(edge < depth && node.entity_type == "children" &&
                          node.comment == String(names[edge], r) + ".children",
                      "real relation lineage");
                ++edge;
            }
            check(edge == (logging ? depth : 0), "logging controls route capture");
            if (logging) {
                check(fact.metadata.result_count == 1, "physical result count");
            }
            ++depth;
        }
        check(depth == 4, "four requests per operation");
        depth = 0;
        for (const auto& log : logs) {
            if (!log.comment || *log.comment != comment) {
                continue;
            }
            check(log.purpose && *log.purpose == purpose && log.trace_path.size() == depth + 4,
                  "safe log intent and exact path size");
            for (unsigned i = 0; i < log.trace_path.size(); ++i) {
                auto kind = i == 0           ? TraceKind::Operation
                            : i == 1         ? TraceKind::Request
                            : i < depth + 2  ? TraceKind::Relation
                            : i == depth + 2 ? TraceKind::Provider
                                             : TraceKind::Sql;
                check(log.trace_path[i].kind == kind, "exact safe trace kinds");
            }
            check(log.trace_path[0].entity_type == names[0] &&
                      log.trace_path[1].entity_type == names[0] &&
                      log.trace_path[depth + 2].entity_type == "sqlite" &&
                      log.trace_path[depth + 3].entity_type == "select",
                  "safe trace entities");
            ++depth;
        }
        check(depth == (logging ? 4 : 0), "four logs per operation");
    }
    std::cout << "LIVE_QUERY_ISOLATION logging=" << logging << ";held=2;physical=8;passed\n";
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    run(false);
    run(true);
}
