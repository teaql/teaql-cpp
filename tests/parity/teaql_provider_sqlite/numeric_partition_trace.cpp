// All four locked Rust 5.0.6 SQLite numeric partition trace tests.
#include "fixture.hpp"
#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/transaction.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <iostream>
using namespace original_provider;
constexpr std::string_view secret = "PRIVATE-NUMERIC-PARTITION";

struct NumericFixture : Fixture {
    NumericFixture() {
        auto r = resource;
        EntityDescriptor parent(std::allocator_arg, Allocator(r), "GroupOwner");
        parent.table("group_owner");
        parent.property("id", DataType::I64).id();
        parent.property("version", DataType::I64).version();
        auto& relation = parent.relation("samples", "MetricSample");
        relation.many = true;
        relation.local_key = "id";
        relation.foreign_key = "owner_id";
        EntityDescriptor child(std::allocator_arg, Allocator(r), "MetricSample");
        child.table("metric_sample");
        child.property("id", DataType::I64).id();
        child.property("version", DataType::I64).version();
        child.property("owner_id", DataType::I64);
        child.property("bucket", DataType::I64);
        child.property("name", DataType::Text);
        child.audit_mask_fields_declared = true;
        child.audit_mask_fields.emplace_back("name");
        check(child.relations.empty(), "bucket is not a model relation");
        metadata.register_entity(std::move(parent));
        metadata.register_entity(std::move(child));
        use_sqlite_provider(*user, *user, transport);
        check(bool(user->ensure_schema().wait()), "numeric schema through Context");
        user->insert_resource(*user, executor);
        auto& work = user->execute_in_transaction<Executor, MutationResult>(
            [](const TransactionScope<Executor>& scope) -> auto& {
                auto r = scope.context().memory_resource();
                InsertCommand parent(std::allocator_arg, Allocator(r), "GroupOwner");
                parent.value("id", std::int64_t(1)).value("version", std::int64_t(1));
                check(bool(scope
                               .mutate(MutationRequest(std::allocator_arg,
                                                       Allocator(r),
                                                       std::move(parent),
                                                       "seed numeric partition owner"))
                               .wait()),
                      "seed owner");
                AsyncOperation<Result<MutationResult, RuntimeError>>* last = nullptr;
                for (std::int64_t index = 0; index < 4; ++index) {
                    InsertCommand command(std::allocator_arg, Allocator(r), "MetricSample");
                    command.value("id", index + 1)
                        .value("version", std::int64_t(1))
                        .value("owner_id", std::int64_t(1))
                        .value("bucket", std::int64_t(index < 2 ? 10 : 20))
                        .value("name", index == 3 ? std::string_view("excluded") : secret);
                    last = &scope.mutate(MutationRequest(std::allocator_arg,
                                                         Allocator(r),
                                                         std::move(command),
                                                         "seed numeric partition sample"));
                    check(bool(last->wait()), "seed sample");
                }
                return *last;
            });
        check(bool(work.wait()), "commit fixture transaction");
        user->enable_all_sql_log();
        user->clear_sql_logs();
    }

    auto& service(std::string_view root) {
        return EntityDataService<Executor>::for_executor(*user, root, *user, executor);
    }
};

using Counts = Vector<std::pair<std::int64_t, std::int64_t>>;

static void verify_counts(Counts& actual, bool having) {
    std::sort(actual.begin(), actual.end());
    check(actual.size() == (having ? 1 : 2) &&
              actual[0] == std::pair<std::int64_t, std::int64_t>{10, 2} &&
              (having || actual[1] == std::pair<std::int64_t, std::int64_t>{20, 1}),
          "exact numeric group counts");
}

static void verify(unsigned shape, bool logging, bool having) {
    NumericFixture f;
    auto r = f.resource;
    if (!logging) {
        f.user->disable_sql_log();
    }
    SelectQuery grouped(std::allocator_arg, Allocator(r), "MetricSample");
    grouped.group_by("owner_id")
        .group_by("bucket")
        .count("n")
        .filter(Expr::eq(r, "name", secret))
        .order_asc("bucket")
        .limit(10);
    if (having) {
        grouped.having(
            Expr::binary(Expr::count_all(r), BinaryOp::Gt, Expr::value(r, std::int64_t(1))));
    }
    auto root = shape == 0 ? "MetricSample" : "GroupOwner";
    SelectQuery query(std::allocator_arg, Allocator(r), root);
    if (shape == 0) {
        grouped.partition_by("bucket");
        query = std::move(grouped);
    } else {
        grouped.top_n_probe_parent_threshold(shape == 1 ? 0 : 32);
        query.project("id").limit(1).relation_query("samples", grouped);
    }
    auto comment = String("count numeric groups ", r) + String(secret, r),
         purpose = String("render numeric partitions ", r) + String(secret, r);
    query.comment(comment);
    PurposedSelectQuery request(std::allocator_arg, Allocator(r), query, purpose);
    auto original = request.as_query().clone(r);
    const auto& result = f.service(root).fetch_all(request).wait();
    check(bool(result), "real grouped query succeeds");
    Counts counts(r);
    if (shape == 0) {
        for (const auto& row : result.value()) {
            check(row.get("bucket") && row.get("n") && row.get("bucket")->try_i64() &&
                      row.get("n")->try_i64(),
                  "I64 grouped result types");
            counts.emplace_back(*row.get("bucket")->try_i64(), *row.get("n")->try_i64());
        }
    } else {
        check(result.value().size() == 1, "one parent");
        const auto* samples = result.value()[0].get("samples");
        check(samples && samples->as_list(), "actual loaded samples");
        for (const auto& value : *samples->as_list()) {
            const auto* row = value.as_object();
            check(row && row->at(String("bucket", r)).try_i64() &&
                      row->at(String("n", r)).try_i64(),
                  "I64 child group types");
            counts.emplace_back(*row->at(String("bucket", r)).try_i64(),
                                *row->at(String("n", r)).try_i64());
        }
    }
    verify_counts(counts, having);
    check(request.as_query() == original, "caller query remains unchanged");
    auto logs = f.user->sql_logs();
    check(logs.size() == (logging ? (shape == 0 ? 1 : 2) : 0), "exact safe log count");
    for (std::size_t index = 0; index < logs.size(); ++index) {
        const auto& log = logs[index];
        const bool relation = shape != 0 && index > 0;
        check(log.trace_path.size() == (relation ? 5 : 4), "scalar partition adds no relation");
        for (std::size_t i = 0; i < log.trace_path.size(); ++i) {
            const auto expected_kind = i == 0                           ? TraceKind::Operation
                                       : i == 1                         ? TraceKind::Request
                                       : relation && i == 2             ? TraceKind::Relation
                                       : i == log.trace_path.size() - 2 ? TraceKind::Provider
                                                                        : TraceKind::Sql;
            auto expected_entity = i < 2                            ? root
                                   : relation && i == 2             ? "samples"
                                   : i == log.trace_path.size() - 2 ? "sqlite"
                                                                    : "select";
            check(log.trace_path[i].kind == expected_kind &&
                      log.trace_path[i].entity_type == expected_entity,
                  "exact real route kinds and entities");
        }
        check(debug_string(r, log).find(secret) == String::npos,
              "private child operand hidden even in earlier root log");
        check(log.comment && log.purpose &&
                  log.comment->find("count numeric groups") != String::npos &&
                  log.purpose->find("render numeric partitions") != String::npos &&
                  log.log_context.execution_outcome == SqlExecutionOutcome::Success,
              "ordinary intent and successful outcome");
        if (log.sql.find("COUNT(") != String::npos) {
            check(log.sql.find("GROUP BY") != String::npos &&
                      (log.sql.find("HAVING") != String::npos) == having &&
                      (log.sql.find("PARTITION BY") != String::npos) == (shape != 2) &&
                      log.sql.find(", id ASC") == String::npos,
                  "correct grouping/having/window without ungrouped ID ordering");
        }
    }
    f.user->clear_sql_logs();
    SelectQuery next(std::allocator_arg, Allocator(r), "GroupOwner");
    next.limit(1).comment(secret);
    PurposedSelectQuery independent(std::allocator_arg, Allocator(r), next, "independent query");
    check(bool(f.service("GroupOwner").fetch_all(independent).wait()),
          "independent query succeeds");
    auto independent_logs = f.user->sql_logs();
    if (logging) {
        check(independent_logs.size() == 1 && independent_logs[0].comment == secret &&
                  independent_logs[0].trace_path.size() == 4,
              "no inherited privacy or relation in next request");
    } else {
        check(independent_logs.empty(), "logging stays disabled");
    }
    std::cout << "NUMERIC_PARTITION shape=" << shape << ";logging=" << logging
              << ";having=" << having << ";passed\n";
}

static void cached_having() {
    for (bool debug : {true, false}) {
        NumericFixture f;
        auto r = f.resource;
        for (std::int64_t threshold : {0, 1}) {
            std::string_view original = threshold ? "NUMERIC" : "PRIVATE";
            SelectQuery query(std::allocator_arg, Allocator(r), "MetricSample");
            query.group_by("bucket")
                .group_by("name")
                .count("n")
                .having(Expr::binary(Expr::count_all(r), BinaryOp::Gt, Expr::value(r, threshold)))
                .and_having(Expr::contain(r, "name", original))
                .order_asc("bucket")
                .limit(10)
                .partition_by("bucket");
            auto intent =
                QueryIntent::create(r,
                                    "verify partition cache rebinding",
                                    String("count grouped ", r) + String(original, r) + " samples");
            QueryRequest request(
                std::allocator_arg, Allocator(r), query, intent.value(), debug, true);
            const auto& result = f.executor.query(r, std::move(request)).wait();
            check(bool(result), "warm plan query executes");
            Counts counts(r);
            for (const auto& row : result.value().rows) {
                check(row.get("bucket") && row.get("n") && row.get("bucket")->try_i64() &&
                          row.get("n")->try_i64(),
                      "warm plan group types");
                counts.emplace_back(*row.get("bucket")->try_i64(), *row.get("n")->try_i64());
            }
            verify_counts(counts, threshold == 1);
            const auto& metadata = result.value().metadata;
            check(metadata.params.size() == 2 && metadata.params[0] == Value(r, threshold) &&
                      metadata.params[1] == Value(r, String("%", r) + String(original, r) + "%"),
                  "exact current HAVING bindings");
            check(!metadata.debug_query && metadata.parameterized_query,
                  "capture flag cannot authorize private interpolation");
            const auto& sql = *metadata.parameterized_query;
            check(sql.find("ROW_NUMBER() OVER (PARTITION BY") != String::npos &&
                      sql.find(" GROUP BY ") != String::npos &&
                      sql.find(" HAVING ") != String::npos,
                  "actual grouped partition SQL");
            Vector<String> secrets(r);
            metadata.sql_log.intent_redactions.extend_secrets(false, secrets);
            check(secrets.size() == 1 && secrets[0] == original,
                  "warm plan retains only current original operand provenance");
        }
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (unsigned shape = 0; shape < 3; ++shape) {
        for (bool logging : {true, false}) {
            for (bool having : {false, true}) {
                verify(shape, logging, having);
            }
        }
    }
    cached_having();
    std::cout << "NUMERIC_PARTITION_SOURCE_TESTS=4;cases=16;passed\n";
}
