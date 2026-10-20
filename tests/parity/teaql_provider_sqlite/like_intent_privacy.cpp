// Semantic port of all five locked Rust 5.0.6 SQLite LIKE privacy tests.
#include "fixture.hpp"
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <iostream>
using namespace original_provider;

struct Probe final : SqlTransport<MutationExecutorError> {
    using allocator_type = Allocator;
    SqliteMutationExecutor& inner;
    Vector<CompiledQuery> reads;

    Probe(std::allocator_arg_t, Allocator a, SqliteMutationExecutor& inner)
        : inner(inner), reads(a.resource()) {}

    DataServiceOperationResult<Rows, MutationExecutorError>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& query) override {
        reads.emplace_back(query);
        return inner.fetch_all_compact_sql(r, query);
    }

    DataServiceOperationResult<std::uint64_t, MutationExecutorError>&
    execute_sql(Memory r, const CompiledQuery& query) override {
        return inner.execute_sql(r, query);
    }
};

struct Policy final : RequestPolicy {
    using allocator_type = Allocator;
    mutable Vector<std::pair<String, String>> seen;

    Policy(std::allocator_arg_t, Allocator a) : seen(a.resource()) {}

    RuntimeHookResult
    enforce_select(Memory r, const UserContext&, SelectQuery& query) const override {
        check(query.comment_text && query.purpose_text, "policy receives intent");
        seen.emplace_back(String(*query.comment_text, r), String(*query.purpose_text, r));
        return RuntimeHookResult::ok(r, {});
    }
};

using ObservedExecutor = SqlDataServiceExecutor<MutationExecutorError, Probe>;

struct Setup : Fixture {
    Probe& probe = user->make<Probe>(transport);
    SqlExecutorState& observed_state = user->make<SqlExecutorState>();
    ObservedExecutor& observed =
        user->make<ObservedExecutor>(dialect, probe, metadata, observed_state);
    Policy& policy = user->make<Policy>();

    Setup(std::string_view operand) {
        auto r = resource;
        EntityDescriptor parent(std::allocator_arg, Allocator(r), "Owner");
        parent.table("like_owner");
        parent.property("id", DataType::I64).id();
        parent.property("version", DataType::I64).version();
        parent.audit_mask_fields_declared = true;
        auto& relation = parent.relation("children", "SecretRow");
        relation.many = true;
        relation.local_key = "id";
        relation.foreign_key = "owner_id";
        EntityDescriptor child(std::allocator_arg, Allocator(r), "SecretRow");
        child.table("like_child");
        child.property("id", DataType::I64).id();
        child.property("version", DataType::I64).version();
        child.property("owner_id", DataType::I64);
        child.property("name", DataType::Text);
        child.property("visible", DataType::Text);
        child.audit_mask_fields_declared = true;
        child.audit_mask_fields.emplace_back("name");
        metadata.register_entity(std::move(parent));
        metadata.register_entity(std::move(child));
        use_sqlite_provider(*user, *user, transport);
        check(bool(user->ensure_schema().wait()), "schema setup");
        const auto& begun = executor.begin(r).wait();
        check(bool(begun), "seed transaction");
        auto& tx = begun.value().get_as<MutationExecutor<Executor::Error>>();
        for (int i = 0; i < 3; ++i) {
            InsertCommand insert(std::allocator_arg, Allocator(r), i ? "SecretRow" : "Owner");
            insert.value("id", std::int64_t(i == 2 ? 2 : 1)).value("version", std::int64_t(1));
            if (i) {
                insert.value("owner_id", std::int64_t(1))
                    .value("name", i == 1 ? operand : "UNRELATED")
                    .value("visible", i == 1 ? operand : "UNRELATED");
            }
            MutationRequest request(
                std::allocator_arg, Allocator(r), std::move(insert), "seed LIKE fixture");
            check(bool(tx.mutate(r, std::move(request)).wait()), "seed row");
        }
        check(bool(begun.value().get().commit(r).wait()), "commit seed");
        user->set_request_policy(*user, policy);
        user->enable_all_sql_log();
        user->clear_sql_logs();
    }

    auto& service(std::string_view entity) {
        return EntityDataService<ObservedExecutor>::for_executor(*user, entity, *user, observed);
    }
};

static Expr expression(Memory r, unsigned kind, std::string_view field, std::string_view operand) {
    switch (kind) {
    case 0:
        return Expr::contain(r, field, operand);
    case 1:
        return Expr::not_contain(r, field, operand);
    case 2:
        return Expr::begin_with(r, field, operand);
    case 3:
        return Expr::not_begin_with(r, field, operand);
    case 4:
        return Expr::end_with(r, field, operand);
    default:
        return Expr::not_end_with(r, field, operand);
    }
}

static String pattern(Memory r, unsigned kind, std::string_view operand) {
    return String(kind < 2 || kind >= 4 ? "%" : "", r) + String(operand, r) + (kind < 4 ? "%" : "");
}

static auto&
fetch(Setup& fixture, std::string_view entity, SelectQuery& query, std::string_view purpose) {
    PurposedSelectQuery request(std::allocator_arg, Allocator(fixture.resource), query, purpose);
    auto& work = fixture.service(entity).fetch_all(request);
    check(bool(work.wait()), "real provider query succeeds");
    return work.wait().value();
}

static void matrix(bool future) {
    unsigned scenarios = 0;
    for (auto operand : {"RUST-LIKE-PRIVATE", "%RUST_LIKE\\_%"}) {
        for (unsigned kind = 0; kind < 6; ++kind) {
            for (bool masked : {false, true}) {
                for (bool logging : {false, true}) {
                    Setup f(operand);
                    auto r = f.resource;
                    if (!logging) {
                        f.user->disable_sql_log();
                    }
                    SelectQuery child(std::allocator_arg, Allocator(r), "SecretRow");
                    child.filter(expression(r, kind, masked ? "name" : "visible", operand))
                        .limit(10);
                    SelectQuery query(std::allocator_arg, Allocator(r), "Owner");
                    if (future) {
                        query.limit(1).relation_query("children", child);
                    } else {
                        query = std::move(child);
                    }
                    auto root = future ? "Owner" : "SecretRow";
                    auto comment = String("load ", r) + operand + " with PLAIN-CONTROL";
                    auto purpose = String("purpose ", r) + operand + " with PLAIN-CONTROL";
                    query.comment(comment);
                    PurposedSelectQuery request(std::allocator_arg, Allocator(r), query, purpose);
                    auto& repository = f.service(root);
                    const auto& result = repository.fetch_all(request).wait();
                    check(bool(result) && result.value().size() == 1, "one returned root");
                    auto expected = std::int64_t(kind % 2 ? 2 : 1);
                    if (future) {
                        const auto* children = result.value()[0].get("children");
                        check(children && children->as_list() && children->as_list()->size() == 1,
                              "one loaded child");
                        const auto* row = children->as_list()->front().as_object();
                        check(row && row->at(String("id", r)).try_i64() == expected,
                              "selected child identity");
                    } else {
                        check(result.value()[0].get("id")->try_i64() == expected,
                              "selected root identity");
                    }
                    check(f.probe.reads.size() == (future ? 2 : 1), "physical read count");
                    if (future) {
                        for (const auto& value : f.probe.reads[0].params) {
                            check(value != Value(r, operand), "no child operand in root bindings");
                        }
                    }
                    const auto& physical = f.probe.reads.back();
                    auto decorated = pattern(r, kind, operand);
                    auto found = std::find(
                        physical.params.begin(), physical.params.end(), Value(r, decorated));
                    check(found != physical.params.end(), "unchanged decorated SQL binding");
                    auto index = std::size_t(found - physical.params.begin());
                    check(
                        physical.log_context.parameter_policies.at(index) ==
                            (masked ? SqlParameterLogPolicy::Masked : SqlParameterLogPolicy::Plain),
                        "parameter privacy classification");
                    check(physical.sql.find(kind % 2 ? "NOT LIKE" : "LIKE") != String::npos,
                          "physical operator");
                    check(request.as_query().comment_text == comment &&
                              request.as_query().purpose_text == purpose,
                          "original caller intent preserved");
                    check(!f.policy.seen.empty(), "Policy invoked");
                    for (const auto& [seen_comment, seen_purpose] : f.policy.seen) {
                        check(seen_comment == comment && seen_purpose == purpose,
                              "Policy sees original intent");
                    }
                    auto logs = f.user->sql_logs();
                    check(logs.size() == (logging ? (future ? 2 : 1) : 0), "safe log count");
                    for (std::size_t depth = 0; depth < logs.size(); ++depth) {
                        const auto& log = logs[depth];
                        check(log.trace_path[0].entity_type == root &&
                                  std::count_if(log.trace_path.begin(),
                                                log.trace_path.end(),
                                                [](const auto& node) {
                                                    return node.kind == TraceKind::Relation;
                                                }) == depth,
                              "trace root and relation depth");
                        check(log.comment && log.purpose &&
                                  log.comment->find("PLAIN-CONTROL") != String::npos &&
                                  log.purpose->find("PLAIN-CONTROL") != String::npos,
                              "ordinary intent remains readable");
                        if (masked) {
                            check(debug_string(r, log).find(operand) == String::npos,
                                  "safe log hides original operand");
                        } else {
                            check(*log.comment == comment && *log.purpose == purpose,
                                  "plain intent unchanged");
                        }
                    }
                    f.user->clear_sql_logs();
                    SelectQuery independent(std::allocator_arg, Allocator(r), root);
                    independent.limit(1).comment(comment);
                    PurposedSelectQuery independent_request(
                        std::allocator_arg, Allocator(r), independent, purpose);
                    check(bool(repository.fetch_all(independent_request).wait()),
                          "same repository independent request");
                    if (logging) {
                        auto independent_logs = f.user->sql_logs();
                        check(independent_logs.size() == 1 &&
                                  independent_logs[0].comment == comment &&
                                  independent_logs[0].purpose == purpose,
                              "request-local privacy does not leak");
                    }
                    ++scenarios;
                }
            }
        }
    }
    std::cout << "LIKE_INTENT scenarios=" << scenarios << " future=" << future << ";passed\n";
}

static void raw_like_control() {
    constexpr auto bare = "RAW_LITERAL\\";
    Setup f(bare);
    auto r = f.resource;
    auto pattern = String("%", r) + bare + "%";
    auto comment = String("bare ", r) + bare + "; exact pattern " + pattern;
    SelectQuery query(std::allocator_arg, Allocator(r), "SecretRow");
    query.filter(Expr::like(r, "name", pattern)).limit(1).comment(comment);
    fetch(f, "SecretRow", query, "raw LIKE control");
    check(f.probe.reads.size() == 1 && f.probe.reads[0].params.size() == 1 &&
              f.probe.reads[0].params[0] == Value(r, pattern),
          "raw exact physical pattern");
    auto logs = f.user->sql_logs();
    check(logs.size() == 1 && logs[0].comment && logs[0].comment->find(bare) != String::npos &&
              logs[0].comment->find(pattern) == String::npos,
          "raw LIKE does not infer original by stripping wildcards");
}

static void cached_like_control() {
    constexpr std::string_view first = "LIKE-FIRST-PRIVATE", second = "LIKE-SECOND-PRIVATE";
    Setup f(first);
    auto r = f.resource;
    auto& repository = f.service("SecretRow");
    for (auto current : {first, second}) {
        auto ordinary = current == first ? second : first;
        f.user->clear_sql_logs();
        auto comment =
            String("private ", r) + String(current, r) + "; ordinary " + String(ordinary, r);
        auto purpose = String("purpose ", r) + comment;
        SelectQuery query(std::allocator_arg, Allocator(r), "SecretRow");
        query.filter(Expr::begin_with(r, "name", current)).limit(1).comment(comment);
        PurposedSelectQuery request(std::allocator_arg, Allocator(r), query, purpose);
        check(bool(repository.fetch_all(request).wait()), "cached query executes");
        auto logs = f.user->sql_logs();
        check(logs.size() == 1 && debug_string(r, logs[0]).find(current) == String::npos &&
                  logs[0].comment && logs[0].purpose &&
                  logs[0].comment->find(ordinary) != String::npos &&
                  logs[0].purpose->find(ordinary) != String::npos,
              "cached plan classifies only current operand");
    }
    check(f.probe.reads.size() == 2 && f.probe.reads[0].sql == f.probe.reads[1].sql,
          "same cached SQL shape");
    for (unsigned i = 0; i < 2; ++i) {
        check(f.probe.reads[i].params.size() == 1 &&
                  f.probe.reads[i].params[0] == Value(r, String(i ? second : first, r) + "%"),
              "cached SQL keeps current binding");
    }
}

static void rewritten_like_control() {
    constexpr auto original = "DISCARDED-LIKE-ORIGINAL";
    for (bool logging : {false, true}) {
        for (bool rewrite_pattern : {false, true}) {
            Setup f(original);
            auto r = f.resource;
            if (!logging) {
                f.user->disable_sql_log();
            }
            auto expr = Expr::begin_with(r, "name", original);
            check(expr.kind == Expr::Kind::Binary && expr.args.size() == 2 &&
                      expr.args[1].kind == Expr::Kind::LikePattern,
                  "typed AST shape");
            if (rewrite_pattern) {
                expr.args[1].literal = Value(r, "UNRELATED%");
            } else {
                expr.op = BinaryOp::Eq;
            }
            auto comment = String("ordinary ", r) + original;
            SelectQuery query(std::allocator_arg, Allocator(r), "SecretRow");
            query.filter(std::move(expr)).limit(10).comment(comment);
            const auto& rows = fetch(f, "SecretRow", query, "rewritten AST control");
            check(rows.size() == unsigned(rewrite_pattern), "rewritten AST actual rows");
            auto expected = rewrite_pattern ? String("UNRELATED%", r) : String(original, r) + "%";
            check(f.probe.reads.size() == 1 && f.probe.reads[0].params.size() == 1 &&
                      f.probe.reads[0].params[0] == Value(r, expected),
                  "rewritten AST physical binding");
            auto logs = f.user->sql_logs();
            check(logs.size() == unsigned(logging), "rewritten AST log switch");
            if (logging) {
                check(logs[0].comment == comment, "stale original provenance discarded");
            }
        }
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    matrix(false);
    matrix(true);
    raw_like_control();
    cached_like_control();
    rewritten_like_control();
    std::cout << "LIKE_PRIVACY_SOURCE_TESTS=5;passed\n";
}
