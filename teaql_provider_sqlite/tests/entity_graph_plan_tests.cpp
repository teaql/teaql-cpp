#include "../../tests/support/entity_graph_plan_fixture.hpp"
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/sql/executor.hpp>
#include <iostream>
using namespace teaql;
using namespace graph_plan_fixture;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void planning_and_retention() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto metadata = user->make_ref<InMemoryMetadataStore>();
    user->get(metadata).register_entity(descriptor(r, "Parent", "id", true));
    user->get(metadata).register_entity(descriptor(r, "Child"));
    user->set_metadata(*user, user->get(metadata));
    auto& generator = user->make<AtomicCounterIdGenerator>(100);
    user->set_internal_id_generator(*user, generator);
    auto& executor = user->make<Executor>();
    executor.rows.emplace_back();
    executor.rows.back().emplace(String("id", r), Value(r, std::uint64_t(9)));
    auto& service = EntityDataService<Executor>::for_executor(*user, "Parent", *user, executor);
    auto& source = user->make<GraphNode>("Parent");
    source.with_operation(GraphOperation::Create).with_comment("root");
    auto& child = user->make<GraphNode>("Child");
    child.value("id", std::int64_t(9)).with_comment("child");
    child.value("name", "changed");
    source.relation("kids", child);
    auto& operation = service.plan_graph(detail::clone_in(r, source));
    check(executor.requests.empty(), "planning is lazy");
    auto* result = operation.poll();
    check(result && *result, "forced create graph plans successfully");
    check(executor.requests.empty() && result->value().items.size() == 2,
          "forced create propagates to Upsert children");
    check(result->value().items[0].values.fields.at(String("id", r)).try_u64() == 101 &&
              result->value().items[1].kind == GraphMutationKind::Create,
          "IDs assigned without overwriting supplied child IDs");
    auto& trace =
        user->make<TraceChain>(result->value().items[1].scope_token->recover_trace_chain(r));
    check(trace.size() == 2 && trace[0].entity_id == 101 && trace[1].entity_id == 9,
          "persistent tokens capture IDs after allocation");
    check(source.values.fields.empty(), "plan owns input snapshot");
    auto& retained = global.make<GraphMutationPlan>(result->value());
    user.reset();
    check(retained.items[1].scope_token->recover_trace_chain(global.memory_resource())[0].comment ==
              "root",
          "Global plan retains copied nested values and trace ancestry after User destruction");
}

static void pending_and_duplicate_identity() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto metadata = user->make_ref<InMemoryMetadataStore>();
    user->get(metadata).register_entity(descriptor(r, "Parent"));
    user->get(metadata).register_entity(descriptor(r, "Child"));
    user->set_metadata(*user, user->get(metadata));
    auto& executor = user->make<Executor>();
    executor.pending = true;
    executor.rows.emplace_back();
    executor.rows.back().emplace(String("id", r), Value(r, std::uint64_t{1}));
    executor.rows.back().emplace(String("version", r), Value(r, std::int64_t{4}));
    executor.rows.back().emplace(String("name", r), Value(r, "original"));
    auto& service = EntityDataService<Executor>::for_executor(*user, "Parent", *user, executor);
    auto& node = user->make<GraphNode>("Parent");
    node.value("id", std::uint64_t(1)).value("name", "changed").value("version", std::int64_t(4));
    node.with_comment("audit");
    node.dirty_fields.emplace(r);
    node.dirty_fields->emplace("name");
    auto& child = user->make<GraphNode>("Child");
    child.value("id", std::uint64_t(2));
    node.relation("kids", child);
    child.value("id", std::int64_t(2));
    child.relation("invalid", child);
    node.relation("kids", child);
    auto& operation = service.plan_graph(detail::clone_in(r, node));
    check(!operation.poll() && executor.requests.size() == 1,
          "lookup suspends without repeating provider entry");
    executor.pending = false;
    executor.child_wake->notify();
    auto* result = operation.poll();
    check(result && *result, "pending lookup resumes");
    check(executor.requests.size() == 2 && result->value().items.size() == 2,
          "signed and unsigned identity aliases skip duplicate subtrees");
    check(result->value().items[0].update_fields.size() == 1 &&
              result->value().items[0].update_fields[0] == "name",
          "dirty fields filter update projection after excluding identity and version");
    auto& remove = user->make<GraphNode>("Missing");
    remove.remove().with_comment("inspect deletion");
    auto& missing_service =
        EntityDataService<Executor>::for_executor(*user, "Missing", *user, executor);
    auto& removed = missing_service.plan_graph(detail::clone_in(r, remove));
    auto* deletion = removed.poll();
    check(deletion && *deletion && deletion->value().items[0].scope_token &&
              deletion->value().items[0].scope_token->recover_trace_chain(r).back().comment ==
                  "inspect deletion",
          "Remove bypasses descriptor lookup and preserves its own intent scope");
}

static void authoritative_readback() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& metadata = user_context->make<InMemoryMetadataStore>();
    metadata.register_entity(descriptor(r, "Parent"));
    user_context->set_metadata(*user_context, metadata);
    auto& executor = user_context->make<Executor>();
    auto& service =
        EntityDataService<Executor>::for_executor(*user_context, "Parent", *user_context, executor);
    auto intent = QueryIntent::create(r, "Inspect target 7", "Read a complete save snapshot");
    check(bool(intent), "readback has explicit valid intent");
    detail::EntityDataServiceAccess::bind_query_intent(service, intent.value());
    Value id(r, std::uint64_t{7});
    auto read = [&]() -> auto& {
        return detail::EntityDataServiceAccess::fetch_graph_current_row_internal(
            service, "Parent", "id", id, {});
    };
    executor.rows.emplace_back();
    executor.rows.back().emplace(String("id", r), Value(r, std::uint64_t{7}));
    auto& failed = read().wait();
    check(!failed &&
              failed.error().to_string(r) ==
                  "graph write error: authoritative readback missing mapped field Parent.version",
          "sparse authoritative rows fail in descriptor order");
    executor.rows.back().emplace(String("version", r), Value(r));
    executor.rows.back().emplace(String("name", r), Value(r));
    executor.rows.back().emplace(String("extra", r), Value(r, "additional projection"));
    auto& complete = read().wait();
    check(complete && complete.value() && complete.value()->get("name")->is_null() &&
              complete.value()->len() == 4,
          "reordered complete rows, extra projections and explicit NULL stay legal");
    check(!detail::EntityDataServiceAccess::intent(service),
          "capturing a readback target does not change reusable parent provenance");
    executor.rows.clear();
    auto& absent = read().wait();
    check(absent && !absent.value(), "no matching row remains an ordinary missing result");
}

static void cancellation_and_expiry() {
    GlobalContext global;
    unsigned cancellations = 0;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto metadata = user->make_ref<InMemoryMetadataStore>();
    user->get(metadata).register_entity(descriptor(r, "Parent"));
    user->set_metadata(*user, user->get(metadata));
    auto& executor = user->make<Executor>();
    executor.pending = true;
    executor.external_cancelled = &cancellations;
    auto& service = EntityDataService<Executor>::for_executor(*user, "Parent", *user, executor);
    auto& node = user->make<GraphNode>("Parent");
    node.value("id", std::uint64_t(1)).with_comment("Inspect pending graph lookup");
    auto& operation = service.plan_graph(detail::clone_in(r, node));
    check(!operation.poll(), "native lookup is pending");
    user->destroy(metadata);
    executor.pending = false;
    bool expired = false;
    try {
        (void)operation.poll();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "destroyed metadata is rejected before resuming traversal");
    auto replacement = user->make_ref<InMemoryMetadataStore>();
    user->get(replacement).register_entity(descriptor(r, "Parent"));
    user->set_metadata(*user, user->get(replacement));
    executor.pending = true;
    auto& closing = service.plan_graph(detail::clone_in(r, node));
    check(!closing.poll(), "second lookup is pending");
    user.reset();
    check(cancellations == 1, "close joins outstanding provider work exactly once");
}

static void actual_sqlite_planning() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& metadata = user->make<InMemoryMetadataStore>();
    auto parent = descriptor(r, "Parent");
    parent.table("parent");
    parent.audit_mask_fields_declared = true;
    metadata.register_entity(std::move(parent));
    user->set_metadata(*user, metadata);
    user->enable_all_sql_log();
    auto& connection = user->make_resource<SqliteConnection>();
    auto& transport = user->make<SqliteMutationExecutor>(connection);
    auto& sql = user->make<CompiledQuery>();
    for (auto statement : {"CREATE TABLE parent(id INTEGER PRIMARY KEY,version INTEGER,name TEXT)",
                           "INSERT INTO parent VALUES(7,3,'old')"}) {
        sql.sql = statement;
        check(bool(transport.execute(r, sql)), "SQLite planning setup");
    }
    auto& dialect = user->make_resource<SqliteDialect>();
    auto& state = user->make<SqlExecutorState>();
    auto& executor = user->make<SqlDataServiceExecutor<MutationExecutorError>>(
        dialect, transport, metadata, state);
    using E = std::remove_reference_t<decltype(executor)>;
    auto& service = EntityDataService<E>::for_executor(*user, "Parent", *user, executor);
    auto& node = user->make<GraphNode>("Parent");
    node.value("id", std::uint64_t(7))
        .value("name", "new")
        .with_comment("Inspect SQLite graph lookup target 7");
    auto* result = service.plan_graph(detail::clone_in(r, node)).poll();
    if (result && !*result) {
        throw std::runtime_error(std::string(result->error().to_string(r)));
    }
    check(result && *result && result->value().items[0].kind == GraphMutationKind::Update,
          "real SQLite existence lookup selects Update without writing");
    auto logs = user->sql_logs();
    check(logs.size() == 1, "the complete readback emits one real SQLite query log");
    check(logs[0].comment && logs[0].comment->find("target [REDACTED]") != String::npos &&
              logs[0].log_context.intent_redactions.is_empty(),
          "safe readback logging scrubs the target and consumes private provenance");
    check(logs[0].params.size() == 1 && logs[0].params[0].try_u64() == 7 &&
              logs[0].log_context.parameter_policies[0] == SqlParameterLogPolicy::Plain,
          "target provenance preserves the ordinary SQL parameter policy and binding");
    check(!detail::EntityDataServiceAccess::intent(service),
          "readback provenance does not modify reusable service state");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        planning_and_retention();
        pending_and_duplicate_identity();
        authoritative_readback();
        cancellation_and_expiry();
        actual_sqlite_planning();
        std::pmr::set_default_resource(previous);
        std::cout << "Graph planning ownership contracts passed\n";
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
}
